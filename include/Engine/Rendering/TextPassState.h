#ifndef TextPassState_h__
#define TextPassState_h__

#include "Rendering/RenderState.h"

class TextPassState : public RenderState
{
public:
    TextPassState(GLuint frameBuffer);
    ~TextPassState();
private:

};

#endif