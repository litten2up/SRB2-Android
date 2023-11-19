// SONIC ROBO BLAST 2
// //-----------------------------------------------------------------------------
// // Copyright (C) 1998-2023 by Sonic Team Junior.
// // Copyright (C) 2023      by Bitten2up
// //
// // This program is free software distributed under the
// // terms of the GNU General Public License, version 2.
// // See the 'LICENSE' file for more details.
// //-----------------------------------------------------------------------------
// /// \file r_ctr.c
// /// \brief Nintendo 3ds GPU API for Sonic Robo Blast 2
// ///
// /// Heavily based on the 2.1 3ds port's renderer, and r_opengl.c

#if defined (HWRENDERER) && defined (NOROPENGL) && defined (__3DS__)

#include <stdarg.h>
#include <math.h>

#include "../../r_local.h" // For rendertimefrac, used for the leveltime shader uniform
#include "r_ctr.h"

//////////////////////////////////
// Steal the 2.1 port's breakfast
//////////////////////////////////

// Used to transfer the final rendered display to the framebuffer
#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

// Used to convert textures to 3DS tiled format
// Note: vertical flip flag set so 0,0 is top left of texture
#define TEXTURE_TRANSFER_BASE_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0))

#define MAX_FOG_DENSITY			255

extern LightEvent workerInitialized;

// XXX arbitrary near and far plane
FCOORD NEAR_CLIPPING_PLANE = NZCLIP_PLANE;
FCOORD FAR_CLIPPING_PLANE = 20000.0f;
const float fov = 62.0f;

// Render targets
static	C3D_RenderTarget *	targetLeft;
static	C3D_RenderTarget *	targetRight;
static	boolean				drawing;

// Buffer and Attribute info
static	C3D_BufInfo *bufInfo;
static	C3D_AttrInfo *attrInfo;

static void *geometryBuf;

// Shader programs
static	const void *vshaderData = /*nds3d_*/vshader_shbin;
//static	size_t vshaderSize = /*nds3d_*/vshader_shbin_size;
static	size_t vshaderSize;
static	DVLB_s *vshader_dvlb;
static	shaderProgram_s shaderProgram;
// Uniforms
static	int uLoc_projection, uLoc_modelView;
// Matrices
static C3D_Mtx defaultModelView;
// Special Sate
static	C3D_FogLut	*fogLUTs[MAX_FOG_DENSITY+1];
static	u32			prevFogDensity = -1;
static	u32			prevFogColor = -1;
static	boolean		fogEnabled;
/// Misc
static	C3D_Tex *prevTex;
static	u32 clearCounter;
static  boolean fading;
static  u32 fadeColor;

static float iod;
static float focalLength;

static n3dsRenderStats renderStats;

//#define NDS3D_INDEX_LIST_DRAWING

///////////////////////////////
// parody of r_opengl.c's vars
///////////////////////////////
static boolean ctr_shadersenabled = false;
static hwdshaderoption_t ctr_allowshaders = HWD_SHADEROPTION_OFF;

//
// I probally should create these
//
EXPORT boolean HWRAPI(CompileShaders) (void)
{
	return false;	
}

// Shader info
// Those are given to the uniforms.
//
EXPORT void HWRAPI(SetShaderInfo) (hwdshaderinfo_t info, INT32 value)
{
	// TODO 3DS: create function
	return;
}

//
// Custom shader loading
// aka um yeah you aren't going to be loading custom shaders on this shit, 3ds only shader mods when?
//
EXPORT void HWRAPI(LoadCustomShader) (int number, char *code, size_t size, boolean isfragment)
{
	return;
}

EXPORT void HWRAPI(SetShader) (int type)
{
	if (type == SHADER_NONE)
	{
		UnSetShader();
		return;
	}

	// check if we are allowing shaders
	if (ctr_allowshaders != HWD_SHADEROPTION_OFF)
	{
		// TODO 3DS: the rest of this function
	}
	ctr_shadersenabled = false;
}

EXPORT void HWRAPI(UnSetShader) (void)
{
	// TODO 3DS: create function
	return;
}

EXPORT void HWRAPI(CleanShaders) (void)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// DeleteTexture    : Deletes a texture from the GPU and frees its data
// -----------------+
EXPORT void HWRAPI(DeleteTexture) (GLMipmap_t *pTexInfo)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// Flush            : flush Citro3d textures
//                  : Clear list of downloaded mipmaps
// -----------------+
void Flush(void)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// Init             : Initialise the Citro3d interface API
// -----------------+
// aka steal the 2.1 port's code/
EXPORT boolean HWRAPI(Init) (void)
{
	//NDS3D_driverLog("NDS3D_Init\n");

	// Initialize the render targets
	targetLeft  = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	targetRight = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(targetLeft,  GFX_TOP, GFX_LEFT,  DISPLAY_TRANSFER_FLAGS);
	C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);
	C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, 0, 0);
	InitTextureUnits();

	//C3D_CullFace(GPU_CULL_BACK_CCW );
	C3D_CullFace(GPU_CULL_NONE);

	C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
	
	vshaderSize = vshader_shbin_size;
	if(vshaderSize == 0)
		NDS3D_driverPanic("Invalid shader bin size!\n");
	
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshaderData, vshaderSize);
	shaderProgramInit(&shaderProgram);
	shaderProgramSetVsh(&shaderProgram, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&shaderProgram);
	
	// Get the location of the uniforms
	uLoc_projection   = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "projection");
	uLoc_modelView    = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "modelView");
	
	InitDefaultMatrices();

	// ensure uniforms are initialized
	NDS3D_SetTransform(NULL);

	InitRendererMode();

	NDS3D_ResetRenderStats();

	NDS3D_driverLog("NDS3D_Init done\n");
	
	return true;
}

