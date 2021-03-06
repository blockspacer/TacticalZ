#ifndef RenderSystem_h__
#define RenderSystem_h__

#include "../Core/System.h"
#include "RenderQueue.h"
#include "../GLM.h"
#include "../OpenGL.h"
#include "../Core/ResourceManager.h"
#include "ESetCamera.h"
#include "Model.h"
#include "../Core/EKeyDown.h"
#include "../Input/EInputCommand.h"
#include "Camera.h"
#include "ModelJob.h"
#include "Renderer.h"
#include "PointLightJob.h"
#include "../Core/TransformSystem.h"
#include "../Core/EPlayerSpawned.h"
#include "../Core/Octree.h"
#include "../Collision/EntityAABB.h"
#include "../Core/ConfigFile.h"
#include "EResolutionChanged.h"

class RenderSystem : public ImpureSystem
{
public:
    RenderSystem(SystemParams params, const IRenderer* renderer, RenderFrame* renderFrame, Octree<EntityAABB>* frustumCullOctree);
    ~RenderSystem();

    virtual void Update(double dt) override;

private:
    const IRenderer* m_Renderer;
    RenderFrame* m_RenderFrame;
    Camera* m_Camera;
    EntityWrapper m_CurrentCamera = EntityWrapper::Invalid;
    EntityWrapper m_LocalPlayer = EntityWrapper::Invalid;
    Octree<EntityAABB>* m_Octree;

    EventRelay<RenderSystem, Events::ResolutionChanged> m_EResolutionChanged;
    bool OnResolutionChanged(Events::ResolutionChanged &event);
    EventRelay<RenderSystem, Events::SetCamera> m_ESetCamera;
    bool OnSetCamera(Events::SetCamera &event);
    EventRelay<RenderSystem, Events::InputCommand> m_EInputCommand;
    bool OnInputCommand(const Events::InputCommand& e);
    EventRelay<RenderSystem, Events::PlayerSpawned> m_EPlayerSpawned;
    bool OnPlayerSpawned(Events::PlayerSpawned& e);

    void fillModels(RenderScene::Queues &jobs);
    void fillText(std::list<std::shared_ptr<RenderJob>>& jobs, World* world);
    void fillPointLights(std::list<std::shared_ptr<RenderJob>>& jobs, World* world);
    void fillDirectionalLights(std::list<std::shared_ptr<RenderJob>>& jobs, World* world);
    void fillLight(std::list<std::shared_ptr<RenderJob>>& jobs);
    void fillSprites(std::list<std::shared_ptr<RenderJob>>& jobs, World* world);

    bool isEntityVisible(EntityWrapper& entity);

    bool isChildOfACamera(EntityWrapper entity);
    bool isChildOfCurrentCamera(EntityWrapper entity);
};

#endif