//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif

#ifdef HAVE_GETOPT_LONG
#	include <getopt.h>
#else
#	include "getopt/getopt.h"
#endif

#include <stdarg.h>
#include <errno.h>
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/cmap.h"
#include "libs/sound/sound.h"
#include "libs/input/input_common.h"
#include "libs/inplib.h"
#include "libs/tasklib.h"
#include "libs/scriptlib.h"
#include "libs/callback/alarm.h"
#include "uqm/controls.h"
#include "uqm/battle.h"
		// For BATTLE_FRAME_RATE
#include "libs/file.h"
#include "types.h"
#include "port.h"
#include "libs/memlib.h"
#include "libs/platform.h"
#include "libs/log.h"
#include "options.h"
#include "uqmversion.h"
#include "uqm/comm.h"
#ifdef NETPLAY
#	include "libs/callback.h"
#	include "libs/alarm.h"
#	include "libs/net.h"
#	include "uqm/supermelee/netplay/netoptions.h"
#	include "uqm/supermelee/netplay/netplay.h"
#endif
#include "uqm/setup.h"
#include "uqm/starcon.h"
#include "libs/math/random.h"
#include "uqm/imgui/uqm_imgui.h"

BOOLEAN restartGame;

#if defined (GFXMODULE_SDL)
#	include SDL_INCLUDE(SDL.h)
			// Including this is actually necessary on OSX.
#endif

#if defined(ANDROID) || defined(__ANDROID__)
#	include "SDL_main.h"
#endif

struct bool_option
{
	bool value;
	bool set;
};

struct int_option
{
	int value;
	bool set;
};

struct float_option
{
	float value;
	bool set;
};

struct options_struct
{
#define DECL_CONFIG_OPTION(type, name) \
	struct type##_option name

#define DECL_CONFIG_OPTION2(type, name, val1, val2) \
	struct { type val1; type val2; bool set; } name

	// Commandline-only options
	const char *logFile;
	enum {
		runMode_normal,
		runMode_usage,
		runMode_version,
	} runMode;

	const char *configDir;
	const char *contentDir;
	const char *addonDir;
	const char **addons;
	int numAddons;

	const char* graphicsBackend;
	
	// Commandline and user config options
	DECL_CONFIG_OPTION(bool,  opengl);
	DECL_CONFIG_OPTION2(int,  resolution, width, height);
	DECL_CONFIG_OPTION(int,   fullscreen);
	DECL_CONFIG_OPTION(bool,  scanlines);
	DECL_CONFIG_OPTION(int,   scaler);
	DECL_CONFIG_OPTION(bool,  showFps);
	DECL_CONFIG_OPTION(int,   keepAspectRatio);
	DECL_CONFIG_OPTION(float, gamma);
	DECL_CONFIG_OPTION(int,   soundDriver);
	DECL_CONFIG_OPTION(int,   soundQuality);
	DECL_CONFIG_OPTION(bool,  use3doMusic);
	DECL_CONFIG_OPTION(bool,  useRemixMusic);
	DECL_CONFIG_OPTION(bool,  useSpeech);
	DECL_CONFIG_OPTION(int,   whichCoarseScan);
	DECL_CONFIG_OPTION(int,   whichMenu);
	DECL_CONFIG_OPTION(int,   whichFonts);
	DECL_CONFIG_OPTION(int,   whichIntro);
	DECL_CONFIG_OPTION(int,   whichShield);
	DECL_CONFIG_OPTION(int,   smoothScroll);
	DECL_CONFIG_OPTION(int,   meleeScale);
	DECL_CONFIG_OPTION(bool,  subtitles);
	DECL_CONFIG_OPTION(bool,  stereoSFX);
	DECL_CONFIG_OPTION(float, musicVolumeScale);
	DECL_CONFIG_OPTION(float, sfxVolumeScale);
	DECL_CONFIG_OPTION(float, speechVolumeScale);
	DECL_CONFIG_OPTION(bool,  safeMode);
	DECL_CONFIG_OPTION(int,   resolutionFactor);
	DECL_CONFIG_OPTION(int,   loresBlowupScale);
 	DECL_CONFIG_OPTION(bool,  cheatMode);
	DECL_CONFIG_OPTION(int,   optGodModes);
	DECL_CONFIG_OPTION(int,   timeDilationScale);
	DECL_CONFIG_OPTION(bool,  bubbleWarp);
	DECL_CONFIG_OPTION(bool,  unlockShips);
	DECL_CONFIG_OPTION(bool,  headStart);
	DECL_CONFIG_OPTION(bool,  autoButtons);
	DECL_CONFIG_OPTION(bool,  infiniteRU);
	DECL_CONFIG_OPTION(bool,  skipIntro);
	DECL_CONFIG_OPTION(bool,  mainMenuMusic);
	DECL_CONFIG_OPTION(bool,  nebulae);
	DECL_CONFIG_OPTION(bool,  orbitingPlanets);
	DECL_CONFIG_OPTION(bool,  texturedPlanets);
	DECL_CONFIG_OPTION(int,   optDateFormat);
	DECL_CONFIG_OPTION(bool,  infiniteFuel);
	DECL_CONFIG_OPTION(bool,  partialPickup);
	DECL_CONFIG_OPTION(bool,  submenu);
	DECL_CONFIG_OPTION(bool,  infiniteCredits);
	DECL_CONFIG_OPTION(bool,  customBorder);
	DECL_CONFIG_OPTION(int,   seedType);
	DECL_CONFIG_OPTION(int,   customSeed);
	DECL_CONFIG_OPTION(bool,  shipSeed);
	DECL_CONFIG_OPTION(int,   sphereColors);
	DECL_CONFIG_OPTION(int,   spaceMusic);
	DECL_CONFIG_OPTION(bool,  volasMusic);
	DECL_CONFIG_OPTION(bool,  wholeFuel);
	DECL_CONFIG_OPTION(int,   dirJoyP1);
	DECL_CONFIG_OPTION(int,   landerHold);
	DECL_CONFIG_OPTION(int,   scrTrans);
	DECL_CONFIG_OPTION(int,   optDifficulty);
	DECL_CONFIG_OPTION(int,   optDiffChooser);
	DECL_CONFIG_OPTION(int,   optFuelRange);
	DECL_CONFIG_OPTION(bool,  extended);
	DECL_CONFIG_OPTION(int,   nomad);
	DECL_CONFIG_OPTION(bool,  gameOver);
	DECL_CONFIG_OPTION(bool,  shipDirectionIP);
	DECL_CONFIG_OPTION(bool,  hazardColors);
	DECL_CONFIG_OPTION(bool,  orzCompFont);
	DECL_CONFIG_OPTION(int,   optControllerType);
	DECL_CONFIG_OPTION(bool,  smartAutoPilot);
	DECL_CONFIG_OPTION(int,   tintPlanSphere);
	DECL_CONFIG_OPTION(int,   planetStyle);
	DECL_CONFIG_OPTION(int,   starBackground);
	DECL_CONFIG_OPTION(int,   scanStyle);
	DECL_CONFIG_OPTION(bool,  nonStopOscill);
	DECL_CONFIG_OPTION(int,   scopeStyle);
	DECL_CONFIG_OPTION(bool,  hyperStars);
	DECL_CONFIG_OPTION(int,   landerStyle);
	DECL_CONFIG_OPTION(bool,  planetTexture);
	DECL_CONFIG_OPTION(int,   flagshipColor);
	DECL_CONFIG_OPTION(bool,  noHQEncounters);
	DECL_CONFIG_OPTION(bool,  deCleansing);
	DECL_CONFIG_OPTION(bool,  meleeObstacles);
	DECL_CONFIG_OPTION(bool,  showVisitedStars);
	DECL_CONFIG_OPTION(bool,  unscaledStarSystem);
	DECL_CONFIG_OPTION(int,   sphereType);
	DECL_CONFIG_OPTION(int,   nebulaevol);
	DECL_CONFIG_OPTION(bool,  slaughterMode);
	DECL_CONFIG_OPTION(bool,  advancedAutoPilot);
	DECL_CONFIG_OPTION(bool,  meleeToolTips);
	DECL_CONFIG_OPTION(int,   musicResume);
	DECL_CONFIG_OPTION(int,   windowType);
	DECL_CONFIG_OPTION(bool,  scatterElements);
	DECL_CONFIG_OPTION(bool,  showUpgrades);
	DECL_CONFIG_OPTION(bool,  fleetPointSys);
	DECL_CONFIG_OPTION(bool,  shipStore);
	DECL_CONFIG_OPTION(bool,  captainNames);
	DECL_CONFIG_OPTION(bool,  dosMenus);
	DECL_CONFIG_OPTION(int,   hyperSpaceColor);
	DECL_CONFIG_OPTION(int,   deadZoneLeftP1);
	DECL_CONFIG_OPTION(int,   deadZoneRightP1);
	DECL_CONFIG_OPTION(int,   deadZoneLeftP2);
	DECL_CONFIG_OPTION(int,   deadZoneRightP2);
	DECL_CONFIG_OPTION(int,   dirJoyP2);
	DECL_CONFIG_OPTION(int,   mouseInput);
	DECL_CONFIG_OPTION(bool,  battleMouse);
	DECL_CONFIG_OPTION(int,   screenShots);
	DECL_CONFIG_OPTION(bool,  highVisOrbits);
	DECL_CONFIG_OPTION(bool,  highVisPlanets);

#define INIT_CONFIG_OPTION(name, val) \
	{ val, false }

#define INIT_CONFIG_OPTION2(name, val1, val2) \
	{ val1, val2, false }
};

