#include "Game.h"
#include "Collision/FillOctreeSystem.h"
#include "Collision/FillFrustumOctreeSystem.h"
#include "Collision/EntityAABB.h"
#include "Collision/TriggerSystem.h"
#include "Collision/CollisionSystem.h"
#include "Systems/RaptorCopterSystem.h"
#include "Systems/HealthSystem.h"
#include "Systems/PlayerMovementSystem.h"
#include "Systems/SpawnerSystem.h"
#include "Systems/PlayerSpawnSystem.h"
#include "Systems/PlayerDeathSystem.h"
#include "Core/EntityFileWriter.h"
#include "Game/Systems/CapturePointSystem.h"
#include "Game/Systems/PickupSpawnSystem.h"
#include "Game/Systems/WeaponSystem.h"
#include "Rendering/AnimationSystem.h"
#include "Rendering/BoneAttachmentSystem.h"
#include "Game/Systems/PlayerHUD.h"
#include "Game/Systems/LifetimeSystem.h"

Game::Game(int argc, char* argv[])
{
    ResourceManager::RegisterType<ConfigFile>("ConfigFile");
    ResourceManager::RegisterType<Sound>("Sound");
    ResourceManager::RegisterType<Model>("Model");
    ResourceManager::RegisterType<RawModel>("RawModel");
    ResourceManager::RegisterType<Texture>("Texture");
    ResourceManager::RegisterType<ShaderProgram>("ShaderProgram");
    ResourceManager::RegisterType<EntityFile>("EntityFile");
    ResourceManager::RegisterType<Font>("FontFile");

    m_Config = ResourceManager::Load<ConfigFile>("Config.ini");
    ResourceManager::UseThreading = m_Config->Get<bool>("Multithreading.ResourceLoading", true);
    DisableMemoryPool::Value = m_Config->Get<bool>("Debug.DisableMemoryPool", false);
    LOG_LEVEL = static_cast<_LOG_LEVEL>(m_Config->Get<int>("Debug.LogLevel", 1));

    // Create the core event broker
    m_EventBroker = new EventBroker();

    // Create the renderer
    m_Renderer = new Renderer(m_EventBroker);
    m_Renderer->SetFullscreen(m_Config->Get<bool>("Video.Fullscreen", false));
    m_Renderer->SetVSYNC(m_Config->Get<bool>("Video.VSYNC", false));
    m_Renderer->SetResolution(Rectangle::Rectangle(
        0,
        0,
        m_Config->Get<int>("Video.Width", 1280),
        m_Config->Get<int>("Video.Height", 720)
        ));
    m_Renderer->Initialize();
    //m_Renderer->Camera()->SetFOV(glm::radians(m_Config->Get<float>("Video.FOV", 90.f)));
    m_RenderFrame = new RenderFrame();

    // Create input manager
    m_InputManager = new InputManager(m_Renderer->Window(), m_EventBroker);
    m_InputProxy = new InputProxy(m_EventBroker);
    m_InputProxy->AddHandler<KeyboardInputHandler>();
    m_InputProxy->AddHandler<MouseInputHandler>();
    m_InputProxy->LoadBindings("Input.ini");

    // Create the root level GUI frame
    m_FrameStack = new GUI::Frame(m_EventBroker);
    m_FrameStack->Width = m_Renderer->Resolution().Width;
    m_FrameStack->Height = m_Renderer->Resolution().Height;

    // Create a world
    m_World = new World(m_EventBroker);
    std::string mapToLoad = m_Config->Get<std::string>("Debug.LoadMap", "");
    if (!mapToLoad.empty()) {
        auto file = ResourceManager::Load<EntityFile>(mapToLoad);
        EntityFilePreprocessor fpp(file);
        fpp.RegisterComponents(m_World);
        EntityFileParser fp(file);
        fp.MergeEntities(m_World);
    }

    // Create the sound manager
    m_SoundManager = new SoundManager(m_World, m_EventBroker);

    // Create Octrees
    // TODO: Perhaps the world bounds should be set in some non-arbitrary way instead of this.
    AABB boxContainingTheWorld(glm::vec3(-300), glm::vec3(300));
    m_OctreeCollision = new Octree<EntityAABB>(boxContainingTheWorld, 4);
    m_OctreeTrigger = new Octree<EntityAABB>(boxContainingTheWorld, 4);
    m_OctreeFrustrumCulling = new Octree<EntityAABB>(boxContainingTheWorld, 4);
    // Create system pipeline
    m_SystemPipeline = new SystemPipeline(m_World, m_EventBroker);

    // All systems with orderlevel 0 will be updated first.
    unsigned int updateOrderLevel = 0;
    m_SystemPipeline->AddSystem<SoundSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<RaptorCopterSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<ExplosionEffectSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<HealthSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<PlayerMovementSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<InterpolationSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<SpawnerSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<PlayerSpawnSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<PlayerDeathSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<WeaponSystem>(updateOrderLevel, m_Renderer);
    m_SystemPipeline->AddSystem<LifetimeSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<CapturePointSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<PickupSpawnSystem>(updateOrderLevel);
    // Populate Octree with collidables
    ++updateOrderLevel;
    m_SystemPipeline->AddSystem<FillOctreeSystem>(updateOrderLevel, m_OctreeCollision, "Collidable");
    m_SystemPipeline->AddSystem<FillOctreeSystem>(updateOrderLevel, m_OctreeTrigger, "Player");
    m_SystemPipeline->AddSystem<FillFrustumOctreeSystem>(updateOrderLevel, m_OctreeFrustrumCulling);
    m_SystemPipeline->AddSystem<PlayerHUD>(updateOrderLevel);
    m_SystemPipeline->AddSystem<AnimationSystem>(updateOrderLevel);

    // Collision and TriggerSystem should update after player.
    ++updateOrderLevel;
    m_SystemPipeline->AddSystem<BoneAttachmentSystem>(updateOrderLevel);
    m_SystemPipeline->AddSystem<CollisionSystem>(updateOrderLevel, m_OctreeCollision);
    m_SystemPipeline->AddSystem<TriggerSystem>(updateOrderLevel, m_OctreeTrigger);
    ++updateOrderLevel;
    m_SystemPipeline->AddSystem<RenderSystem>(updateOrderLevel, m_Renderer, m_RenderFrame, m_OctreeFrustrumCulling);
    ++updateOrderLevel;
    m_SystemPipeline->AddSystem<EditorSystem>(updateOrderLevel, m_Renderer, m_RenderFrame);

    // Invoke network
    if (m_Config->Get<bool>("Networking.StartNetwork", false)) {
        //boost::thread workerThread(&Game::networkFunction, this);
        networkFunction();
    }

    m_LastTime = glfwGetTime();
}

