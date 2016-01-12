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

class RenderSystem : public ImpureSystem
{
public:
    RenderSystem(EventBroker* eventBrokerer, const IRenderer* renderer, RenderFrame* renderFrame);

    virtual void Update(World* world, double dt) override;

    static glm::vec3 AbsolutePosition(World* world, EntityID entity);
    static glm::quat AbsoluteOrientation(World* world, EntityID entity);
    static glm::vec3 AbsoluteScale(World* world, EntityID entity);

    

private:
    World* m_World = nullptr;
    const IRenderer* m_Renderer = nullptr;

    RenderFrame* m_RenderFrame;
    bool m_SwitchCamera = false;
    Camera* m_Camera = nullptr;
    Camera* m_DefaultCamera = nullptr;
    
    std::list<ComponentWrapper> m_CameraComponents;

    EventRelay<RenderSystem, Events::SetCamera> m_ESetCamera;
    bool OnSetCamera(const Events::SetCamera &event);
    EntityID m_CurrentCamera = EntityID_Invalid;

    void switchCamera(EntityID entity);

    void initialize();
    void updateCamera(World* world, double dt);
    void updateProjectionMatrix(ComponentWrapper& cameraComponent);
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjectionMatrix;
    
    glm::mat4 ModelMatrix(EntityID entity, World* world);
    void fillModels(std::list<std::shared_ptr<RenderJob>>& jobs, World* world);

    EventRelay<RenderSystem, Events::InputCommand> m_EInputCommand;
    bool OnInputCommand(const Events::InputCommand& e);
};

#endif