struct option_list_value
{
	const char *str;
	int value;
};

static const struct option_list_value scalerList[] = 
{
	{"bilinear", TFB_GFXFLAGS_SCALE_BILINEAR},
	{"biadapt",  TFB_GFXFLAGS_SCALE_BIADAPT},
	{"biadv",    TFB_GFXFLAGS_SCALE_BIADAPTADV},
	{"triscan",  TFB_GFXFLAGS_SCALE_TRISCAN},
	{"hq",       TFB_GFXFLAGS_SCALE_HQXX},
	{"none",     0},
	{"no",       0}, /* uqm.cfg value */
	{NULL, 0}
};

static const struct option_list_value meleeScaleList[] = 
{
	{"smooth",   TFB_SCALE_TRILINEAR},
	{"3do",      TFB_SCALE_TRILINEAR},
	{"step",     TFB_SCALE_STEP},
	{"pc",       TFB_SCALE_STEP},
	{"bilinear", TFB_SCALE_BILINEAR},
	{"nearest",  TFB_SCALE_NEAREST},
	{NULL, 0}
};

static const struct option_list_value audioDriverList[] = 
{
	{"openal",  audio_DRIVER_OPENAL},
	{"mixsdl",  audio_DRIVER_MIXSDL},
	{"none",    audio_DRIVER_NOSOUND},
	{"nosound", audio_DRIVER_NOSOUND},
	{NULL, 0}
};

static const struct option_list_value audioQualityList[] = 
{
	{"low",    audio_QUALITY_LOW},
	{"medium", audio_QUALITY_MEDIUM},
	{"high",   audio_QUALITY_HIGH},
	{NULL, 0}
};

static const struct option_list_value choiceList[] = 
{
	{"pc",  OPT_PC},
	{"3do", OPT_3DO},
	{NULL, 0}
};

static const struct option_list_value accelList[] = 
{
	{"mmx",    PLATFORM_MMX},
	{"sse",    PLATFORM_SSE},
	{"3dnow",  PLATFORM_3DNOW},
	{"none",   PLATFORM_C},
	{"detect", PLATFORM_NULL},
	{NULL, 0}
};

// Looks up the given string value in the given list and passes
// the associated int value back. returns true if value was found.
// The list is terminated by a NULL 'str' value.
static bool lookupOptionValue (const struct option_list_value *list,
		const char *strval, int *ret);

// Error message buffer used for when we cannot use logging facility yet
static char errBuffer[512];

static void saveError (const char *fmt, ...)
		PRINTF_FUNCTION(1, 2);

static int parseOptions (int argc, char *argv[],
		struct options_struct *options);
static void getUserConfigOptions (struct options_struct *options);
static void usage (FILE *out, const struct options_struct *defaultOptions);
static int parseIntOption (const char *str, int *result,
		const char *optName);
static int parseFloatOption (const char *str, float *f,
		const char *optName);
static void parseIntVolume (int intVol, float *vol);
static int InvalidArgument (const char *supplied, const char *opt_name);
static const char *choiceOptString (const struct int_option *option);
static const char *boolOptString (const struct bool_option *option);
static const char *boolNotOptString (const struct bool_option *option);

