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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gameev.h"

#include "build.h"
#include "clock.h"
#include "starmap.h"
#include "gendef.h"
#include "globdata.h"
#include "hyper.h"
#include "resinst.h"
#include "lua/luaevent.h"
#include "lua/luafuncs/customfuncs.h"
#include "libs/compiler.h"
#include "libs/log.h"
#include "libs/mathlib.h"
#include "nameref.h"
#include "options.h"
#include <stdlib.h>
#include "setup.h"
#include "sounds.h"
#include "planets/lander.h"

static int arilou_entrance_event (int arg);
static int arilou_exit_event (int arg);
static void check_race_growth (void);
static int hyperspace_encounter_event (int arg);
static int kohr_ah_victorious_event (int arg);
static int advance_pkunk_mission (int arg);
static int advance_thradd_mission (int arg);
static int zoqfot_distress_event (int arg);
static int zoqfot_death_event (int arg);
static int shofixti_return_event (int arg);
static int advance_utwig_supox_mission (int arg);
static int kohr_ah_genocide_event (int arg);
static int spathi_shield_event (int arg);
static int advance_ilwrath_mission (int arg);
static int advance_mycon_mission (int arg);
static int arilou_umgah_check (int arg);
static int yehat_rebel_event (int arg);
static int slylandro_ramp_up (int arg);
static int slylandro_ramp_down (int arg);

static const char *eventNames[] = {
	"ARILOU_ENTRANCE_EVENT",
	"ARILOU_EXIT_EVENT",
	"HYPERSPACE_ENCOUNTER_EVENT",
	"KOHR_AH_VICTORIOUS_EVENT",
	"ADVANCE_PKUNK_MISSION",
	"ADVANCE_THRADD_MISSION",
	"ZOQFOT_DISTRESS_EVENT",
	"ZOQFOT_DEATH_EVENT",
	"SHOFIXTI_RETURN_EVENT",
	"ADVANCE_UTWIG_SUPOX_MISSION",
	"KOHR_AH_GENOCIDE_EVENT",
	"SPATHI_SHIELD_EVENT",
	"ADVANCE_ILWRATH_MISSION",
	"ADVANCE_MYCON_MISSION",
	"ARILOU_UMGAH_CHECK",
	"YEHAT_REBEL_EVENT",
	"SLYLANDRO_RAMP_UP",
	"SLYLANDRO_RAMP_DOWN"
};

void
initEventSystem (void) {
	// Register functions which can be called from Lua through
	// 'custom.<functionName>'. Right now, these are the event functions
	// which have not been converted to Lua yet.
	static const luaUqm_custom_Function eventFuncs[] = {
		{ "arilou_entrance_event",       arilou_entrance_event },
		{ "arilou_exit_event",           arilou_exit_event },
		{ "hyperspace_encounter_event",  hyperspace_encounter_event },
		{ "kohr_ah_victorious_event",    kohr_ah_victorious_event },
		{ "advance_pkunk_mission",       advance_pkunk_mission },
		{ "advance_thradd_mission",      advance_thradd_mission },
		{ "zoqfot_distress_event",       zoqfot_distress_event },
		{ "zoqfot_death_event",          zoqfot_death_event },
		{ "shofixti_return_event",       shofixti_return_event },
		{ "advance_utwig_supox_mission", advance_utwig_supox_mission },
		{ "kohr_ah_genocide_event",      kohr_ah_genocide_event },
		{ "spathi_shield_event",         spathi_shield_event },
		{ "advance_ilwrath_mission",     advance_ilwrath_mission },
		{ "advance_mycon_mission",       advance_mycon_mission },
		{ "arilou_umgah_check",          arilou_umgah_check },
		{ "yehat_rebel_event",           yehat_rebel_event },
		{ "slylandro_ramp_up",           slylandro_ramp_up },
		{ "slylandro_ramp_down",         slylandro_ramp_down },
		{ NULL, NULL }
	};

	luaUqm_event_init (eventFuncs, EVENT_SCRIPT);
}

