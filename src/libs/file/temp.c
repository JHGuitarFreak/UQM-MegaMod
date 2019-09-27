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

// Contains code handling temporary files and dirs

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifdef WIN32
#	include <io.h>
#endif
#include <string.h>
#include <time.h>
#include "filintrn.h"
#include "libs/timelib.h"
#include "port.h"
#include "libs/compiler.h"
#include "libs/log.h"
#include "libs/memlib.h"

static char *tempDirName;
uio_DirHandle *tempDir;

static void
removeTempDir (void)
{
	rmdir (tempDirName);
}

// Try if the null-terminated path 'dir' to a directory is valid
// as temp path.
// On success, 'buf' will be filled with the path, with a trailing /,
// null-terminated, and 0 is returned.
// On failure, EINVAL, ENAMETOOLONG, or one of the errors access() can return
// is returned, and the contents of buf is unspecified.
static int
tryTempDir (char *buf, size_t buflen, const char *dir)
{
	size_t len;
	int haveSlash;
	
	if (dir == NULL)
		return EINVAL;
	
	if (dir[0] == '\0')
		return EINVAL;

	len = strlen (dir);
	haveSlash = (dir[len - 1] == '/'
#ifdef WIN32
			|| dir[len - 1] == '\\'
#endif
			);
	if ((haveSlash ? len : len + 1) >= buflen)
		return ENAMETOOLONG;
	
	strcpy (buf, dir);
#if 0
	//def WIN32
	{
		char *bufPtr;
		for (bufPtr = buf; *bufPtr != '\0'; bufPtr++)
		{
			if (*bufPtr == '\\')
				*bufPtr = '/';
		}
	}
#endif
	if (!haveSlash)
	{
		buf[len] = '/';
		len++;
		buf[len] = '\0';
	}
	if (access (buf, R_OK | W_OK) == -1)
		return errno;

	return 0;
}

static void
getTempDir (char *buf, size_t buflen) {
	char cwd[PATH_MAX];
	
	if (tryTempDir (buf, buflen, getenv("TMP")) &&
			tryTempDir (buf, buflen, getenv("TEMP")) &&
#if !defined(WIN32) || defined (__CYGWIN__)
			tryTempDir (buf, buflen, "/tmp/") &&
			tryTempDir (buf, buflen, "/var/tmp/") &&
#endif
			tryTempDir (buf, buflen, getcwd (cwd, sizeof cwd)))
	{
		log_add (log_Fatal, "Fatal Error: Cannot find a suitable location "
				"to store temporary files.");
		exit (EXIT_FAILURE);
	}
}

// Sets the global var 'tempDir'
static int
mountTempDir(const char *name) {
	static uio_AutoMount *autoMount[] = { NULL };
	uio_MountHandle *tempHandle;
	extern uio_Repository *repository;

	tempHandle = uio_mountDir (repository, "/tmp/",
			uio_FSTYPE_STDIO, NULL, NULL, name, autoMount,
			uio_MOUNT_TOP, NULL);
	if (tempHandle == NULL) {
		int saveErrno = errno;
		log_add (log_Fatal, "Fatal error: Couldn't mount temp dir '%s': "
				"%s", name, strerror (errno));
		errno = saveErrno;
		return -1;
	}

	tempDir = uio_openDir (repository, "/tmp", 0);
	if (tempDir == NULL) {
		int saveErrno = errno;
		log_add (log_Fatal, "Fatal error: Could not open temp dir: %s",
				strerror (errno));
		errno = saveErrno;
		return -1;
	}
	return 0;
}

#define NUM_TEMP_RETRIES 16
		// Number of files to try to open before giving up.
void
initTempDir (void) {
	size_t len;
	DWORD num;
	int i;
	char *tempPtr;
			// Pointer to the location in the tempDirName string where the
			// path to the temp dir ends and the dir starts.

	tempDirName = HMalloc (PATH_MAX);
	getTempDir (tempDirName, PATH_MAX - 21);
			// reserve 8 chars for dirname, 1 for slash, and 12 for filename
	len = strlen(tempDirName);

	num = ((DWORD) time (NULL));
//	num = GetTimeCounter () % 0xffffffff;
	tempPtr = tempDirName + len;
	for (i = 0; i < NUM_TEMP_RETRIES; i++)
	{
		sprintf (tempPtr, "%08x", num + i);
		if (createDirectory (tempDirName, 0700) == -1)
			continue;
		
		// Success, we've got a temp dir.
		tempDirName = HRealloc (tempDirName, len + 9);
		atexit (removeTempDir);
		if (mountTempDir (tempDirName) == -1)
			exit (EXIT_FAILURE);
		return;
	}
	
	// Failure, could not make a temporary directory.
	log_add (log_Fatal, "Fatal error: Cannot get a name for a temporary "
			"directory.");
	exit (EXIT_FAILURE);
}

void
unInitTempDir (void) {
	uio_closeDir(tempDir);
	// the removing of the dir is handled via atexit
}

// return the path to a file in the temp dir with the specified filename.
// returns a pointer to a static buffer.
char *
tempFilePath (const char *filename) {
	static char file[PATH_MAX];
	
	if (snprintf (file, PATH_MAX, "%s/%s", tempDirName, filename) == -1) {
		log_add (log_Fatal, "Path to temp file too long.");
		exit (EXIT_FAILURE);
	}
	return file;
}