#ifdef ANDROID
int SDL_main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
	struct options_struct options = {
		/* .logFile = */            NULL,
		/* .runMode = */            runMode_normal,
		/* .configDir = */          NULL,
		/* .contentDir = */         NULL,
		/* .addonDir = */           NULL,
		/* .addons = */             NULL,
		/* .numAddons = */          0,
		/* .graphicsBackend = */    NULL,

		INIT_CONFIG_OPTION(  opengl,            false ),
		INIT_CONFIG_OPTION2( resolution,        640, 480 ),
		INIT_CONFIG_OPTION(  fullscreen,        2 ),
		INIT_CONFIG_OPTION(  scanlines,         false ),
		INIT_CONFIG_OPTION(  scaler,            0 ),
		INIT_CONFIG_OPTION(  showFps,           false ),
		INIT_CONFIG_OPTION(  keepAspectRatio,   1 ),
		INIT_CONFIG_OPTION(  gamma,             1.0f ),
		INIT_CONFIG_OPTION(  soundDriver,       audio_DRIVER_MIXSDL ),
		INIT_CONFIG_OPTION(  soundQuality,      audio_QUALITY_HIGH ),
		INIT_CONFIG_OPTION(  use3doMusic,       true ),
		INIT_CONFIG_OPTION(  useRemixMusic,     false ),
		INIT_CONFIG_OPTION(  useSpeech,         true ),
		INIT_CONFIG_OPTION(  whichCoarseScan,   0 ),
		INIT_CONFIG_OPTION(  whichMenu,         OPT_PC ),
		INIT_CONFIG_OPTION(  whichFonts,        OPT_PC ),
		INIT_CONFIG_OPTION(  whichIntro,        OPT_PC ),
		INIT_CONFIG_OPTION(  whichShield,       OPT_PC ),
		INIT_CONFIG_OPTION(  smoothScroll,      OPT_PC ),
		INIT_CONFIG_OPTION(  meleeScale,        TFB_SCALE_TRILINEAR),
		INIT_CONFIG_OPTION(  subtitles,         true ),
		INIT_CONFIG_OPTION(  stereoSFX,         false ),
		INIT_CONFIG_OPTION(  musicVolumeScale,  1.0f ),
		INIT_CONFIG_OPTION(  sfxVolumeScale,    1.0f ),
		INIT_CONFIG_OPTION(  speechVolumeScale, 1.0f ),
		INIT_CONFIG_OPTION(  safeMode,          false ),
		// Begin MegaMod defaults
		INIT_CONFIG_OPTION(  resolutionFactor,  0 ),
		INIT_CONFIG_OPTION(  loresBlowupScale,  1),
		INIT_CONFIG_OPTION(  cheatMode,         false ),
		INIT_CONFIG_OPTION(  optGodModes,       0 ),
		INIT_CONFIG_OPTION(  timeDilationScale, 0 ),
		INIT_CONFIG_OPTION(  bubbleWarp,        false ),
		INIT_CONFIG_OPTION(  unlockShips,       false ),
		INIT_CONFIG_OPTION(  headStart,         false ),
		INIT_CONFIG_OPTION(  autoButtons,       true ),
		INIT_CONFIG_OPTION(  infiniteRU,        false ),
		INIT_CONFIG_OPTION(  skipIntro,         false ),
		INIT_CONFIG_OPTION(  mainMenuMusic,     true ),
		INIT_CONFIG_OPTION(  nebulae,           false ),
		INIT_CONFIG_OPTION(  orbitingPlanets,   false ),
		INIT_CONFIG_OPTION(  texturedPlanets,   false ),
		INIT_CONFIG_OPTION(  optDateFormat,     0 ),
		INIT_CONFIG_OPTION(  infiniteFuel,      false ),
		INIT_CONFIG_OPTION(  partialPickup,     false ),
		INIT_CONFIG_OPTION(  submenu,           false ),
		INIT_CONFIG_OPTION(  infiniteCredits,   false ),
		INIT_CONFIG_OPTION(  customBorder,      false ),
		INIT_CONFIG_OPTION(  seedType,          0 ),
		INIT_CONFIG_OPTION(  customSeed,        PrimeA ),
		INIT_CONFIG_OPTION(  shipSeed,          false ),
		INIT_CONFIG_OPTION(  sphereColors,      0 ),
		INIT_CONFIG_OPTION(  spaceMusic,        0 ),
		INIT_CONFIG_OPTION(  volasMusic,        false ),
		INIT_CONFIG_OPTION(  wholeFuel,         false ),
		INIT_CONFIG_OPTION(  dirJoyP1,          0 ),
		INIT_CONFIG_OPTION(  landerHold,        OPT_3DO ),
		INIT_CONFIG_OPTION(  scrTrans,          OPT_3DO ),
		INIT_CONFIG_OPTION(  optDifficulty,     0 ),
		INIT_CONFIG_OPTION(  optDiffChooser,    3 ),
		INIT_CONFIG_OPTION(  optFuelRange,      0 ),
		INIT_CONFIG_OPTION(  extended,          false ),
		INIT_CONFIG_OPTION(  nomad,             0 ),
		INIT_CONFIG_OPTION(  gameOver,          false ),
		INIT_CONFIG_OPTION(  shipDirectionIP,   false ),
		INIT_CONFIG_OPTION(  hazardColors,      false ),
		INIT_CONFIG_OPTION(  orzCompFont,       false ),
		INIT_CONFIG_OPTION(  optControllerType, 0 ),
		INIT_CONFIG_OPTION(  smartAutoPilot,    false ),
		INIT_CONFIG_OPTION(  tintPlanSphere,    OPT_3DO ),
		INIT_CONFIG_OPTION(  planetStyle,       OPT_3DO ),
		INIT_CONFIG_OPTION(  starBackground,    0 ),
		INIT_CONFIG_OPTION(  scanStyle,         OPT_3DO ),
		INIT_CONFIG_OPTION(  nonStopOscill,     false ),
		INIT_CONFIG_OPTION(  scopeStyle,        OPT_3DO ),
		INIT_CONFIG_OPTION(  hyperStars,        false ),
		INIT_CONFIG_OPTION(  landerStyle,       OPT_3DO ),
		INIT_CONFIG_OPTION(  planetTexture,     true ),
		INIT_CONFIG_OPTION(  flagshipColor,     OPT_PC ),
		INIT_CONFIG_OPTION(  noHQEncounters,    false ),
		INIT_CONFIG_OPTION(  deCleansing,       false ),
		INIT_CONFIG_OPTION(  meleeObstacles,    false ),
		INIT_CONFIG_OPTION(  showVisitedStars,  false ),
		INIT_CONFIG_OPTION(  unscaledStarSystem,false ),
		INIT_CONFIG_OPTION(  sphereType,        2 ),
		INIT_CONFIG_OPTION(  nebulaevol,        16 ),
		INIT_CONFIG_OPTION(  slaughterMode,     false ),
		INIT_CONFIG_OPTION(  advancedAutoPilot, false ),
		INIT_CONFIG_OPTION(  meleeToolTips,     false ),
		INIT_CONFIG_OPTION(  musicResume,       0 ),
		INIT_CONFIG_OPTION(  windowType,        2 ),
		INIT_CONFIG_OPTION(  scatterElements,   false ),
		INIT_CONFIG_OPTION(  showUpgrades,      false ),
		INIT_CONFIG_OPTION(  fleetPointSys,     false ),
		INIT_CONFIG_OPTION(  shipStore,         false ),
		INIT_CONFIG_OPTION(  captainNames,      false ),
		INIT_CONFIG_OPTION(  dosMenus,          false ),
		INIT_CONFIG_OPTION(  hyperSpaceColor,   OPT_3DO ),
		INIT_CONFIG_OPTION(  deadZoneLeftP1,    DEFAULT_DZONE ),
		INIT_CONFIG_OPTION(  deadZoneRightP1,   DEFAULT_DZONE ),
		INIT_CONFIG_OPTION(  deadZoneLeftP2,    DEFAULT_DZONE ),
		INIT_CONFIG_OPTION(  deadZoneRightP2,   DEFAULT_DZONE ),
		INIT_CONFIG_OPTION(  dirJoyP2,          0 ),
		INIT_CONFIG_OPTION(  mouseInput,        0 ),
		INIT_CONFIG_OPTION(  battleMouse,       false ),
		INIT_CONFIG_OPTION(  screenShots,       0 ),
		INIT_CONFIG_OPTION(  highVisOrbits,     false ),
		INIT_CONFIG_OPTION(  highVisPlanets,    false ),
	};
	struct options_struct defaults = options;
	int optionsResult;
	int gfxDriver;
	int gfxFlags;
	int i;

	// NOTE: we cannot use the logging facility yet because we may have to
	//   log to a file, and we'll only get the log file name after parsing
	//   the options.
	optionsResult = parseOptions (argc, argv, &options);

	log_init (15);

	if (options.logFile != NULL)
	{
		int i;
		if (!freopen (options.logFile, "w", stderr))
		{
			printf ("Error %d calling freopen() on stderr\n", errno);
			return EXIT_FAILURE;
		}
#ifdef UNBUFFERED_LOGFILE
		setbuf (stderr, NULL);
#endif
		for (i = 0; i < argc; ++i)
			log_add (log_User, "argv[%d] = [%s]", i, argv[i]);
	}

	if (options.runMode == runMode_version)
	{
		printf ("%d.%d.%d %s\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
				UQM_PATCH_VERSION,
				(resolutionFactor ? "HD " UQM_EXTRA_VERSION :
					UQM_EXTRA_VERSION));
		log_showBox (false, false);
		return EXIT_SUCCESS;
	}
	
	log_add (log_User, "The Ur-Quan Masters v%d.%d.%d %s (compiled %s %s)\n"
			"This software comes with ABSOLUTELY NO WARRANTY;\n"
			"for details see the included 'COPYING' file.\n",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION,
			(resolutionFactor ? "HD " UQM_EXTRA_VERSION : UQM_EXTRA_VERSION),
			__DATE__, __TIME__);
#ifdef NETPLAY
	log_add (log_User, "Netplay protocol version %d.%d. Netplay opponent "
			"must have UQM %d.%d.%d or later.\n",
			NETPLAY_PROTOCOL_VERSION_MAJOR, NETPLAY_PROTOCOL_VERSION_MINOR,
			NETPLAY_MIN_UQM_VERSION_MAJOR, NETPLAY_MIN_UQM_VERSION_MINOR,
			NETPLAY_MIN_UQM_VERSION_PATCH);
#endif

	 // Compiler info to help with future debugging.
#ifdef _MSC_VER
		log_add (log_Info, "MSC_VER: %d", _MSC_VER);
		log_add (log_Info, "MSC_FULL_VER: %d", _MSC_FULL_VER);
		log_add (log_Info, "MSC_BUILD: %d", _MSC_BUILD);
#endif // _MSC_VER