void
uninitEventSystem (void) {
	luaUqm_event_uninit ();
}

int
eventIdStrToNum (const char *eventIdStr)
{
	size_t eventCount = sizeof eventNames / sizeof eventNames[0];
	size_t eventI;

	// Linear search; acceptable for such a small number of events.
	for (eventI = 0; eventI < eventCount; eventI++)
	{
		if (strcmp (eventIdStr, eventNames[eventI]) == 0)
			return eventI;
	}
	return -1;
}

const char *
eventIdNumToStr (int eventNum)
{
	size_t eventCount = sizeof eventNames / sizeof eventNames[0];
	if (eventNum < 0 || (size_t) eventNum >= eventCount)
		return NULL;
	return eventNames[eventNum];
}

void
AddInitialGameEvents (void) {	
	COUNT kohrah_winning_years = optCheatMode ? YEARS_TO_KOHRAH_VICTORY + 25 : YEARS_TO_KOHRAH_VICTORY;
	AddEvent (RELATIVE_EVENT, 0, 1, 0, HYPERSPACE_ENCOUNTER_EVENT);
	AddEvent (ABSOLUTE_EVENT, 3, 17, START_YEAR, ARILOU_ENTRANCE_EVENT);
	AddEvent (RELATIVE_EVENT, 0, 0, kohrah_winning_years,
			KOHR_AH_VICTORIOUS_EVENT);
	AddEvent (RELATIVE_EVENT, 0, 0, 0, SLYLANDRO_RAMP_UP);
}

void
EventHandler (BYTE selector)
{
	const char *eventIdStr;

	eventIdStr = eventIdNumToStr (selector);
	if (eventIdStr == NULL) {
		log_add(log_Warning, "Warning: EventHandler(): Event %d is "
				"unknown.", selector);
		return;
	}

	luaUqm_event_callEvent(eventIdStr);
}

void
SetRaceDest (BYTE which_race, COORD x, COORD y, BYTE days_left,
		BYTE func_index)
{
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), which_race);
	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	FleetPtr->dest_loc.x = x;
	FleetPtr->dest_loc.y = y;
	FleetPtr->days_left = days_left;
	FleetPtr->func_index = func_index;

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
}


static int
arilou_entrance_event (int arg)
{
	SET_GAME_STATE (ARILOU_SPACE, OPENING);
	AddEvent (RELATIVE_EVENT, 0, 3, 0, ARILOU_EXIT_EVENT);
	(void) arg;
	return 0;
}

static int
arilou_exit_event (int arg)
{
	COUNT month_index, year_index;

	year_index = GLOBAL (GameClock.year_index);
	if ((month_index = GLOBAL (GameClock.month_index) % 12) == 0)
		++year_index;
	++month_index;

	SET_GAME_STATE (ARILOU_SPACE, CLOSING);
	AddEvent (ABSOLUTE_EVENT,
			month_index, 17, year_index, ARILOU_ENTRANCE_EVENT);

	(void) arg;
	return 0;
}

static void
check_race_growth (void)
{
	HFLEETINFO hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (FleetPtr);

		if (FleetPtr->actual_strength
				&& FleetPtr->actual_strength != INFINITE_RADIUS)
		{
			SIZE delta_strength;

			delta_strength = (SBYTE)FleetPtr->growth;
			if (FleetPtr->growth_err_term <= FleetPtr->growth_fract)
			{
				if (delta_strength <= 0)
					--delta_strength;
				else
					++delta_strength;
			}
			FleetPtr->growth_err_term -= FleetPtr->growth_fract;

			delta_strength += FleetPtr->actual_strength;
			if (delta_strength <= 0)
			{
				delta_strength = 0;
				FleetPtr->allied_state = DEAD_GUY;
			}
			else if (delta_strength > MAX_FLEET_STRENGTH)
				delta_strength = MAX_FLEET_STRENGTH;
				
			FleetPtr->actual_strength = (COUNT)delta_strength;
			if (FleetPtr->actual_strength && FleetPtr->days_left)
			{
				FleetPtr->loc.x += (FleetPtr->dest_loc.x - FleetPtr->loc.x)
						/ FleetPtr->days_left;
				FleetPtr->loc.y += (FleetPtr->dest_loc.y - FleetPtr->loc.y)
						/ FleetPtr->days_left;

				if (--FleetPtr->days_left == 0
						&& FleetPtr->func_index != (BYTE) ~0)
					EventHandler (FleetPtr->func_index);
			}
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}
}

