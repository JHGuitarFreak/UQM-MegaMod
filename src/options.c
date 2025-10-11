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

#include "options.h"

#include "port.h"
#include "libs/graphics/gfx_common.h"
#include "libs/file.h"
#include "config.h"
#include "libs/compiler.h"
#include "libs/uio.h"
#include "libs/strlib.h"
#include "libs/log.h"
#include "libs/reslib.h"
#include "libs/memlib.h"
#include "uqm/starmap.h"
#include "uqm/planets/scan.h"
#include "types.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#ifdef __APPLE__
#	include <libgen.h>
			/* for dirname() */
#endif


int optWhichCoarseScan;
int optWhichMenu;
int optWhichFonts;
int optWhichIntro;
int optWhichShield;
int optSmoothScroll;
int optMeleeScale;
const char **optAddons;

unsigned int loresBlowupScale;
unsigned int resolutionFactor;
unsigned int audioDriver;
unsigned int audioQuality;

// Added options
BOOLEAN optRequiresReload;
BOOLEAN optRequiresRestart;
OPT_ENABLABLE optCheatMode;
int optGodModes;
int timeDilationScale;
OPT_ENABLABLE optBubbleWarp;
OPT_ENABLABLE optUnlockShips;
OPT_ENABLABLE optHeadStart;
OPT_ENABLABLE optUnlockUpgrades;
OPT_ENABLABLE optInfiniteRU;
DWORD oldRU;
OPT_ENABLABLE optSkipIntro;
OPT_ENABLABLE optMainMenuMusic;
OPT_ENABLABLE optNebulae;
OPT_ENABLABLE optOrbitingPlanets;
OPT_ENABLABLE optTexturedPlanets;
int optDateFormat;
OPT_ENABLABLE optInfiniteFuel;
DWORD loadFuel;
OPT_ENABLABLE optPartialPickup;
OPT_ENABLABLE optSubmenu;
OPT_ENABLABLE optInfiniteCredits;
BOOLEAN optSuperMelee;
BOOLEAN optLoadGame;
OPT_ENABLABLE optCustomBorder;
int optSeedType;
int optCustomSeed;
OPT_ENABLABLE optShipSeed;
int optSphereColors;
int spaceMusicBySOI;
int optSpaceMusic;
OPT_ENABLABLE optVolasMusic;
OPT_ENABLABLE optWholeFuel;
OPT_ENABLABLE optDirectionalJoystick;
int optLanderHold;
int optScrTrans;
int optDifficulty;
int optDiffChooser;
int optFuelRange;
OPT_ENABLABLE optExtended;
int optNomad;
OPT_ENABLABLE optGameOver;
OPT_ENABLABLE optShipDirectionIP;
OPT_ENABLABLE optHazardColors;
OPT_ENABLABLE optOrzCompFont;
int optControllerType;
OPT_ENABLABLE optSmartAutoPilot;
int optTintPlanSphere;
int optPlanetStyle;
int optStarBackground;
int optScanStyle;
OPT_ENABLABLE optNonStopOscill;
int optScopeStyle;
int optSuperPC;
OPT_ENABLABLE optHyperStars;
OPT_ENABLABLE optPlanetTexture;
int optFlagshipColor;
OPT_ENABLABLE optNoHQEncounters;
OPT_ENABLABLE optDeCleansing;
OPT_ENABLABLE optMeleeObstacles;
OPT_ENABLABLE optShowVisitedStars;
OPT_ENABLABLE optUnscaledStarSystem;
int optScanSphere;
int optNebulaeVolume;
OPT_ENABLABLE optSlaughterMode;
BOOLEAN optMaskOfDeceit;
OPT_ENABLABLE optAdvancedAutoPilot;
OPT_ENABLABLE optMeleeToolTips;
int optMusicResume;
DWORD optWindowType;
BOOLEAN optNoClassic;
OPT_ENABLABLE optScatterElements;
OPT_ENABLABLE optShowUpgrades;
OPT_ENABLABLE optFleetPointSys;
OPT_ADD_REMOVE optDeviceArray[28];
OPT_ADD_REMOVE optUpgradeArray[13];

