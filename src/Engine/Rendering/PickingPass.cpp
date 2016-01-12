#include "Rendering/PickingPass.h"

PickingPass::PickingPass(IRenderer* renderer, EventBroker* eb)
{
    m_Renderer = renderer;
    m_EventBroker = eb;

    InitializeTextures();
    InitializeFrameBuffers();
    InitializeShaderPrograms();
}

PickingPass::~PickingPass()
{

}

void PickingPass::InitializeTextures()
{
    GenerateTexture(&m_PickingTexture, GL_CLAMP_TO_BORDER, GL_LINEAR,
        glm::vec2(m_Renderer->Resolution().Width, m_Renderer->Resolution().Height), GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
}

void PickingPass::InitializeFrameBuffers()
{
    glGenRenderbuffers(1, &m_DepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_Renderer->Resolution().Width, m_Renderer->Resolution().Height);

    m_PickingBuffer.AddResource(std::shared_ptr<BufferResource>(new RenderBuffer(&m_DepthBuffer, GL_DEPTH_ATTACHMENT)));
    m_PickingBuffer.AddResource(std::shared_ptr<BufferResource>(new Texture2D(&m_PickingTexture, GL_COLOR_ATTACHMENT0)));
    m_PickingBuffer.Generate();
}

void PickingPass::InitializeShaderPrograms()
{
    m_PickingProgram = ResourceManager::Load<ShaderProgram>("#PickingProgram");

    m_PickingProgram->AddShader(std::shared_ptr<Shader>(new VertexShader("Shaders/Picking.vert.glsl")));
    m_PickingProgram->AddShader(std::shared_ptr<Shader>(new FragmentShader("Shaders/Picking.frag.glsl")));
    m_PickingProgram->Compile();
    m_PickingProgram->BindFragDataLocation(0, "TextureFragment");
    m_PickingProgram->Link();
}

void PickingPass::Draw(RenderFrame& rf)
{
    m_PickingColorsToEntity.clear();
    PickingPassState* state = new PickingPassState(m_PickingBuffer.GetHandle());

    int r = 0;
    int g = 0;
    //TODO: Render: Add code for more jobs than modeljobs.

    GLuint ShaderHandle = m_PickingProgram->GetHandle();
    m_PickingProgram->Bind();

    std::map<EntityID, glm::vec2> entityColors;

    for(auto scene : rf.RenderScenes)
    {
        m_Camera = scene->Camera;

        for (auto &job : scene->ForwardJobs) {
            auto modelJob = std::dynamic_pointer_cast<ModelJob>(job);

            if (modelJob) {
                int pickColor[2] = { r, g };
                auto color = entityColors.find(modelJob->Entity);
                if (color != entityColors.end()) {
                    pickColor[0] = color->second[0];
                    pickColor[1] = color->second[1];
                } else {
                    entityColors[modelJob->Entity] = glm::vec2(pickColor[0], pickColor[1]);
                    if (r + 10 > 255) {
                        r = 0;
                        g += 1;
                    } else {
                        r += 1;
                    }
                }
                m_PickingColorsToEntity[glm::vec2(pickColor[0], pickColor[1])] = modelJob->Entity;

                glUniformMatrix4fv(glGetUniformLocation(ShaderHandle, "Matrix"), 1, GL_FALSE, glm::value_ptr(modelJob->Matrix));
                glUniform2fv(glGetUniformLocation(ShaderHandle, "PickingColor"), 1, glm::value_ptr(glm::vec2(pickColor[0], pickColor[1])));

                glBindVertexArray(modelJob->Model->VAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelJob->Model->ElementBuffer);
                glDrawElementsBaseVertex(GL_TRIANGLES, modelJob->EndIndex - modelJob->StartIndex + 1, GL_UNSIGNED_INT, nullptr, modelJob->StartIndex);
            }
        }
    }
    
    m_PickingBuffer.Unbind();
    GLERROR("PickingPass Error");


    delete state;
}



PickData PickingPass::Pick(glm::vec2 screenCoord)
{
    int fbWidth;
    int fbHeight;
    glfwGetFramebufferSize(m_Renderer->Window(), &fbWidth, &fbHeight);

    Rectangle resolution = Rectangle(fbWidth, fbHeight);
    PickData pickData;
    // Invert screen y coordinate
    screenCoord.y = resolution.Height - screenCoord.y;
    ScreenCoords::PixelData data = ScreenCoords::ToPixelData(screenCoord, &m_PickingBuffer, m_DepthBuffer);
    pickData.Depth = data.Depth;

    auto it = m_PickingColorsToEntity.find(glm::vec2(data.Color[0], data.Color[1]));
    if (it != m_PickingColorsToEntity.end()) {
        pickData.Entity = it->second;
    } else {
        pickData.Entity = EntityID_Invalid;
    }
    pickData.Position = ScreenCoords::ToWorldPos(screenCoord.x, screenCoord.y, data.Depth, resolution, m_Camera->ProjectionMatrix(), m_Camera->ViewMatrix());

    return pickData;
}

void PickingPass::GenerateTexture(GLuint* texture, GLenum wrapping, GLenum filtering, glm::vec2 dimensions, GLint internalFormat, GLint format, GLenum type) const
{
    //TODO: Renderer: Make this in a sparate class
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, dimensions.x, dimensions.y, 0, format, type, nullptr);//TODO: Renderer: Fix the precision and Resolution
    GLERROR("Texture initialization failed");
}
