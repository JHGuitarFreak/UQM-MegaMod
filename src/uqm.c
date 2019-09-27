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
	
	// Commandline and user config options
	DECL_CONFIG_OPTION(bool, opengl);
	DECL_CONFIG_OPTION2(int, resolution, width, height);
	DECL_CONFIG_OPTION(bool, fullscreen);
	DECL_CONFIG_OPTION(bool, scanlines);
	DECL_CONFIG_OPTION(int, scaler);
	DECL_CONFIG_OPTION(bool, showFps);
	DECL_CONFIG_OPTION(bool, keepAspectRatio);
	DECL_CONFIG_OPTION(float, gamma);
	DECL_CONFIG_OPTION(int, soundDriver);
	DECL_CONFIG_OPTION(int, soundQuality);
	DECL_CONFIG_OPTION(bool, use3doMusic);
	DECL_CONFIG_OPTION(bool, useRemixMusic);
	DECL_CONFIG_OPTION(bool, useSpeech);
	DECL_CONFIG_OPTION(int, whichCoarseScan);
	DECL_CONFIG_OPTION(int, whichMenu);
	DECL_CONFIG_OPTION(int, whichFonts);
	DECL_CONFIG_OPTION(int, whichIntro);
	DECL_CONFIG_OPTION(int, whichShield);
	DECL_CONFIG_OPTION(int, smoothScroll);
	DECL_CONFIG_OPTION(int, meleeScale);
	DECL_CONFIG_OPTION(bool, subtitles);
	DECL_CONFIG_OPTION(bool, stereoSFX);
	DECL_CONFIG_OPTION(float, musicVolumeScale);
	DECL_CONFIG_OPTION(float, sfxVolumeScale);
	DECL_CONFIG_OPTION(float, speechVolumeScale);
	DECL_CONFIG_OPTION(bool, safeMode);
	DECL_CONFIG_OPTION(int, resolutionFactor); // JMS_GFX
	DECL_CONFIG_OPTION(int, loresBlowupScale); // JMS_GFX
 	DECL_CONFIG_OPTION(bool, cheatMode); // JMS
	// Serosis
	DECL_CONFIG_OPTION(bool, godMode);
	DECL_CONFIG_OPTION(int, timeDilationScale);
	DECL_CONFIG_OPTION(bool, bubbleWarp);
	DECL_CONFIG_OPTION(bool, unlockShips);
	DECL_CONFIG_OPTION(bool, headStart);
	DECL_CONFIG_OPTION(bool, unlockUpgrades);
	DECL_CONFIG_OPTION(bool, infiniteRU);
	DECL_CONFIG_OPTION(bool, skipIntro);
	// JMS
	DECL_CONFIG_OPTION(bool, mainMenuMusic);
	DECL_CONFIG_OPTION(bool, nebulae);
	DECL_CONFIG_OPTION(bool, orbitingPlanets);
	DECL_CONFIG_OPTION(bool, texturedPlanets);
	// Nic
	DECL_CONFIG_OPTION(int, optDateFormat);
	// Serosis
	DECL_CONFIG_OPTION(bool, infiniteFuel);
	DECL_CONFIG_OPTION(bool, partialPickup);
	DECL_CONFIG_OPTION(bool, submenu);
	DECL_CONFIG_OPTION(bool, addDevices);
	DECL_CONFIG_OPTION(bool, scalePlanets);
	DECL_CONFIG_OPTION(bool, customBorder);
	DECL_CONFIG_OPTION(int, customSeed);
	DECL_CONFIG_OPTION(bool, spaceMusic);
	DECL_CONFIG_OPTION(bool, volasMusic);
	DECL_CONFIG_OPTION(bool, wholeFuel);
	DECL_CONFIG_OPTION(bool, directionalJoystick); // For Android
	DECL_CONFIG_OPTION(bool, landerHold);
	DECL_CONFIG_OPTION(int,  ipTrans);
	DECL_CONFIG_OPTION(int,  optDifficulty);
	DECL_CONFIG_OPTION(bool, fuelRange);
	DECL_CONFIG_OPTION(bool, extended);
	DECL_CONFIG_OPTION(bool, nomad);

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
#if defined(ANDROID) || defined(__ANDROID__)
		INIT_CONFIG_OPTION(	 opengl,            false ),
		INIT_CONFIG_OPTION2( resolution,        320, 240 ),
		INIT_CONFIG_OPTION(	 fullscreen,        true ),