static int
hyperspace_encounter_event (int arg)
{
	check_race_growth ();
	if (inHyperSpace ())
		check_hyperspace_encounter ();

	AddEvent (RELATIVE_EVENT, 0, 1, 0, HYPERSPACE_ENCOUNTER_EVENT);
	
	(void) arg;
	return 0;
}

static int
kohr_ah_victorious_event (int arg)
{
	if (GET_GAME_STATE (UTWIG_SUPOX_MISSION))
	{
		// The Utwig/Supox mission delayed the genocide.
		// Try again in one year.
		AddEvent (RELATIVE_EVENT, 0, 0, 1, KOHR_AH_GENOCIDE_EVENT);
		return 0;
	}

	// No more delay; start the genocide.
	return kohr_ah_genocide_event (arg);
}

static int
advance_pkunk_mission (int arg)
{
	HFLEETINFO hPkunk;
	FLEET_INFO *PkunkPtr;

	hPkunk = GetStarShipFromIndex (&GLOBAL (avail_race_q), PKUNK_SHIP);
	PkunkPtr = LockFleetInfo (&GLOBAL (avail_race_q), hPkunk);

	if (PkunkPtr->actual_strength)
	{
		BYTE MissionState;

		MissionState = GET_GAME_STATE (PKUNK_MISSION);
		if (PkunkPtr->days_left == 0 && MissionState)
		{
			if ((MissionState & 1)
							/* made it to Yehat space */
					|| (PkunkPtr->loc.x == 4970
					&& PkunkPtr->loc.y == 400))
				PkunkPtr->actual_strength = 0;
			else if (PkunkPtr->loc.x == 502
					&& PkunkPtr->loc.y == 401
					&& GET_GAME_STATE (PKUNK_ON_THE_MOVE))
			{
				SET_GAME_STATE (PKUNK_ON_THE_MOVE, 0);
				AddEvent (RELATIVE_EVENT, 3, 0, 0, ADVANCE_PKUNK_MISSION);
				UnlockFleetInfo (&GLOBAL (avail_race_q), hPkunk);
				return 0;
			}
		}

		if (PkunkPtr->actual_strength == 0)
		{
			SET_GAME_STATE (YEHAT_ABSORBED_PKUNK, 1);
			PkunkPtr->allied_state = DEAD_GUY;
			StartSphereTracking (YEHAT_SHIP);
		}
		else
		{
			COORD x, y;

			if (!(MissionState & 1))
			{
				x = 4970;
				y = 400;
			}
			else
			{
				x = 502;
				y = 401;
			}
			SET_GAME_STATE (PKUNK_ON_THE_MOVE, 1);
			SET_GAME_STATE (PKUNK_SWITCH, 0);
			SetRaceDest (PKUNK_SHIP, x, y,
					(BYTE)((365 >> 1) - PkunkPtr->days_left),
					ADVANCE_PKUNK_MISSION);
		}
		SET_GAME_STATE (PKUNK_MISSION, MissionState + 1);
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hPkunk);

	(void) arg;
	return 0;
}

