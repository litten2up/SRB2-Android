// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.c
/// \brief Handles multiple resolutions

#include "doomdef.h"
#include "doomstat.h"
#include "screen.h"
#include "console.h"
#include "am_map.h"
#include "i_time.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "d_main.h"
#include "netcode/d_clisrv.h"
#include "f_finale.h"
#include "y_inter.h" // usebuffer
#include "i_sound.h" // closed captions
#include "s_sound.h" // ditto
#include "g_game.h" // ditto
#include "p_local.h" // P_AutoPause()

#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#include "hardware/hw_model.h"
#endif

#ifdef TOUCHINPUTS
#include "ts_main.h" // touchfingers, NUMTOUCHFINGERS
#endif

// SRB2Kart
#include "r_fps.h" // R_GetFramerateCap

#include "lua_hud.h" // LUA_HudEnabled

// --------------------------------------------
// assembly or c drawer routines for 8bpp/16bpp
// --------------------------------------------
void (*colfunc)(void);
void (*colfuncs[COLDRAWFUNC_MAX])(void);

void (*spanfunc)(void);
void (*spanfuncs[SPANDRAWFUNC_MAX])(void);
void (*spanfuncs_npo2[SPANDRAWFUNC_MAX])(void);

// ------------------
// global video state
// ------------------
viddef_t vid;
INT32 setmodeneeded; //video mode change needed if > 0 (the mode number to set + 1)
UINT8 setrenderneeded = 0;
UINT8 renderswitcherror = 0;

static CV_PossibleValue_t scr_depth_cons_t[] = {{8, "8 bits"}, {16, "16 bits"}, {24, "24 bits"}, {32, "32 bits"}, {0, NULL}};

