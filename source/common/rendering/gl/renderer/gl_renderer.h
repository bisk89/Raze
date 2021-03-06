#ifndef __GL_RENDERER_H
#define __GL_RENDERER_H

#include "v_video.h"
#include "vectors.h"
#include "matrix.h"
#include "gl/renderer/gl_renderbuffers.h"
#include <functional>

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

struct particle_t;
class FCanvasTexture;
class FFlatVertexBuffer;
class FSkyVertexBuffer;
class FShaderManager;
class HWPortal;
class FLightBuffer;
class DPSprite;
class FGLRenderBuffers;
class FGL2DDrawer;
class FHardwareTexture;
class SWSceneDrawer;
class HWViewpointBuffer;
struct FRenderViewpoint;

namespace OpenGLRenderer
{
	class FSamplerManager;
	class OpenGLFrameBuffer;
	class FPresentShaderBase;
	class FPresentShader;
	class FPresent3DCheckerShader;
	class FPresent3DColumnShader;
	class FPresent3DRowShader;

class FGLRenderer
{
public:

	OpenGLFrameBuffer *framebuffer;
	int mMirrorCount = 0;
	int mPlaneMirrorCount = 0;
	//FShaderManager *mShaderManager = nullptr;
	//FSamplerManager *mSamplerManager = nullptr;
	unsigned int mFBID;
	unsigned int mVAOID;
	//unsigned int PortalQueryObject;
	unsigned int mStencilValue = 0;

	int mOldFBID;

	FGLRenderBuffers *mBuffers = nullptr;
	FGLRenderBuffers *mScreenBuffers = nullptr;
	FGLRenderBuffers *mSaveBuffers = nullptr;
	FPresentShader *mPresentShader = nullptr;
	FPresent3DCheckerShader *mPresent3dCheckerShader = nullptr;
	FPresent3DColumnShader *mPresent3dColumnShader = nullptr;
	FPresent3DRowShader *mPresent3dRowShader = nullptr;

	//FRotator mAngles;

	//SWSceneDrawer *swdrawer = nullptr;

	FGLRenderer(OpenGLFrameBuffer *fb);
	~FGLRenderer() ;

	void Initialize(int width, int height);

	void ClearBorders();

	void PresentStereo();
	void RenderScreenQuad();
	void PostProcessScene(int fixedcm, const std::function<void()> &afterBloomDrawEndScene2D);
	void AmbientOccludeScene(float m5);
	void ClearTonemapPalette();
	void BlurScene(float gameinfobluramount);
	void CopyToBackbuffer(const IntRect *bounds, bool applyGamma);
	void DrawPresentTexture(const IntRect &box, bool applyGamma);
	void Flush();
	//void Draw2D(F2DDrawer *data);
	void WriteSavePic(FileWriter *file, int width, int height);
	void BeginFrame();
    

	bool StartOffscreen();
	void EndOffscreen();

	void BindToFrameBuffer(FTexture* tex);

private:

	void DrawScene(HWDrawInfo *di, int drawmode);


};

struct TexFilter_s
{
	int minfilter;
	int magfilter;
	bool mipmapping;
} ;


extern FGLRenderer *GLRenderer;

}
#endif
