// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.h
/// \brief Handles multiple resolutions, 8bpp/16bpp(highcolor) modes

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "command.h"

#if defined (_WIN32) && !defined (__CYGWIN__)
#define RPC_NO_WINDOWS_H
#include <windows.h>
#define DNWH HWND
#else
#define DNWH void * // unused in DOS version
#endif

// quickhack for V_Init()... to be cleaned up
#ifdef NOPOSTPROCESSING
#define NUMSCREENS 2
#else
#define NUMSCREENS 5
#endif

// Size of statusbar.
#define ST_HEIGHT 32
#define ST_WIDTH 320

// we try to re-allocate a minimum of buffers for stability of the memory,
// so all the small-enough tables based on screen size, are allocated once
// and for all at the maximum size.

#define MAXVIDWIDTH 2160 // don't set this too high because actually
#define MAXVIDHEIGHT 1200 // lots of tables are allocated with the MAX size.

// NEVER CHANGE THIS! This is the original resolution of the graphics.
#define BASEVIDWIDTH 320
#define BASEVIDHEIGHT 200

// global video state
typedef struct viddef_s
{
	INT32 modenum; // vidmode num indexes videomodes list

	UINT8 *buffer; // invisible screens buffer
	size_t rowbytes; // bytes per scanline of the VIDEO mode
	INT32 width; // PIXELS per scanline
	INT32 height;
	union { // don't need numpages for OpenGL, so we can use it for fullscreen/windowed mode
		INT32 numpages; // always 1, page flipping todo
		INT32 windowed; // windowed or fullscren mode?
	} u;
	INT32 recalc; // if true, recalc vid-based stuff
	UINT8 *direct; // linear frame buffer, or vga base mem.
	INT32 dupx, dupy; // scale 1, 2, 3 value for menus & overlays
	INT32/*fixed_t*/ fdupx, fdupy; // same as dupx, dupy, but exact value when aspect ratio isn't 320/200
	INT32 bpp; // BYTES per pixel: 1 = 256color, 2 = highcolor

	// Sky scaling
	struct {
		INT32 dup;
		INT32 fdup;
	} sky;

	// for Win32 version
	DNWH WndParent; // handle of the application's window
	UINT8 smalldupx, smalldupy; // factor for a little bit of scaling
	UINT8 meddupx, meddupy; // factor for moderate, but not full, scaling
#ifdef HWRENDER
	INT32/*fixed_t*/ fsmalldupx, fsmalldupy;
	INT32/*fixed_t*/ fmeddupx, fmeddupy;
	INT32 glstate;
#endif
} viddef_t;

enum
{
	VID_GL_LIBRARY_NOTLOADED  = 0,
	VID_GL_LIBRARY_LOADED     = 1,
	VID_GL_LIBRARY_ERROR      = -1,
};

// internal additional info for vesa modes only
typedef struct
{
	INT32 vesamode; // vesa mode number plus LINEAR_MODE bit
	void *plinearmem; // linear address of start of frame buffer
} vesa_extra_t;
// a video modes from the video modes list,
// note: video mode 0 is always standard VGA320x200.
typedef struct vmode_s
{
	struct vmode_s *pnext;
	char *name;
	UINT32 width, height;
	UINT32 rowbytes; // bytes per scanline
	UINT32 bytesperpixel; // 1 for 256c, 2 for highcolor
	INT32 windowed; // if true this is a windowed mode
	INT32 numpages;
	vesa_extra_t *pextradata; // vesa mode extra data
#ifdef _WIN32
	INT32 (WINAPI *setmode)(viddef_t *lvid, struct vmode_s *pcurrentmode);
#else
	INT32 (*setmode)(viddef_t *lvid, struct vmode_s *pcurrentmode);
#endif
	INT32 misc; // misc for display driver (r_opengl.dll etc)
} vmode_t;

#define NUMSPECIALMODES  4
extern vmode_t specialmodes[NUMSPECIALMODES];

// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

#define BASEDRAWFUNC 0

enum
{
	COLDRAWFUNC_BASE = BASEDRAWFUNC,
	COLDRAWFUNC_FUZZY,
	COLDRAWFUNC_TRANS,
	COLDRAWFUNC_SHADE,
	COLDRAWFUNC_SHADOWED,
	COLDRAWFUNC_TRANSTRANS,
	COLDRAWFUNC_TWOSMULTIPATCH,
	COLDRAWFUNC_TWOSMULTIPATCHTRANS,
	COLDRAWFUNC_FOG,