//added : 03-02-98: default screen mode, as loaded/saved in config
consvar_t cv_scr_width = CVAR_INIT ("scr_width", "1280", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_height = CVAR_INIT ("scr_height", "800", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_width_w = CVAR_INIT ("scr_width_w", "640", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_height_w = CVAR_INIT ("scr_height_w", "400", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_depth = CVAR_INIT ("scr_depth", "16 bits", CV_SAVE, scr_depth_cons_t, NULL);


#ifdef NATIVESCREENRES
static void SCR_ToggleNativeRes(void);
static void SCR_NativeResDivChanged(void);
static void SCR_NativeResAutoChanged(void);

static CV_PossibleValue_t nativeresdiv_cons_t[] = {{FRACUNIT, "MIN"}, {20 * FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t nativerescompare_cons_t[] = {{0, "Width"}, {1, "Height"}, {0, NULL}};

#define NATIVERESCVAR_FLAGS(name, default, possiblevalue, func, flags) CVAR_INIT (name, default, (CV_CALL | CV_SAVE | CV_NOINIT | flags), possiblevalue, func)
#define NATIVERESCVAR_CALL(name, default, possiblevalue, func) NATIVERESCVAR_FLAGS(name, default, possiblevalue, func, 0)
#define NATIVERESCVAR(name, default, possiblevalue) NATIVERESCVAR_CALL(name, default, possiblevalue, SCR_ToggleNativeRes)

consvar_t cv_nativeres = NATIVERESCVAR("nativeres", "On", CV_OnOff);
consvar_t cv_nativeresdiv = NATIVERESCVAR_FLAGS("nativeresdiv", "1", nativeresdiv_cons_t, SCR_NativeResDivChanged, CV_FLOAT);
consvar_t cv_nativeresauto = NATIVERESCVAR_CALL("nativeresauto", "On", CV_OnOff, SCR_NativeResAutoChanged);
consvar_t cv_nativeresfov = NATIVERESCVAR_CALL("nativeresfov", "On", CV_OnOff, R_SetViewSize);
consvar_t cv_nativerescompare = NATIVERESCVAR("nativerescompare", "Height", nativerescompare_cons_t);

#undef NATIVERESCVAR

#endif

CV_PossibleValue_t cv_renderer_t[] = {
	{1, "Software"},
#ifdef HWRENDER
	{2, "OpenGL"},
#endif
	{0, NULL}
};

consvar_t cv_renderer = CVAR_INIT ("renderer", "Software", CV_SAVE|CV_CALL, cv_renderer_t, SCR_ChangeRenderer);

static void SCR_ChangeFullscreen(void);

consvar_t cv_fullscreen = CVAR_INIT ("fullscreen", "Yes", CV_SAVE|CV_CALL, CV_YesNo, SCR_ChangeFullscreen);

// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

static boolean scr_startupmodeset = false;

INT32 scr_bpp; // current video mode bytes per pixel

#ifdef NATIVESCREENRES
float scr_resdiv = 1.0f;
#endif

// =========================================================================

void SCR_SetDrawFuncs(void)
{
	//
	//  setup the right draw routines
	//
	if (vid.bpp == 1) //Always run in 8bpp.
	{
		colfuncs[BASEDRAWFUNC] = R_DrawColumn_8;
		spanfuncs[BASEDRAWFUNC] = R_DrawSpan_8;

		colfunc = colfuncs[BASEDRAWFUNC];
		spanfunc = spanfuncs[BASEDRAWFUNC];

		colfuncs[COLDRAWFUNC_FUZZY] = R_DrawTranslucentColumn_8;
		colfuncs[COLDRAWFUNC_TRANS] = R_DrawTranslatedColumn_8;
		colfuncs[COLDRAWFUNC_SHADE] = R_DrawShadeColumn_8;
		colfuncs[COLDRAWFUNC_SHADOWED] = R_DrawColumnShadowed_8;
		colfuncs[COLDRAWFUNC_TRANSTRANS] = R_DrawTranslatedTranslucentColumn_8;
		colfuncs[COLDRAWFUNC_CLAMPED] = R_DrawColumnClamped_8;
		colfuncs[COLDRAWFUNC_CLAMPEDTRANS] = R_DrawTranslucentColumnClamped_8;
		colfuncs[COLDRAWFUNC_TWOSMULTIPATCH] = R_Draw2sMultiPatchColumn_8;
		colfuncs[COLDRAWFUNC_TWOSMULTIPATCHTRANS] = R_Draw2sMultiPatchTranslucentColumn_8;
		colfuncs[COLDRAWFUNC_FOG] = R_DrawFogColumn_8;

		spanfuncs[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTED] = R_DrawTiltedSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTiltedTranslucentSpan_8;
		spanfuncs[SPANDRAWFUNC_SPLAT] = R_DrawSplat_8;
		spanfuncs[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_8;
		spanfuncs[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawTiltedSplat_8;
		spanfuncs[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawTiltedFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTiltedTranslucentFloorSprite_8;
		spanfuncs[SPANDRAWFUNC_WATER] = R_DrawWaterSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDWATER] = R_DrawTiltedWaterSpan_8;
		spanfuncs[SPANDRAWFUNC_SOLID] = R_DrawSolidColorSpan_8;
		spanfuncs[SPANDRAWFUNC_TRANSSOLID] = R_DrawTransSolidColorSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDSOLID] = R_DrawTiltedSolidColorSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDTRANSSOLID] = R_DrawTiltedTransSolidColorSpan_8;
		spanfuncs[SPANDRAWFUNC_WATERSOLID] = R_DrawWaterSolidColorSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDWATERSOLID] = R_DrawTiltedWaterSolidColorSpan_8;
		spanfuncs[SPANDRAWFUNC_FOG] = R_DrawFogSpan_8;
		spanfuncs[SPANDRAWFUNC_TILTEDFOG] = R_DrawTiltedFogSpan_8;

		spanfuncs_npo2[BASEDRAWFUNC] = R_DrawSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTED] = R_DrawTiltedSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTiltedTranslucentSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_SPLAT] = R_DrawSplat_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawTiltedSplat_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawTiltedFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTiltedTranslucentFloorSprite_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_WATER] = R_DrawWaterSpan_NPO2_8;
		spanfuncs_npo2[SPANDRAWFUNC_TILTEDWATER] = R_DrawTiltedWaterSpan_NPO2_8;
	}
	else
		I_Error("unknown bytes per pixel mode %d\n", vid.bpp);
}

void SCR_SetMode(void)
{
	if (dedicated)
		return;

	if (!(setmodeneeded || setrenderneeded) || WipeInAction)
		return; // should never happen and don't change it during a wipe, BAD!

	// Lactozilla: Renderer switching
	if (setrenderneeded)
	{
		// stop recording movies (APNG only)
		if (setrenderneeded && (moviemode == MM_APNG))
			M_StopMovie();

		// VID_SetMode will call VID_CheckRenderer itself,
		// so no need to do this in here.
		if (!setmodeneeded)
			VID_CheckRenderer();

		vid.recalc = 1;
	}

	// Set the video mode in the video interface.
	if (setmodeneeded)
		VID_SetMode(setmodeneeded - 1);

	V_SetPalette(0);

	SCR_SetDrawFuncs();

	// set the apprpriate drawer for the sky (tall or INT16)
	setmodeneeded = 0;
	setrenderneeded = 0;
}

// do some initial settings for the game loading screen
//
void SCR_Startup(void)
{
	if (dedicated)
	{
		V_Init();
		V_SetPalette(0);
		return;
	}

	vid.modenum = 0;

	V_Init();
	V_Recalc();

	CV_RegisterVar(&cv_ticrate);
	CV_RegisterVar(&cv_constextsize);

	V_SetPalette(0);
}

// Called at new frame, if the video mode has changed
//
void SCR_Recalc(void)
{
	if (dedicated)
		return;

	// bytes per pixel quick access
	scr_bpp = vid.bpp;

	V_Recalc();

	// toggle off (then back on) the automap because some screensize-dependent values will
	// be calculated next time the automap is activated.
	if (automapactive)
	{
		am_recalc = true;
		AM_Start();
	}

	// set the screen[x] ptrs on the new vidbuffers
	V_Init();

	// scr_viewsize doesn't change, neither detailLevel, but the pixels
	// per screenblock is different now, since we've changed resolution.
	R_SetViewSize(); //just set setsizeneeded true now ..

	// vid.recalc lasts only for the next refresh...
	con_recalc = true;
	am_recalc = true;

#ifdef HWRENDER
	// Shoot! The screen texture was flushed!
	if ((rendermode == render_opengl) && (gamestate == GS_INTERMISSION))
		usebuffer = false;
#endif
}

// Check for screen cmd-line parms: to force a resolution.
//
// Set the video mode to set at the 1st display loop (setmodeneeded)
//

void SCR_CheckDefaultMode(void)
{
	INT32 scr_forcex, scr_forcey; // resolution asked from the cmd-line

	scr_startupmodeset = true;

	if (dedicated)
		return;

	// 0 means not set at the cmd-line
	scr_forcex = scr_forcey = 0;

	if (M_CheckParm("-width") && M_IsNextParm())
		scr_forcex = atoi(M_GetNextParm());

	if (M_CheckParm("-height") && M_IsNextParm())
		scr_forcey = atoi(M_GetNextParm());

#ifdef NATIVESCREENRES
	if (cv_nativeres.value)
		SCR_CheckNativeMode();
#endif

	if (scr_forcex && scr_forcey)
	{
		CONS_Printf(M_GetText("Using resolution: %d x %d\n"), scr_forcex, scr_forcey);
		// returns -1 if not found, thus will be 0 (no mode change) if not found
		setmodeneeded = VID_GetModeForSize(scr_forcex, scr_forcey) + 1;
	}
	else
	{
		CONS_Printf(M_GetText("Default resolution: %d x %d\n"), cv_scr_width.value, cv_scr_height.value);
		CONS_Printf(M_GetText("Windowed resolution: %d x %d\n"), cv_scr_width_w.value, cv_scr_height_w.value);
		CONS_Printf(M_GetText("Default bit depth: %d bits\n"), cv_scr_depth.value);
		if (cv_fullscreen.value)
			setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1; // see note above
		else
			setmodeneeded = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value) + 1; // see note above

		if (setmodeneeded <= 0)
			CONS_Alert(CONS_WARNING, "Invalid resolution given, defaulting to base resolution\n");
	}

	if (cv_renderer.value != (signed)rendermode)
	{
		if (chosenrendermode == render_none) // nothing set at command line
			SCR_ChangeRenderer();
		else
		{
			// Set cv_renderer to the current render mode
			CV_StealthSetValue(&cv_renderer, rendermode);
		}
	}
}

// sets the modenum as the new default video mode to be saved in the config file
void SCR_SetDefaultMode(void)
{
	CV_SetValue(cv_fullscreen.value ? &cv_scr_width : &cv_scr_width_w, vid.width);
	CV_SetValue(cv_fullscreen.value ? &cv_scr_height : &cv_scr_height_w, vid.height);
}

// Set the mode number based on the resolution saved in the config
void SCR_SetModeFromConfig(void)
{
	setmodeneeded = max(0, VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value)) + 1;
}