#ifdef __GNUC__
		log_add (log_Info, "GCC_VERSION: %d.%d.%d",
				__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif // __GNUC__

#ifdef __clang__
		log_add (log_Info, "CLANG_VERSION: %d.%d.%d",
				__clang_major__, __clang_minor__, __clang_patchlevel__);
#endif // __clang__

#if defined(__MINGW64__)
		log_add (log_Info, "MINGW64_VERSION: %d.%d",
				__MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#elif defined(__MINGW32__)
		log_add (log_Info, "MINGW32_VERSION: %d.%d",
				__MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#endif

		log_add (log_Info, "Build Time: %s %s\n", __DATE__, __TIME__);

	if (errBuffer[0] != '\0')
	{	// Have some saved error to log
		log_add (log_Error, "%s", errBuffer);
		errBuffer[0] = '\0';
	}

	if (options.runMode == runMode_usage)
	{
		usage (stdout, &defaults);
		log_showBox (true, false);
		return EXIT_SUCCESS;
	}

	if (optionsResult != EXIT_SUCCESS)
	{	// Options parsing failed. Oh, well.
		log_add (log_Fatal, "Run with -h to see the allowed arguments.");
		return optionsResult;
	}

	TFB_PreInit ();
	mem_init ();
	InitThreadSystem ();
	log_initThreads ();
	initIO ();
	prepareConfigDir (options.configDir);

	PlayerControls[0] = CONTROL_TEMPLATE_PL_1;
	PlayerControls[1] = CONTROL_TEMPLATE_PL_2;

	// Fill in the options struct based on uqm.cfg
	if (!options.safeMode.value)
	{
		LoadResourceIndex (configDir, "uqm.cfg", "config.");
		LoadResourceIndex (configDir, "cheats.cfg", "cheat.");
		LoadResourceIndex (configDir, "megamod.cfg", "mm.");
		getUserConfigOptions (&options);
	}

	{	/* remove old control template names */
		int i;

		for (i = 0; i < 6; ++i)
		{
			char cfgkey[64];

			snprintf(cfgkey, sizeof(cfgkey), "config.keys.%d.name", i + 1);
			cfgkey[sizeof(cfgkey) - 1] = '\0';

			res_Remove (cfgkey);
		}
	}

	/* TODO: Once threading is gone, these become local variables
	   again.  In the meantime, they must be global so that
	   initAudio (in StarCon2Main) can see them.  initAudio needed
	   to be moved there because calling AssignTask in the main
	   thread doesn't work */
	snddriver = options.soundDriver.value;
	soundflags = options.soundQuality.value;

	// Fill in global variables:
	opt3doMusic = options.use3doMusic.value;
	optRemixMusic = options.useRemixMusic.value;
	optSpeech = options.useSpeech.value;
	optWhichCoarseScan = options.whichCoarseScan.value;
	optWhichMenu = options.whichMenu.value;
	optWhichFonts = options.whichFonts.value;
	optWhichIntro = options.whichIntro.value;
	optWhichShield = options.whichShield.value;
	optSmoothScroll = options.smoothScroll.value;
	optMeleeScale = options.meleeScale.value;
	optSubtitles = options.subtitles.value;
	optStereoSFX = options.stereoSFX.value;
	musicVolumeScale = options.musicVolumeScale.value;
	sfxVolumeScale = options.sfxVolumeScale.value;
	speechVolumeScale = options.speechVolumeScale.value;
	optAddons = options.addons;

	optGodModes = options.optGodModes.value;
	timeDilationScale = options.timeDilationScale.value;
	optBubbleWarp = options.bubbleWarp.value;
	optUnlockShips = options.unlockShips.value;
	optHeadStart = options.headStart.value;
	optAutoButtons = options.autoButtons.value;
	optInfiniteRU = options.infiniteRU.value;
	optSkipIntro = options.skipIntro.value;
	optMainMenuMusic = options.mainMenuMusic.value;
	optNebulae = options.nebulae.value;
	optOrbitingPlanets = options.orbitingPlanets.value;
	optTexturedPlanets = options.texturedPlanets.value;
 	optCheatMode = options.cheatMode.value;
	optDateFormat = options.optDateFormat.value;
	optInfiniteFuel = options.infiniteFuel.value;
	optPartialPickup = options.partialPickup.value;
	optSubmenu = options.submenu.value;
	optInfiniteCredits = options.infiniteCredits.value;
	optCustomBorder = options.customBorder.value;
	optSeedType = options.seedType.value;
	optCustomSeed = options.customSeed.value;
	optShipSeed = options.shipSeed.value;
	optSphereColors = options.sphereColors.value;
	optRequiresReload = FALSE;
	optRequiresRestart = FALSE;
	optSpaceMusic = options.spaceMusic.value;
	optVolasMusic = options.volasMusic.value;
	optWholeFuel = options.wholeFuel.value;
	optDirJoy[0] = options.dirJoyP1.value;
	optLanderHold = options.landerHold.value;
	optScrTrans = options.scrTrans.value;
	optDifficulty = options.optDifficulty.value;
	optDiffChooser = options.optDiffChooser.value;
	optFuelRange = options.optFuelRange.value;
	optExtended = options.extended.value;
	optNomad = options.nomad.value;
	optGameOver = options.gameOver.value;
	optShipDirectionIP = options.shipDirectionIP.value;
	optHazardColors = options.hazardColors.value;
	optOrzCompFont = options.orzCompFont.value;
	optControllerType = options.optControllerType.value;
	optSmartAutoPilot = options.smartAutoPilot.value;
	optTintPlanSphere = options.tintPlanSphere.value;
	optPlanetStyle = options.planetStyle.value;
	optStarBackground = options.starBackground.value;
	optScanStyle = options.scanStyle.value;
	optNonStopOscill = options.nonStopOscill.value;
	optScopeStyle = options.scopeStyle.value;
	optHyperStars = options.hyperStars.value;
	optSuperPC = options.landerStyle.value;
	optPlanetTexture = options.planetTexture.value;
	optFlagshipColor = options.flagshipColor.value;
	optNoHQEncounters = options.noHQEncounters.value;
	optDeCleansing = options.deCleansing.value;
	optMeleeObstacles = options.meleeObstacles.value;
	optShowVisitedStars = options.showVisitedStars.value;
	optUnscaledStarSystem = options.unscaledStarSystem.value;
	optScanSphere = options.sphereType.value;
	optNebulaeVolume = options.nebulaevol.value;
	optSlaughterMode = options.slaughterMode.value;
	optAdvancedAutoPilot = options.advancedAutoPilot.value;
	optMeleeToolTips = options.meleeToolTips.value;
	optMusicResume = options.musicResume.value;
	optScatterElements = options.scatterElements.value;
	optShowUpgrades = options.showUpgrades.value;
	optFleetPointSys = options.fleetPointSys.value;
	optShipStore = options.shipStore.value;
	optCaptainNames = options.captainNames.value;
	optDosMenus = options.dosMenus.value;
	optHyperSpaceColor = options.hyperSpaceColor.value;
	DeadZoneLeftStick[0] = options.deadZoneLeftP1.value;
	DeadZoneRightStick[0] = options.deadZoneRightP1.value;
	DeadZoneLeftStick[1] = options.deadZoneLeftP2.value;
	DeadZoneRightStick[1] = options.deadZoneRightP2.value;
	optDirJoy[1] = options.dirJoyP2.value;
	optMouseInput = options.mouseInput.value;
	optBattleMouse = options.battleMouse.value;
	optScreenShots = options.screenShots.value;
	optHighVisOrbits = options.highVisOrbits.value;
	optHighVisPlanets = options.highVisPlanets.value;

	prepareContentDir (options.contentDir, options.addonDir, argv[0]);

	resolutionFactor = isAddonAvailable (HD_MODE) ?
		(unsigned int)options.resolutionFactor.value : 0;
	options.resolutionFactor.value = resolutionFactor;
	options.resolutionFactor.set = true;

	loresBlowupScale = (unsigned int)options.loresBlowupScale.value;
	optKeepAspectRatio = options.keepAspectRatio.value;

	optWindowType = OPTVAL_UQM_WINDOW;
	if (options.windowType.value < OPTVAL_UQM_WINDOW &&
		isAddonAvailable (WINDOW_MODE (resolutionFactor,
			options.windowType.value)))
	{
		optWindowType = options.windowType.value;
	}
	options.windowType.value = optWindowType;
	options.windowType.set = true;

	{
		int w = 320;
		int h = (DOS_BOOL (240, 200));
		int scaleFactor = loresBlowupScale + 1; // stored value starts with 0

		CanvasWidth = w << resolutionFactor;
		CanvasHeight = h << resolutionFactor;

		if (loresBlowupScale < 6) {
			SavedWidth = loresBlowupScale ? (w * scaleFactor) : CanvasWidth;
			SavedHeight = loresBlowupScale ? (h * scaleFactor) : CanvasHeight;
		}
		else
		{
			SavedWidth = inBounds(options.resolution.width, 320, 1920);
			SavedHeight = inBounds(options.resolution.height, 200, 1440);
		}

		if (optKeepAspectRatio == 2)
		{
			float threshold = 0.75f;
			float ratio = (float)SavedHeight / (float)SavedWidth;

			if (ratio > threshold) // screen is narrower than 4:3
				options.resolution.width = SavedHeight / threshold;
			else if (ratio < threshold) // screen is wider than 4:3
				options.resolution.height = SavedWidth * threshold;
		}
		else
		{
			options.resolution.width = SavedWidth;
			options.resolution.height = SavedHeight;
		}
	}

	prepareMeleeDir ();
	prepareSaveDir ();
	prepareScrShotDir ();
	prepareShadowAddons (options.addons);
#if 0
	initTempDir ();
#endif

	InitTimeSystem ();
	InitTaskSystem ();
	
	luaUqm_init ();

	Alarm_init ();
	Callback_init ();

#ifdef NETPLAY
	Network_init ();
	NetManager_init ();
#endif

	gfxDriver = TFB_GFXDRIVER_SDL_PURE;
	gfxFlags = options.scaler.value;
	if (options.fullscreen.value)
	{
		if (options.fullscreen.value > 1)
			gfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
		else
			gfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
	}
	if (options.scanlines.value)
		gfxFlags |= TFB_GFXFLAGS_SCANLINES;
	if (options.showFps.value)
		gfxFlags |= TFB_GFXFLAGS_SHOWFPS;
	TFB_InitGraphics (gfxDriver, gfxFlags, options.graphicsBackend,
			options.resolution.width, options.resolution.height,
			&resolutionFactor, &optWindowType);
	if (options.gamma.set && setGammaCorrection (options.gamma.value))
		optGamma = options.gamma.value;
	else
		optGamma = 1.0f; // failed or default
	
	InitColorMaps ();
	init_communication ();
	/* TODO: Once threading is gone, restore initAudio here.
	 *   initAudio calls AssignTask, which currently blocks on
	 *   ProcessThreadLifecycles... */
	// initAudio (snddriver, soundflags);
	// Make sure that the compiler treats multidim arrays the way we expect
	assert (sizeof (int [NUM_TEMPLATES * NUM_KEYS]) ==
			sizeof (int [NUM_TEMPLATES][NUM_KEYS]));
	TFB_SetInputVectors (ImmediateInputState.menu, NUM_MENU_KEYS,
			(volatile int *)ImmediateInputState.key, NUM_TEMPLATES,
			NUM_KEYS);
	TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);

	StartThread (Starcon2Main, NULL, 1024, "Starcon2Main");

	for (i = 0; i < 2000 && !MainExited; )
	{
		if (QuitPosted)
		{	/* Try to stop the main thread, but limited number of times */
			SignalStopMainThread ();
			++i;
		}
		else if (!GameActive)
		{	// Throttle down the main loop when game is inactive
			HibernateThread (ONE_SECOND / 4);
		}

		TFB_ProcessEvents ();
		ProcessUtilityKeys ();
		ProcessThreadLifecycles ();
		TFB_FlushGraphics ();

		if (menu_visible)
			TFB_SwapBuffers (TFB_REDRAW_YES);
	}

	/* Currently, we use atexit() callbacks everywhere, so we
	 *   cannot simply call unInitAudio() and the like, because other
	 *   tasks might still be using it */
	if (MainExited)
	{
		FreeJournal ();
		TFB_UninitInput ();
		unInitAudio ();
		uninit_communication ();
		
		TFB_PurgeDanglingGraphics ();
		// Purge above refers to colormaps which have to be still up
		UninitColorMaps ();
		TFB_UninitGraphics ();

#ifdef NETPLAY
		NetManager_uninit ();
		Network_uninit ();
#endif

		Callback_uninit ();
		Alarm_uninit ();
		
		luaUqm_uninit ();

		CleanupTaskSystem ();
		UnInitTimeSystem ();
#if 0
		unInitTempDir ();
#endif
		unprepareAllDirs ();
		uninitIO ();
		UnInitThreadSystem ();
		mem_uninit ();
	}

	HFree (options.addons);
	
	return EXIT_SUCCESS;
}

static void
saveErrorV (const char *fmt, va_list list)
{
	int len = strlen (errBuffer);
	int left = sizeof (errBuffer) - len;
	if (len > 0 && left > 0)
	{	// Already something there
		errBuffer[len] = '\n';
		++len;
		--left;
	}
	vsnprintf (errBuffer + len, left, fmt, list);
	errBuffer[sizeof (errBuffer) - 1] = '\0';
}

static void
saveError (const char *fmt, ...)
{
	va_list list;

	va_start (list, fmt);
	saveErrorV (fmt, list);
	va_end (list);
}


static bool
lookupOptionValue (const struct option_list_value *list,
		const char *strval, int *ret)
{
	if (!list)
		return false;

	// The list is terminated by a NULL 'str' value.
	while (list->str && strcmp (strval, list->str) != 0)
		++list;
	if (!list->str)
		return false;

	*ret = list->value;
	return true;
}

static void
getBoolConfigValue (struct bool_option *option, const char *config_val)
{
	if (option->set || !res_IsBoolean (config_val))
		return;

	option->value = res_GetBoolean (config_val);
	option->set = true;
}

static void
getBoolConfigValueXlat (struct int_option *option, const char *config_val,
		int true_val, int false_val)
{
	if (option->set || !res_IsBoolean (config_val))
		return;

	option->value = res_GetBoolean (config_val) ? true_val : false_val;
	option->set = true;
}

static void
getVolumeConfigValue (struct float_option *option, const char *config_val)
{
	if (option->set || !res_IsInteger (config_val))
		return;

	parseIntVolume (res_GetInteger (config_val), &option->value);
	option->set = true;
}

static void
getGammaConfigValue (struct float_option *option, const char *config_val)
{
	int val;

	if (option->set || !res_IsInteger (config_val))
		return;

	val = res_GetInteger (config_val);
	// gamma config option is a fixed-point number
	// ignore ridiculously out-of-range values
	if (val < (int)(0.03 * GAMMA_SCALE) || val > (int)(9.9 * GAMMA_SCALE))
		return;
	option->value = val / (float)GAMMA_SCALE;
	// avoid setting gamma when not necessary
	if (option->value != 1.0f)
		option->set = true;
}

static bool
getListConfigValue (struct int_option *option, const char *config_val,
		const struct option_list_value *list)
{
	const char *strval;
	bool found;

	if (option->set || !res_IsString (config_val) || !list)
		return false;

	strval = res_GetString (config_val);
	found = lookupOptionValue (list, strval, &option->value);
	option->set = found;

	return found;
}

static void getDeadzoneConfigValue (struct int_option *option,
		const char *config_val)
{
	if (!res_IsInteger (config_val))
		return;

	if (option->set)
		return;

	option->value = res_GetInteger (config_val);
	option->set = true;

	if (option->value > AXIS_MAX || option->value < 0)
		option->value = DEFAULT_DZONE;
}

static void
getUserConfigOptions (struct options_struct *options)
{
	// Most of the user config options are only applied if they
	// have not already been set (i.e. on the commandline)

	if (res_IsInteger ("config.reswidth")
			&& res_IsInteger ("config.resheight")
			&& !options->resolution.set)
	{
		options->resolution.width = res_GetInteger ("config.reswidth");
		options->resolution.height = res_GetInteger ("config.resheight");
		options->resolution.set = true;
	}

	if (res_IsBoolean ("config.alwaysgl") && !options->opengl.set)
	{	// config.alwaysgl is processed differently than others
		// Only set when it's 'true'
		if (res_GetBoolean ("config.alwaysgl"))
		{
			options->opengl.value = true;
			options->opengl.set = true;
		}
	}
	getBoolConfigValue (&options->opengl, "config.usegl");

	getListConfigValue (&options->scaler, "config.scaler", scalerList);

	//getBoolConfigValue (&options->fullscreen, "config.fullscreen");
	if (res_IsInteger ("config.fullscreen") && !options->fullscreen.set)
		options->fullscreen.value = res_GetInteger ("config.fullscreen");
	getBoolConfigValue (&options->scanlines, "config.scanlines");
	getBoolConfigValue (&options->showFps, "config.showfps");

	if (res_IsInteger ("config.keepaspectratio")
			&& !options->keepAspectRatio.set)
	{
		options->keepAspectRatio.value =
				res_GetInteger ("config.keepaspectratio");
	}

	getGammaConfigValue (&options->gamma, "config.gamma");

	getBoolConfigValue (&options->subtitles, "config.subtitles");
	
	getBoolConfigValueXlat (&options->whichMenu, "config.textmenu",
			OPT_3DO, OPT_PC);
	getBoolConfigValueXlat (&options->whichFonts, "config.textgradients",
			OPT_3DO, OPT_PC);
	if (res_IsInteger ("config.iconicscan") && !options->whichCoarseScan.set)
		options->whichCoarseScan.value = res_GetInteger ("config.iconicscan");
	getBoolConfigValueXlat (&options->smoothScroll, "config.smoothscroll",
			OPT_3DO, OPT_PC);
	getBoolConfigValueXlat (&options->whichShield, "config.pulseshield",
			OPT_3DO, OPT_PC);
	getBoolConfigValueXlat (&options->whichIntro, "config.3domovies",
			OPT_3DO, OPT_PC);

	getBoolConfigValue (&options->use3doMusic, "config.3domusic");
	getBoolConfigValue (&options->useRemixMusic, "config.remixmusic");
	getBoolConfigValue (&options->useSpeech, "config.speech");


#ifdef MELEE_ZOOM
	if (res_IsInteger("config.smoothmelee") && !options->meleeScale.set)
	{
		options->meleeScale.value = res_GetInteger("config.smoothmelee");
		options->meleeScale.set = true;
	}
#else	
	getBoolConfigValueXlat (&options->meleeScale, "config.smoothmelee",
			TFB_SCALE_TRILINEAR, TFB_SCALE_STEP);
#endif

	getListConfigValue (&options->soundDriver, "config.audiodriver",
			audioDriverList);
	getListConfigValue (&options->soundQuality, "config.audioquality",
			audioQualityList);
	getBoolConfigValue (&options->stereoSFX, "config.positionalsfx");
	getVolumeConfigValue (&options->musicVolumeScale, "config.musicvol");
	getVolumeConfigValue (&options->sfxVolumeScale, "config.sfxvol");
	getVolumeConfigValue (&options->speechVolumeScale, "config.speechvol");
	
	
	if (res_IsInteger ("config.resolutionfactor")
			&& !options->resolutionFactor.set)
	{
		options->resolutionFactor.value =
				res_GetInteger ("config.resolutionfactor");
		options->resolutionFactor.set = true;
	}
	
	
	if (res_IsInteger ("config.loresBlowupScale"))
	{
		options->loresBlowupScale.value =
				res_GetInteger ("config.loresBlowupScale");
		options->loresBlowupScale.set = true;
	}

	getBoolConfigValue (&options->cheatMode, "cheat.kohrStahp");
	
	if (res_IsInteger ("cheat.godModes") && !options->optGodModes.set)
		options->optGodModes.value = res_GetInteger ("cheat.godModes");

	if (res_IsInteger ("cheat.timeDilation")
			&& !options->timeDilationScale.set)
	{
		options->timeDilationScale.value =
				res_GetInteger ("cheat.timeDilation");
	}
	getBoolConfigValue (&options->bubbleWarp, "cheat.bubbleWarp");
	getBoolConfigValue (&options->unlockShips, "cheat.unlockShips");
	getBoolConfigValue (&options->headStart, "cheat.headStart");
	getBoolConfigValue (&options->autoButtons, "mm.autoButtons");
	getBoolConfigValue (&options->infiniteRU, "cheat.infiniteRU");
	getBoolConfigValue (&options->skipIntro, "mm.skipIntro");
	getBoolConfigValue (&options->mainMenuMusic, "mm.mainMenuMusic");
	getBoolConfigValue (&options->nebulae, "mm.nebulae");
	getBoolConfigValue (&options->orbitingPlanets, "mm.orbitingPlanets");
	getBoolConfigValue (&options->texturedPlanets, "mm.texturedPlanets");
		
	if (res_IsInteger ("mm.dateFormat") && !options->optDateFormat.set)
	{
		options->optDateFormat.value = res_GetInteger ("mm.dateFormat");
	}
		
	getBoolConfigValue (&options->infiniteFuel, "cheat.infiniteFuel");
	getBoolConfigValue (&options->partialPickup, "mm.partialPickup");
	getBoolConfigValue (&options->submenu, "mm.submenu");
	getBoolConfigValue (&options->infiniteCredits, "cheat.infiniteCredits");
	getBoolConfigValue (&options->customBorder, "mm.customBorder");
	if (res_IsInteger ("mm.seedType") && !options->seedType.set)
	{
		options->seedType.value = res_GetInteger ("mm.seedType");
	}
	if (res_IsInteger ("mm.customSeed") && !options->customSeed.set)
	{
		options->customSeed.value = res_GetInteger ("mm.customSeed");
		if (!SANE_SEED (options->customSeed.value))
			options->customSeed.value = PrimeA;
	}
	getBoolConfigValue (&options->shipSeed, "mm.shipSeed");
	if (res_IsInteger ("mm.sphereColors") && !options->sphereColors.set)
	{
		options->sphereColors.value = res_GetInteger ("mm.sphereColors");
	}
	if (res_IsInteger ("mm.spaceMusic") && !options->spaceMusic.set)
	{
		options->spaceMusic.value = res_GetInteger ("mm.spaceMusic");
	}
	getBoolConfigValue (&options->volasMusic, "mm.volasMusic");
	getBoolConfigValue (&options->wholeFuel, "mm.wholeFuel");

	if (res_IsInteger ("mm.dirJoyP1") && !options->dirJoyP1.set)
	{
		options->dirJoyP1.value = res_GetInteger ("mm.dirJoyP1");
	}

	/*getBoolConfigValueXlat (&options->landerHold, "mm.landerHold",
		OPT_3DO, OPT_PC);*/
	getBoolConfigValueXlat (&options->scrTrans, "mm.scrTransition",
		OPT_3DO, OPT_PC);
	if (res_IsInteger ("mm.difficulty") && !options->optDifficulty.set)
	{
		options->optDifficulty.value = res_GetInteger ("mm.difficulty");
		options->optDiffChooser.value = options->optDifficulty.value;
		if (options->optDifficulty.value > 2)
			options->optDifficulty.value = 0;
	}
	if (res_IsInteger ("mm.fuelRange") && !options->optFuelRange.set)
	{
		options->optFuelRange.value = res_GetInteger ("mm.fuelRange");
	}
	getBoolConfigValue (&options->extended, "mm.extended");
	if (res_IsInteger ("mm.nomad") && !options->nomad.set)
	{
		options->nomad.value = res_GetInteger ("mm.nomad");
		if (options->nomad.value > 2)
			options->nomad.value = 0;
	}
	getBoolConfigValue (&options->gameOver, "mm.gameOver");
	getBoolConfigValue (&options->shipDirectionIP, "mm.shipDirectionIP");
	getBoolConfigValue (&options->hazardColors, "mm.hazardColors");
	getBoolConfigValue (&options->orzCompFont, "mm.orzCompFont");

	if (res_IsInteger ("mm.controllerType")
			&& !options->optControllerType.set)
	{
		options->optControllerType.value =
				res_GetInteger ("mm.controllerType");
	}

	getBoolConfigValue (&options->smartAutoPilot, "mm.smartAutoPilot");
	getBoolConfigValueXlat (&options->tintPlanSphere, "mm.tintPlanSphere",
			OPT_3DO, OPT_PC);
	getBoolConfigValueXlat (&options->planetStyle, "mm.planetStyle",
			OPT_3DO, OPT_PC);

	if (res_IsInteger ("mm.starBackground")
			&& !options->starBackground.set)
	{
		options->starBackground.value =
				res_GetInteger ("mm.starBackground");
	}

	getBoolConfigValueXlat (&options->scanStyle, "mm.scanStyle",
			OPT_3DO, OPT_PC);

	getBoolConfigValue (&options->nonStopOscill, "mm.nonStopOscill");

	getBoolConfigValueXlat (&options->scopeStyle, "mm.scopeStyle",
			OPT_3DO, OPT_PC);

	getBoolConfigValue (&options->hyperStars, "mm.hyperStars");

	getBoolConfigValueXlat (&options->landerStyle, "mm.landerStyle",
			OPT_3DO, OPT_PC);

	getBoolConfigValue (&options->planetTexture, "mm.planetTexture");

	getBoolConfigValueXlat (&options->flagshipColor, "mm.flagshipColor",
			OPT_3DO, OPT_PC);

	getBoolConfigValue (&options->noHQEncounters, "cheat.noHQEncounters");

	getBoolConfigValue (&options->deCleansing, "cheat.deCleansing");

	getBoolConfigValue (&options->meleeObstacles, "cheat.meleeObstacles");

	getBoolConfigValue (&options->showVisitedStars, "mm.showVisitedStars");

	getBoolConfigValue (&options->unscaledStarSystem,
			"mm.unscaledStarSystem");

	if (res_IsInteger("mm.sphereType") && !options->sphereType.set)
	{
		options->sphereType.value = res_GetInteger("mm.sphereType");
	}

	if (res_IsInteger ("mm.nebulaevol") && !options->nebulaevol.set)
	{
		options->nebulaevol.value = res_GetInteger ("mm.nebulaevol");
		if (options->nebulaevol.value > 50)
			options->nebulaevol.value = 11;
	}

	getBoolConfigValue (&options->slaughterMode, "mm.slaughterMode");
	getBoolConfigValue (&options->advancedAutoPilot,
			"mm.advancedAutoPilot");
	getBoolConfigValue (&options->meleeToolTips, "mm.meleeToolTips");

	if (res_IsInteger ("mm.musicResume") && !options->musicResume.set)
	{
		options->musicResume.value = res_GetInteger ("mm.musicResume");
	}

	if (res_IsInteger ("mm.windowType") && !options->windowType.set)
	{
		options->windowType.value = res_GetInteger ("mm.windowType");
	}

	getBoolConfigValue (&options->scatterElements, "mm.scatterElements");

	getBoolConfigValue (&options->showUpgrades, "mm.showUpgrades");

	getBoolConfigValue (&options->fleetPointSys, "mm.fleetPointSys");

	getBoolConfigValue (&options->shipStore, "mm.shipStore");

	getBoolConfigValue (&options->captainNames, "mm.captainNames");

	getBoolConfigValue (&options->dosMenus, "mm.dosMenus");

	if (res_IsInteger ("mm.mouseInput") && !options->mouseInput.set)
	{
		options->mouseInput.value = res_GetInteger ("mm.mouseInput");
	}

	getBoolConfigValueXlat (&options->hyperSpaceColor, "mm.hyperSpaceColor",
			OPT_3DO, OPT_PC);

	getDeadzoneConfigValue (&options->deadZoneLeftP1,  "mm.deadZoneLeftP1");
	getDeadzoneConfigValue (&options->deadZoneRightP1, "mm.deadZoneRightP1");
	getDeadzoneConfigValue (&options->deadZoneLeftP2,  "mm.deadZoneLeftP2");
	getDeadzoneConfigValue (&options->deadZoneRightP2, "mm.deadZoneRightP2");

	if (res_IsInteger ("mm.dirJoyP2") && !options->dirJoyP2.set)
	{
		options->dirJoyP2.value = res_GetInteger ("mm.dirJoyP2");
	}

	getBoolConfigValue (&options->battleMouse, "mm.battleMouse");

	if (res_IsInteger ("mm.screenShots") && !options->screenShots.set)
	{
		options->screenShots.value = res_GetInteger ("mm.screenShots");
	}

	getBoolConfigValue (&options->highVisOrbits, "mm.highVisOrbits");
	getBoolConfigValue (&options->highVisPlanets, "mm.highVisPlanets");
}

enum
{
	CSCAN_OPT = 1000,
	MENU_OPT,
	FONT_OPT,
	SHIELD_OPT,
	SCROLL_OPT,
	SOUND_OPT,
	STEREOSFX_OPT,
	ADDON_OPT,
	ADDONDIR_OPT,
	ACCEL_OPT,
	SAFEMODE_OPT,
	RENDERER_OPT,
#ifdef NETPLAY
	NETHOST1_OPT,
	NETPORT1_OPT,
	NETHOST2_OPT,
	NETPORT2_OPT,
	NETDELAY_OPT,
#endif
};

static const char *optString = "+r:f:oc:b:spC:n:?hM:S:T:q:ug:l:i:vwxk";
static struct option longOptions[] = 
{
	{"res", 1, NULL, 'r'},
	{"fullscreen", 1, NULL, 'f'},
	{"opengl", 0, NULL, 'o'},
	{"scale", 1, NULL, 'c'},
	{"meleezoom", 1, NULL, 'b'},
	{"scanlines", 0, NULL, 's'},
	{"fps", 0, NULL, 'p'},
	{"configdir", 1, NULL, 'C'},
	{"contentdir", 1, NULL, 'n'},
	{"help", 0, NULL, 'h'},
	{"musicvol", 1, NULL, 'M'},
	{"sfxvol", 1, NULL, 'S'},
	{"speechvol", 1, NULL, 'T'},
	{"audioquality", 1, NULL, 'q'},
	{"nosubtitles", 0, NULL, 'u'},
	{"gamma", 1, NULL, 'g'},
	{"logfile", 1, NULL, 'l'},
	{"intro", 1, NULL, 'i'},
	{"version", 0, NULL, 'v'},
	{"windowed", 0, NULL, 'w'},
	{"nogl", 0, NULL, 'x'},
	{"keepaspectratio", 0, NULL, 'k'},

	//  options with no short equivalent:
	{"cscan", 1, NULL, CSCAN_OPT},
	{"menu", 1, NULL, MENU_OPT},
	{"font", 1, NULL, FONT_OPT},
	{"shield", 1, NULL, SHIELD_OPT},
	{"scroll", 1, NULL, SCROLL_OPT},
	{"sound", 1, NULL, SOUND_OPT},
	{"stereosfx", 0, NULL, STEREOSFX_OPT},
	{"addon", 1, NULL, ADDON_OPT},
	{"addondir", 1, NULL, ADDONDIR_OPT},
	{"accel", 1, NULL, ACCEL_OPT},
	{"safe", 0, NULL, SAFEMODE_OPT},
	{"renderer", 1, NULL, RENDERER_OPT},
#ifdef NETPLAY
	{"nethost1", 1, NULL, NETHOST1_OPT},
	{"netport1", 1, NULL, NETPORT1_OPT},
	{"nethost2", 1, NULL, NETHOST2_OPT},
	{"netport2", 1, NULL, NETPORT2_OPT},
	{"netdelay", 1, NULL, NETDELAY_OPT},
#endif
	{0, 0, 0, 0}
};

static inline void
setBoolOption (struct bool_option *option, bool value)
{
	option->value = value;
	option->set = true;
}

static bool
setFloatOption (struct float_option *option, const char *strval,
		const char *optName)
{
	if (parseFloatOption (strval, &option->value, optName) != 0)
		return false;
	option->set = true;
	return true;
}

// returns true is value was found and set successfully
static bool
setListOption (struct int_option *option, const char *strval,
		const struct option_list_value *list)
{
	bool found;

	if (!list)
		return false; // not found

	found = lookupOptionValue (list, strval, &option->value);
	option->set = found;

	return found;
}

static inline bool
setChoiceOption (struct int_option *option, const char *strval)
{
	return setListOption (option, strval, choiceList);
}

static bool
setVolumeOption (struct float_option *option, const char *strval,
		const char *optName)
{
	int intVol;
	
	if (parseIntOption (strval, &intVol, optName) != 0)
		return false;
	parseIntVolume (intVol, &option->value);
	option->set = true;
	return true;
}

static int
parseOptions (int argc, char *argv[], struct options_struct *options)
{
	int optionIndex;
	bool badArg = false;

	opterr = 0;

	options->addons = HMalloc (1 * sizeof (const char *));
	options->addons[0] = NULL;
	options->numAddons = 0;

	if (argc == 0)
	{
		saveError ("Error: Bad command line.");
		return EXIT_FAILURE;
	}

#ifdef __APPLE__
	// If we are launched by double-clicking an application bundle, Finder
	// sticks a "-psn_<some_number>" argument into the list, which makes
	// getopt extremely unhappy. Check for this case and wipe out the
	// entire command line if it looks like it happened.
	if ((argc >= 2) && (strncmp(argv[1], "-psn_", 5) == 0))
	{
		return EXIT_SUCCESS;
	}
#endif

	while (!badArg)
	{
		int c;
		optionIndex = -1;
		c = getopt_long (argc, argv, optString, longOptions, &optionIndex);
		if (c == -1)
			break;

		switch (c)
		{
			case '?':
				if (optopt != '?')
				{
					saveError ("\nInvalid option or its argument");
					badArg = true;
					break;
				}
				// fall through
			case 'h':
				options->runMode = runMode_usage;
				return EXIT_SUCCESS;
			case 'v':
				options->runMode = runMode_version;
				return EXIT_SUCCESS;
			case 'r':
			{
				int width, height;
				if (sscanf (optarg, "%dx%d", &width, &height) != 2)
				{
					saveError ("Error: invalid argument specified "
							"as resolution.");
					badArg = true;
					break;
				}
				options->resolution.width = width;
				options->resolution.height = height;
				options->resolution.set = true;
				break;
			}
			case 'f':
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Fullscreen") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nFullscreen has to be 0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->fullscreen.value = temp;
					options->fullscreen.set = true;
				}
				break;
			}
			case 'w':
				options->fullscreen.value = 0;
				options->fullscreen.set = true;
				break;
			case 'o':
				setBoolOption (&options->opengl, true);
				break;
			case 'x':
				setBoolOption (&options->opengl, false);
				break;
			case 'k':
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Aspect Ratio") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nAspect Ratio has to be 0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->keepAspectRatio.value = temp;
					options->keepAspectRatio.set = true;
				}
				break;
			}
			case 'c':
				if (!setListOption (&options->scaler, optarg, scalerList))
				{
					InvalidArgument (optarg, "--scale or -c");
					badArg = true;
				}
				break;
			case 'b':
				if (!setListOption (&options->meleeScale, optarg,
						meleeScaleList))
				{
					InvalidArgument (optarg, "--meleezoom or -b");
					badArg = true;
				}
				break;
			case 's':
				setBoolOption (&options->scanlines, true);
				break;
			case 'p':
				setBoolOption (&options->showFps, true);
				break;
			case 'n':
				options->contentDir = optarg;
				break;
			case 'M':
				if (!setVolumeOption (&options->musicVolumeScale, optarg,
						"music volume"))
				{
					badArg = true;
				}
				break;
			case 'S':
				if (!setVolumeOption (&options->sfxVolumeScale, optarg,
						"sfx volume"))
				{
					badArg = true;
				}
				break;
			case 'T':
				if (!setVolumeOption (&options->speechVolumeScale, optarg,
						"speech volume"))
				{
					badArg = true;
				}
				break;
			case 'q':
				if (!setListOption (&options->soundQuality, optarg,
						audioQualityList))
				{
					InvalidArgument (optarg, "--audioquality or -q");
					badArg = true;
				}
				break;
			case 'u':
				setBoolOption (&options->subtitles, false);
				break;
			case 'g':
				if (!setFloatOption (&options->gamma, optarg,
						"gamma correction"))
				{
					badArg = true;
				}
				break;
			case 'l':
				options->logFile = optarg;
				break;
			case 'C':
				options->configDir = optarg;
				break;
			case 'i':
				if (!setChoiceOption (&options->whichIntro, optarg))
				{
					InvalidArgument (optarg, "--intro or -i");
					badArg = true;
				}
				break;
			case CSCAN_OPT:
				{
					int temp;
					if (parseIntOption (optarg, &temp,
							"Coarse Scans") == -1)
					{
						badArg = true;
						break;
					}
					else if (temp < 0 || temp > 2)
					{
						saveError("\nCoarse Scan has to be "
								"0, 1, 2 or 3.\n");
						badArg = true;
					}
					else
					{
						options->whichCoarseScan.value = temp;
					}
					break;
				}
			case MENU_OPT:
				if (!setChoiceOption (&options->whichMenu, optarg))
				{
					InvalidArgument (optarg, "--menu");
					badArg = true;
				}
				break;
			case FONT_OPT:
				if (!setChoiceOption (&options->whichFonts, optarg))
				{
					InvalidArgument (optarg, "--font");
					badArg = true;
				}
				break;
			case SHIELD_OPT:
				if (!setChoiceOption (&options->whichShield, optarg))
				{
					InvalidArgument (optarg, "--shield");
					badArg = true;
				}
				break;
			case SCROLL_OPT:
				if (!setChoiceOption (&options->smoothScroll, optarg))
				{
					InvalidArgument (optarg, "--scroll");
					badArg = true;
				}
				break;
			case SOUND_OPT:
				if (!setListOption (&options->soundDriver, optarg,
						audioDriverList))
				{
					InvalidArgument (optarg, "--sound");
					badArg = true;
				}
				break;
			case STEREOSFX_OPT:
				setBoolOption (&options->stereoSFX, true);
				break;
			case ADDON_OPT:
				options->numAddons++;
				options->addons = HRealloc ((void *)options->addons,
						(options->numAddons + 1) * sizeof (const char *));
				options->addons[options->numAddons - 1] = optarg;
				options->addons[options->numAddons] = NULL;
				break;
			case ADDONDIR_OPT:
				options->addonDir = optarg;
				break;
			case ACCEL_OPT:
			{
				int value;
				if (lookupOptionValue (accelList, optarg, &value))
				{
					force_platform = value;
				}
				else
				{
					InvalidArgument (optarg, "--accel");
					badArg = true;
				}
				break;
			}
			case SAFEMODE_OPT:
				setBoolOption (&options->safeMode, true);
				break;
			case RENDERER_OPT:
				options->graphicsBackend = optarg;
				break;
