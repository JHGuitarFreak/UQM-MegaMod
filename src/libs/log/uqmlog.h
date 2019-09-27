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

#ifndef UQMLOG_H_INCL__
#define UQMLOG_H_INCL__

#include "port.h"
#include "types.h"
#include <stdio.h>
#include <stdarg.h>

extern void log_init (int max_lines);
extern void log_initThreads (void);
extern int log_exit (int code);

extern FILE * log_setOutput (FILE *out);
		// sets the new output stream and returns the previous one
extern void log_setLevel (int level);
extern void log_showBox (bool show, bool err);
extern void log_captureLines (int num);
#define LOG_CAPTURE_ALL 1000000  // unreasonably big number

typedef enum
{
	log_Nothing = 0,
	log_User,
	log_Fatal = log_User,
	log_Error,
	log_Warning,
	log_Info,
	log_Debug,
	log_All,

} log_Level;

extern void log_add (log_Level, const char *fmt, ...)
		PRINTF_FUNCTION(2, 3);
extern void log_addV (log_Level, const char *fmt, va_list)
		VPRINTF_FUNCTION(2);
extern void log_add_nothread (log_Level, const char *fmt, ...)
		PRINTF_FUNCTION(2, 3);
extern void log_add_nothreadV (log_Level, const char *fmt, va_list)
		VPRINTF_FUNCTION(2);


#endif /* UQMLOG_H_INCL__ */