// Change fullscreen on/off according to cv_fullscreen
void SCR_ChangeFullscreen(void)
{
#ifdef DIRECTFULLSCREEN
	// allow_fullscreen is set by VID_PrepareModeList
	// it is used to prevent switching to fullscreen during startup
	if (!allow_fullscreen)
		return;

	if (graphics_started)
	{
		VID_PrepareModeList();

		if (cv_fullscreen.value)
			setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
		else
			setmodeneeded = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value) + 1;

		if (setmodeneeded <= 0) // hacky safeguard
		{
			CONS_Alert(CONS_WARNING, "Invalid resolution given, defaulting to base resolution.\n");
			setmodeneeded = VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT) + 1;
		}
	}
	return;
#endif
}

void SCR_ChangeRenderer(void)
{
	if (chosenrendermode != render_none
	|| (signed)rendermode == cv_renderer.value)
		return;

#ifdef HWRENDER
	// Check if OpenGL loaded successfully (or wasn't disabled) before switching to it.
	if ((vid.glstate == VID_GL_LIBRARY_ERROR)
	&& (cv_renderer.value == render_opengl))
	{
		if (M_CheckParm("-nogl"))
			CONS_Alert(CONS_ERROR, "OpenGL rendering was disabled!\n");
		else
		{
			renderswitcherror = render_opengl;
			CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");
		}

		return;
	}

	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED)) // Clear these out before switching to software
		HWR_ClearAllTextures();
