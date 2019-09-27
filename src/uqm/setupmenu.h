// Copyright Michael Martin, 2004.

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef UQM_SETUPMENU_H_
#define UQM_SETUPMENU_H_

#include "controls.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	OPTVAL_DISABLED,
	OPTVAL_ENABLED
} OPT_ENABLABLE;

typedef enum {
	OPTVAL_PC,
	OPTVAL_3DO
} OPT_CONSOLETYPE;

typedef enum {
	OPTVAL_NO_SCALE,
	OPTVAL_BILINEAR_SCALE,
	OPTVAL_BIADAPT_SCALE,
	OPTVAL_BIADV_SCALE,
	OPTVAL_TRISCAN_SCALE,
	OPTVAL_HQXX_SCALE,
} OPT_SCALETYPE;

typedef enum {
	OPTVAL_320_240,
	OPTVAL_REAL_1280_960, // JMS_GFX
} OPT_RESTYPE;

typedef enum {
	NO_BLOWUP,
	OPTVAL_SCALE_640_480,
	OPTVAL_SCALE_960_720,
	OPTVAL_SCALE_1280_960,
	OPTVAL_SCALE_1600_1200,
	OPTVAL_SCALE_1920_1440,
} OPT_RESSCALER;

typedef enum {
	OPTVAL_PURE_IF_POSSIBLE,
	OPTVAL_ALWAYS_GL
} OPT_DRIVERTYPE;

typedef enum {
	OPTVAL_SILENCE,
	OPTVAL_MIXSDL,
	OPTVAL_OPENAL
} OPT_ADRIVERTYPE;

typedef enum {
	OPTVAL_LOW,
	OPTVAL_MEDIUM,
	OPTVAL_HIGH
} OPT_AQUALITYTYPE;
 
typedef enum {
	OPTVAL_NORMAL,
	OPTVAL_SLOW,
	OPTVAL_FAST
} OPT_TDTYPE;

typedef enum {
	OPTVAL_MMMDDYYYY,
	OPTVAL_MMDDYYYY,
	OPTVAL_DDMMMYYYY,
	OPTVAL_DDMMYYYY
} OPT_DATETYPE;

typedef enum {
	OPTVAL_NORM,
	OPTVAL_EASY,
	OPTVAL_HARD,
	OPTVAL_IMPO
} OPT_DIFFICULTY;

typedef enum {
	OPTVAL_STEP,
	OPTVAL_NEAREST,
	OPTVAL_BILINEAR,
	OPTVAL_TRILINEAR
} OPT_MELEEZOOM;

/* At the moment, CONTROL_TEMPLATE is directly in this structure.  If
 * CONTROL_TEMPLATE and the options available diverge, this will need
 * to change */
typedef struct globalopts_struct {
	OPT_SCALETYPE scaler;
	OPT_RESTYPE screenResolution;
	OPT_RESSCALER loresBlowup;
	OPT_DRIVERTYPE driver;
	OPT_ADRIVERTYPE adriver;
	OPT_AQUALITYTYPE aquality;
	OPT_TDTYPE tdType; // Serosis
	OPT_DATETYPE dateType; // Serosis
	OPT_DIFFICULTY difficulty; // Serosis
	OPT_MELEEZOOM meleezoom;
	OPT_ENABLABLE fullscreen, subtitles, scanlines, fps, stereo, music3do, musicremix, speech, keepaspect,
		cheatMode, godMode, bubbleWarp, unlockShips, headStart, unlockUpgrades, infiniteRU, skipIntro, // Serosis: except for cheatMode = JMS
		mainMenuMusic, nebulae, orbitingPlanets, texturedPlanets, // JMS
		infiniteFuel, partialPickup, submenu, addDevices, scalePlanets, customBorder, spaceMusic,	// Serosis
		volasMusic, directionalJoystick, wholeFuel, fuelRange, extended, nomad; // Serosis
	OPT_CONSOLETYPE menu, text, cscan, scroll, intro, shield, ipTrans, landerHold;
	CONTROL_TEMPLATE player1, player2;
	int speechvol, musicvol, sfxvol;
	int gamma, customSeed;
} GLOBALOPTS;

void SetupMenu (void);

void GetGlobalOptions (GLOBALOPTS *opts);
void SetGlobalOptions (GLOBALOPTS *opts);

#if defined(__cplusplus)
}
#endif

#endif // UQM_SETUPMENU_H_
