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

#include "uqmlog.h"
#include "loginternal.h"
#include "msgbox.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif
#include "libs/threadlib.h"

#ifndef MAX_LOG_ENTRY_SIZE
#	define MAX_LOG_ENTRY_SIZE 1024
#endif

#ifndef MAX_LOG_ENTRIES
#	define MAX_LOG_ENTRIES 128
#endif

typedef char log_Entry[MAX_LOG_ENTRY_SIZE];

// static buffers in case we run out of memory
static log_Entry queue[MAX_LOG_ENTRIES];
static log_Entry msgNoThread;
static char msgBuf[16384];

static int maxLevel = log_Error;
static int maxStreamLevel = log_Debug;
static int maxDisp = 10;
static int qtotal = 0;
static int qhead = 0;
static int qtail = 0;
static volatile bool noThreadReady = false;
static bool showBox = true;
static bool errorBox = true;

FILE *streamOut;

static volatile int qlock = 0;
static Mutex qmutex;

static void exitCallback (void);
static void displayLog (bool isError);

static void
lockQueue (void)
{
	if (!qlock)
		return;

	LockMutex (qmutex);
}

static void
unlockQueue (void)
{
	if (!qlock)
		return;

	UnlockMutex (qmutex);
}

static void
removeExcess (int room)
{
	room = maxDisp - room;
	if (room < 0)
		room = 0;

	for ( ; qtotal > room; --qtotal, ++qtail)
		;
	qtail %= MAX_LOG_ENTRIES;
}

static int
acquireSlot (void)
{
	int slot;

	lockQueue ();
	
	removeExcess (1);
	slot = qhead;
	qhead = (qhead + 1) % MAX_LOG_ENTRIES;
	++qtotal;
	
	unlockQueue ();

	return slot;
}

// queues the non-threaded message when present
static void
queueNonThreaded (void)
{
	int slot;

	// This is not perfect. A race condition still exists
	// between buffering the no-thread message and setting
	// the noThreadReady flag. Neither does this prevent
	// the fully or partially overwritten message (by
	// another competing thread). But it is 'good enough'
	if (!noThreadReady)
		return;
	noThreadReady = false;

	slot = acquireSlot ();
	memcpy (queue[slot], msgNoThread, sizeof (msgNoThread));
}

void
log_init (int max_lines)
{
	int i;

	maxDisp = max_lines;
	streamOut = stderr;

	// pre-term queue strings
	for (i = 0; i < MAX_LOG_ENTRIES; ++i)
		queue[i][MAX_LOG_ENTRY_SIZE - 1] = '\0';
	
	msgBuf[sizeof (msgBuf) - 1] = '\0';
	msgNoThread[sizeof (msgNoThread) - 1] = '\0';

	// install exit handlers
	atexit (exitCallback);
}

void
log_initThreads (void)
{
	qmutex = CreateMutex ("Logging Lock", SYNC_CLASS_RESOURCE);
	qlock = 1;
}

int
log_exit (int code)
{
	showBox = false;

	if (qlock)
	{
		qlock = 0;
		DestroyMutex (qmutex);
		qmutex = 0;
	}

	return code;
}

void
log_setLevel (int level)
{
	maxLevel = level;
	//maxStreamLevel = level;
}

FILE *
log_setOutput (FILE *out)
{
	FILE *old = streamOut;
	streamOut = out;
	
	return old;
}

void
log_addV (log_Level level, const char *fmt, va_list list) {
	log_Entry full_msg;
	vsnprintf (full_msg, sizeof (full_msg) - 1, fmt, list);
	full_msg[sizeof (full_msg) - 1] = '\0';
	
	if ((int)level <= maxStreamLevel) {
		fprintf (streamOut, "%s\n", full_msg);
#if defined(ANDROID) || defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_INFO, "Ur-Quan Masters MegaMod", "%s", full_msg);
#endif
	}

	if ((int)level <= maxLevel) {
		int slot;

		queueNonThreaded ();
		
		slot = acquireSlot ();
		memcpy (queue[slot], full_msg, sizeof (queue[0]));
	}
}

void
log_add (log_Level level, const char *fmt, ...)
{
	va_list list;

	va_start (list, fmt);
	log_addV (level, fmt, list);
	va_end (list);
}

// non-threaded version of 'add'
// uses single-instance static storage with entry into the
// queue delayed until the next threaded 'add' or 'exit'
void
log_add_nothreadV (log_Level level, const char *fmt, va_list list) {
	log_Entry full_msg;
	vsnprintf (full_msg, sizeof (full_msg) - 1, fmt, list);
	full_msg[sizeof (full_msg) - 1] = '\0';
	
	if ((int)level <= maxStreamLevel) {
		fprintf (streamOut, "%s\n", full_msg);
#if defined(ANDROID) || defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_INFO, "Ur-Quan Masters MegaMod", "%s", full_msg);
#endif
	}

	if ((int)level <= maxLevel) {
		memcpy (msgNoThread, full_msg, sizeof (msgNoThread));
		noThreadReady = true;
	}
}

void
log_add_nothread (log_Level level, const char *fmt, ...)
{
	va_list list;

	va_start (list, fmt);
	log_add_nothreadV (level, fmt, list);
	va_end (list);
}

void
log_showBox (bool show, bool err)
{
	showBox = show;
	errorBox = err;
}

// sets the maximum log lines captured for the final
// display to the user on failure exit
void
log_captureLines (int num)
{
	if (num > MAX_LOG_ENTRIES)
		num = MAX_LOG_ENTRIES;
	if (num < 1)
		num = 1;
	maxDisp = num;

	// remove any extra lines already on queue
	lockQueue ();
	removeExcess (0);
	unlockQueue ();
}

static void
exitCallback (void)
{
	if (showBox)
		displayLog (errorBox);

	log_exit (0);
}

static void
displayLog (bool isError)
{
	char *p = msgBuf;
	int left = sizeof (msgBuf) - 1;
	int len;
	int ptr;

	if (isError)
	{
		strcpy (p, "The Ur-Quan Masters encountered a fatal error.\n\n"
				"Part of the log follows:\n\n");
		len = strlen (p);
		p += len;
		left -= len;
	}

	// Glue the log entries together
	// Locking is not a good idea at this point and we do not
	// really need it -- the worst that can happen is we get
	// an extra or an incomplete message
	for (ptr = qtail; ptr != qhead && left > 0;
			ptr = (ptr + 1) % MAX_LOG_ENTRIES)
	{
		len = strlen (queue[ptr]) + 1;
		if (len > left)
			len = left;
		memcpy (p, queue[ptr], len);
		p[len - 1] = '\n';
		p += len;
		left -= len;
	}

	// Glue the non-threaded message if present
	if (noThreadReady)
	{
		noThreadReady = false;
		len = strlen (msgNoThread);
		if (len > left)
			len = left;
		memcpy (p, msgNoThread, len);
		p += len;
		left -= len;
	}
	
	*p = '\0';

	log_displayBox ("The Ur-Quan Masters", isError, msgBuf);
}