// -----------------+
// ClearMipMapCache : Flush OpenGL textures from memory
// -----------------+
EXPORT void HWRAPI(ClearMipMapCache) (void)
{
	// GL_DBG_Printf ("HWR_Flush(exe)\n");
	Flush();
}

// -----------------+
// ReadRect         : Read a rectangle region of the truecolor framebuffer
//                  : store pixels as 16bit 565 RGB
// Returns          : 16bit 565 RGB pixel array stored in dst_data
// -----------------+
EXPORT void HWRAPI(ReadRect) (INT32 x, INT32 y, INT32 width, INT32 height,
		                                INT32 dst_stride, UINT16 * dst_data)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// GClipRect        : Defines the 2D hardware clipping window
// -----------------+
EXPORT void HWRAPI(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// ClearBuffer      : Clear the color/alpha/depth buffer(s)
// -----------------+
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask,
                                    FBOOLEAN DepthMask,
				    FRGBAFloat * ClearColor)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// HWRAPI Draw2DLine: Render a 2D line
// -----------------+
EXPORT void HWRAPI(Draw2DLine) (F2DCoord * v1,
                                   F2DCoord * v2,
                                   RGBA_t Color)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// SetBlend         : Set render mode
// -----------------+
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// UpdateTexture    : Updates texture data.
// -----------------+
EXPORT void HWRAPI(UpdateTexture) (GLMipmap_t *pTexInfo)
{
	// TODO 3DS: create function
	return;
}	

// -----------------+
// SetTexture       : The mipmap becomes the current texture source
// -----------------+
EXPORT void HWRAPI(SetTexture) (GLMipmap_t *pTexInfo)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// DrawPolygon      : Render a polygon, set the texture, set render mode
// -----------------+
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags)
{
	// TODO 3DS: create function
	return;
}

EXPORT void HWRAPI(DrawIndexedTriangles) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, UINT32 *IndexArray)
{
	// TODO 3DS: create function
}

EXPORT void HWRAPI(RenderSkyDome) (gl_sky_t *sky)
{
	// TODO 3DS: create function
	return;
}

// ==========================================================================
//
// ==========================================================================
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value)
{
	// TODO 3DS: create function
	return;
}

EXPORT void HWRAPI(CreateModelVBOs) (model_t *model)
{
	// TODO 3DS: create function
	return;
}

// don't ask me why this exists, because i don't fucking know
static void DrawModelEx(model_t *model, INT32 frameIndex, float duration, float tics, INT32 nextFrameIndex, FTransform *pos, float hscale, float vscale, UINT8 flipped, UINT8 hflipped, FSurfaceInfo *Surface)
{
	// TODO 3DS: create function
	return;
}

// -----------------+
// HWRAPI DrawModel : Draw a model
// -----------------+
EXPORT void HWRAPI(DrawModel) (model_t *model, INT32 frameIndex, float duration, float tics, INT32 nextFrameIndex, FTransform *pos, float hscale, float vscale, UINT8 flipped, UINT8 hflipped, FSurfaceInfo *Surface)
{
		DrawModelEx(model, frameIndex, duration, tics, nextFrameIndex, pos, hscale, vscale, flipped, hflipped, Surface);
}

// -----------------+
// SetTransform     :
// -----------------+
EXPORT void HWRAPI(SetTransform) (FTransform *stransform)
{
	// TODO 3DS: create function
	return;
}

EXPORT INT32  HWRAPI(GetTextureUsed) (void)
{
	// TODO 3DS: create function
	return 0;
}

EXPORT void HWRAPI(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2])
{
	// TODO 3DS: create function
	return;
}

// Sryder:	This needs to be called whenever the screen changes resolution in order to reset the screen textures to use
//			a new size
EXPORT void HWRAPI(FlushScreenTextures) (void)
{
	// TODO 3DS: create function
	return;
}
// Create Screen to fade from
EXPORT void HWRAPI(StartScreenWipe) (void)
{
	// TODO 3DS: create function
	return;
}

// Create Screen to fade to
EXPORT void HWRAPI(EndScreenWipe)(void)
{
	// TODO 3DS: create function
	return;
}

// Draw the last scene under the intermission
EXPORT void HWRAPI(DrawIntermissionBG)(void)
{
	// TODO 3DS: create function
	return;
}

// Do screen fades!
EXPORT void HWRAPI(DoScreenWipe)(void)
{
	// TODO 3DS: create function
	return;
}

// Create a texture from the screen.
EXPORT void HWRAPI(MakeScreenTexture) (void)
{
	// TODO 3DS: create function
	return;
}

EXPORT void HWRAPI(MakeScreenFinalTexture) (void)
{
	// TODO 3DS: create function
	return;
}

EXPORT void HWRAPI(DrawScreenFinalTexture)(int width, int height)
{
	// TODO 3DS: create function
	return;
}

#endif //HWRENDER