#endif

	// Set the new render mode
	setrenderneeded = cv_renderer.value;
}

boolean SCR_IsAspectCorrect(INT32 width, INT32 height)
{
	return
	 (  width % BASEVIDWIDTH == 0
	 && height % BASEVIDHEIGHT == 0
	 && width / BASEVIDWIDTH == height / BASEVIDHEIGHT
	 );
}

#ifdef NATIVESCREENRES
void SCR_CheckNativeMode(void)
{
	INT32 w, h;

	VID_GetNativeResolution(&w, &h);

	if (w || h)
		SCR_SetMaxNativeResDivider(SCR_GetMaxNativeResDivider(w, h));

	if (cv_nativeresauto.value)
		scr_resdiv = SCR_GetNativeResDivider(w, h);
	else
		scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
}

void SCR_ResetNativeResDivider(void)
{
	float resdiv = atof(cv_nativeresdiv.defaultvalue);
	char f[9];

	scr_resdiv = resdiv;

	snprintf(f, sizeof(f), "%.6f", resdiv);
	CV_StealthSet(&cv_nativeresdiv, cv_nativeresdiv.defaultvalue);
}

static void SCR_ToggleNativeRes(void)
{
	INT32 mode = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value);
	if (mode == -1)
		mode = VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT);

	setmodeneeded = mode + 1;
	scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
}

