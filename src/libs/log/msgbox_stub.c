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

#include "msgbox.h"
#include "loginternal.h"
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif

void
log_displayBox (const /*UTF-8*/char *title, int isError,
		const /*UTF-8*/char *msg)
{
	// We do not know how to display a box. Perhaps it's done with a
	// hefty dose of pixie dust, or perhaps with a hammer and nails.
	// So just inform the user of our predicament
	fprintf (streamOut, "Do not know how to display %s box\n",
			isError ? "an error" : "a");
#if defined(ANDROID) || defined(__ANDROID__)
	__android_log_print( isError ? ANDROID_LOG_FATAL : ANDROID_LOG_INFO, "Ur-Quan Masters MegaMod", "%s: %s", title, msg );
#endif
	
	// Suppress the compiler warnings in any case.
	(void)title;
	(void)msg;
}