OPT_ENABLABLE opt3doMusic;
OPT_ENABLABLE optRemixMusic;
OPT_ENABLABLE optSpeech;
OPT_ENABLABLE optSubtitles;
OPT_ENABLABLE optStereoSFX;
OPT_ENABLABLE optKeepAspectRatio;
float optGamma;
uio_DirHandle *contentDir;
uio_DirHandle *configDir;
uio_DirHandle *saveDir;
uio_DirHandle *meleeDir;
uio_DirHandle *scrShotDir;
uio_MountHandle* contentMountHandle;

char *contentDirPath;
char *addonDirPath;

char baseContentPath[PATH_MAX];

// addon availability
ADDON_COUNT addonList;

extern uio_Repository *repository;
extern uio_DirHandle *rootDir;

INPUT_TEMPLATE input_templates[6];

static const char *findFileInDirs (const char *locs[], int numLocs,
		const char *file);
static uio_MountHandle *mountContentDir (uio_Repository *repository,
		const char *contentPath);
static void mountAddonDir (uio_Repository *repository,
		uio_MountHandle *contentMountHandle, const char *addonDirName);

static void mountDirZips (uio_DirHandle *dirHandle, const char *mountPoint,
		int relativeFlags, uio_MountHandle *relativeHandle);

static void mountBaseZip (uio_DirHandle *dirHandle, const char *mountPoint,
		int relativeFlags, uio_MountHandle *relativeHandle);

// Looks for a file 'file' in all 'numLocs' locations from 'locs'.
// returns the first element from locs where 'file' is found.
// If there is no matching location, NULL will be returned and
// errno will be set to 'ENOENT'.
// Entries from 'locs' that together with 'file' are longer than
// PATH_MAX will be ignored, except for a warning given to stderr.
static const char *
findFileInDirs (const char *locs[], int numLocs, const char *file)
{
	int locI;
	char path[PATH_MAX];
	size_t fileLen;

	fileLen = strlen (file);
	for (locI = 0; locI < numLocs; locI++)
	{
		size_t locLen;
		const char *loc;
		bool needSlash;
		
		loc = locs[locI];
		locLen = strlen (loc);

		needSlash = (locLen != 0 && loc[locLen - 1] != '/');
		if (locLen + (needSlash ? 1 : 0) + fileLen + 1 >= sizeof path)
		{
			// This dir plus the file name is too long.
			log_add (log_Warning, "Warning: path '%s' is ignored"
					" because it is too long.", loc);
			continue;
		}
		
		snprintf (path, sizeof path, "%s%s%s", loc, needSlash ? "/" : "",
				file);
		if (fileExists (path))
			return loc;
	}

	// No matching location was found.
	errno = ENOENT;
	return NULL;
}

// contentDirName is an explicitely specified location for the content,
// or NULL if none was explicitely specified.
// execFile is the path to the uqm executable, as acquired through
// main()'s argv[0].
void
prepareContentDir (const char *contentDirName, const char* addonDirName,
		const char *execFile)
{
	const char *testFile = "version";
	const char *loc;

	if (contentDirName == NULL)
	{
		// Try the default content locations.
		const char *locs[] = {
			CONTENTDIR, /* defined in config.h */
			"content",
		};
		loc = findFileInDirs (locs, ARRAY_SIZE (locs), testFile);

#ifdef __APPLE__
		/* On OSX, if the content can't be found in any of the static
		 * locations, attempt to look inside the application bundle,
		 * by looking relative to the location of the uqm executable. */
		if (loc == NULL)
		{
			char *tempDir = (char *) HMalloc (PATH_MAX);
			char* execFileDup;

			/* dirname can modify its argument, so we need a local
			 * mutable copy of it. */
			execFileDup = (char *) HMalloc (strlen (execFile) + 1);
			strcpy (execFileDup, execFile);
			snprintf (tempDir, PATH_MAX, "%s/../Resources/content",
					dirname (execFileDup));
			loc = findFileInDirs ((const char **) &tempDir, 1, testFile);
			HFree (execFileDup);
			HFree (tempDir);
		}
#endif
	}
	else
	{
		// Only use the explicitely supplied content dir.
		loc = findFileInDirs (&contentDirName, 1, testFile);
	}
	if (loc == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not find content.");
		exit (EXIT_FAILURE);
	}

	if (expandPath (baseContentPath, sizeof baseContentPath, loc,
			EP_ALL_SYSTEM) == -1)
	{
		log_add (log_Fatal, "Fatal error: Could not expand path to content "
				"directory: %s", strerror (errno));
		exit (EXIT_FAILURE);
	}
	
	log_add (log_Debug, "Using '%s' as base content dir.", baseContentPath);
	contentMountHandle = mountContentDir (repository, baseContentPath);

	if (contentDirName && contentDirPath == NULL)
		contentDirPath = (char *)contentDirName;

	if (addonDirName)
	{
		if (addonDirPath == NULL)
			addonDirPath = (char *)addonDirName;

		log_add (log_Debug, "Using '%s' as addon dir.", addonDirName);
	}
	mountAddonDir (repository, contentMountHandle, addonDirName);

#ifndef __APPLE__
	(void) execFile;
#endif
}