#else
		INIT_CONFIG_OPTION(	 opengl,            true ),
		INIT_CONFIG_OPTION2( resolution,        640, 480 ),
		INIT_CONFIG_OPTION(	 fullscreen,        false ),
#endif
		INIT_CONFIG_OPTION(  scanlines,         false ),
		INIT_CONFIG_OPTION(  scaler,            0 ),
		INIT_CONFIG_OPTION(  showFps,           false ),
		INIT_CONFIG_OPTION(  keepAspectRatio,   true ),
		INIT_CONFIG_OPTION(  gamma,             1.0f ),
		INIT_CONFIG_OPTION(  soundDriver,       audio_DRIVER_MIXSDL ),
		INIT_CONFIG_OPTION(  soundQuality,      audio_QUALITY_HIGH ),
		INIT_CONFIG_OPTION(  use3doMusic,       true ),
		INIT_CONFIG_OPTION(  useRemixMusic,     false ),
		INIT_CONFIG_OPTION(  useSpeech,         true ),
		INIT_CONFIG_OPTION(  whichCoarseScan,   OPT_PC ),
		INIT_CONFIG_OPTION(  whichMenu,         OPT_3DO ),
		INIT_CONFIG_OPTION(  whichFonts,        OPT_3DO ),
		INIT_CONFIG_OPTION(  whichIntro,        OPT_PC ),
		INIT_CONFIG_OPTION(  whichShield,       OPT_3DO ),
		INIT_CONFIG_OPTION(  smoothScroll,      OPT_PC ),
		INIT_CONFIG_OPTION(	 meleeScale,        TFB_SCALE_TRILINEAR),
		INIT_CONFIG_OPTION(  subtitles,         true ),
		INIT_CONFIG_OPTION(  stereoSFX,         false ),
		INIT_CONFIG_OPTION(  musicVolumeScale,  1.0f ),
		INIT_CONFIG_OPTION(  sfxVolumeScale,    1.0f ),
		INIT_CONFIG_OPTION(  speechVolumeScale, 0.8f ),
		INIT_CONFIG_OPTION(  safeMode,          false ),
		INIT_CONFIG_OPTION(  resolutionFactor,  0 ),
#if defined(ANDROID) || defined(__ANDROID__)
		INIT_CONFIG_OPTION(	 loresBlowupScale,  0),
#else
		INIT_CONFIG_OPTION(	 loresBlowupScale,  1),
#endif
		INIT_CONFIG_OPTION(  cheatMode,			false ), // JMS
		//Serosis
		INIT_CONFIG_OPTION(  godMode,			false ), 
		INIT_CONFIG_OPTION(  timeDilationScale,	0 ),
		INIT_CONFIG_OPTION(  bubbleWarp,		false ),
		INIT_CONFIG_OPTION(  unlockShips,		false ),
		INIT_CONFIG_OPTION(  headStart,			false ),
		INIT_CONFIG_OPTION(  unlockUpgrades,	false ),
		INIT_CONFIG_OPTION(  infiniteRU,		false ),
		INIT_CONFIG_OPTION(  skipIntro,			false ),
		// JMS
		INIT_CONFIG_OPTION(  mainMenuMusic,     true ),
		INIT_CONFIG_OPTION(  nebulae,			true ),
		INIT_CONFIG_OPTION(  orbitingPlanets,	false ),
		INIT_CONFIG_OPTION(  texturedPlanets,	true ),
		// Nic
		INIT_CONFIG_OPTION(  optDateFormat,		0 ),
		//Serosis
		INIT_CONFIG_OPTION(  infiniteFuel,		false ),
		INIT_CONFIG_OPTION(  partialPickup,		false ),
		INIT_CONFIG_OPTION(  submenu,			true ),
		INIT_CONFIG_OPTION(  addDevices,		false ),
		INIT_CONFIG_OPTION(  scalePlanets,		true ),
		INIT_CONFIG_OPTION(  customBorder,		true ),
		INIT_CONFIG_OPTION(  customSeed,		PrimeA ),
		INIT_CONFIG_OPTION(  spaceMusic,		true ),
		INIT_CONFIG_OPTION(	 volasMusic,		false ),
		INIT_CONFIG_OPTION(	 wholeFuel,			false ),
		// For Android
