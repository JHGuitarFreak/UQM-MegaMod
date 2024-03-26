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
#include "uqm/setupmenu.h"

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
extern unsigned int loresBlowupScale;
extern unsigned int resolutionFactor;
extern unsigned int audioDriver;
extern unsigned int audioQuality;

// Added options
extern BOOLEAN optRequiresReload;
extern BOOLEAN optRequiresRestart;
extern OPT_ENABLABLE optCheatMode;
extern int optGodModes;
extern int timeDilationScale;
extern OPT_ENABLABLE optBubbleWarp;
extern OPT_ENABLABLE optUnlockShips;
extern OPT_ENABLABLE optHeadStart;
extern OPT_ENABLABLE optUnlockUpgrades;
extern OPT_ENABLABLE optInfiniteRU;
extern DWORD oldRU;
extern OPT_ENABLABLE optSkipIntro;
extern OPT_ENABLABLE optMainMenuMusic;
extern OPT_ENABLABLE optNebulae;
extern OPT_ENABLABLE optOrbitingPlanets;
extern OPT_ENABLABLE optTexturedPlanets;
extern int optDateFormat;
extern OPT_ENABLABLE optInfiniteFuel;
extern DWORD loadFuel;
extern OPT_ENABLABLE optPartialPickup;
extern OPT_ENABLABLE optSubmenu;
extern OPT_ENABLABLE optAddDevices;
extern BOOLEAN optSuperMelee;
extern BOOLEAN optLoadGame;
extern OPT_ENABLABLE optCustomBorder;
extern int optCustomSeed;
extern int spaceMusicBySOI;
extern OPT_ENABLABLE optSpaceMusic;
extern OPT_ENABLABLE optVolasMusic;
extern OPT_ENABLABLE optWholeFuel;
extern OPT_ENABLABLE optDirectionalJoystick;
extern int optLanderHold;
extern int optScrTrans;
extern int optDifficulty;
extern int optDiffChooser;
extern int optFuelRange;
extern OPT_ENABLABLE optExtended;
extern OPT_ENABLABLE optNomad;
extern OPT_ENABLABLE optGameOver;
extern OPT_ENABLABLE optShipDirectionIP;
extern OPT_ENABLABLE optHazardColors;
extern OPT_ENABLABLE optOrzCompFont;
extern int optControllerType;
extern OPT_ENABLABLE optSmartAutoPilot;
extern int optTintPlanSphere;
extern int optPlanetStyle;
extern int optStarBackground;
extern int optScanStyle;
extern OPT_ENABLABLE optNonStopOscill;
extern int optScopeStyle;
extern int optSuperPC;
extern OPT_ENABLABLE optHyperStars;
extern OPT_ENABLABLE optPlanetTexture;
extern int optFlagshipColor;
extern OPT_ENABLABLE optNoHQEncounters;
extern OPT_ENABLABLE optDeCleansing;
extern OPT_ENABLABLE optMeleeObstacles;
extern OPT_ENABLABLE optShowVisitedStars;
extern OPT_ENABLABLE optUnscaledStarSystem;
extern int optScanSphere;
extern int optNebulaeVolume;
extern OPT_ENABLABLE optSlaughterMode;
extern BOOLEAN optMaskOfDeceit;
extern OPT_ENABLABLE optAdvancedAutoPilot;
extern OPT_ENABLABLE optMeleeToolTips;
extern int optMusicResume;
extern DWORD optWindowType;

extern OPT_ENABLABLE opt3doMusic;
extern OPT_ENABLABLE optRemixMusic;
extern OPT_ENABLABLE optSpeech;
extern OPT_ENABLABLE optSubtitles;
extern OPT_ENABLABLE optStereoSFX;
extern OPT_ENABLABLE optKeepAspectRatio;
extern BOOLEAN restartGame;

#define GAMMA_SCALE  1000
extern float optGamma;

extern uio_DirHandle *contentDir;
extern uio_DirHandle *configDir;
extern uio_DirHandle *saveDir;
extern uio_DirHandle *meleeDir;
extern uio_DirHandle *scrShotDir;
extern char baseContentPath[PATH_MAX];

extern char *contentDirPath;
extern char *addonDirPath;

extern const char **optAddons;

// addon availability
typedef struct
{
	DWORD name_hash[PATH_MAX];
	DWORD amount;
} ADDON_COUNT;

extern ADDON_COUNT addonList;

// addon names to check against
#define THREEDO_MUSIC "3domusic"
#define REMIX_MUSIC   "remix"
#define VOL_RMX_MUSIC "volasaurus-remix-pack"
#define REGION_MUSIC  "SpaceMusic"
#define HD_MODE       "mm-hd"

#define DOS_MODE(a)     ((a) ? "dos-mode-hd" : "dos-mode-sd")
#define THREEDO_MODE(a) ((a) ? "3do-mode-hd" : "3do-mode-sd")

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
void prepareScrShotDir (void);
void prepareAddons (const char **addons);
void prepareShadowAddons (const char **addons);
void unprepareAllDirs (void);

BOOLEAN loadAddon (const char *addon);
int loadIndices (uio_DirHandle *baseDir);
BOOLEAN isAddonAvailable (const char *addon_name);

bool setGammaCorrection (float gamma);

#if defined(__cplusplus)
}
#endif

#endif