/*
 * Copyright (C) 2003  Serge van den Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef LIBS_UIO_UIOPORT_H_
#define LIBS_UIO_UIOPORT_H_

#ifdef _MSC_VER
#	include <io.h>
#else
#	include <unistd.h>
#endif


// Compilation related
#ifndef inline
#	ifdef _MSC_VER
#		define inline __inline
#	else
#		define inline __inline__
#	endif
#endif

// Paths
#ifdef WIN32
#	include <stdlib.h>
#	define PATH_MAX  _MAX_PATH
#	define NAME_MAX  _MAX_FNAME
		// _MAX_DIR and FILENAME_MAX could also be candidates.
		// If anyone can tell me which one matches NAME_MAX, please
		// let me know.
#elif defined(_WIN32_WCE)
#	include <sys/syslimits.h>
#else
#	include <limits.h>
		/* PATH_MAX is per POSIX defined in <limits.h>, but:
		 * "A definition of one of the values from Table 2.6 shall bea
		 * omitted from <limits.h> on specific implementations where the
		 * corresponding value is equal to or greater than the
		 * stated minimum, but where the value can vary depending
		 * on the file to which it is applied. The actual value supported
		 * for a specific pathname shall be provided by the pathconf()
		 * function."
		 * _POSIX_NAME_MAX will provide a minimum (14).
		 * This is relevant (at least) for Solaris.
		 */
#	ifndef NAME_MAX
#		define NAME_MAX _POSIX_NAME_MAX
#	endif
#endif

// Variations in path handling
#if defined(WIN32) || defined(__SYMBIAN32__)
	// HAVE_DRIVE_LETTERS is defined to signify that DOS/Windows style drive
	// letters are to be recognised on this platform.
#	define HAVE_DRIVE_LETTERS
	// BACKSLASH_IS_PATH_SEPARATOR is defined to signify that the backslash
	// character is to be recognised as a path separator on this platform.
	// This does not affect the acceptance of forward slashes as path
	// separators.
#	define BACKSLASH_IS_PATH_SEPARATOR
#endif
#if defined(WIN32)
	// HAVE_UNC_PATHS is defined to signify that Universal Naming Convention
	// style paths are to be recognised on this platform.
#	define HAVE_UNC_PATHS
#endif

// User ids
#ifdef WIN32
typedef short uid_t;
typedef short gid_t;
#endif

// Some types
#ifdef _MSC_VER
typedef int ssize_t;
typedef unsigned short mode_t;
#endif

// Directories
#include <sys/stat.h>
#ifdef WIN32
#	ifdef _MSC_VER
#		define MKDIR(name, mode) ((void) mode, _mkdir(name))
#	else
#		define MKDIR(name, mode) ((void) mode, mkdir(name))
#	endif
#else
#	define MKDIR mkdir
#endif
#ifdef _MSC_VER
#	include <direct.h>
#	define chdir _chdir
#	define getcwd _getcwd
#	define chdir _chdir
#	define getcwd _getcwd
#	define access _access
#	define F_OK 0
#	define W_OK 2
#	define R_OK 4
#	define open _open
#	define read _read
#	define rmdir _rmdir
#	define lseek _lseek
#	define lstat _lstat
#	define fstat _fstat
#	define S_IRUSR S_IREAD
#	define S_IWUSR S_IWRITE
#	define S_IXUSR S_IEXEC
#	define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#	define S_IRGRP 0
#	define S_IWGRP 0
#	define S_IXGRP 0
#	define S_IROTH 0
#	define S_IWOTH 0
#	define S_IXOTH 0
#	define S_IRWXG 0
#	define S_IRWXO 0
#	define S_ISUID 0
#	define S_ISGID 0
#	define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#	define S_IFMT _S_IFMT
#	define S_IFREG _S_IFREG
#	define S_IFCHR _S_IFCHR
#	define S_IFDIR _S_IFDIR
#	define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#	define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#	define write _write
#	define stat _stat
#	define unlink _unlink
#elif defined (__MINGW32__)
#	define S_IRGRP 0
#	define S_IWGRP 0
#	define S_IXGRP 0
#	define S_IROTH 0
#	define S_IWOTH 0
#	define S_IXOTH 0
#	define S_IRWXG 0
#	define S_IRWXO 0
#	define S_ISUID 0
#	define S_ISGID 0
#	define S_IFMT _S_IFMT
#	define S_IFREG _S_IFREG
#	define S_IFCHR _S_IFCHR
#	define S_IFDIR _S_IFDIR
#endif
#ifdef __SYMBIAN32__
	// TODO: Symbian doesn't have readdir_r(). If uio is to be usable
	// outside of uqm (which defines its own backup readdir_r()), an
	// implementation of that function needs to be added to uio.
#	include <dirent.h>
	int readdir_r (DIR *dirp, struct dirent *entry, struct dirent **result);
#endif

#endif  /* LIBS_UIO_UIOPORT_H_ */