// Send the Thraddash to fight the Kohr-Ah.
static int
advance_thradd_mission (int arg)
{
	BYTE MissionState;
	HFLEETINFO hThradd;
	FLEET_INFO *ThraddPtr;

	hThradd = GetStarShipFromIndex (&GLOBAL (avail_race_q), THRADDASH_SHIP);
	ThraddPtr = LockFleetInfo (&GLOBAL (avail_race_q), hThradd);

	MissionState = GET_GAME_STATE (THRADD_MISSION);
	if (ThraddPtr->actual_strength && MissionState < 3)
	{
		COORD x, y;

		if (MissionState < 2)
		{	/* attacking */
			x = 4879;
			y = 7201;
		}
		else
		{	/* returning */
			x = 2535;
			y = 8358;
		}

		if (MissionState == 1)
		{	/* arrived at Kohr-Ah, engaging */
			SIZE strength_loss;

			strength_loss = (SIZE)(ThraddPtr->actual_strength >> 1);
			ThraddPtr->growth = (BYTE)(-strength_loss / 14);
			ThraddPtr->growth_fract = (BYTE)(((strength_loss % 14) << 8) / 14);
			ThraddPtr->growth_err_term = 255 >> 1;
		}
		else
		{
			if (MissionState != 0)
			{	/* stop losses */
				ThraddPtr->growth = 0;
				ThraddPtr->growth_fract = 0;
			}
		}
		SetRaceDest (THRADDASH_SHIP, x, y, 14, ADVANCE_THRADD_MISSION);
	}
	++MissionState;
	SET_GAME_STATE (THRADD_MISSION, MissionState);

	if (MissionState == 4 && GET_GAME_STATE (ILWRATH_FIGHT_THRADDASH))
	{	/* returned home - notify the Ilwrath */
		AddEvent (RELATIVE_EVENT, 0, 0, 0, ADVANCE_ILWRATH_MISSION);
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hThradd);

	(void) arg;
	return 0;
}

static int
zoqfot_distress_event (int arg)
{
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& CurStarDescPtr
			&& CurStarDescPtr->Index == ZOQFOT_DEFINED)
	{
		AddEvent (RELATIVE_EVENT, 0, 7, 0, ZOQFOT_DISTRESS_EVENT);
	}
	else
	{
		SET_GAME_STATE (ZOQFOT_DISTRESS, 1);
		AddEvent (RELATIVE_EVENT, 6, 0, 0, ZOQFOT_DEATH_EVENT);
	}

	(void) arg;
	return 0;
}

static int
zoqfot_death_event (int arg)
{
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& CurStarDescPtr
			&& CurStarDescPtr->Index == ZOQFOT_DEFINED)
	{
		AddEvent (RELATIVE_EVENT, 0, 7, 0, ZOQFOT_DEATH_EVENT);
	}
	else if (GET_GAME_STATE (ZOQFOT_DISTRESS))
	{
		HFLEETINFO hZoqFot;
		FLEET_INFO *ZoqFotPtr;

		hZoqFot = GetStarShipFromIndex (&GLOBAL (avail_race_q),
				ZOQFOTPIK_SHIP);
		ZoqFotPtr = LockFleetInfo (&GLOBAL (avail_race_q), hZoqFot);
		ZoqFotPtr->actual_strength = 0;
		ZoqFotPtr->allied_state = DEAD_GUY;
		UnlockFleetInfo (&GLOBAL (avail_race_q), hZoqFot);

		SET_GAME_STATE (ZOQFOT_DISTRESS, 2);
	}

	(void) arg;
	return 0;
}

static int
shofixti_return_event (int arg)
{
	SetRaceAllied (SHOFIXTI_SHIP, TRUE);
	GLOBAL (CrewCost) -= IF_HARD(2, 1);
			/* crew is not an issue anymore */
	SET_GAME_STATE (CREW_PURCHASED0, 0);
	SET_GAME_STATE (CREW_PURCHASED1, 0);

	(void) arg;
	return 0;
}

