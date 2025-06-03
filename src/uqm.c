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

BOOLEAN restartGame;

#if defined (GFXMODULE_SDL)
#	include SDL_INCLUDE(SDL.h)
			// Including this is actually necessary on OSX.
#endif

#if defined(ANDROID) || defined(__ANDROID__)
#	include <SDL_android.h>
static void AndroidAppPutToBackgroundCallback(void) {
	SDL_ANDROID_PauseAudioPlayback();
	GameActive = FALSE;
}

static void SDLCALL AndroidAppRestoredCallback(void) {
	SDL_ANDROID_ResumeAudioPlayback();
	GameActive = TRUE;
}
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
	DECL_CONFIG_OPTION(bool,  keepAspectRatio);
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
	DECL_CONFIG_OPTION(bool,  unlockUpgrades);
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
	DECL_CONFIG_OPTION(bool,  addDevices);
	DECL_CONFIG_OPTION(bool,  customBorder);
	DECL_CONFIG_OPTION(int,   seedType);
	DECL_CONFIG_OPTION(int,   customSeed);
	DECL_CONFIG_OPTION(int,   sphereColors);
	DECL_CONFIG_OPTION(int,   spaceMusic);
	DECL_CONFIG_OPTION(bool,  volasMusic);
	DECL_CONFIG_OPTION(bool,  wholeFuel);
	DECL_CONFIG_OPTION(bool,  directionalJoystick);
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

int
main (int argc, char *argv[])
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

#if defined(ANDROID) || defined(__ANDROID__)
		INIT_CONFIG_OPTION(  opengl,            false ),
		INIT_CONFIG_OPTION2( resolution,        320, 240 ),
#else
		INIT_CONFIG_OPTION(  opengl,            false ),
		INIT_CONFIG_OPTION2( resolution,        640, 480 ),
#endif
		INIT_CONFIG_OPTION(  fullscreen,        2 ),
		INIT_CONFIG_OPTION(  scanlines,         false ),
		INIT_CONFIG_OPTION(  scaler,            0 ),
		INIT_CONFIG_OPTION(  showFps,           false ),
		INIT_CONFIG_OPTION(  keepAspectRatio,   false ),
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
#if defined(ANDROID) || defined(__ANDROID__)
		INIT_CONFIG_OPTION(  loresBlowupScale,  0),
#else
		INIT_CONFIG_OPTION(  loresBlowupScale,  1),
#endif
		INIT_CONFIG_OPTION(  cheatMode,         false ),
		INIT_CONFIG_OPTION(  optGodModes,       0 ),
		INIT_CONFIG_OPTION(  timeDilationScale, 0 ),
		INIT_CONFIG_OPTION(  bubbleWarp,        false ),
		INIT_CONFIG_OPTION(  unlockShips,       false ),
		INIT_CONFIG_OPTION(  headStart,         false ),
		INIT_CONFIG_OPTION(  unlockUpgrades,    false ),
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
		INIT_CONFIG_OPTION(  addDevices,        false ),
		INIT_CONFIG_OPTION(  customBorder,      false ),
		INIT_CONFIG_OPTION(  seedType,          0 ),
		INIT_CONFIG_OPTION(  customSeed,        PrimeA ),
		INIT_CONFIG_OPTION(  sphereColors,      0 ),
		INIT_CONFIG_OPTION(  spaceMusic,        0 ),
		INIT_CONFIG_OPTION(  volasMusic,        false ),
		INIT_CONFIG_OPTION(  wholeFuel,         false ),
#if defined(ANDROID) || defined(__ANDROID__)
		INIT_CONFIG_OPTION(  directionalJoystick, true ),
#else
		INIT_CONFIG_OPTION(  directionalJoystick, false ),
#endif
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
				(resolutionFactor ? "HD " UQM_EXTRA_VERSION : UQM_EXTRA_VERSION));
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
		printf("MSC_VER: %d\n", _MSC_VER);
		printf("MSC_FULL_VER: %d\n", _MSC_FULL_VER);
		printf("MSC_BUILD: %d\n\n", _MSC_BUILD);
		log_add(log_Info, "MSC_VER: %d\n", _MSC_VER);
		log_add(log_Info, "MSC_FULL_VER: %d\n", _MSC_FULL_VER);
		log_add(log_Info, "MSC_BUILD: %d\n", _MSC_BUILD);
#endif // _MSC_VER