#if defined(ANDROID) || defined(__ANDROID__)
		INIT_CONFIG_OPTION(	 directionalJoystick, true ),
#else
		INIT_CONFIG_OPTION(	 directionalJoystick, false ),
#endif
		INIT_CONFIG_OPTION(	 landerHold,		OPT_PC),
		INIT_CONFIG_OPTION(	 ipTrans,			OPT_PC),
		INIT_CONFIG_OPTION(  optDifficulty,		0 ),
		INIT_CONFIG_OPTION(	 fuelRange,			false),
		INIT_CONFIG_OPTION(	 extended,			false),
		INIT_CONFIG_OPTION(	 nomad,				false),

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
#if defined(DEBUG)	
	else {
		// MB: Output log to logfile by default, not console
		freopen("uqm.log", "w", stderr);
	}
#endif

	if (options.runMode == runMode_version)
	{
 		printf ("%d.%d.%f %s\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
				UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
		log_showBox (false, false);
		return EXIT_SUCCESS;
	}
	
	log_add (log_User, "The Ur-Quan Masters v%d.%d.%g %s (compiled %s %s)\n"
	        "This software comes with ABSOLUTELY NO WARRANTY;\n"
			"for details see the included 'COPYING' file.\n",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION, UQM_EXTRA_VERSION,
			__DATE__, __TIME__);
#ifdef NETPLAY
	log_add (log_User, "Netplay protocol version %d.%d. Netplay opponent "
			"must have UQM %d.%d.%g or later.\n",
			NETPLAY_PROTOCOL_VERSION_MAJOR, NETPLAY_PROTOCOL_VERSION_MINOR,
			NETPLAY_MIN_UQM_VERSION_MAJOR, NETPLAY_MIN_UQM_VERSION_MINOR,
			NETPLAY_MIN_UQM_VERSION_PATCH);
#endif

	// Serosis - Compiler info to help with future debugging.
#ifdef _MSC_VER
		printf("MSC_VER: %d\n", _MSC_VER);
		printf("MSC_FULL_VER: %d\n", _MSC_FULL_VER);
		printf("MSC_BUILD: %d\n\n", _MSC_BUILD);
		log_add(log_Info, "MSC_VER: %d", _MSC_VER);
		log_add(log_Info, "MSC_FULL_VER: %d", _MSC_FULL_VER);
		log_add(log_Info, "MSC_BUILD: %d\n", _MSC_BUILD);
#endif // _MSC_VER

#ifdef __GNUC__
		printf("GCC_VERSION: %d.%d.%d\n\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		log_add(log_Info, "GCC_VERSION: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif // __GNUC__

		printf("Buid Time: %s %s\n\n", __DATE__, __TIME__);

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
	
	resolutionFactor = (unsigned int) options.resolutionFactor.value; // JMS_GFX
	loresBlowupScale = (unsigned int) options.loresBlowupScale.value; // JMS_GFX
	
	optGodMode = options.godMode.value; // JMS
	// Serosis
	timeDilationScale = options.timeDilationScale.value;
	optBubbleWarp = options.bubbleWarp.value;
	optUnlockShips = options.unlockShips.value;
	optHeadStart = options.headStart.value;
	optUnlockUpgrades = options.unlockUpgrades.value;
	optInfiniteRU = options.infiniteRU.value;
	optSkipIntro = options.skipIntro.value;
	// JMS
	optMainMenuMusic = options.mainMenuMusic.value;
	optNebulae = options.nebulae.value;
	optOrbitingPlanets = options.orbitingPlanets.value;
	optTexturedPlanets = options.texturedPlanets.value;
 	optCheatMode = options.cheatMode.value;
	// Nic
	optDateFormat = options.optDateFormat.value;
	// Serosis	
	optInfiniteFuel = options.infiniteFuel.value;
	optPartialPickup = options.partialPickup.value;
	optSubmenu = options.submenu.value;
	optAddDevices = options.addDevices.value;
	optScalePlanets = options.scalePlanets.value;
	optCustomBorder = options.customBorder.value;
	optCustomSeed = options.customSeed.value;
	optRequiresReload = FALSE; // Serosis
	optRequiresRestart = FALSE; // JMS_GFX
	optSpaceMusic = options.spaceMusic.value;
	optVolasMusic = options.volasMusic.value;
	optWholeFuel = options.wholeFuel.value;
	optDirectionalJoystick = options.directionalJoystick.value; // For Android
	optLanderHold = options.landerHold.value;
	optIPScaler = options.ipTrans.value;
	optDifficulty = options.optDifficulty.value;
	optFuelRange = options.fuelRange.value;
	optExtended = options.extended.value;
	optNomad = options.nomad.value;

	prepareContentDir (options.contentDir, options.addonDir, argv[0]);
	prepareMeleeDir ();
	prepareSaveDir ();
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

	gfxDriver = options.opengl.value ?
			TFB_GFXDRIVER_SDL_OPENGL : TFB_GFXDRIVER_SDL_PURE;
	gfxFlags = options.scaler.value;
	if (options.fullscreen.value)
		gfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
	if (options.scanlines.value)
		gfxFlags |= TFB_GFXFLAGS_SCANLINES;
	if (options.showFps.value)
		gfxFlags |= TFB_GFXFLAGS_SHOWFPS;
	TFB_InitGraphics (gfxDriver, gfxFlags, options.resolution.width,
			options.resolution.height, &resolutionFactor);
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
			(volatile int *)ImmediateInputState.key, NUM_TEMPLATES, NUM_KEYS);
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

	if (res_IsInteger ("config.reswidth") && res_IsInteger ("config.resheight")
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

	getBoolConfigValue (&options->fullscreen, "config.fullscreen");
	getBoolConfigValue (&options->scanlines, "config.scanlines");
	getBoolConfigValue (&options->showFps, "config.showfps");
	getBoolConfigValue (&options->keepAspectRatio, "config.keepaspectratio");
	getGammaConfigValue (&options->gamma, "config.gamma");

	getBoolConfigValue (&options->subtitles, "config.subtitles");
	
	getBoolConfigValueXlat (&options->whichMenu, "config.textmenu",
			OPT_PC, OPT_3DO);
	getBoolConfigValueXlat (&options->whichFonts, "config.textgradients",
			OPT_PC, OPT_3DO);
	getBoolConfigValueXlat (&options->whichCoarseScan, "config.iconicscan",
			OPT_3DO, OPT_PC);
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
	if (res_IsInteger("config.smoothmelee") && !options->meleeScale.set) {
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
	
	// JMS_GFX
	if (res_IsInteger ("config.resolutionfactor") && !options->resolutionFactor.set)
	{
		options->resolutionFactor.value = res_GetInteger ("config.resolutionfactor");
		options->resolutionFactor.set = true;
	}
	
	// JMS_GFX
	if (res_IsInteger ("config.loresBlowupScale"))
	{
		options->loresBlowupScale.value = res_GetInteger ("config.loresBlowupScale");
		options->loresBlowupScale.set = true;
	}

	getBoolConfigValue (&options->cheatMode, "cheat.kohrStahp"); // JMS
	// Serosis
	getBoolConfigValue (&options->godMode, "cheat.godMode");
	if (res_IsInteger ("cheat.timeDilation") && !options->timeDilationScale.set) {
		options->timeDilationScale.value = res_GetInteger ("cheat.timeDilation");
	}
	getBoolConfigValue (&options->bubbleWarp, "cheat.bubbleWarp");
	getBoolConfigValue (&options->unlockShips, "cheat.unlockShips");
	getBoolConfigValue (&options->headStart, "cheat.headStart");
	getBoolConfigValue (&options->unlockUpgrades, "cheat.unlockUpgrades");
	getBoolConfigValue (&options->infiniteRU, "cheat.infiniteRU");
	getBoolConfigValue (&options->skipIntro, "config.skipIntro");
	// JMS
	getBoolConfigValue (&options->mainMenuMusic, "config.mainMenuMusic");
	getBoolConfigValue (&options->nebulae, "config.nebulae");
	getBoolConfigValue (&options->orbitingPlanets, "config.orbitingPlanets");
	getBoolConfigValue (&options->texturedPlanets, "config.texturedPlanets");
	// Nic	
	if (res_IsInteger ("config.dateFormat") && !options->optDateFormat.set) {
		options->optDateFormat.value = res_GetInteger ("config.dateFormat");
	}
	// Serosis	
	getBoolConfigValue (&options->infiniteFuel, "cheat.infiniteFuel");
	getBoolConfigValue (&options->partialPickup, "config.partialPickup");
	getBoolConfigValue (&options->submenu, "config.submenu");
	getBoolConfigValue (&options->addDevices, "cheat.addDevices");
	getBoolConfigValue (&options->scalePlanets, "config.scalePlanets");
	getBoolConfigValue (&options->customBorder, "config.customBorder");
	if (res_IsInteger ("config.customSeed") && !options->customSeed.set) {
		options->customSeed.value = res_GetInteger ("config.customSeed");
	}
	getBoolConfigValue (&options->spaceMusic, "config.spaceMusic");
	getBoolConfigValue(&options->volasMusic, "config.volasMusic");
	getBoolConfigValue(&options->wholeFuel, "config.wholeFuel");

#if defined(ANDROID) || defined(__ANDROID__)
	getBoolConfigValue (&options->directionalJoystick, "config.directionaljoystick"); // For Android
#endif

	getBoolConfigValueXlat(&options->landerHold, "config.landerhold",
		OPT_3DO, OPT_PC);
	getBoolConfigValueXlat(&options->ipTrans, "config.iptransition",
		OPT_3DO, OPT_PC);
	if (res_IsInteger("config.difficulty") && !options->optDifficulty.set) {
		options->optDifficulty.value = res_GetInteger("config.difficulty");
	}
	getBoolConfigValue(&options->fuelRange, "config.fuelrange");
	getBoolConfigValue(&options->extended, "config.extended");
	getBoolConfigValue(&options->nomad, "config.nomad");
	
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
	CHEATMODE_OPT, //Serosis
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
	SCALEPLAN_OPT,
	CUSTBORD_OPT,
	EXSEED_OPT,
	SPACEMUSIC_OPT,
	WHOLEFUEL_OPT,
	DIRJOY_OPT,
	LANDHOLD_OPT,
	IPTRANS_OPT,
	DIFFICULTY_OPT,
	FUELRANGE_OPT,
	EXTENDED_OPT,
	NOMAD_OPT,
	MELEE_OPT,
	LOADGAME_OPT,
#ifdef NETPLAY
	NETHOST1_OPT,
	NETPORT1_OPT,
	NETHOST2_OPT,
	NETPORT2_OPT,
	NETDELAY_OPT,
#endif
};

static const char *optString = "+r:foc:b:spC:n:?hM:S:T:q:ug:l:i:vwxk";
static struct option longOptions[] = 
{
	{"res", 1, NULL, 'r'},
	{"fullscreen", 0, NULL, 'f'},
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
	{"kohrstahp", 0, NULL, CHEATMODE_OPT}, //Serosis
	{"godmode", 0, NULL, GODMODE_OPT},
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
	{"scaledevices", 0, NULL, SCALEPLAN_OPT},
	{"customborder", 0, NULL, CUSTBORD_OPT},
	{"customseed", 1, NULL, EXSEED_OPT},
	{"spacemusic", 0, NULL, SPACEMUSIC_OPT},
	{"wholefuel", 0, NULL, WHOLEFUEL_OPT},
	{"dirjoystick", 0, NULL, DIRJOY_OPT},
	{"landerhold", 0, NULL, LANDHOLD_OPT},
	{"iptrans", 1, NULL, IPTRANS_OPT},
	{"melee", 0, NULL, MELEE_OPT},
	{"loadgame", 0, NULL, LOADGAME_OPT},
	{"difficulty", 1, NULL, DIFFICULTY_OPT},
	{"fuelrange", 0, NULL, FUELRANGE_OPT},
	{"extended", 0, NULL, EXTENDED_OPT},
	{"nomad", 0, NULL, NOMAD_OPT},
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
				setBoolOption (&options->fullscreen, true);
				break;
			case 'w':
				setBoolOption (&options->fullscreen, false);
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
				if (!setChoiceOption (&options->whichCoarseScan, optarg))
				{
					InvalidArgument (optarg, "--cscan");
					badArg = true;
				}
				break;
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
			case CHEATMODE_OPT:
				setBoolOption (&options->cheatMode, true); //Serosis
				break;
			case GODMODE_OPT:
				setBoolOption (&options->godMode, true);
				break;
			case TDM_OPT:{
				int temp;
				if (parseIntOption (optarg, &temp, "Time Dilation scale") == -1) {
					badArg = true;
					break;
				} else if (temp < 0 || temp > 2) {					
					saveError ("\nTime Dilation scale has to be 0, 1, or 2.\n");
					badArg = true;
				} else {
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
				setBoolOption (&options->unlockUpgrades, true);
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
			case DATE_OPT:{
				int temp;
				if (parseIntOption (optarg, &temp, "Date Format") == -1) {
					badArg = true;
					break;
				} else if (temp < 0 || temp > 3) {					
					saveError ("\nDate Format has to be 0, 1, 2, or 3.\n");
					badArg = true;
				} else {
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
				setBoolOption (&options->addDevices, true);
				break;
			case SCALEPLAN_OPT:
				setBoolOption (&options->scalePlanets, true);
				break;
			case CUSTBORD_OPT:
				setBoolOption(&options->customBorder, true);
				break;
			case EXSEED_OPT:{
				int temp;
				if (parseIntOption (optarg, &temp, "Custom Seed") == -1) {
					badArg = true;
					break;
				} else if (!SANE_SEED(temp)) {
					saveError ("\nCustom Seed can not be less than 2 or greater than 2147483645.\n");
					badArg = true;
				} else {
					options->customSeed.value = temp;
					options->customSeed.set = true;
				}
				break;
			}
			case SPACEMUSIC_OPT:
				setBoolOption(&options->spaceMusic, true);
				break;
			case WHOLEFUEL_OPT:
				setBoolOption(&options->wholeFuel, true);
				break;
			case DIRJOY_OPT:
				setBoolOption(&options->directionalJoystick, true);
				break;
			case LANDHOLD_OPT:
				if (!setChoiceOption(&options->landerHold, optarg)) {
					InvalidArgument(optarg, "--landerhold");
					badArg = true;
				}
				break;
			case IPTRANS_OPT:
				if (!setChoiceOption(&options->ipTrans, optarg)) {
					InvalidArgument(optarg, "--iptrans");
					badArg = true;
				}
				break;
			case DIFFICULTY_OPT: {
				int temp;
				if (parseIntOption(optarg, &temp, "Difficulty") == -1) {
					badArg = true;
					break;
				}
				else if (temp < 0 || temp > 3) {
					saveError("\nDifficulty has to be 0, 1, 2, or 3.\n");
					badArg = true;
				}
				else {
					options->optDifficulty.value = temp;
					options->optDifficulty.set = true;
				}
				break;
			}
			case FUELRANGE_OPT:
				setBoolOption(&options->fuelRange, true);
				break;
			case EXTENDED_OPT:
				setBoolOption(&options->extended, true);
				break;
			case NOMAD_OPT:
				setBoolOption(&options->nomad, true);
				break;
			case MELEE_OPT:
				optSuperMelee = TRUE;
				break;
			case LOADGAME_OPT:
				optLoadGame = TRUE;
				break;
			case ADDON_OPT:
				options->numAddons++;
				options->addons = HRealloc ((void *) options->addons,
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

	if (str[0] == '\0')
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
	log_add (log_User, "  -f, --fullscreen (default: %s)",
			boolOptString (&defaults->fullscreen));
	log_add (log_User, "  -w, --windowed (default: %s)",
			boolNotOptString (&defaults->fullscreen));
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

	log_add (log_User, "The following options are for the Mega Mod"); // Serosis
	log_add (log_User, "  --kohrstahp : Stops Kohr-Ah advancing.    (default: %s)",
			boolOptString (&defaults->cheatMode));
	log_add (log_User, "  --godmode : Player ships and lander invulnerable. "
			"Also refills energy every shot during melee.    (default: %s)",
			boolOptString (&defaults->godMode));
	log_add (log_User, "  --timedilation : =1 Time is slowed down times 6. "
			"=2 Time is sped up times 5    (default: 0)");
	log_add (log_User, "  --bubblewarp : Instantaneous travel to any point on "
			"the Starmap.    (default: %s)",
			boolOptString (&defaults->bubbleWarp));
	log_add (log_User, "  --unlockships : Allows you to purchase ships that you can't "
			"normally acquire in the main game.    (default: %s)",
			boolOptString (&defaults->unlockShips));
	log_add (log_User, "  --headstart : Gives you an extra storage bay full of minerals, Fwiffo, "
			"and the Moonbase during a new game   (default: %s)",
			boolOptString (&defaults->headStart));
	log_add (log_User, "  --unlockupgrades : Unlocks every upgrade for your flagship "
			"and landers.    (default: %s)",
			boolOptString (&defaults->unlockUpgrades));
	log_add (log_User, "  --infiniteru : Gives you infinite R.U. as long as the cheat is on "
			" (default: %s)",
			boolOptString (&defaults->infiniteRU));
	log_add (log_User, "  --skipintro : Skips the intro and Logo fmv    (default: %s)",
			boolOptString (&defaults->skipIntro));
	log_add (log_User, "  --mainmenumusic : Switches the main menu music on/off    (default: %s)",
			boolOptString (&defaults->mainMenuMusic));
	log_add (log_User, "  --nebulae : Enables/Disables nebulae in star systems    (default: %s)",
			boolOptString (&defaults->nebulae));
	log_add (log_User, "  --orbitingplanets : Enables/Disables orbiting planets in star systems    (default: %s)",
			boolOptString (&defaults->orbitingPlanets));
	log_add (log_User, "  --texturedplanets : Enables/Disables textured planets in star systems    (default: %s)",
			boolOptString (&defaults->texturedPlanets));
	log_add (log_User, "  --infinitefuel : Infinite fuel in the main game    (default: %s)",
			boolOptString (&defaults->infiniteFuel));
	log_add (log_User, "  --partialpickup : Enables/Disables partial mineral pickup    (default: %s)",
			boolOptString (&defaults->partialPickup));
	log_add (log_User, "  --submenu : Enables/Disables mineral and star map keys submenu    (default: %s)",
			boolOptString (&defaults->submenu));
	log_add (log_User, "  --dateformat : 0: MMM DD.YYYY | 1: MM.DD.YYYY | "
			"2: DD MMM.YYYY | 3: DD.MM.YYYY   (default: 0)");
	log_add (log_User, "  --adddevices : Gives you all available devices    (default: %s)",
			boolOptString (&defaults->addDevices));
	log_add (log_User, "  --scaleplanets : Scales textured planets in HD    (default: %s)",
			boolOptString (&defaults->scalePlanets));
	log_add (log_User, "  --melee : Takes you straight to Super Melee after the splash screen.");
	log_add (log_User, "  --loadgame : Takes you straight to the Load Game sceen after the splash screen.");
	log_add (log_User, "  --customborder : Enables the custom border frame.    (default: %s)",
		boolOptString(&defaults->customBorder));
	log_add(log_User, "  --customseed=# : Allows you to customize the internal seed used to generate the solar systems in-game.");
	log_add(log_User, "  --spacemusic : Enables localized music for races when you are in their sphere of influence    (default: %s)",
		boolOptString(&defaults->spaceMusic));
	log_add(log_User, "  --wholefuel : Enables the display of the whole fuel value in the ship status    (default: %s)",
		boolOptString(&defaults->wholeFuel));
	log_add(log_User, "  --dirjoystick : Enables the use of directional joystick controls for Android    (default: %s)",
		boolOptString(&defaults->directionalJoystick));
	log_add(log_User, "  --landerhold : Switch between PC/3DO max lander hold, pc=64, 3do=50 (default: %s)",
		choiceOptString(&defaults->landerHold));
	log_add(log_User, "  --iptrans : Interplanetary transitions, pc=stepped, "
		"3do=crossfade (default: %s)",
		choiceOptString(&defaults->ipTrans));
	log_add(log_User, "  --difficulty : 0: Normal | 1: Easy | 2: Hard | 3: Impossible  (default: 0)");
	log_add(log_User, "  --fuelrange : Enables 'point of no return' fuel range    (default: %s)",
		boolOptString(&defaults->fuelRange));
	log_add(log_User, "  --extended : Enables Extended Edition features    (default: %s)",
		boolOptString(&defaults->extended));
	log_add(log_User, "  --nomad : Enables 'Nomad Mode' (No Starbase)    (default: %s)",
		boolOptString(&defaults->nomad));
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