#ifdef NETPLAY
			case NETHOST1_OPT:
				netplayOptions.peer[0].isServer = false;
				netplayOptions.peer[0].host = optarg;
				break;
			case NETPORT1_OPT:
				netplayOptions.peer[0].port = optarg;
				break;
			case NETHOST2_OPT:
				netplayOptions.peer[1].isServer = false;
				netplayOptions.peer[1].host = optarg;
				break;
			case NETPORT2_OPT:
				netplayOptions.peer[1].port = optarg;
				break;
			case NETDELAY_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "network input delay")
						== -1)
				{
					badArg = true;
					break;
				}
				netplayOptions.inputDelay = temp;

				if (netplayOptions.inputDelay > BATTLE_FRAME_RATE)
				{
					saveError ("Network input delay is absurdly large.");
					badArg = true;
				}
				break;
			}
#endif
			default:
				saveError ("Error: Unknown option '%s'",
						optionIndex < 0 ? "<unknown>" :
						longOptions[optionIndex].name);
				badArg = true;
				break;
		}
	}

	if (!badArg && optind != argc)
	{
		saveError ("\nError: Extra arguments found on the command line.");
		badArg = true;
	}

	return badArg ? EXIT_FAILURE : EXIT_SUCCESS;
}

static void
parseIntVolume (int intVol, float *vol)
{
	if (intVol < 0)
	{
		*vol = 0.0f;
		return;
	}

	if (intVol > 100)
	{
		*vol = 1.0f;
		return;
	}

	*vol = intVol / 100.0f;
	return;
}