static void SCR_NativeResDivChanged(void)
{
	CV_StealthSetValue(&cv_nativeresauto, 0);
	CV_StealthSetValue(&cv_nativeres, 1);
	SCR_ToggleNativeRes();
}

static void SCR_NativeResAutoChanged(void)
{
	if (!scr_startupmodeset)
		return;

	if (cv_nativeresauto.value)
	{
		INT32 w = 0, h = 0;
		char f[16];

		// Set for next resolution change
		VID_GetNativeResolution(&w, &h);
		scr_resdiv = SCR_GetNativeResDivider(w, h);

		// Stealth change current resolution divider variable
		snprintf(f, sizeof(f), "%.6f", scr_resdiv);
		CV_StealthSet(&cv_nativeresdiv, f);
	}
	else
		SCR_ResetNativeResDivider();

	if (cv_nativeres.value)
		SCR_SetModeFromConfig();
}

#define RESDIVFACTOR (1.0f / 16.0f)

static INT32 SCR_CalcDup(INT32 width, INT32 height)
{
	INT32 dupx = max(1, width / BASEVIDWIDTH);
	INT32 dupy = max(1, height / BASEVIDHEIGHT);

	if (!cv_nativerescompare.value)
		return (dupx >= dupy ? dupx : dupy);
	else
		return (dupx < dupy ? dupx : dupy);
}

float SCR_GetNativeResDivider(INT32 width, INT32 height)
{
	if (cv_nativeresauto.value)
	{
		float w = (float)width;
		float h = (float)height;
		float wsize, hsize;
		float div = 1.0f;

		while (true)
		{
			INT32 iw, ih;
			INT32 dup, corner;

			wsize = (w / div);
			hsize = (h / div);

			iw = (INT32)wsize;
			ih = (INT32)hsize;

			dup = SCR_CalcDup(iw, ih);
			corner = (iw - (BASEVIDWIDTH * dup)) / 2;

			if (corner < iw / 5)
				break;

			if (wsize <= BASEVIDWIDTH || hsize <= BASEVIDHEIGHT)
				break;

			div += 0.25f;
		}

		return min(div, FixedToFloat(nativeresdiv_cons_t[1].value));
	}

	return FixedToFloat(cv_nativeresdiv.value);
}

float SCR_GetMaxNativeResDivider(INT32 nw, INT32 nh)
{
	float w, h;
	float div = 1.0f;

	if (!nw || !nh)
		VID_GetNativeResolution(&nw, &nh);

	w = (float)nw;
	h = (float)nh;

	while (true)
	{
		w = ((float)nw / div);
		h = ((float)nh / div);
		if (w <= (INT32)BASEVIDWIDTH || h <= (INT32)BASEVIDHEIGHT)
			return div;
		div += RESDIVFACTOR;
	}

	return div;
}

void SCR_SetMaxNativeResDivider(float max)
{
	nativeresdiv_cons_t[1].value = FloatToFixed(max);
}
#endif

double averageFPS = 0.0f;

#define USE_FPS_SAMPLES

#ifdef USE_FPS_SAMPLES
#define MAX_FRAME_TIME 0.05
#define NUM_FPS_SAMPLES (16) // Number of samples to store

static double total_frame_time = 0.0;
static int frame_index;
#endif

static boolean fps_init = false;
static precise_t fps_enter = 0;