	COLDRAWFUNC_MAX
};

extern void (*colfunc)(void);
extern void (*colfuncs[COLDRAWFUNC_MAX])(void);

enum
{
	SPANDRAWFUNC_BASE = BASEDRAWFUNC,
	SPANDRAWFUNC_TRANS,
	SPANDRAWFUNC_TILTED,
	SPANDRAWFUNC_TILTEDTRANS,

	SPANDRAWFUNC_SPLAT,
	SPANDRAWFUNC_TRANSSPLAT,
	SPANDRAWFUNC_TILTEDSPLAT,

	SPANDRAWFUNC_SPRITE,
	SPANDRAWFUNC_TRANSSPRITE,
	SPANDRAWFUNC_TILTEDSPRITE,
	SPANDRAWFUNC_TILTEDTRANSSPRITE,

	SPANDRAWFUNC_WATER,
	SPANDRAWFUNC_TILTEDWATER,

	SPANDRAWFUNC_SOLID,
	SPANDRAWFUNC_TRANSSOLID,
	SPANDRAWFUNC_TILTEDSOLID,
	SPANDRAWFUNC_TILTEDTRANSSOLID,
	SPANDRAWFUNC_WATERSOLID,
	SPANDRAWFUNC_TILTEDWATERSOLID,

	SPANDRAWFUNC_FOG,
	SPANDRAWFUNC_TILTEDFOG,

	SPANDRAWFUNC_MAX
};

extern void (*spanfunc)(void);
extern void (*spanfuncs[SPANDRAWFUNC_MAX])(void);
extern void (*spanfuncs_npo2[SPANDRAWFUNC_MAX])(void);

// -----
// CPUID
// -----
extern boolean R_ASM;
extern boolean R_486;
extern boolean R_586;
extern boolean R_MMX;
extern boolean R_3DNow;
extern boolean R_MMXExt;
extern boolean R_SSE2;

// ----------------
// screen variables
// ----------------
extern viddef_t vid;
extern INT32 setmodeneeded; // mode number to set if needed, or 0
extern UINT8 setrenderneeded;
extern UINT8 renderswitcherror;

extern double averageFPS;

void SCR_ChangeRenderer(void);

extern CV_PossibleValue_t cv_renderer_t[];

extern INT32 scr_bpp;
extern UINT8 *scr_borderpatch; // patch used to fill the view borders

extern consvar_t cv_scr_width, cv_scr_height, cv_scr_depth, cv_renderview, cv_renderer, cv_fullscreen;

#ifdef NATIVESCREENRES
extern consvar_t cv_nativeres;
extern consvar_t cv_nativeresdiv, cv_nativeresauto;
extern consvar_t cv_nativeresfov, cv_nativerescompare;

void SCR_CheckNativeMode(void);
float SCR_GetNativeResDivider(INT32 width, INT32 height);

float SCR_GetMaxNativeResDivider(INT32 nw, INT32 nh);
void SCR_SetMaxNativeResDivider(float max);

void SCR_ResetNativeResDivider(void);

extern float scr_resdiv;
#endif

extern consvar_t cv_renderhitbox, cv_renderhitboxinterpolation, cv_renderhitboxgldepth;
// wait for page flipping to end or not
extern consvar_t cv_vidwait;
extern consvar_t cv_timescale;

// Initialize the screen
void SCR_Startup(void);

// Change video mode, only at the start of a refresh.
void SCR_SetMode(void);

// Set drawer functions for Software
void SCR_SetDrawFuncs(void);

// Recalc screen size dependent stuff
void SCR_Recalc(void);

// Check parms once at startup
void SCR_CheckDefaultMode(void);
// Saves the current resolution in the config
void SCR_SetDefaultMode(void);
// Set the mode number based on the resolution saved in the config
void SCR_SetModeFromConfig(void);

void SCR_CalculateFPS(void);

FUNCMATH boolean SCR_IsAspectCorrect(INT32 width, INT32 height);

// move out to main code for consistency
void SCR_DisplayTicRate(void);
void SCR_ClosedCaptions(void);
void SCR_DisplayLocalPing(void);
void SCR_DisplayMarathonInfo(void);
#undef DNWH
#endif //__SCREEN_H__