static int
advance_utwig_supox_mission (int arg)
{
	BYTE MissionState;
	HFLEETINFO hUtwig, hSupox;
	FLEET_INFO *UtwigPtr;
	FLEET_INFO *SupoxPtr;

	hUtwig = GetStarShipFromIndex (&GLOBAL (avail_race_q), UTWIG_SHIP);
	UtwigPtr = LockFleetInfo (&GLOBAL (avail_race_q), hUtwig);
	hSupox = GetStarShipFromIndex (&GLOBAL (avail_race_q), SUPOX_SHIP);
	SupoxPtr = LockFleetInfo (&GLOBAL (avail_race_q), hSupox);

	MissionState = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
	if (UtwigPtr->actual_strength && MissionState < 5)
	{
		if (MissionState == 1)
		{
			SIZE strength_loss;

			AddEvent (RELATIVE_EVENT, 0, (160 >> 1), 0,
					ADVANCE_UTWIG_SUPOX_MISSION);

			strength_loss = (SIZE)(UtwigPtr->actual_strength >> 1);
			UtwigPtr->growth = (BYTE)(-strength_loss / 160);
			UtwigPtr->growth_fract =
					(BYTE)(((strength_loss % 160) << 8) / 160);
			UtwigPtr->growth_err_term = 255 >> 1;

			strength_loss = (SIZE)(SupoxPtr->actual_strength >> 1);
			if (strength_loss)
			{
				SupoxPtr->growth = (BYTE)(-strength_loss / 160);
				SupoxPtr->growth_fract =
						(BYTE)(((strength_loss % 160) << 8) / 160);
				SupoxPtr->growth_err_term = 255 >> 1;
			}

			SET_GAME_STATE (UTWIG_WAR_NEWS, 0);
			SET_GAME_STATE (SUPOX_WAR_NEWS, 0);
		}
		else if (MissionState == 2)
		{
			AddEvent (RELATIVE_EVENT, 0, (160 >> 1), 0,
					ADVANCE_UTWIG_SUPOX_MISSION);
			++MissionState;
		}
		else
		{
			COORD ux, uy, sx, sy;

			if (MissionState == 0)
			{
				ux = 7208;
				uy = 7000;

				sx = 6479;
				sy = 7541;
			}
			else
			{
				ux = 8534;
				uy = 8797;

				sx = 7468;
				sy = 9246;

				UtwigPtr->growth = 0;
				UtwigPtr->growth_fract = 0;
				SupoxPtr->growth = 0;
				SupoxPtr->growth_fract = 0;

				SET_GAME_STATE (UTWIG_WAR_NEWS, 0);
				SET_GAME_STATE (SUPOX_WAR_NEWS, 0);
			}
			SET_GAME_STATE (UTWIG_VISITS, 0);
			SET_GAME_STATE (UTWIG_INFO, 0);
			SET_GAME_STATE (SUPOX_VISITS, 0);
			SET_GAME_STATE (SUPOX_INFO, 0);
			SetRaceDest (UTWIG_SHIP, ux, uy, 21, ADVANCE_UTWIG_SUPOX_MISSION);
			SetRaceDest (SUPOX_SHIP, sx, sy, 21, (BYTE)~0);
		}
	}
	SET_GAME_STATE (UTWIG_SUPOX_MISSION, MissionState + 1);

	UnlockFleetInfo (&GLOBAL (avail_race_q), hSupox);
	UnlockFleetInfo (&GLOBAL (avail_race_q), hUtwig);

	(void) arg;
	return 0;
}

