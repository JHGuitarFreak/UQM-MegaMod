/* This file contains some compile-time configuration options for Symbian
 */

#ifndef SYMBIAN_CONFIG_H_
#define SYMBIAN_CONFIG_H_

/* Directory where the UQM game data is located */
#define CONTENTDIR "content"

/* Directory where game data will be stored */
#define USERDIR "userdata"

/* Directory where config files will be stored */
#define CONFIGDIR USERDIR

/* Directory where supermelee teams will be stored */
#define MELEEDIR "userdata\\teams\\"

/* Directory where save games will be stored */
#define SAVEDIR "userdata\\save\\"

/* Define if words are stored with the most significant byte first */
#undef WORDS_BIGENDIAN

/* Defined if your system has readdir_r of its own */
#undef HAVE_READDIR_R

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
#undef HAVE_GETOPT_LONG

/* Defined if your system has iswgraph of its own*/
#define HAVE_ISWGRAPH

/* Defined if your system has wchar_t of its own */
#define HAVE_WCHAR_T

/* Defined if your system has wint_t of its own */
#define HAVE_WINT_T

#define HAVE__BOOL

#define PATH_MAX _POSIX_PATH_MAX

#endif /* SYMBIAN_CONFIG_H_ */
