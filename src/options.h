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

/*
 * Eventually this should include all configuration stuff, 
 * for now there's few options which indicate 3do/pc flavors.
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include "port.h"
#include "libs/compiler.h"
#include "libs/uio.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define OPT_3DO 0x01
#define OPT_PC  0x02
#define OPT_ALL 0xFF

extern int optWhichCoarseScan;
extern int optWhichMenu;
extern int optWhichFonts;
extern int optWhichIntro;
extern int optWhichShield;
extern int optSmoothScroll;
extern int optMeleeScale;

// JMS_GFX
extern unsigned int loresBlowupScale;
extern unsigned int resolutionFactor;
extern unsigned int audioDriver;
extern unsigned int audioQuality;
extern BOOLEAN optRequiresReload;
extern BOOLEAN optRequiresRestart;

extern BOOLEAN optCheatMode; // JMS
// Serosis
extern BOOLEAN optGodMode;
extern int timeDilationScale;
extern BOOLEAN optBubbleWarp;
extern BOOLEAN optUnlockShips;
extern BOOLEAN optHeadStart;
extern BOOLEAN optUnlockUpgrades;
extern BOOLEAN optInfiniteRU;
extern DWORD oldRU;
extern BOOLEAN optSkipIntro;
// JMS
extern BOOLEAN optMainMenuMusic;
extern BOOLEAN optNebulae;
extern BOOLEAN optOrbitingPlanets;
extern BOOLEAN optTexturedPlanets;
// Nic
extern int optDateFormat;
// Serosis
extern BOOLEAN optInfiniteFuel;
extern DWORD loadFuel;
extern BOOLEAN optPartialPickup;
extern BOOLEAN optSubmenu;
extern BOOLEAN optAddDevices;
extern BOOLEAN optScalePlanets;
extern BOOLEAN optSuperMelee;
extern BOOLEAN optLoadGame;
extern BOOLEAN optCustomBorder;
extern int optCustomSeed;
extern int spaceMusicBySOI;
extern BOOLEAN optSpaceMusic;
extern BOOLEAN optVolasMusic;
extern BOOLEAN optWholeFuel;
extern BOOLEAN optDirectionalJoystick; // For Android
extern BOOLEAN optLanderHold;
extern int optIPScaler;
extern int optDifficulty;
extern BOOLEAN optFuelRange;
extern BOOLEAN optExtended;
extern BOOLEAN optNomad;

extern BOOLEAN opt3doMusic;
extern BOOLEAN optRemixMusic;
extern BOOLEAN optSpeech;
extern BOOLEAN optSubtitles;
extern BOOLEAN optStereoSFX;
extern BOOLEAN optKeepAspectRatio;

extern BOOLEAN restartGame;

#define GAMMA_SCALE  1000
extern float optGamma;

extern uio_DirHandle *contentDir;
extern uio_DirHandle *configDir;
extern uio_DirHandle *saveDir;
extern uio_DirHandle *meleeDir;
extern char baseContentPath[PATH_MAX];

extern const char **optAddons;

/* These get edited by TEXTENTRY widgets, so they should have room to
 * hold as much as one of them allows by default. */
typedef struct _input_template {
	char name[30];

	/* This should eventually also hold things like Joystick Port
	 * and whether or not joysticks are enabled at all, and
	 * possibly the whole configuration scheme.  If we do that, we
	 * can actually ditch much of VControl. */
} INPUT_TEMPLATE;

extern INPUT_TEMPLATE input_templates[6];

void prepareContentDir (const char *contentDirName, const char *addonDirName, const char *execFile);
void prepareConfigDir (const char *configDirName);
void prepareMeleeDir (void);
void prepareSaveDir (void);
void prepareAddons (const char **addons);
void prepareShadowAddons (const char **addons);
void unprepareAllDirs (void);

BOOLEAN loadAddon (const char *addon);
int loadIndices (uio_DirHandle *baseDir);

bool setGammaCorrection (float gamma);

#if defined(__cplusplus)
}
#endif

#endif