static int
kohr_ah_genocide_event (int arg)
{
	BYTE Index;
	long best_dist;
	SIZE best_dx, best_dy;
	HFLEETINFO hStarShip, hNextShip;
	HFLEETINFO hBlackUrquan;
	FLEET_INFO *BlackUrquanPtr;

	if (!GET_GAME_STATE (KOHR_AH_FRENZY)
			&& LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
					&& CurStarDescPtr
			&& CurStarDescPtr->Index == SAMATRA_DEFINED) {
		AddEvent (RELATIVE_EVENT, 0, 7, 0, KOHR_AH_GENOCIDE_EVENT);
		return 0;
	}

	hBlackUrquan = GetStarShipFromIndex (&GLOBAL (avail_race_q),
			BLACK_URQUAN_SHIP);
	BlackUrquanPtr = LockFleetInfo (&GLOBAL (avail_race_q), hBlackUrquan);

	best_dist = -1;
	best_dx = SOL_X - BlackUrquanPtr->loc.x;
	best_dy = SOL_Y - BlackUrquanPtr->loc.y;
	for (Index = 0, hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; ++Index, hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (FleetPtr);

		if (Index != BLACK_URQUAN_SHIP
				&& Index != URQUAN_SHIP
				&& FleetPtr->actual_strength != INFINITE_RADIUS)
		{
			SIZE dx, dy;

			dx = FleetPtr->loc.x - BlackUrquanPtr->loc.x;
			dy = FleetPtr->loc.y - BlackUrquanPtr->loc.y;
			if (dx == 0 && dy == 0)
			{
				// Arrived at the victim's home world. Cleanse it.
				FleetPtr->allied_state = DEAD_GUY;
				FleetPtr->actual_strength = 0;
			}
			else if (FleetPtr->actual_strength)
			{
				long dist;

				dist = (long)dx * dx + (long)dy * dy;
				if (best_dist < 0 || dist < best_dist || Index == DRUUGE_SHIP)
				{
					best_dist = dist;
					best_dx = dx;
					best_dy = dy;

					if (Index == DRUUGE_SHIP)
						hNextShip = 0;
				}
			}
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}

	if (best_dist < 0 && best_dx == 0 && best_dy == 0)
	{
		// All spheres of influence are gone - game over.
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
		GLOBAL_SIS (CrewEnlisted) = (COUNT)~0;

		SET_GAME_STATE (KOHR_AH_KILLED_ALL, 1);
	}
	else
	{
		// Moving towards new race to cleanse.
		COUNT speed;

		if (best_dist < 0)
			best_dist = (long)best_dx * best_dx + (long)best_dy * best_dy;

		speed = square_root (best_dist) / 158;
		if (speed == 0)
			speed = 1;
		else if (speed > 255)
			speed = 255;
 
		if (optCheatMode)
			speed = 0;

		SET_GAME_STATE (KOHR_AH_FRENZY, 1);
		SET_GAME_STATE (KOHR_AH_VISITS, 0);
		SET_GAME_STATE (KOHR_AH_REASONS, 0);
		SET_GAME_STATE (KOHR_AH_PLEAD, 0);
		SET_GAME_STATE (KOHR_AH_INFO, 0);
		SET_GAME_STATE (URQUAN_VISITS, 0);
		SetRaceDest (BLACK_URQUAN_SHIP,
				BlackUrquanPtr->loc.x + best_dx,
				BlackUrquanPtr->loc.y + best_dy,
				(BYTE)speed, KOHR_AH_GENOCIDE_EVENT);
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hBlackUrquan);

	(void) arg;
	return 0;
}

static int
spathi_shield_event (int arg)
{
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& CurStarDescPtr
			&& CurStarDescPtr->Index == SPATHI_DEFINED)
	{
		AddEvent (RELATIVE_EVENT, 0, 7, 0, SPATHI_SHIELD_EVENT);
	}
	else
	{
		HFLEETINFO hSpathi;
		FLEET_INFO *SpathiPtr;

		hSpathi = GetStarShipFromIndex (&GLOBAL (avail_race_q),
				SPATHI_SHIP);
		SpathiPtr = LockFleetInfo (&GLOBAL (avail_race_q), hSpathi);

		if (SpathiPtr->actual_strength)
		{
			SetRaceAllied (SPATHI_SHIP, FALSE);
			if(DIF_HARD)
				RemoveEscortShips (SPATHI_SHIP);
			SET_GAME_STATE (SPATHI_SHIELDED_SELVES, 1);
			SpathiPtr->actual_strength = 0;
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hSpathi);
	}

	(void) arg;
	return 0;
}

