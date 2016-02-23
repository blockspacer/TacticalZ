#ifndef CubeMapPass_h__
#define CubeMapPass_h__

#include "IRenderer.h"
#include "ShaderProgram.h"

class CubeMapPass
{
public:
    CubeMapPass(IRenderer* renderer);
    ~CubeMapPass() { }

    void LoadTextures();
    void FillCubeMap(glm::vec3 originPosition);
    void GenerateCubeMapTexture();

    //GLuint CubeMapTexture() const { return m_CubeMapTexture; }
    GLuint m_CubeMapTexture;

private:
    IRenderer* m_Renderer;

    std::vector<Texture*> m_CubeMapTestTextures;
};

#endif 