void SCR_CalculateFPS(void)
{
	precise_t fps_finish = 0;

	double frameElapsed = 0.0;

	if (fps_init == false)
	{
		fps_enter = I_GetPreciseTime();
		fps_init = true;
	}

	fps_finish = I_GetPreciseTime();
	frameElapsed = (double)((INT64)(fps_finish - fps_enter)) / I_GetPrecisePrecision();
	fps_enter = fps_finish;

#ifdef USE_FPS_SAMPLES
	total_frame_time += frameElapsed;
	if (frame_index++ >= NUM_FPS_SAMPLES || total_frame_time >= MAX_FRAME_TIME)
	{
		averageFPS = 1.0 / (total_frame_time / frame_index);
		total_frame_time = 0.0;
		frame_index = 0;
	}
#else
	// Direct, unsampled counter.
	averageFPS = 1.0 / frameElapsed;
#endif
}

void SCR_DisplayTicRate(void)
{
	INT32 ticcntcolor = 0;
	const INT32 h = vid.height-(8*vid.dup);
	UINT32 cap = R_GetFramerateCap();
	double fps = round(averageFPS);

	if (gamestate == GS_NULL)
		return;

	if (cap > 0)
	{
		if (fps <= cap / 2.0) ticcntcolor = V_REDMAP;
		else if (fps <= cap * 0.90) ticcntcolor = V_YELLOWMAP;
		else ticcntcolor = V_GREENMAP;
	}
	else
	{
		ticcntcolor = V_GREENMAP;
	}

	if (cv_ticrate.value == 2) // compact counter
	{
		V_DrawRightAlignedString(vid.width, h,
			ticcntcolor|V_NOSCALESTART|V_USERHUDTRANS, va("%04.2f", averageFPS)); // use averageFPS directly
	}
	else if (cv_ticrate.value == 1) // full counter
	{
		const char *drawnstr;
		INT32 width;

		// The highest assignable cap is < 1000, so 3 characters is fine.
		if (cap > 0)
			drawnstr = va("%3.0f/%3u", fps, cap);
		else
			drawnstr = va("%4.2f", averageFPS);

		width = V_StringWidth(drawnstr, V_NOSCALESTART);

		V_DrawString(vid.width - ((7 * 8 * vid.dup) + V_StringWidth("FPS: ", V_NOSCALESTART)), h,
			V_YELLOWMAP|V_NOSCALESTART|V_USERHUDTRANS, "FPS:");
		V_DrawString(vid.width - width, h,
			ticcntcolor|V_NOSCALESTART|V_USERHUDTRANS, drawnstr);
	}
}

void SCR_DisplayLocalPing(void)
{
	UINT32 ping = playerpingtable[consoleplayer];	// consoleplayer's ping is everyone's ping in a splitnetgame :P
	if (cv_showping.value == 1 || (cv_showping.value == 2 && servermaxping && ping > servermaxping))	// only show 2 (warning) if our ping is at a bad level
	{
		INT32 dispy = cv_ticrate.value ? 180 : 189;
		HU_drawPing(307, dispy, ping, true, V_SNAPTORIGHT | V_SNAPTOBOTTOM);
	}
}

