/*
 *  Copyright 2012  Serge van den Boom <svdb@stack.nl>
 *
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "async.h"

#include "libs/alarm.h"
#include "libs/callback.h"


// Process all alarms and callbacks.
// First, all scheduled callbacks are called.
// Then each alarm due is called, and after each of these alarms, the
// callbacks scheduled by this alarm are called.
void
Async_process(void)
{
	// Call pending callbacks.
	Callback_process();

	for (;;) {
		if (!Alarm_processOne())
			return;

		// Call callbacks scheduled from the last alarm.
		Callback_process();
	}
}

// Returns the next time that some asynchronous callback is
// to be called. Note that all values lower than the current time
// should be considered as 'somewhere in the past'.
uint32
Async_timeBeforeNextMs(void) {
	if (Callback_haveMore()) {
		// Any time before the current time is ok, though we reserve 0 so
		// that the caller may use it as a special value in its own code.
		return 1;
	}
	return Alarm_timeBeforeNextMs();
}

