// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by StarManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  apk_main.c
/// \brief Android Operating System

#include "apk_main.h"
#include "ts_draw.h"
#include "doomstat.h"
#include "st_stuff.h"

struct android_helpers_s android_helpers; // Misc. Android Stuff

static CV_PossibleValue_t liveshudpos_cons_t[] = {{0, "Bottom left"}, {1, "Top right"}, {2, "Automatic"}, {0, NULL}};
consvar_t cv_liveshudpos = CVAR_INIT ("liveshudpos", "Automatic", CV_SAVE, liveshudpos_cons_t, NULL);

//
// STATUS BAR CODE
//

boolean APK_ST_UseAltLivesHUD(void)
{
#ifdef TOUCHINPUTS
	if (cv_liveshudpos.value == 2)
		return TS_CanDrawButtons();
#endif
	return (cv_liveshudpos.value == 1);
}

hudinfo_t *APK_ST_GetLivesHUDInfo(void)
{
	if (APK_ST_UseAltLivesHUD())
		return &hudinfo[ANDROID_HUD_LIVES];
	return &hudinfo[HUD_LIVES];
}

boolean APK_ST_AltLivesHUDEnabled(void)
{
	return (APK_ST_UseAltLivesHUD() && !modeattacking);
}