void SCR_ClosedCaptions(void)
{
	UINT8 i;
	boolean gamestopped = (paused || P_AutoPause());
	INT32 basey = BASEVIDHEIGHT - 20;

	if (gamestate != wipegamestate)
		return;

	if (gamestate == GS_LEVEL)
	{
		if (promptactive)
			basey -= 42;
		else if (splitscreen)
			basey -= 8;
		else if ((modeattacking == ATTACKING_NIGHTS)
		|| (!(maptol & TOL_NIGHTS)
		&& LUA_HudEnabled(hud_powerups)
		&& ((cv_powerupdisplay.value == 2) // "Always"
		 || (cv_powerupdisplay.value == 1 && !camera.chase)))) // "First-person only"
			basey -= 16;
	}

	for (i = 0; i < NUMCAPTIONS; i++)
	{
		INT32 flags;
		fixed_t y;
		char dot;
		boolean music;
		const char *caption;

		if (!closedcaptions[i].s)
			continue;

		music = (closedcaptions[i].s-S_sfx == sfx_None);

		if (music && !gamestopped && (closedcaptions[i].t < flashingtics) && (closedcaptions[i].t & 1))
			continue;

		flags = V_SNAPTORIGHT|V_SNAPTOBOTTOM|V_ALLOWLOWERCASE;
		// bitten fix this
#if 0
		x = BASEVIDWIDTH - 20;
		y = basey-((i + 2)*h);
#else
		y = (basey-(i*10)) * FRACUNIT;
#endif

		if (closedcaptions[i].b)
		{
			if (renderisnewtic)
				closedcaptions[i].b--;

			if (closedcaptions[i].b) // If the caption hasn't reached its final destination...
			{
				y -= closedcaptions[i].b * 4 * FRACUNIT; // ...move it per tic...
				y += (rendertimefrac % FRACUNIT) * 4; // ...and interpolate it per frame
				// We have to modulo it by FRACUNIT, so that it won't be a tic ahead with interpolation disabled
				// Unlike everything else, captions are (intentionally) interpolated from T to T+1 instead of T-1 to T
			}
		}

		if (closedcaptions[i].t < CAPTIONFADETICS)
			flags |= (((CAPTIONFADETICS-closedcaptions[i].t)/2)*V_10TRANS);

		if (music)
			dot = '\x19';
		else if (closedcaptions[i].c && closedcaptions[i].c->origin)
			dot = '\x1E';
		else
			dot = ' ';

			// bitten note fix
#if 0
		caption = va("%c [%s]", dot, (closedcaptions[i].s->caption[0] ? closedcaptions[i].s->caption : closedcaptions[i].s->name));
		V_DrawRightAlignedString(x, y, flags, caption);
#else
		V_DrawRightAlignedStringAtFixed((BASEVIDWIDTH-20) * FRACUNIT, y, flags,
			va("%c [%s]", dot, (closedcaptions[i].s->caption[0] ? closedcaptions[i].s->caption : closedcaptions[i].s->name)));
#endif
	}
}

void SCR_DisplayMarathonInfo(void)
{
	INT32 flags = V_SNAPTOBOTTOM;
	static tic_t entertic, oldentertics = 0, antisplice[2] = {48,0};
	const char *str;
#if 0 // eh, this probably isn't going to be a problem
	if (((signed)marathontime) < 0)
	{
		flags |= V_REDMAP;
		str = "No waiting out the clock to submit a bogus time.";
	}
	else
#endif
	{
		entertic = I_GetTime();
		if (gamecomplete)
			flags |= V_YELLOWMAP;
		else if (marathonmode & MA_INGAME)
			; // see also G_Ticker
		else if (marathonmode & MA_INIT)
			marathonmode &= ~MA_INIT;
		else
			marathontime += entertic - oldentertics;

		// Create a sequence of primes such that their LCM is nice and big.
#define PRIMEV1 13
#define PRIMEV2 17 // I can't believe it! I'm on TV!
		antisplice[0] += (entertic - oldentertics)*PRIMEV2;
		antisplice[0] %= PRIMEV1*((vid.width/vid.dup)+1);
		antisplice[1] += (entertic - oldentertics)*PRIMEV1;
		antisplice[1] %= PRIMEV1*((vid.width/vid.dup)+1);
		str = va("%i:%02i:%02i.%02i",
			G_TicsToHours(marathontime),
			G_TicsToMinutes(marathontime, false),
			G_TicsToSeconds(marathontime),
			G_TicsToCentiseconds(marathontime));
		oldentertics = entertic;
	}
	V_DrawFill((antisplice[0]/PRIMEV1)-1, BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT);
	V_DrawFill((antisplice[0]/PRIMEV1),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT|31);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)-1), BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT|31);
#undef PRIMEV1
#undef PRIMEV2
	V_DrawPromptBack(-8, cons_backcolor.value);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-8, flags, str);
}
