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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UQM_GAMEEV_H_
#define UQM_GAMEEV_H_

#include "libs/compiler.h"
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif


enum
{
	ARILOU_ENTRANCE_EVENT = 0,
	ARILOU_EXIT_EVENT,
	HYPERSPACE_ENCOUNTER_EVENT,
	KOHR_AH_VICTORIOUS_EVENT,
	ADVANCE_PKUNK_MISSION,
	ADVANCE_THRADD_MISSION,
	ZOQFOT_DISTRESS_EVENT,
	ZOQFOT_DEATH_EVENT,
	SHOFIXTI_RETURN_EVENT,
	ADVANCE_UTWIG_SUPOX_MISSION,
	KOHR_AH_GENOCIDE_EVENT,
	SPATHI_SHIELD_EVENT,
	ADVANCE_ILWRATH_MISSION,
	ADVANCE_MYCON_MISSION,
	ARILOU_UMGAH_CHECK,
	YEHAT_REBEL_EVENT,
	SLYLANDRO_RAMP_UP,
	SLYLANDRO_RAMP_DOWN,

	NUM_EVENTS
};

typedef enum
{
	CLOSING = 0,
	OPENING
} ARILOU_GATE_STATE;

extern int eventIdStrToNum (const char *eventIdStr);
extern const char *eventIdNumToStr (int eventNum);

extern void initEventSystem (void);
extern void uninitEventSystem (void);

extern void AddInitialGameEvents (void);
extern void EventHandler (BYTE selector);
extern void SetRaceDest (BYTE which_race, COORD x, COORD y, BYTE days_left,
		BYTE func_index);


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_GAMEEV_H_ */