static int
advance_ilwrath_mission (int arg)
{
	BYTE ThraddState = GET_GAME_STATE(THRADD_MISSION);
	HFLEETINFO	hIlwrath = GetStarShipFromIndex(&GLOBAL(avail_race_q), ILWRATH_SHIP),
		hThradd = GetStarShipFromIndex(&GLOBAL(avail_race_q), THRADDASH_SHIP);
	FLEET_INFO	*IlwrathPtr = LockFleetInfo(&GLOBAL(avail_race_q), hIlwrath),
		*ThraddPtr = LockFleetInfo(&GLOBAL(avail_race_q), hThradd);

	if (IlwrathPtr->loc.x == ((2500 + 2535) >> 1)
			&& IlwrathPtr->loc.y == ((8070 + 8358) >> 1))
	{
		IlwrathPtr->actual_strength = 0;
		ThraddPtr->actual_strength = 0;
		if ((EXTENDED && ThraddPtr->allied_state != GOOD_GUY) || !EXTENDED) {
			ThraddPtr->actual_strength = 0;
			ThraddPtr->allied_state = DEAD_GUY;
		}
	}
	else if (IlwrathPtr->actual_strength)
	{
		if (!GET_GAME_STATE (ILWRATH_FIGHT_THRADDASH)
				&& (IlwrathPtr->dest_loc.x != 2500
				|| IlwrathPtr->dest_loc.y != 8070))
		{
			SetRaceDest (ILWRATH_SHIP, 2500, 8070, 90,
					ADVANCE_ILWRATH_MISSION);
		}
		else
		{
#define MADD_LENGTH 128
			SIZE strength_loss;

			if (IlwrathPtr->days_left == 0)
			{	/* arrived for battle */
				SET_GAME_STATE (ILWRATH_FIGHT_THRADDASH, 1);
				SET_GAME_STATE (HELIX_UNPROTECTED, 1);
				strength_loss = (SIZE)IlwrathPtr->actual_strength;
				IlwrathPtr->growth = (BYTE)(-strength_loss / MADD_LENGTH);
				IlwrathPtr->growth_fract =
						(BYTE)(((strength_loss % MADD_LENGTH) << 8) / MADD_LENGTH);
				SetRaceDest (ILWRATH_SHIP,
						(2500 + 2535) >> 1, (8070 + 8358) >> 1,
						MADD_LENGTH - 1, ADVANCE_ILWRATH_MISSION);

				if (EXTENDED && ThraddPtr->allied_state == GOOD_GUY) {
					strength_loss = (SIZE)(ThraddPtr->actual_strength * 0.25); // Smarterer math
					ThraddPtr->growth = (BYTE)(-strength_loss / MADD_LENGTH);
					ThraddPtr->growth_fract = (BYTE)(((strength_loss % MADD_LENGTH) << 8) / MADD_LENGTH);
					ThraddPtr->growth_err_term = 255 >> 1;
				} else {
					SET_GAME_STATE(THRADD_VISITS, 0);
					strength_loss = (SIZE)ThraddPtr->actual_strength;
					ThraddPtr->growth = (BYTE)(-strength_loss / MADD_LENGTH);
					ThraddPtr->growth_fract = (BYTE)(((strength_loss % MADD_LENGTH) << 8) / MADD_LENGTH);
				}
			}

			if (ThraddState == 0 || ThraddState > 3)
			{	/* never went to Kohr-Ah or returned */
				SetRaceDest (THRADDASH_SHIP,
						(2500 + 2535) >> 1, (8070 + 8358) >> 1,
						IlwrathPtr->days_left + 1, (BYTE)~0);
			}
			else if (ThraddState < 3)
			{	/* recall on the double */
				SetRaceDest (THRADDASH_SHIP, 2535, 8358, 10,
						ADVANCE_THRADD_MISSION);
				SET_GAME_STATE (THRADD_MISSION, 3);
			}
		}
	}

	if (EXTENDED && ThraddPtr->allied_state == GOOD_GUY && !IlwrathPtr->actual_strength) {
		ThraddPtr->growth = 0;
		ThraddPtr->growth_fract = 0;
		SET_GAME_STATE(ILWRATH_FIGHT_THRADDASH, 0);
		SetRaceDest(THRADDASH_SHIP, 2535, 8358, 3, (BYTE)~0);
		if (!GET_GAME_STATE(AQUA_HELIX)) {
			SET_GAME_STATE(HELIX_UNPROTECTED, 0);
		}
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hThradd);
	UnlockFleetInfo (&GLOBAL (avail_race_q), hIlwrath);
	
	(void) arg;
	return 0;
}

