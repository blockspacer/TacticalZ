#include "Editor/EditorRenderSystem.h"

EditorRenderSystem::EditorRenderSystem(SystemParams params, IRenderer* renderer, RenderFrame* renderFrame) 
    : System(params)
    , m_Renderer(renderer)
    , m_RenderFrame(renderFrame)
{
    EVENT_SUBSCRIBE_MEMBER(m_ESetCamera, &EditorRenderSystem::OnSetCamera);
    auto resolution = Rectangle::Rectangle(1280, 720);
    m_EditorCamera = new Camera((float)resolution.Width / resolution.Height, glm::radians(45.f), 0.001f, 500.f);
}

void EditorRenderSystem::Update(double dt)
{
    if (m_CurrentCamera.Valid()) {
        ComponentWrapper cameraTransform = m_CurrentCamera["Transform"];
        m_EditorCamera->SetPosition(cameraTransform["Position"]);
        m_EditorCamera->SetOrientation(glm::quat((const glm::vec3&)cameraTransform["Orientation"]));
    }

    RenderScene scene;
    scene.ClearDepth = true;
    scene.Camera = m_EditorCamera;
    scene.Viewport = Rectangle(1920, 1080);

    auto cSceneLight = m_World->GetComponents("SceneLight");
    if (cSceneLight != nullptr) {
        //these are hardcoded since they want special light treatment and a component just for widgets is stupid.
        scene.AmbientColor = glm::vec4(0.8, 0.8, 0.8, 1.0);
    }

    auto models = m_World->GetComponents("Model");
    if (models != nullptr) {
        for (auto& cModel : *models) {
            if (!(bool)cModel["Visible"]) {
                continue;
            }

            const std::string& resource = cModel["Resource"];

            Model* model;
            try {
                model = ResourceManager::Load<::Model, true>(resource);
            } catch (const Resource::StillLoadingException&) {
                continue;
            } catch (const std::exception&) {
                try {
                    model = ResourceManager::Load<::Model>("Models/Core/Error.mesh");
                } catch (const std::exception&) {
                    continue;
                }
            }

            EntityWrapper entity(m_World, cModel.EntityID);
            glm::mat4 modelMatrix = TransformSystem::ModelMatrix(entity.ID, entity.World);
            for (auto matGroup : model->MaterialGroups()) {
                std::shared_ptr<ModelJob> modelJob = std::make_shared<ModelJob>(model, scene.Camera, modelMatrix, matGroup, cModel, entity.World, glm::vec4(0), 0.f, false, false);
                if (cModel["Transparent"]) {
                    scene.Jobs.TransparentObjects.push_back(modelJob);
                } else {
                    scene.Jobs.OpaqueObjects.push_back(modelJob);
                }
            }
        }
    }

    auto pointLights = m_World->GetComponents("PointLight");
    if (pointLights != nullptr) {
        for (auto& cPointLight : *pointLights) {
            bool visible = cPointLight["Visible"];
            if (!visible) {
                continue;
            }

            EntityWrapper entity(m_World, cPointLight.EntityID);
            ComponentWrapper& cTransform = entity["Transform"];
            std::shared_ptr<PointLightJob> pointLightJob = std::make_shared<PointLightJob>(cTransform, cPointLight, entity.World);
            scene.Jobs.PointLight.push_back(pointLightJob);
        }
    }

    m_RenderFrame->Add(scene);
}

bool EditorRenderSystem::OnSetCamera(Events::SetCamera& e)
{
    ComponentWrapper cTransform = e.CameraEntity["Transform"];
    ComponentWrapper cCamera = e.CameraEntity["Camera"];
    m_EditorCamera->SetFOV(glm::radians(static_cast<float>((double)cCamera["FOV"])));
    m_EditorCamera->SetNearClip(static_cast<float>((double)cCamera["NearClip"]));
    m_EditorCamera->SetFarClip(static_cast<float>((double)cCamera["FarClip"]));
    m_EditorCamera->SetPosition(cTransform["Position"]);
    m_EditorCamera->SetOrientation(glm::quat((const glm::vec3&)cTransform["Orientation"]));
    m_CurrentCamera = e.CameraEntity;
    return true;
}