Game::~Game()
{
    delete m_SystemPipeline;
    delete m_OctreeFrustrumCulling;
    delete m_OctreeCollision;
    delete m_OctreeTrigger;
    delete m_SoundManager;
    delete m_World;
    delete m_FrameStack;
    delete m_InputProxy;
    delete m_InputManager;
    delete m_RenderFrame;
    delete m_Renderer;
    delete m_EventBroker;
}

void Game::Tick()
{
    glfwPollEvents();

    double currentTime = glfwGetTime();
    double dt = currentTime - m_LastTime;
    m_LastTime = currentTime;

    // Handle input in a weird looking but responsive way
    m_EventBroker->Process<InputManager>();
    m_EventBroker->Swap();
    m_InputManager->Update(dt);
    m_EventBroker->Swap();
    m_InputProxy->Update(dt);
    m_EventBroker->Swap();
    m_InputProxy->Process();
    m_EventBroker->Swap();

    m_SoundManager->Update(dt);

    // Update network
    if (m_IsClientOrServer) {
        if (m_IsServer)
            m_Server->Update();
        else if (!m_IsServer) {
            m_Client->Update();
        }
    }
    //m_SoundManager->Update(dt);

    // Iterate through systems and update world!
    m_EventBroker->Process<SystemPipeline>();
    m_SystemPipeline->Update(dt);
    debugTick(dt);
    m_Renderer->Update(dt);
    GLERROR("Game::Tick m_RenderQueueFactory->Update");
    m_Renderer->Draw(*m_RenderFrame);
    m_RenderFrame->Clear();
    GLERROR("Game::Tick m_Renderer->Draw");
    m_EventBroker->Swap();
    m_EventBroker->Clear();
}

void Game::debugTick(double dt)
{
    m_EventBroker->Process<Game>();
}

void Game::networkFunction()
{
    m_IsServer = m_Config->Get<bool>("Networking.IsServer", false);
    if (!m_IsServer) {
        m_IsClientOrServer = true;
        m_Client = std::unique_ptr<Client>(new Client(m_Config));
        m_Client->Start(m_World, m_EventBroker);
    }
    //if (!isServer) {
    //    m_IsClientOrServer = true;
    //    m_ClientOrServer = new UDPClient(m_Config);
    //    //m_ClientOrServer = new TCPClient(m_Config);
    //    //m_ClientOrServer = new HybridClient(m_Config);
    //}
    if (m_IsServer) {
        m_IsClientOrServer = true;
        // m_ClientOrServer = new UDPServer();
        m_Server = std::unique_ptr<Server>(new Server());
        //m_ClientOrServer = new HybridServer();
        m_Server->Start(m_World, m_EventBroker);
    }


}