static int
advance_mycon_mission (int arg)
{
	HFLEETINFO hMycon;
	FLEET_INFO *MyconPtr;

	hMycon = GetStarShipFromIndex (&GLOBAL (avail_race_q), MYCON_SHIP);
	MyconPtr = LockFleetInfo (&GLOBAL (avail_race_q), hMycon);

	if (MyconPtr->actual_strength)
	{
		if (MyconPtr->growth)
		{
			// Head back.
			SET_GAME_STATE (MYCON_KNOW_AMBUSH, 1);
			SetRaceDest (MYCON_SHIP, 6392, 2200, 30, (BYTE)~0);

			if(EXTENDED)
				SetRaceDest (SYREEN_SHIP, 4125, 3770, 15, (BYTE)~0);

			MyconPtr->growth = 0;
			MyconPtr->growth_fract = 0;
		}
		else if (MyconPtr->loc.x != 6858 || MyconPtr->loc.y != 577)
			SetRaceDest (MYCON_SHIP, 6858, 577, 30, ADVANCE_MYCON_MISSION);
					// To Organon.
		else
		{
			// Endure losses at Organon.
			SIZE strength_loss;

			AddEvent (RELATIVE_EVENT, 0, 14, 0, ADVANCE_MYCON_MISSION);
			strength_loss = (SIZE)(MyconPtr->actual_strength >> 1);
			MyconPtr->growth = (BYTE)(-strength_loss / 14);
			MyconPtr->growth_fract = (BYTE)(((strength_loss % 14) << 8) / 14);
			MyconPtr->growth_err_term = 255 >> 1;
		}
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hMycon);
	
	(void) arg;
	return 0;
}

static int
arilou_umgah_check (int arg)
{
	SET_GAME_STATE (ARILOU_CHECKED_UMGAH, 2);

	(void) arg;
	return 0;
}

static int
yehat_rebel_event (int arg)
{
	HFLEETINFO hRebel, hRoyalist;
	FLEET_INFO *RebelPtr;
	FLEET_INFO *RoyalistPtr;

	hRebel = GetStarShipFromIndex (&GLOBAL (avail_race_q), YEHAT_REBEL_SHIP);
	RebelPtr = LockFleetInfo (&GLOBAL (avail_race_q), hRebel);
	hRoyalist = GetStarShipFromIndex (&GLOBAL (avail_race_q), YEHAT_SHIP);
	RoyalistPtr = LockFleetInfo (&GLOBAL (avail_race_q), hRoyalist);
	RoyalistPtr->actual_strength = RoyalistPtr->actual_strength * 2 / 3;
	RebelPtr->actual_strength = RoyalistPtr->actual_strength;
	RebelPtr->loc.x = 5150;
	RebelPtr->loc.y = 0;
	UnlockFleetInfo (&GLOBAL (avail_race_q), hRoyalist);
	UnlockFleetInfo (&GLOBAL (avail_race_q), hRebel);
	StartSphereTracking (YEHAT_REBEL_SHIP);

	(void) arg;
	return 0;
}

static int
slylandro_ramp_up (int arg)
{
	if (!GET_GAME_STATE (DESTRUCT_CODE_ON_SHIP))
	{
		BYTE ramp_factor;

		ramp_factor = GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
		if (++ramp_factor <= 4)
		{
			SET_GAME_STATE (SLYLANDRO_MULTIPLIER, ramp_factor);
			AddEvent (RELATIVE_EVENT, 0, 182, 0, SLYLANDRO_RAMP_UP);
		}
	}

	(void) arg;
	return 0;
}

static int
slylandro_ramp_down (int arg)
{
	BYTE ramp_factor;

	ramp_factor = GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
	if (--ramp_factor)
		AddEvent (RELATIVE_EVENT, 0, 23, 0, SLYLANDRO_RAMP_DOWN);
	SET_GAME_STATE (SLYLANDRO_MULTIPLIER, ramp_factor);

	(void) arg;
	return 0;
}