void
prepareConfigDir (const char *configDirName) {
	char buf[PATH_MAX];
	static uio_AutoMount *autoMount[] = { NULL };
	uio_MountHandle *contentHandle;

	if (configDirName == NULL)
	{
		configDirName = getenv("UQM_CONFIG_DIR");

		if (configDirName == NULL)
			configDirName = CONFIGDIR;
	}

	if (expandPath (buf, PATH_MAX - 13, configDirName, EP_ALL_SYSTEM)
			== -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		log_add (log_Fatal, "Fatal error: Invalid path to config files.");
		exit (EXIT_FAILURE);
	}
	configDirName = buf;

	log_add (log_Debug, "Using config dir '%s'", configDirName);

	// Set the environment variable UQM_CONFIG_DIR so UQM_MELEE_DIR
	// and UQM_SAVE_DIR can refer to it.
	setenv("UQM_CONFIG_DIR", configDirName, 1);

	// Create the path upto the config dir, if not already existing.
	if (mkdirhier (configDirName) == -1)
		exit (EXIT_FAILURE);

	contentHandle = uio_mountDir (repository, "/",
			uio_FSTYPE_STDIO, NULL, NULL, configDirName, autoMount,
			uio_MOUNT_TOP, NULL);
	if (contentHandle == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not mount config dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}

	configDir = uio_openDir (repository, "/", 0);
	if (configDir == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not open config dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

void
prepareSaveDir (void) {
	char buf[PATH_MAX];
	const char *saveDirName;

	saveDirName = getenv("UQM_SAVE_DIR");
	if (saveDirName == NULL)
		saveDirName = SAVEDIR;
	
	if (expandPath (buf, PATH_MAX - 13, saveDirName, EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		log_add (log_Fatal, "Fatal error: Invalid path to config files.");
		exit (EXIT_FAILURE);
	}

	saveDirName = buf;
	setenv("UQM_SAVE_DIR", saveDirName, 1);

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier (saveDirName) == -1)
		exit (EXIT_FAILURE);

	log_add (log_Debug, "Saved games are kept in %s.", saveDirName);

	saveDir = uio_openDirRelative (configDir, "save", 0);
			// TODO: this doesn't work if the save dir is not
			//       "save" in the config dir.
	if (saveDir == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not open save dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

void
prepareMeleeDir (void) {
	char buf[PATH_MAX];
	const char *meleeDirName;

	meleeDirName = getenv("UQM_MELEE_DIR");
	if (meleeDirName == NULL)
		meleeDirName = MELEEDIR;
	
	if (expandPath (buf, PATH_MAX - 13, meleeDirName, EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		log_add (log_Fatal, "Fatal error: Invalid path to config files.");
		exit (EXIT_FAILURE);
	}

	meleeDirName = buf;
	setenv("UQM_MELEE_DIR", meleeDirName, 1);

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier (meleeDirName) == -1)
		exit (EXIT_FAILURE);

	meleeDir = uio_openDirRelative (configDir, "teams", 0);
			// TODO: this doesn't work if the save dir is not
			//       "teams" in the config dir.
	if (meleeDir == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not open melee teams dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

void
prepareScrShotDir (void)
{
	char buf[PATH_MAX];
	const char *shotDirName;

	shotDirName = getenv ("UQM_SCR_SHOT_DIR");
	if (shotDirName == NULL)
		shotDirName = SCRSHOTDIR;
	
	if (expandPath (buf, PATH_MAX - 13, shotDirName, EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		log_add (log_Fatal, "Fatal error: Invalid path to config files.");
		exit (EXIT_FAILURE);
	}

	shotDirName = buf;
	setenv("UQM_SCR_SHOT_DIR", shotDirName, 1);

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier (shotDirName) == -1)
		exit (EXIT_FAILURE);

	scrShotDir = uio_openDirRelative (configDir, "screenshots", 0);
			// TODO: this doesn't work if the save dir is not
			//       "screenshots" in the SCRSHOTDIR macro.
	if (scrShotDir == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not open screenshot dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

static uio_MountHandle *
mountContentDir (uio_Repository *repository, const char *contentPath)
{
	uio_DirHandle *packagesDir;
	static uio_AutoMount *autoMount[] = { NULL };
	uio_MountHandle *contentMountHandle;

	contentMountHandle = uio_mountDir (repository, "/",
			uio_FSTYPE_STDIO, NULL, NULL, contentPath, autoMount,
			uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
	if (contentMountHandle == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not mount content dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}

	contentDir = uio_openDir (repository, "/", 0);
	if (contentDir == NULL)
	{
		log_add (log_Fatal, "Fatal error: Could not open content dir: %s",
				strerror (errno));
		exit (EXIT_FAILURE);
	}

	packagesDir = uio_openDir (repository, "/packages", 0);
	if (packagesDir != NULL)
	{
		mountBaseZip (packagesDir, "/", uio_MOUNT_BELOW,
				contentMountHandle);
		uio_closeDir (packagesDir);
	}

	return contentMountHandle;
}

static void
mountAddonDir (uio_Repository *repository, uio_MountHandle *contentMountHandle,
		const char *addonDirName)
{
	uio_DirHandle *addonsDir;
	static uio_AutoMount *autoMount[] = { NULL };
	uio_MountHandle *mountHandle;
	uio_DirList *availableAddons;

	if (addonDirName != NULL)
	{
		mountHandle = uio_mountDir (repository, "addons",
				uio_FSTYPE_STDIO, NULL, NULL, addonDirName, autoMount,
				uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
		if (mountHandle == NULL)
		{
			log_add (log_Warning, "Warning: Could not mount addon directory: %s"
					";\n\t'--addon' options are ignored.", strerror (errno));
			return;
		}
	}
	else
	{
		mountHandle = contentMountHandle;
	}

	// NB: note the difference between addonsDir and addonDir.
	//     the former is the dir 'addons', the latter a directory
	//     in that dir.
	addonsDir = uio_openDirRelative (contentDir, "addons", 0);
	if (addonsDir == NULL)
	{	// No addon dir found.
		log_add (log_Warning, "Warning: There's no 'addons' "
				"directory in the 'content' directory;\n\t'--addon' "
				"options are ignored.");
		return;
	}

	mountDirZips (addonsDir, "addons", uio_MOUNT_BELOW, mountHandle);
			
	availableAddons = uio_getDirList (addonsDir, "", "", match_MATCH_PREFIX);
	if (availableAddons != NULL)
	{
		int i, count;
		
		// count the actual addon dirs
		count = 0;
		for (i = 0; i < availableAddons->numNames; ++i)
		{
			struct stat sb;

			if (availableAddons->names[i][0] == '.' ||
					uio_stat (addonsDir, availableAddons->names[i], &sb) == -1
					|| !S_ISDIR (sb.st_mode))
			{	// this dir entry ignored
				availableAddons->names[i] = NULL;
				continue;
			}
			++count;
		}
		log_add (log_Info, "%d available addon pack%s.", count,
				count == 1 ? "" : "s");

		count = 0;
		for (i = 0; i < availableAddons->numNames; ++i)
		{
			char mountname[128];
			uio_DirHandle *addonDir;
			const char *addon = availableAddons->names[i];
			
			if (!addon)
				continue;

			addonList.name_hash[count] = crc32b (addon);

			++count;
			log_add (log_Info, "    %d. %s", count, addon);
		
			snprintf (mountname, sizeof mountname, "addons/%s", addon);

			addonDir = uio_openDirRelative (addonsDir, addon, 0);
			if (addonDir == NULL)
			{
				log_add (log_Warning, "Warning: directory 'addons/%s' "
					 "not found; addon skipped.", addon);
				continue;
			}
			mountDirZips (addonDir, mountname, uio_MOUNT_BELOW, mountHandle);
			uio_closeDir (addonDir);
		}

		addonList.amount = count;
	}
	else
	{
		log_add (log_Info, "0 available addon packs.");
	}

	uio_DirList_free (availableAddons);
	uio_closeDir (addonsDir);
}

BOOLEAN
isAddonAvailable (const char *addon_name)
{
	COUNT i;
	DWORD name_hash = crc32b (addon_name);

	if (!name_hash)
		return FALSE;

	for (i = 0; i < addonList.amount; i++)
	{
		if (addonList.name_hash[i] == name_hash)
			return TRUE;
	}

	return FALSE;
}

static void
mountDirZips (uio_DirHandle *dirHandle, const char *mountPoint,
		int relativeFlags, uio_MountHandle *relativeHandle)
{
	static uio_AutoMount *autoMount[] = { NULL };
	uio_DirList *dirList;
	const char *pattern = "\\.([zZ][iI][pP]|[uU][qQ][mM])$";

	dirList = uio_getDirList (dirHandle, "", pattern, match_MATCH_REGEX);
	if (dirList != NULL)
	{
		int i;
		
		for (i = 0; i < dirList->numNames; i++)
		{
			if (uio_mountDir (repository, mountPoint, uio_FSTYPE_ZIP,
					dirHandle, dirList->names[i], "/", autoMount,
					relativeFlags | uio_MOUNT_RDONLY,
					relativeHandle) == NULL)
			{
				log_add (log_Warning, "Warning: Could not mount '%s': %s.",
						dirList->names[i], strerror (errno));
			}
		}
	}
	uio_DirList_free (dirList);
}

static void
mountBaseZip (uio_DirHandle *dirHandle, const char *mountPoint,
		int relativeFlags, uio_MountHandle *relativeHandle)
{
	static uio_AutoMount *autoMount[] = { NULL };
	uio_DirList *dirList;
	const char *pattern = "\\.([zZ][iI][pP]|[uU][qQ][mM])$";
	const DWORD name_hash = crc32b (BASE_CONTENT_NAME);

	dirList = uio_getDirList (dirHandle, "", pattern, match_MATCH_REGEX);
	if (dirList != NULL)
	{
		DWORD names_hash = 0;
		int i;
		
		for (i = 0; i < dirList->numNames; i++)
		{
			names_hash = crc32b (dirList->names[i]);
			if (name_hash == names_hash)
				break;
		}

		if (i == dirList->numNames)
		{
			if (name_hash != names_hash)
			{
				log_add (log_Warning, "Warning: Could not find '%s': %s.",
						BASE_CONTENT_NAME, strerror (errno));

				uio_DirList_free (dirList);
				return;
			}
			else
				i--;
		}

		if (uio_mountDir (repository, mountPoint, uio_FSTYPE_ZIP,
				dirHandle, dirList->names[i], "/", autoMount,
				relativeFlags | uio_MOUNT_RDONLY,
				relativeHandle) == NULL)
		{
			log_add (log_Warning, "Warning: Could not mount '%s': %s.",
					dirList->names[i], strerror (errno));
		}
	}
	uio_DirList_free (dirList);
}

int
loadIndices (uio_DirHandle *dir)
{
	uio_DirList *indices;
	int numLoaded = 0;

	indices = uio_getDirList (dir, "", "\\.[rR][mM][pP]$",
			match_MATCH_REGEX);		

	if (indices != NULL)
	{
		int i;
		
		for (i = 0; i < indices->numNames; i++)
		{
			log_add (log_Debug, "Loading resource index '%s'",
					indices->names[i]);
			LoadResourceIndex (dir, indices->names[i], NULL);
			numLoaded++;
		}
	}
	uio_DirList_free (indices);
	
	/* Return the number of index files loaded. */
	return numLoaded;
}

BOOLEAN
loadAddon (const char *addon)
{
	uio_DirHandle *addonsDir, *addonDir;
	int numLoaded;

	addonsDir = uio_openDirRelative (contentDir, "addons", 0);
	if (addonsDir == NULL)
	{
		// No addon dir found.
		log_add (log_Warning, "Warning: There's no 'addons' "
				"directory in the 'content' directory;\n\t'--addon' "
				"options are ignored.");
		return FALSE;
	}
	addonDir = uio_openDirRelative (addonsDir, addon, 0);
	if (addonDir == NULL)
	{
		log_add (log_Warning, "Warning: Addon '%s' not found", addon);
		uio_closeDir (addonsDir);
		return FALSE;
	}

	numLoaded = loadIndices (addonDir);
	if (!numLoaded)
	{
		log_add (log_Error, "No RMP index files were loaded for addon '%s'",
				addon);
	}

	uio_closeDir (addonDir);
	uio_closeDir (addonsDir);
	
	return (numLoaded > 0);
}

void
prepareShadowAddons (const char **addons)
{
	uio_DirHandle *addonsDir;
	const char *shadowDirName = "shadow-content";

	addonsDir = uio_openDirRelative (contentDir, "addons", 0);
	// If anything fails here, it will fail again later, so
	// we'll just keep quiet about it for now
	if (addonsDir == NULL)
		return;

	for (; *addons != NULL; addons++)
	{
		const char *addon = *addons;
		uio_DirHandle *addonDir;
		uio_DirHandle *shadowDir;

		addonDir = uio_openDirRelative (addonsDir, addon, 0);
		if (addonDir == NULL)
			continue;

		// Mount addon's "shadow-content" on top of "/"
		shadowDir = uio_openDirRelative (addonDir, shadowDirName, 0);
		if (shadowDir)
		{
			log_add (log_Debug, "Mounting shadow content of '%s' addon", addon);
			mountDirZips (shadowDir, "/", uio_MOUNT_ABOVE, contentMountHandle);
			// Mount non-zipped shadow content
			if (uio_transplantDir ("/", shadowDir, uio_MOUNT_RDONLY |
					uio_MOUNT_ABOVE, contentMountHandle) == NULL)
			{
				log_add (log_Warning, "Warning: Could not mount shadow content"
						" of '%s': %s.", addon, strerror (errno));
			}

			uio_closeDir (shadowDir);
		}
		uio_closeDir (addonDir);
	}

	uio_closeDir (addonsDir);
}

void
prepareAddons (const char **addons)
{
	for (; *addons != NULL; addons++)
	{
		log_add (log_Info, "Loading addon '%s'", *addons);
		if (!loadAddon (*addons))
		{
			// TODO: Should we do something like inform the user?
			//   Why simply refuse to load other addons?
			//   Maybe exit() to inform the user of the failure?
			break;
		}
	}
}

void
unprepareAllDirs (void)
{
	if (saveDir)
	{
		uio_closeDir (saveDir);
		saveDir = 0;
	}
	if (meleeDir)
	{
		uio_closeDir (meleeDir);
		meleeDir = 0;
	}
	if (contentDir)
	{
		uio_closeDir (contentDir);
		contentDir = 0;
	}
	if (configDir)
	{
		uio_closeDir (configDir);
		configDir = 0;
	}
	if (scrShotDir)
	{
		uio_closeDir (scrShotDir);
		scrShotDir = 0;
	}
}

bool
setGammaCorrection (float gamma)
{
	bool set = TFB_SetGamma (gamma);
	if (set)
		log_add (log_Info, "Gamma correction set to %.4f.", gamma);
	else
		log_add (log_Warning, "Unable to set gamma correction.");
	return set;
}