static int
parseIntOption (const char *str, int *result, const char *optName)
{
	char *endPtr;
	int temp;

	if (str == NULL || str[0] == '\0')
	{
		saveError ("Error: Invalid value for '%s'.", optName);
		return -1;
	}
	temp = (int) strtol (str, &endPtr, 10);
	if (*endPtr != '\0')
	{
		saveError ("Error: Junk characters in argument '%s'.", optName);
		return -1;
	}

	*result = temp;
	return 0;
}

static int
parseFloatOption (const char *str, float *f, const char *optName)
{
	char *endPtr;
	float temp;

	if (str[0] == '\0')
	{
		saveError ("Error: Invalid value for '%s'.", optName);
		return -1;
	}
	temp = (float) strtod (str, &endPtr);
	if (*endPtr != '\0')
	{
		saveError ("Error: Junk characters in argument '%s'.", optName);
		return -1;
	}

	*f = temp;
	return 0;
}

static void
usage (FILE *out, const struct options_struct *defaults)
{
	FILE *old = log_setOutput (out);
	log_captureLines (LOG_CAPTURE_ALL);
	
	log_add (log_User, "Options:");
	log_add (log_User, "  -r, --res=WIDTHxHEIGHT (default: 640x480, bigger "
			"works only with --opengl)");
	log_add (log_User, "  -f, --fullscreen (default: 0)");
	log_add (log_User, "  -w, --windowed (default: true)");
	log_add (log_User, "  -o, --opengl (default: %s)",
			boolOptString (&defaults->opengl));
	log_add (log_User, "  -x, --nogl (default: %s)",
			boolNotOptString (&defaults->opengl));
	log_add (log_User, "  -k, --keepaspectratio (default: 1)");
	log_add (log_User, "  -c, --scale=MODE (bilinear, biadapt, biadv, "
			"triscan, hq or none (default) )");
	log_add (log_User, "  -b, --meleezoom=MODE (step, aka pc, or smooth, "
			"aka 3do; default is 3do)");
	log_add (log_User, "  -s, --scanlines (default: %s)",
			boolOptString (&defaults->scanlines));
	log_add (log_User, "  -p, --fps (default: %s)",
			boolOptString (&defaults->showFps));
	log_add (log_User, "  -g, --gamma=CORRECTIONVALUE (default: 1.0, which "
			"causes no change)");
	log_add (log_User, "  -C, --configdir=CONFIGDIR");
	log_add (log_User, "  -n, --contentdir=CONTENTDIR");
	log_add (log_User, "  -M, --musicvol=VOLUME (0-100, default 100)");
	log_add (log_User, "  -S, --sfxvol=VOLUME (0-100, default 100)");
	log_add (log_User, "  -T, --speechvol=VOLUME (0-100, default 100)");
	log_add (log_User, "  -q, --audioquality=QUALITY (high, medium or low, "
			"default medium)");
	log_add (log_User, "  -u, --nosubtitles");
	log_add (log_User, "  -l, --logfile=FILE (sends console output to "
			"logfile FILE)");
	log_add (log_User, "  --addon ADDON (using a specific addon; "
			"may be specified multiple times)");
	log_add (log_User, "  --addondir=ADDONDIR (directory where addons "
			"reside)");
	log_add(log_User, "  --renderer=name (Select named rendering engine "
			"if possible)");
	log_add (log_User, "  --sound=DRIVER (openal, mixsdl, none; default "
			"mixsdl)");
	log_add (log_User, "  --stereosfx (enables positional sound effects, "
			"currently only for openal)");
	log_add (log_User, "  --safe (start in safe mode)");
#ifdef NETPLAY
	log_add (log_User, "  --nethostN=HOSTNAME (server to connect to for "
			"player N (1=bottom, 2=top)");
	log_add (log_User, "  --netportN=PORT (port to connect to/listen on for "
			"player N (1=bottom, 2=top)");
	log_add (log_User, "  --netdelay=FRAMES (number of frames to "
			"buffer/delay network input for");
#endif
	log_add (log_User, "The following options can take either '3do' or 'pc' "
			"as an option:");
	log_add (log_User, "  -i, --intro : Intro/ending version (default: %s)",
			choiceOptString (&defaults->whichIntro));
	log_add (log_User, "  --cscan     : coarse-scan display, pc=text, "
			"3do=hieroglyphs (default: %s)",
			choiceOptString (&defaults->whichCoarseScan));
	log_add (log_User, "  --menu      : menu type, pc=text, 3do=graphical "
			"(default: %s)", choiceOptString (&defaults->whichMenu));
	log_add (log_User, "  --font      : font types and colors (default: %s)",
			choiceOptString (&defaults->whichFonts));
	log_add (log_User, "  --shield    : slave shield type; pc=static, "
			"3do=throbbing (default: %s)",
			choiceOptString (&defaults->whichShield));
	log_add (log_User, "  --scroll    : ff/frev during comm.  pc=per-page, "
			"3do=smooth (default: %s)",
			choiceOptString (&defaults->smoothScroll));

	log_setOutput (old);
}

static int
InvalidArgument (const char *supplied, const char *opt_name)
{
	saveError ("Invalid argument '%s' to option %s.", supplied, opt_name);
	return EXIT_FAILURE;
}

static const char *
choiceOptString (const struct int_option *option)
{
	switch (option->value)
	{
		case OPT_3DO:
			return "3do";
		case OPT_PC:
			return "pc";
		default:  /* 0 */
			return "none";
	}
}

static const char *
boolOptString (const struct bool_option *option)
{
	return option->value ? "on" : "off";
}

static const char *
boolNotOptString (const struct bool_option *option)
{
	return option->value ? "off" : "on";
}