#ifdef __GNUC__
		printf("GCC_VERSION: %d.%d.%d\n\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		log_add(log_Info, "GCC_VERSION: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif // __GNUC__

#ifdef __clang__
		printf("CLANG_VERSION: %d.%d.%d\n\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
		log_add(log_Info, "CLANG_VERSION: %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif // __clang__

#ifdef __MINGW32__
		printf("MINGW32_VERSION: %d.%d\n\n", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
		log_add(log_Info, "MINGW32_VERSION: %d.%d\n", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#endif // __MINGW32__

#ifdef __MINGW64__
		printf("MINGW64_VERSION: %d.%d\n\n", __MINGW64_MAJOR_VERSION, __MINGW64_MINOR_VERSION);
		log_add(log_Info, "MINGW64_VERSION: %d.%d\n", __MINGW64_MAJOR_VERSION, __MINGW64_MINOR_VERSION);
#endif // __MINGW64__

		printf("Build Time: %s %s\n\n", __DATE__, __TIME__);

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

	PlayerControls[0] = CONTROL_TEMPLATE_KB_1;
	PlayerControls[1] = CONTROL_TEMPLATE_JOY_1;

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
	optKeepAspectRatio = options.keepAspectRatio.value;
	optSubtitles = options.subtitles.value;
	optStereoSFX = options.stereoSFX.value;
	musicVolumeScale = options.musicVolumeScale.value;
	sfxVolumeScale = options.sfxVolumeScale.value;
	speechVolumeScale = options.speechVolumeScale.value;
	optAddons = options.addons;
	
	resolutionFactor = (unsigned int) options.resolutionFactor.value;
	loresBlowupScale = (unsigned int) options.loresBlowupScale.value;
	
	optGodModes = options.optGodModes.value;
	timeDilationScale = options.timeDilationScale.value;
	optBubbleWarp = options.bubbleWarp.value;
	optUnlockShips = options.unlockShips.value;
	optHeadStart = options.headStart.value;
	//optUnlockUpgrades = options.unlockUpgrades.value;
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
	//optAddDevices = options.addDevices.value;
	optCustomBorder = options.customBorder.value;
	optSeedType = options.seedType.value;
	optCustomSeed = options.customSeed.value;
	optSphereColors = options.sphereColors.value;
	optRequiresReload = FALSE;
	optRequiresRestart = FALSE;
	optSpaceMusic = options.spaceMusic.value;
	optVolasMusic = options.volasMusic.value;
	optWholeFuel = options.wholeFuel.value;
	optDirectionalJoystick = options.directionalJoystick.value;
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
	optWindowType = options.windowType.value;
	optScatterElements = options.scatterElements.value;
	optShowUpgrades = options.showUpgrades.value;
	optFleetPointSys = options.fleetPointSys.value;

	prepareContentDir (options.contentDir, options.addonDir, argv[0]);

	if (resolutionFactor && !isAddonAvailable (HD_MODE))
	{
		resolutionFactor = 0;
		options.resolutionFactor.value = 0;
		options.resolutionFactor.set = true;
		options.resolution.width = 320 * (loresBlowupScale + 1);
		options.resolution.height = 240 * (loresBlowupScale + 1);
	}

	switch (optWindowType)
	{
		case 0:
			if (!isAddonAvailable (DOS_MODE (resolutionFactor)))
			{
				optWindowType = 2;
				options.windowType.value = 2;
				options.windowType.set = true;
				options.resolution.width = 320 * (loresBlowupScale + 1);
				options.resolution.height = 240 * (loresBlowupScale + 1);
			}
			break;
		case 1:
			if (!isAddonAvailable (THREEDO_MODE (resolutionFactor)))
			{
				optWindowType = 2;
				options.windowType.value = 2;
				options.windowType.set = true;
			}
			break;
		default:
			break;
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

#if SDL_MAJOR_VERSION == 1
	gfxDriver = options.opengl.value ?
			TFB_GFXDRIVER_SDL_OPENGL : TFB_GFXDRIVER_SDL_PURE;
#else
	gfxDriver = TFB_GFXDRIVER_SDL_PURE;
#endif
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
	   initAudio calls AssignTask, which currently blocks on
	   ProcessThreadLifecycles... */
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
	}

	/* Currently, we use atexit() callbacks everywhere, so we
	 *   cannot simply call unInitAudio() and the like, because other
	 *   tasks might still be using it */
	if (MainExited)
	{
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
	getBoolConfigValue (&options->keepAspectRatio,
			"config.keepaspectratio");
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


#if defined(ANDROID) || defined(__ANDROID__)
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
	//getBoolConfigValue (&options->unlockUpgrades, "cheat.unlockUpgrades");
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
	//getBoolConfigValue (&options->addDevices, "cheat.addDevices");
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

#if defined(ANDROID) || defined(__ANDROID__)
	getBoolConfigValue (&options->directionalJoystick,
			"mm.directionalJoystick"); // For Android
#endif

	getBoolConfigValueXlat (&options->landerHold, "mm.landerHold",
		OPT_3DO, OPT_PC);
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

	memset (&optDeviceArray, 0, sizeof (optDeviceArray));

	memset (&optUpgradeArray , 0, sizeof (optUpgradeArray));
	
	if (res_IsInteger ("config.player1control"))
	{
		PlayerControls[0] = res_GetInteger ("config.player1control");
		/* This is an unsigned, so no < 0 check is necessary */
		if (PlayerControls[0] >= NUM_TEMPLATES)
		{
			log_add (log_Error, "Illegal control template '%d' for Player "
					"One.", PlayerControls[0]);
			PlayerControls[0] = CONTROL_TEMPLATE_KB_1;
		}
	}
	
	if (res_IsInteger ("config.player2control"))
	{
		/* This is an unsigned, so no < 0 check is necessary */
		PlayerControls[1] = res_GetInteger ("config.player2control");
		if (PlayerControls[1] >= NUM_TEMPLATES)
		{
			log_add (log_Error, "Illegal control template '%d' for Player "
					"Two.", PlayerControls[1]);
			PlayerControls[1] = CONTROL_TEMPLATE_JOY_1;
		}
	}
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
	CHEATMODE_OPT,
	GODMODE_OPT,
	TDM_OPT,
	BWARP_OPT,
	UNLOCKSHIPS_OPT,
	HEADSTART_OPT,
	UPGRADES_OPT,
	INFINITERU_OPT,
	SKIPINTRO_OPT,
	MENUMUS_OPT,
	NEBU_OPT,
	ORBITS_OPT,
	TEXTPLAN_OPT,
	DATE_OPT,
	INFFUEL_OPT,
	PICKUP_OPT,
	SUBMENU_OPT,
	DEVICES_OPT,
	CUSTBORD_OPT,
	SEEDTYPE_OPT,
	EXSEED_OPT,
	SPHERECOLORS_OPT,
	SPACEMUSIC_OPT,
	WHOLEFUEL_OPT,
	DIRJOY_OPT,
	LANDHOLD_OPT,
	SCRTRANS_OPT,
	DIFFICULTY_OPT,
	FUELRANGE_OPT,
	EXTENDED_OPT,
	NOMAD_OPT,
	GAMEOVER_OPT,
	SHIPDIRIP_OPT,
	HAZCOLORS_OPT,
	ORZFONT_OPT,
	CONTYPE_OPT,
	SISFACEHS_OPT,
	COLORPLAN_OPT,
	PLANSTYLE_OPT,
	STARBACK_OPT,
	SCANSTYLE_OPT,
	OSCILLO_OPT,
	OSCSTYLE_OPT,
	HYPERSTARS_OPT,
	LANDSTYLE_OPT,
	PLANTEX_OPT,
	SISENGINE_OPT,
	NOHSENC_OPT,
	DECLEANSE_OPT,
	NOMELEEOBJ_OPT,
	SHOWSTARS_OPT,
	UNSCALEDSS_OPT,
	SCANSPH_OPT,
	SLAUGHTER_OPT,
	SISADVAP_OPT,
	MELEETIPS_OPT,
	MUSICRESUME_OPT,
	WINDTYPE_OPT,
	SCATTERELEMS_OPT,
	SHOWUPG_OPT,
	FLTPTSYS_OPT,
	MELEE_OPT,
	LOADGAME_OPT,
	NEBUVOL_OPT,
	CLAPAK_OPT,
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
	{"kohrstahp", 0, NULL, CHEATMODE_OPT},
	{"precursormode", 1, NULL, GODMODE_OPT},
	{"timedilation", 1, NULL, TDM_OPT},
	{"bubblewarp", 0, NULL, BWARP_OPT},
	{"unlockships", 0, NULL, UNLOCKSHIPS_OPT},
	{"headstart", 0, NULL, HEADSTART_OPT},
	{"unlockupgrades", 0, NULL, UPGRADES_OPT},
	{"infiniteru", 0, NULL, INFINITERU_OPT},
	{"skipintro", 0, NULL, SKIPINTRO_OPT},
	{"mainmenumusic", 0, NULL, MENUMUS_OPT},
	{"nebulae", 0, NULL, NEBU_OPT},
	{"orbitingplanets", 0, NULL, ORBITS_OPT},
	{"texturedplanets", 0, NULL, TEXTPLAN_OPT},
	{"dateformat", 1, NULL, DATE_OPT},
	{"infinitefuel", 0, NULL, INFFUEL_OPT},
	{"partialpickup", 0, NULL, PICKUP_OPT},
	{"submenu", 0, NULL, SUBMENU_OPT},
	{"adddevices", 0, NULL, DEVICES_OPT},
	{"customborder", 0, NULL, CUSTBORD_OPT},
	{"seedtype", 0, NULL, SEEDTYPE_OPT},
	{"customseed", 1, NULL, EXSEED_OPT},
	{"spherecolors", 0, NULL, SPHERECOLORS_OPT},
	{"spacemusic", 1, NULL, SPACEMUSIC_OPT},
	{"wholefuel", 0, NULL, WHOLEFUEL_OPT},
	{"dirjoystick", 0, NULL, DIRJOY_OPT},
	{"landerhold", 0, NULL, LANDHOLD_OPT},
	{"scrtrans", 1, NULL, SCRTRANS_OPT},
	{"melee", 0, NULL, MELEE_OPT},
	{"loadgame", 0, NULL, LOADGAME_OPT},
	{"difficulty", 1, NULL, DIFFICULTY_OPT},
	{"fuelrange", 1, NULL, FUELRANGE_OPT},
	{"extended", 0, NULL, EXTENDED_OPT},
	{"nomad", 1, NULL, NOMAD_OPT},
	{"gameover", 0, NULL, GAMEOVER_OPT},
	{"shipdirectionip", 0, NULL, SHIPDIRIP_OPT},
	{"hazardcolors", 0, NULL, HAZCOLORS_OPT},
	{"orzcompfont", 0, NULL, ORZFONT_OPT},
	{"smartautopilot", 0, NULL, SISFACEHS_OPT},
	{"tintplansphere", 1, NULL, COLORPLAN_OPT},
	{"planetstyle", 1, NULL, PLANSTYLE_OPT},
	{"starbackground", 1, NULL, STARBACK_OPT},
	{"scanstyle", 1, NULL, SCANSTYLE_OPT},
	{"nonstoposcill", 0, NULL, OSCILLO_OPT},
	{"scopestyle", 1, NULL, OSCSTYLE_OPT},
	{"animhyperstars", 0, NULL, HYPERSTARS_OPT},
	{"landerview", 1, NULL, LANDSTYLE_OPT},
	{"planettexture", 1, NULL, PLANTEX_OPT},
	{"sisenginecolor", 1, NULL, SISENGINE_OPT},
	{"nohqencounters", 0, NULL, NOHSENC_OPT},
	{"decleanse", 0, NULL, DECLEANSE_OPT},
	{"nomeleeobstacles", 0, NULL, NOMELEEOBJ_OPT},
	{"showvisitstars", 0, NULL, SHOWSTARS_OPT},
	{"unscaledstarsystem", 0, NULL, UNSCALEDSS_OPT},
	{"spheretype", 1, NULL, SCANSPH_OPT},
	{"nebulaevol", 1, NULL, NEBUVOL_OPT},
	{"slaughtermode", 0, NULL, SLAUGHTER_OPT},
	{"advancedautopilot", 0, NULL, SISADVAP_OPT},
	{"meleetooltips", 0, NULL, MELEETIPS_OPT},
	{"musicresume", 1, NULL, MUSICRESUME_OPT},
	{"windowtype", 1, NULL, WINDTYPE_OPT},
	{"noclassic", 0, NULL, CLAPAK_OPT},
	{"scatterelements", 0, NULL, SCATTERELEMS_OPT},
	{"showupgrades", 0, NULL, SHOWUPG_OPT},
	{"fleetpointsys", 0, NULL, FLTPTSYS_OPT},
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
				setBoolOption (&options->keepAspectRatio, true);
				break;
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
			case CHEATMODE_OPT:
				setBoolOption (&options->cheatMode, true);
				break;
			case GODMODE_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "God Modes") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nGod Mode has to be 0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->optGodModes.value = temp;
					options->optGodModes.set = true;
				}
				break;
			}
			case TDM_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp,
						"Time Dilation scale") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nTime Dilation scale has to be "
							"0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->timeDilationScale.value = temp;
					options->timeDilationScale.set = true;
				}
				break;
			}
			case BWARP_OPT:
				setBoolOption (&options->bubbleWarp, true);
				break;
			case UNLOCKSHIPS_OPT:
				setBoolOption (&options->unlockShips, true);
				break;
			case HEADSTART_OPT:
				setBoolOption (&options->headStart, true);
				break;
			case UPGRADES_OPT:
				//setBoolOption (&options->unlockUpgrades, true);
				break;
			case INFINITERU_OPT:
				setBoolOption (&options->infiniteRU, true);
				break;
			case SKIPINTRO_OPT:
				setBoolOption (&options->skipIntro, true);
				break;
			case MENUMUS_OPT:
				setBoolOption (&options->mainMenuMusic, true);
				break;
			case NEBU_OPT:
				setBoolOption (&options->nebulae, true);
				break;
			case ORBITS_OPT:
				setBoolOption (&options->orbitingPlanets, true);
				break;
			case TEXTPLAN_OPT:
				setBoolOption (&options->texturedPlanets, true);
				break;
			case DATE_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Date Format") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 3)
				{
					saveError ("\nDate Format has to be 0, 1, 2, or 3.\n");
					badArg = true;
				}
				else
				{
					options->optDateFormat.value = temp;
					options->optDateFormat.set = true;
				}
				break;
			}
			case INFFUEL_OPT:
				setBoolOption (&options->infiniteFuel, true);
				break;
			case PICKUP_OPT:
				setBoolOption (&options->partialPickup, true);
				break;
			case SUBMENU_OPT:
				setBoolOption (&options->submenu, true);
				break;
			case DEVICES_OPT:
				//setBoolOption (&options->addDevices, true);
				break;
			case CUSTBORD_OPT:
				setBoolOption (&options->customBorder, true);
				break;
			case SEEDTYPE_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Seed Type") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 3)
				{
					saveError ("\nSeed Type has to be 0, 1, 2, or 3.\n");
					badArg = true;
				}
				else
				{
					options->seedType.value = temp;
					options->seedType.set = true;
				}
				break;
			}
			case EXSEED_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Custom Seed") == -1)
				{
					badArg = true;
					break;
				}
				else if (!SANE_SEED (temp))
				{
					saveError ("\nCustom Seed can not be less than %d or "
							"greater than %d.\n", MIN_SEED, MAX_SEED);
					badArg = true;
				}
				else
				{
					options->customSeed.value = temp;
					options->customSeed.set = true;
				}
				break;
			}
			case SPHERECOLORS_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Sphere Colors") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 1)
				{
					saveError ("\nSphere Colors has to 0 or 1.\n");
					badArg = true;
				}
				else
				{
					options->sphereColors.value = temp;
					options->sphereColors.set = true;
				}
				break;
			}
			case SPACEMUSIC_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp,
						"Ambient Space Music") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nAmbient Space Music has to be "
							"0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->spaceMusic.value = temp;
					options->spaceMusic.set = true;
				}
				break;
			}
			case WHOLEFUEL_OPT:
				setBoolOption (&options->wholeFuel, true);
				break;
			case DIRJOY_OPT:
				setBoolOption (&options->directionalJoystick, true);
				break;
			case LANDHOLD_OPT:
				if (!setChoiceOption (&options->landerHold, optarg)) {
					InvalidArgument (optarg, "--landerhold");
					badArg = true;
				}
				break;
			case SCRTRANS_OPT:
				if (!setChoiceOption (&options->scrTrans, optarg)) {
					InvalidArgument (optarg, "--scrtrans");
					badArg = true;
				}
				break;
			case DIFFICULTY_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Difficulty") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 3)
				{
					saveError ("\nDifficulty has to be 0, 1, 2, or 3.\n");
					badArg = true;
				}
				else {
					options->optDiffChooser.value = options->optDifficulty.value = temp;
					if (temp > 2)
						options->optDifficulty.value = 0;
					options->optDifficulty.set = true;
				}
				break;
			}
			case FUELRANGE_OPT:
			{
				int temp;
				if (parseIntOption (
						optarg, &temp, "Fuel range indicator") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 3)
				{
					saveError ("\nFuel range indicator has to be 0, 1, 2,"
							" or 3.\n");
					badArg = true;
				}
				else
				{
					options->optFuelRange.value = temp;
					options->optFuelRange.set = true;
				}
				break;
			}
			case EXTENDED_OPT:
				setBoolOption (&options->extended, true);
				break;
			case NOMAD_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp,
					"Nomad Mode type") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nNomad Mode has to be "
						"0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->nomad.value = temp;
					options->nomad.set = true;
				}
				break;
			}
			case GAMEOVER_OPT:
				setBoolOption (&options->gameOver, true);
				break;
			case SHIPDIRIP_OPT:
				setBoolOption (&options->shipDirectionIP, true);
				break;
			case HAZCOLORS_OPT:
				setBoolOption (&options->hazardColors, true);
				break;
			case ORZFONT_OPT:
				setBoolOption (&options->orzCompFont, true);
				break;
			case CONTYPE_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp,
						"Controller Type") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError (
							"\nController type has to be 0, 1, or 2.\n");
					badArg = true;
				}
				else
				{
					options->optControllerType.value = temp;
					options->optControllerType.set = true;
				}
				break;
			}
			case SISFACEHS_OPT:
				setBoolOption (&options->smartAutoPilot, true);
				break;
			case COLORPLAN_OPT:
				if (!setChoiceOption (&options->tintPlanSphere, optarg))
				{
					InvalidArgument (optarg, "--tintplansphere");
					badArg = true;
				}
				break;
			case PLANSTYLE_OPT:
				if (!setChoiceOption (&options->planetStyle, optarg))
				{
					InvalidArgument (optarg, "--planetstyle");
					badArg = true;
				}
				break;
			case STARBACK_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp,
						"Star Background") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 3)
				{
					saveError (
							"\nStar background has to be between 0-3.\n");
					badArg = true;
				}
				else
				{
					options->starBackground.value = temp;
					options->starBackground.set = true;
				}
				break;
			}
			case SCANSTYLE_OPT:
				if (!setChoiceOption (&options->scanStyle, optarg))
				{
					InvalidArgument (optarg, "--scanstyle");
					badArg = true;
				}
				break;
			case OSCILLO_OPT:
				setBoolOption (&options->nonStopOscill, true);
				break;
			case OSCSTYLE_OPT:
				if (!setChoiceOption (&options->scopeStyle, optarg))
				{
					InvalidArgument (optarg, "--scopestyle");
					badArg = true;
				}
				break;
			case HYPERSTARS_OPT:
				setBoolOption (&options->hyperStars, true);
				break;
			case LANDSTYLE_OPT:
				if (!setChoiceOption (&options->landerStyle, optarg))
				{
					InvalidArgument (optarg, "--landerview");
					badArg = true;
				}
				break;
			case PLANTEX_OPT:
				if (strcmp (optarg, "uqm") == 0)
					setBoolOption (&options->planetTexture, true);
				else if (strcmp (optarg, "3do") == 0)
					setBoolOption (&options->planetTexture, false);
				else
				{
					InvalidArgument (optarg, "--planettexture");
					saveError ("\nPlanet Texture can only be set to '3do' "
							"or 'uqm'\n");
					badArg = true;
				}
				break;
			case SISENGINE_OPT:
				if (!setChoiceOption (&options->flagshipColor, optarg))
				{
					InvalidArgument (optarg, "--sisenginecolor");
					badArg = true;
				}
				break;
			case NOHSENC_OPT:
				setBoolOption (&options->noHQEncounters, true);
				break;
			case DECLEANSE_OPT:
				setBoolOption (&options->deCleansing, true);
				break;
			case NOMELEEOBJ_OPT:
				setBoolOption (&options->meleeObstacles, true);
				break;
			case SHOWSTARS_OPT:
				setBoolOption (&options->showVisitedStars, true);
				break;
			case UNSCALEDSS_OPT:
				setBoolOption (&options->unscaledStarSystem, true);
				break;
			case SCANSPH_OPT:
			{
				int temp;
				if (parseIntOption(optarg, &temp, "Sphere Type") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError("\nSphere Type has to be between 0-2\n");
					badArg = true;
				}
				else
				{
					options->sphereType.value = temp;
					options->sphereType.set = true;
				}
				break;
			}
			case SLAUGHTER_OPT:
				setBoolOption (&options->slaughterMode, true);
				break;
			case SISADVAP_OPT:
				setBoolOption (&options->advancedAutoPilot, true);
				break;
			case MELEETIPS_OPT:
				setBoolOption (&options->meleeToolTips, true);
				break;
			case MUSICRESUME_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Music Resume") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nMusic Resume has to be between 0-2\n");
					badArg = true;
				}
				else
				{
					options->musicResume.value = temp;
					options->musicResume.set = true;
				}
				break;
			}
			case WINDTYPE_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Window Type") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 2)
				{
					saveError ("\nWindow type has to be between 0-2\n");
					badArg = true;
				}
				else
				{
					options->windowType.value = temp;
					options->windowType.set = true;
				}
				break;
			}
			case SCATTERELEMS_OPT:
				setBoolOption (&options->scatterElements, true);
				break;
			case SHOWUPG_OPT:
				setBoolOption (&options->showUpgrades, true);
				break;
			case FLTPTSYS_OPT:
				setBoolOption (&options->fleetPointSys, true);
				break;
			case MELEE_OPT:
				optSuperMelee = TRUE;
				break;
			case LOADGAME_OPT:
				optLoadGame = TRUE;
				break;
			case NEBUVOL_OPT:
			{
				int temp;
				if (parseIntOption (optarg, &temp, "Nebulae Volume") == -1)
				{
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 50)
				{
					saveError ("\nNebulae volume has to be between 0-50\n");
					badArg = true;
				}
				else
				{
					options->nebulaevol.value = temp;
					options->nebulaevol.set = true;
				}
				break;
			}
			case CLAPAK_OPT:
				optNoClassic = TRUE;
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
	log_add (log_User, "  -k, --keepaspectratio (default: %s)",
			boolOptString (&defaults->keepAspectRatio));
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

	log_add (log_User, "\nThe following options are MegaMod specific\n");

	log_add (log_User, "  --kohrstahp : Stops Kohr-Ah advancing. "
			"(default: %s)", boolOptString (&defaults->cheatMode));
	log_add(log_User, "  --precursormode : =1 Infinite ship battery. =2 No damage"
			"=3 Infinite ship battery and no damage (default: 0)");
	log_add (log_User, "  --timedilation : =1 Time is slowed down times 6. "
			"=2 Time is sped up times 5 (default: 0)");
	log_add (log_User, "  --bubblewarp : Instantaneous travel to any point on "
			"the Starmap. (default: %s)",
			boolOptString (&defaults->bubbleWarp));
	log_add (log_User, "  --unlockships : Allows you to purchase ships that you"
			"can't normally acquire in the main game. (default: %s)",
			boolOptString (&defaults->unlockShips));
	log_add (log_User, "  --headstart : Gives you an extra storage bay full of"
			"minerals, Fwiffo, and the Moonbase during a new game (default: %s)",
			boolOptString (&defaults->headStart));
	log_add (log_User, "  --unlockupgrades : Unlocks every upgrade for your flagship "
			"and landers. (default: %s)",
			boolOptString (&defaults->unlockUpgrades));
	log_add (log_User, "  --infiniteru : Gives you infinite R.U. as long as the"
			"cheat is on (default: %s)",
			boolOptString (&defaults->infiniteRU));
	log_add (log_User, "  --skipintro : Skips the intro and Logo fmv "
			"(default: %s)",
			boolOptString (&defaults->skipIntro));
	log_add (log_User, "  --mainmenumusic : Switches the main menu"
			"music on/off (default: %s)",
			boolOptString (&defaults->mainMenuMusic));
	log_add (log_User, "  --nebulae : Enables/Disables nebulae in"
			"star systems (default: %s)",
			boolOptString (&defaults->nebulae));
	log_add (log_User, "  --orbitingplanets : Enables/Disables orbiting"
		"planets in star systems (default: %s)",
			boolOptString (&defaults->orbitingPlanets));
	log_add (log_User, "  --texturedplanets : Enables/Disables textured"
			"planets in star systems (default: %s)",
			boolOptString (&defaults->texturedPlanets));
	log_add (log_User, "  --infinitefuel : Infinite fuel in the"
			"main game (default: %s)",
			boolOptString (&defaults->infiniteFuel));
	log_add (log_User, "  --partialpickup : Enables/Disables partial"
			"mineral pickup  (default: %s)",
			boolOptString (&defaults->partialPickup));
	log_add (log_User, "  --submenu : Enables/Disables mineral and star"
			"map keys submenu  (default: %s)",
			boolOptString (&defaults->submenu));
	log_add (log_User, "  --dateformat : 0: MMM DD.YYYY | 1: MM.DD.YYYY | "
			"2: DD MMM.YYYY | 3: DD.MM.YYYY (default: 0)");
	log_add (log_User, "  --adddevices : Gives you all available "
			"devices (default: %s)",
			boolOptString (&defaults->addDevices));
	log_add (log_User, "  --melee : Takes you straight to Super Melee"
			"after the splash screen.");
	log_add (log_User, "  --loadgame : Takes you straight to the Load"
			"Game sceen after the splash screen.");
	log_add (log_User, "  --customborder : Enables the custom border"
			"frame. (default: %s)",
			boolOptString (&defaults->customBorder));
	log_add (log_User, "  --seedtype: 0: Default seed | 1: Seed planets "
			"| 2: Seed Melnorme/Rainbow/Quasispace "
			"| 3: Seed Starmap (default: 0)");
	log_add (log_User, "  --customseed=# : Allows you to customize the "
			"internal seed used to generate the solar systems in-game."
			" (default: 16807)");
	log_add (log_User, "  --spherecolors: 0: Default colors "
			"| 1: StarSeed colors (default: 0)");
	log_add (log_User, "  --spacemusic #: Enables localized music for "
			"aliens when you are in their sphere of influence\n"
			"0: Default (OFF) | 1: No Spoilers | 2: Spoilers");
	log_add (log_User, "  --wholefuel : Enables the display of the whole "
			"fuel value in the ship status (default: %s)",
			boolOptString (&defaults->wholeFuel));
	log_add (log_User, "  --dirjoystick : Enables the use of directional"
			"joystick controls for Android (default: %s)",
			boolOptString (&defaults->directionalJoystick));
	log_add (log_User, "  --landerhold : Switch between PC/3DO max lander "
			"hold, pc=64, 3do=50 (default: %s)",
			choiceOptString (&defaults->landerHold));
	log_add (log_User, "  --scrtrans : Screen transitions, "
			"pc=instantaneous, 3do=crossfade (default: %s)",
			choiceOptString (&defaults->scrTrans));
	log_add (log_User, "  --difficulty : 0: Normal | 1: Easy | 2: Hard"
			"| 3: Choose at Start (default: 0)");
	log_add (log_User, "  --fuelrange : Enables extra fuel range "
			"indicators : 0: No indicators | 1: Fuel range at destination "
			"| 2: Remaining fuel range to Sol | 3: Both option 1 and 2 "
			"enabled simultaneously (default: 0)");
	log_add (log_User, "  --extended : Enables Extended Edition"
			"features (default: %s)",
			boolOptString (&defaults->extended));
	log_add (log_User, "  --nomad : Enables 'Nomad Mode' (No Starbase) : "
			"0: Off | 1: Easy | 2: Normal (default: 0)");
	log_add (log_User, "  --gameover : Enables Game Over cutscenes "
			"(default: %s)", boolOptString (&defaults->gameOver));
	log_add (log_User, "  --shipdirectionip : Enable NPC ships in IP"
			"to face their direction of travel (default: %s)",
			boolOptString (&defaults->shipDirectionIP));
	log_add (log_User, "  --hazardcolors : Enable colored text based on"
			"hazard severity when viewing planetary scans (default: %s)",
			boolOptString (&defaults->hazardColors));
	log_add (log_User, "  --orzcompfont : Enable alternate font for"
			"untranslatable Orz speech (default: %s)",
			boolOptString (&defaults->orzCompFont));
	log_add (log_User, "  --smartautopilot : Activating Auto-Pilot "
			"within Solar System pilots the Flagship out via the shortest "
			"route. (default: %s)",
			boolOptString (&defaults->smartAutoPilot));
	log_add (log_User, "  --controllertype : 0: Keyboard | 1: Xbox | "
			"2: PlayStation 4 (default: 0)");
	log_add (log_User, "  --tintplansphere : Tint the planet sphere"
			" with scan color during scan (default: %s)",
			choiceOptString (&defaults->tintPlanSphere));
	log_add (log_User, "  --planetstyle : Choose between PC or 3DO planet"
			" color and shading (default: %s)",
			choiceOptString (&defaults->planetStyle));
	log_add (log_User, "  --starbackground : Set the background stars"
			" in solar system between PC, 3DO, UQM, or HD-mod patterns "
			"(default: pc)");
	log_add (log_User, "  --scanstyle : Choose between PC or 3DO scanning"
			" types (default: %s)",
			choiceOptString (&defaults->scanStyle));
	log_add (log_User, "  --nonstoposcill : Oscilloscope uses both voice "
			" and music data (default: %s)",
			boolOptString (&defaults->nonStopOscill));
	log_add (log_User, "  --scopestyle : Choose between either the PC or"
			" 3DO oscilloscope type (default: %s)",
			choiceOptString (&defaults->scopeStyle));
	log_add (log_User, "  --animhyperstars : HD only - Use old HD-mod "
			"animated HyperSpace stars (default: %s)",
			boolOptString (&defaults->hyperStars));
	log_add (log_User, "  --landerview : Choose between either the PC or"
			" 3DO lander view (default: %s)",
			choiceOptString (&defaults->landerStyle));
	log_add (log_User, "  --planettexture : Choose between either 3DO or"
			" UQM planet map texture [when not using custom seed] "
			"(default: 3do)");
	log_add (log_User, "  --sisenginecolor : Choose between either the PC"
			" or 3DO Flagship engine color (default: %s)",
			choiceOptString (&defaults->flagshipColor));
	log_add (log_User, "  --nohqencounters : Disables HyperSpace "
			"encounters (default: %s)",
			boolOptString (&defaults->noHQEncounters));
	log_add (log_User, "  --decleanse : Moves the Death March 100 years"
			" ahead from its actual start date [does not work once the"
			" Death March has started] (default: %s)",
			boolOptString (&defaults->deCleansing));
	log_add (log_User, "  --nomeleeobstacles : Removes the planet and "
			"asteroids from Super Melee (default: %s)",
			boolOptString (&defaults->meleeObstacles));
	log_add (log_User, "  --showvisitstars : Dim visited stars on the "
			" StarMap and encase the star name in parenthesis "
			"(default: %s)", boolOptString (&defaults->showVisitedStars));
	log_add (log_User, "  --unscaledstarsystem : Show the classic HD-mod "
			" Beta Star System view (default: %s)",
			boolOptString (&defaults->unscaledStarSystem));
	log_add (log_User, "  --spheretype : Choose between PC, 3DO, or UQM"
			" scan sphere styles (default: %s)",
			choiceOptString (&defaults->sphereType));
	log_add (log_User, "--nebulaevol=VOLUME (0-50, default 11)");
	log_add (log_User, "--slaughtermode : Affect a race's SOI by "
			"destroying their ships in battle (default: %s)",
			boolOptString (&defaults->slaughterMode));
	log_add (log_User, "  --advancedautopilot : Finds the route that uses"
			"the least amount of fuel through HyperSpace or QuasiSpace "
			"and Auto-Pilots the Flagship on the best route (default: %s)",
			boolOptString (&defaults->advancedAutoPilot));
	log_add (log_User, "  --meleetooltips : Show SC1-style ship"
			"description tooltips at the bottom of the Super-Melee screen"
			"when picking a ship for your fleet (default: %s)",
			boolOptString (&defaults->meleeToolTips));
	log_add (log_User, "  --musicresume : Resumes the music"
			"in UQM where it last left off : 0: Off | 1: 5 Minutes | "
			"2: Indefinite (default: 0)");
	log_add (log_User, "  --windowtype : Choose between DOS, 3DO or "
			"UQM window types : 0: DOS | 1: 3DO | 2: UQM (default: 0)");
	log_add (log_User, "  --scatterelements : Scatter a percentage of the"
			"elements in the lander's cargo hold onto the planet's surface"
			" when the lander explodes (default: %s)",
			boolOptString (&defaults->scatterElements));
	log_add (log_User, "  --showupgrades : Show lander upgrade graphics "
			"when exploring planets (default: %s)",
			boolOptString (&defaults->showUpgrades));
	log_add (log_User, "  --fleetpointsys : Restrict the amount of ships "
			"that can be purchased via their melee points (default: %s)",
			boolOptString (&defaults->fleetPointSys));

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
