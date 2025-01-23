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

#ifndef __APK_MAIN__
#define __APK_MAIN__

#include "doomdef.h"
#include "command.h"
#include "st_stuff.h"

// Misc. Android Stuff
extern struct android_helpers_s
{
	INT32 demo_inputdrawn;
} android_helpers;

#define APK_ST_WEAPONS_X ((BASEVIDWIDTH / 2) - (NUM_WEAPONS * 10) - 6)
#define APK_ST_WEAPONS_Y 176 // HUD_LIVES
#define APK_ST_WEAPONS_W 20
#define APK_ST_WEAPONS_H 20

extern consvar_t cv_liveshudpos; // lives HUD position

boolean APK_ST_UseAltLivesHUD(void);
hudinfo_t *APK_ST_GetLivesHUDInfo(void);
boolean APK_ST_AltLivesHUDEnabled(void);

#endif // __APK_MAIN__
