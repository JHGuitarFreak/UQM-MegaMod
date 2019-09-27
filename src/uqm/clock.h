//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifndef UQM_CLOCK_H_
#define UQM_CLOCK_H_

#include "libs/tasklib.h"
#include "displist.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define START_YEAR 2155

typedef struct
{
	BYTE day_index, month_index;
	COUNT year_index;
	SIZE tick_count, day_in_ticks;

	QUEUE event_q;
			/* Queue element is EVENT */
} CLOCK_STATE;

typedef HLINK HEVENT;

typedef struct event
{
	// LINK elements; must be first
	HEVENT pred, succ;

	BYTE day_index, month_index;
	COUNT year_index;
	BYTE func_index;
} EVENT;

typedef enum
{
	ABSOLUTE_EVENT = 0,
	RELATIVE_EVENT
} EVENT_TYPE;

#define AllocEvent() AllocLink (&GLOBAL (GameClock.event_q))
#define PutEvent(h) PutQueue (&GLOBAL (GameClock.event_q), (h))
#define InsertEvent(h,i) InsertQueue (&GLOBAL (GameClock.event_q), (h), (i))
#define GetHeadEvent() GetHeadLink (&GLOBAL (GameClock.event_q))
#define GetTailEvent() GetTailLink (&GLOBAL (GameClock.event_q))
#define LockEvent(h,ppe) (*(ppe) = (EVENT*)LockLink (&GLOBAL (GameClock.event_q), h))
#define UnlockEvent(h) UnlockLink (&GLOBAL (GameClock.event_q), (h))
#define RemoveEvent(h) RemoveQueue (&GLOBAL (GameClock.event_q), (h))
#define FreeEvent(h) FreeLink (&GLOBAL (GameClock.event_q), (h))
#define GetPredEvent(l) _GetPredLink (l)
#define GetSuccEvent(l) _GetSuccLink (l)
#define ForAllEvents(callback, arg) ForAllLinks(&GLOBAL (GameClock.event_q), \
		(void (*)(LINK *, void *)) (callback), (arg))

// Rates are in seconds per game day
#define HYPERSPACE_CLOCK_RATE 5
// XXX: the IP rate is based on 24 ticks/second (see SetGameClockRate),
//   however, IP runs at 30 fps right now. So in reality, the IP clock
//   rate is closer to 23 seconds per game day. The clock is faster, but
//   the flagship also moves faster.
#define INTERPLANETARY_CLOCK_RATE 30

extern BOOLEAN InitGameClock (void);
extern BOOLEAN UninitGameClock (void);

extern float daysElapsed (void);

extern void SetGameClockRate (COUNT seconds_per_day);
extern BOOLEAN ValidateEvent (EVENT_TYPE type, COUNT *pmonth_index,
		COUNT *pday_index, COUNT *pyear_index);
extern HEVENT AddEvent (EVENT_TYPE type, COUNT month_index, COUNT
		day_index, COUNT year_index, BYTE func_index);
extern void EventHandler (BYTE selector);
extern void GameClockTick (void);
extern void MoveGameClockDays (COUNT days);

// The lock/unlock/running functions are for debugging use only
// Locking will block the GameClockTick() function and thus
// the thread moving the clock.
extern void LockGameClock (void);
extern void UnlockGameClock (void);
// A weak indicator of the clock moving. Suitable for debugging,
// but not much else
extern BOOLEAN GameClockRunning (void);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_CLOCK_H_ */

