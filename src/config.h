/* This file contains some compile-time configuration options.
 */

#ifdef _MSC_VER
	/* In this case, build.sh is not run to generate a config file, so
	 * we use a default file config_vc6.h instead.
	 * If you want anything else than the defaults, you'll have to edit
	 * that file manually. */
#	include "config_vc6.h"
#elif defined(__SYMBIAN32__)
#	include "symbian/config.h"
#elif defined (__MINGW32__) || defined (__CYGWIN__)
	/* If we're compiling on MS Windows using build.sh, use
	 * config_win.h, generated from src/config_win.h.in. */
#	include "config_win.h"
#else
	/* If we're compiling in unix, use config_unix.h, generated from
	 * src/config_unix.h.in by build.sh. */
#	include "config_unix.h"
#endif

