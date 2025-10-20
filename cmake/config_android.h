/* This file contains some compile-time configuration options for *nix
 * systems.
 * config_unix.h is generated from config_unix.h.in by build.sh
 * When building on MS Windows using build.sh (MinGW, Cygwin),
 * config_win.h is generated from src/config_win.h.in.
 * When using MSVC on MS Windows, you'll have to edit src/config_vc6.h
 * manually if you want anything else than the defaults.
 */

#ifndef CONFIG_ANDROID_H_
#define CONFIG_ANDROID_H_

/* Directory where the UQM game data is located */
#define CONTENTDIR "/storage/emulated/0/alpha3/uqm/content/"

/* Directory where game data will be stored */
#define USERDIR "/storage/emulated/0/alpha3/uqm/"

/* Directory where config files will be stored */
#define CONFIGDIR USERDIR

/* Directory where supermelee teams will be stored */
#define MELEEDIR "/storage/emulated/0/alpha3/uqm/teams/"

/* Directory where save games will be stored */
#define SAVEDIR "/storage/emulated/0/alpha3/uqm/save/"

/* Directory where screenshots will be stored */
#define SCRSHOTDIR "/storage/emulated/0/alpha3/uqm/screenshots/"

/* Defined if words are stored with the most significant byte first */
#undef WORDS_BIGENDIAN

/* Defined if your system has readdir_r of its own */
#define HAVE_READDIR_R

/* Defined if your system has setenv of its own */
#define HAVE_SETENV

/* Defined if your system has strupr of its own */
#undef HAVE_STRUPR

/* Defined if your system has strcasecmp of its own */
#define HAVE_STRCASECMP_UQM
		// Not using "HAVE_STRCASECMP" as that conflicts with SDL.

/* Defined if your system has stricmp of its own */
#undef HAVE_STRICMP

/* Defined if your system has getopt_long */
#define HAVE_GETOPT_LONG

/* Defined if your system has iswgraph of its own*/
#define HAVE_ISWGRAPH

/* Defined if your system has wchar_t of its own */
#define HAVE_WCHAR_T

/* Defined if your system has wint_t of its own */
#define HAVE_WINT_T

/* Defined if your system has _Bool of its own */
#define HAVE__BOOL

#endif  /* CONFIG_ANDROID_H_ */

