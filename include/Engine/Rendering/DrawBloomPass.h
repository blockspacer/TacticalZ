#ifndef DrawBloomPass_h__
#define DrawBloomPass_h__

#include "IRenderer.h"
#include "DrawBloomPassState.h"
//#include "LightCullingPass.h" Finalpass om den skall skickas in
#include "FrameBuffer.h"
#include "ShaderProgram.h"
//#include "Util/UnorderedMapVec2.h"
#include "Texture.h"

class DrawBloomPass
{
public:
    DrawBloomPass(IRenderer* renderer, ConfigFile* config);
	~DrawBloomPass();
    void InitializeTextures();
    void InitializeFrameBuffers();
    void InitializeShaderPrograms();
    void InitializeBuffers();
    void ClearBuffer();

    void FillGaussianBuffer(FrameBuffer* fb);

    void Draw(GLuint texture);
	void ChangeQuality(int quality);

    void OnWindowResize();

    //Getters
    //Return the blurred result of the texture that was sent into draw
	GLuint GaussianTexture() const { 
		if (m_Quality == 0) { 
			return m_BlackTexture->m_Texture;
		} else { 
			return m_FinalGaussianTexture;
		} 
	}


private:
    void GaussianLodPass(GLuint mipMap, GLuint texture);
    void CombineGaussianBlur();
    Texture* m_BlackTexture;
    Model* m_ScreenQuad;

    const IRenderer* m_Renderer;
	ConfigFile* m_Config;
    //const LightCullingPass* m_LightCullingPass 
    int m_Iterations;
	int m_Quality = 0;
    int m_BloomLod = 5;

    GLuint m_GaussianTexture_horiz = 0;
    GLuint m_GaussianTexture_vert = 0;
    GLuint m_FinalGaussianTexture = 0;

    FrameBuffer* m_GaussianFrameBuffer_horiz = nullptr;
    FrameBuffer* m_GaussianFrameBuffer_vert = nullptr;
    FrameBuffer m_GaussianCombineBuffer;

    ShaderProgram* m_GaussianProgram_horiz;
    ShaderProgram* m_GaussianProgram_vert;
    ShaderProgram* m_GaussianCombineProgram;

};

#endif 