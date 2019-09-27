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

#include <assert.h>

#include "build.h"
#include "encount.h"
#include "gameev.h"
#include "starmap.h"
#include "libs/file.h"
#include "globdata.h"
#include "options.h"
#include "save.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"

#include "libs/tasklib.h"
#include "libs/log.h"
#include "libs/misc.h"

//#define DEBUG_LOAD

ACTIVITY NextActivity;

static inline size_t
read_8 (void *fp, BYTE *v)
{
	BYTE t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 1, 1, fp);
}

static inline size_t
read_16 (void *fp, UWORD *v)
{
	UWORD t = 0;
	int shift, i;
	for (i = 0, shift = 0; i < 2; ++i, shift += 8)
	{
		BYTE b;
		if (read_8 (fp, &b) != 1)
			return 0;
		t |= ((UWORD)b) << shift;
	}

	if (v)
		*v = t;

	return 1;
}

static inline size_t
read_16s (void *fp, SWORD *v)
{
	return read_16 (fp, v);
}

static inline size_t
read_32 (void *fp, DWORD *v)
{
	DWORD t = 0;
	int shift, i;
	for (i = 0, shift = 0; i < 4; ++i, shift += 8)
	{
		BYTE b;
		if (read_8 (fp, &b) != 1)
			return 0;
		t |= ((DWORD)b) << shift;
	}

	if (v)
		*v = t;

	return 1;
}

static inline size_t
read_32s (void *fp, SDWORD *v)
{
	return read_32 (fp, v);
}

static inline size_t
read_a8 (void *fp, BYTE *ar, COUNT count)
{
	assert (ar != NULL);
	return ReadResFile (ar, 1, count, fp) == count;
}

static inline size_t
skip_8 (void *fp, COUNT count)
{
	int i;
	for (i = 0; i < count; ++i)
	{
		if (read_8(fp, NULL) != 1)
			return 0;
	}
	return 1;
}

static inline size_t
read_str (void *fp, char *str, COUNT count)
{
	// no type conversion needed for strings
	return read_a8 (fp, (BYTE *)str, count);
}

static inline size_t
read_a16 (void *fp, UWORD *ar, COUNT count)
{
	assert (ar != NULL);

	for ( ; count > 0; --count, ++ar)
	{
		if (read_16 (fp, ar) != 1)
			return 0;
	}
	return 1;
}

static void
LoadShipQueue (void *fh, QUEUE *pQueue, DWORD size)
{
	COUNT num_links = size / 11;

	while (num_links--)
	{
		HSHIPFRAG hStarShip;
		SHIP_FRAGMENT *FragPtr;
		COUNT Index;

		read_16 (fh, &Index);

		hStarShip = CloneShipFragment (Index, pQueue, 0);
		FragPtr = LockShipFrag (pQueue, hStarShip);

		// Read SHIP_FRAGMENT elements
		read_8  (fh, &FragPtr->captains_name_index);
		read_8  (fh, &FragPtr->race_id);
		read_8  (fh, &FragPtr->index);
		read_16 (fh, &FragPtr->crew_level);
		read_16 (fh, &FragPtr->max_crew);
		read_8  (fh, &FragPtr->energy_level);
		read_8  (fh, &FragPtr->max_energy);

		UnlockShipFrag (pQueue, hStarShip);
	}
}

static void
LoadRaceQueue (void *fh, QUEUE *pQueue, DWORD size)
{
	COUNT num_links = size / 30;

	while (num_links--)
	{
		HFLEETINFO hStarShip;
		FLEET_INFO *FleetPtr;
		COUNT Index;

		read_16 (fh, &Index);

		hStarShip = GetStarShipFromIndex (pQueue, Index);
		FleetPtr = LockFleetInfo (pQueue, hStarShip);

		// Read FLEET_INFO elements
		read_16 (fh, &FleetPtr->allied_state);
		read_8  (fh, &FleetPtr->days_left);
		read_8  (fh, &FleetPtr->growth_fract);
		read_16 (fh, &FleetPtr->crew_level);
		read_16 (fh, &FleetPtr->max_crew);
		read_8  (fh, &FleetPtr->growth);
		read_8  (fh, &FleetPtr->max_energy);
		read_16s(fh, &FleetPtr->loc.x);
		read_16s(fh, &FleetPtr->loc.y);

		read_16 (fh, &FleetPtr->actual_strength);
		read_16 (fh, &FleetPtr->known_strength);
		read_16s(fh, &FleetPtr->known_loc.x);
		read_16s(fh, &FleetPtr->known_loc.y);
		read_8  (fh, &FleetPtr->growth_err_term);
		read_8  (fh, &FleetPtr->func_index);
		read_16s(fh, &FleetPtr->dest_loc.x);
		read_16s(fh, &FleetPtr->dest_loc.y);

		UnlockFleetInfo (pQueue, hStarShip);
	}
}

static void
LoadGroupQueue (void *fh, QUEUE *pQueue, DWORD size)
{
	COUNT num_links = size / 13;

	while (num_links--)
	{
		HIPGROUP hGroup;
		IP_GROUP *GroupPtr;

		hGroup = BuildGroup (pQueue, 0);
		GroupPtr = LockIpGroup (pQueue, hGroup);

		read_16 (fh, &GroupPtr->group_counter);
		read_8  (fh, &GroupPtr->race_id);
		read_8  (fh, &GroupPtr->sys_loc);
		read_8  (fh, &GroupPtr->task);
		read_8  (fh, &GroupPtr->in_system); /* was crew_level */
		read_8  (fh, &GroupPtr->dest_loc);
		read_8  (fh, &GroupPtr->orbit_pos);
		read_8  (fh, &GroupPtr->group_id); /* was max_energy */
		read_16s(fh, &GroupPtr->loc.x);
		read_16s(fh, &GroupPtr->loc.y);

		UnlockIpGroup (pQueue, hGroup);
	}
}

static void
LoadEncounter (ENCOUNTER *EncounterPtr, void *fh)
{
	COUNT i;

	EncounterPtr->pred = 0;
	EncounterPtr->succ = 0;
	EncounterPtr->hElement = 0;
	read_16s (fh, &EncounterPtr->transition_state);
	read_16s (fh, &EncounterPtr->origin.x);
	read_16s (fh, &EncounterPtr->origin.y);
	read_16  (fh, &EncounterPtr->radius);
	// former STAR_DESC fields
	read_16s (fh, &EncounterPtr->loc_pt.x);
	read_16s (fh, &EncounterPtr->loc_pt.y);
	read_8   (fh, &EncounterPtr->race_id);
	read_8   (fh, &EncounterPtr->num_ships);
	read_8   (fh, &EncounterPtr->flags);

	// Load each entry in the BRIEF_SHIP_INFO array
	for (i = 0; i < MAX_HYPER_SHIPS; i++)
	{
		BRIEF_SHIP_INFO *ShipInfo = &EncounterPtr->ShipList[i];

		read_8   (fh, &ShipInfo->race_id);
		read_16  (fh, &ShipInfo->crew_level);
		read_16  (fh, &ShipInfo->max_crew);
		read_8   (fh, &ShipInfo->max_energy);
	}

	// Load the stuff after the BRIEF_SHIP_INFO array
	read_32s (fh, &EncounterPtr->log_x);
	read_32s (fh, &EncounterPtr->log_y);

	EncounterPtr->log_x <<= RESOLUTION_FACTOR;
	EncounterPtr->log_y <<= RESOLUTION_FACTOR;
}

static void
LoadEvent (EVENT *EventPtr, void *fh)
{
	EventPtr->pred = 0;
	EventPtr->succ = 0;
	read_8   (fh, &EventPtr->day_index);
	read_8   (fh, &EventPtr->month_index);
	read_16  (fh, &EventPtr->year_index);
	read_8   (fh, &EventPtr->func_index);
}

static void
LoadClockState (CLOCK_STATE *ClockPtr, void *fh)
{
	read_8   (fh, &ClockPtr->day_index);
	read_8   (fh, &ClockPtr->month_index);
	read_16  (fh, &ClockPtr->year_index);
	read_16s (fh, &ClockPtr->tick_count);
	read_16s (fh, &ClockPtr->day_in_ticks);
}

static BOOLEAN
LoadGameState (GAME_STATE *GSPtr, void *fh)
{
	DWORD magic;
	BYTE res_scale; // JMS
	read_32 (fh, &magic);
	if (magic != GLOBAL_STATE_TAG)
	{
		return FALSE;
	}
	read_32 (fh, &magic);
	if (magic != 75)
	{
		/* Chunk is the wrong size. */
		return FALSE;
	}
	read_8   (fh, &GSPtr->glob_flags);
	read_8   (fh, &GSPtr->CrewCost);
	read_8   (fh, &GSPtr->FuelCost);
	// JMS: Now that we have read the fuelcost, we can compare it
	// to the correct value. Fuel cost is always FUEL_COST_RU, and if
	// the savefile tells otherwise, we have read it with the wrong method
	// (The savegame is from vanilla UQM and we've been reading it as if it
	// were UQM-HD save.)
	//
	// At this point we must then cease reading the savefile, close it
	// and re-open it again, this time using the vanilla-reading method.
	if (GSPtr->FuelCost != FUEL_COST_RU)
		return FALSE;
	read_a8  (fh, GSPtr->ModuleCost, NUM_MODULES);
	read_a8  (fh, GSPtr->ElementWorth, NUM_ELEMENT_CATEGORIES);
	read_16  (fh, &GSPtr->CurrentActivity);

	// JMS
	if (LOBYTE (GSPtr->CurrentActivity) != IN_INTERPLANETARY)
		res_scale = RESOLUTION_FACTOR;
	else
		res_scale = 0;

	LoadClockState (&GSPtr->GameClock, fh);

	read_16s (fh, &GSPtr->autopilot.x);
	read_16s (fh, &GSPtr->autopilot.y);
	read_16s (fh, &GSPtr->ip_location.x);
	read_16s (fh, &GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	read_16s (fh, &GSPtr->ShipStamp.origin.x);
	read_16s (fh, &GSPtr->ShipStamp.origin.y);
	read_16  (fh, &GSPtr->ShipFacing);
	read_8   (fh, &GSPtr->ip_planet);
	read_8   (fh, &GSPtr->in_orbit);

	// JMS: Let's make savegames work even between different resolution modes.
	GSPtr->ShipStamp.origin.x <<= RESOLUTION_FACTOR; 
	GSPtr->ShipStamp.origin.y <<= RESOLUTION_FACTOR; 
	/* VELOCITY_DESC velocity */
	read_16  (fh, &GSPtr->velocity.TravelAngle);
	read_16s (fh, &GSPtr->velocity.vector.width);
	read_16s (fh, &GSPtr->velocity.vector.height);
	read_16s (fh, &GSPtr->velocity.fract.width);
	read_16s (fh, &GSPtr->velocity.fract.height);
	read_16s (fh, &GSPtr->velocity.error.width);
	read_16s (fh, &GSPtr->velocity.error.height);
	read_16s (fh, &GSPtr->velocity.incr.width);
	read_16s (fh, &GSPtr->velocity.incr.height);

	// JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.vector.width  <<= res_scale; 
	GSPtr->velocity.vector.height <<= res_scale; 
	GSPtr->velocity.fract.width	  <<= res_scale; 
	GSPtr->velocity.fract.height  <<= res_scale; 
	GSPtr->velocity.error.width	  <<= res_scale; 
	GSPtr->velocity.error.height  <<= res_scale; 
	GSPtr->velocity.incr.width	  <<= res_scale; 
	GSPtr->velocity.incr.height	  <<= res_scale; 
	read_32 (fh, &magic);
	if (magic != GAME_STATE_TAG)
	{
		return FALSE;
	}
	{
		size_t gameStateByteCount = (NUM_GAME_STATE_BITS + 7) >> 3;
		BYTE *buf;
		BOOLEAN result;

		read_32 (fh, &magic);
		if (magic < gameStateByteCount)
		{
			log_add (log_Error, "Warning: Savegame is corrupt: saved game "
					"state is too small.");
			return FALSE;
		}

		buf = HMalloc (gameStateByteCount);
		if (buf == NULL)
		{
			log_add (log_Error, "Warning: Cannot allocate enough bytes for "
					"the saved game state (%zu bytes).", gameStateByteCount);
			return FALSE;
		}

		read_a8 (fh, buf, gameStateByteCount);
		result = deserialiseGameState (gameStateBitMap, buf,
				gameStateByteCount);
		HFree(buf);
		if (result == FALSE)
		{
			// An error message is already printed.
			return FALSE;
		}

		if (magic > gameStateByteCount)
		{
			skip_8 (fh, magic - gameStateByteCount);
		}
	}
	return TRUE;
}

static BOOLEAN
LoadSisState (SIS_STATE *SSPtr, void *fp)
{
	if (
			read_32s (fp, &SSPtr->log_x) != 1 ||
			read_32s (fp, &SSPtr->log_y) != 1 ||
			read_32  (fp, &SSPtr->ResUnits) != 1 ||
			read_32  (fp, &SSPtr->FuelOnBoard) != 1 ||
			read_16  (fp, &SSPtr->CrewEnlisted) != 1 ||
			read_16  (fp, &SSPtr->TotalElementMass) != 1 ||
			read_16  (fp, &SSPtr->TotalBioMass) != 1 ||
			read_a8  (fp, SSPtr->ModuleSlots, NUM_MODULE_SLOTS) != 1 ||
			read_a8  (fp, SSPtr->DriveSlots, NUM_DRIVE_SLOTS) != 1 ||
			read_a8  (fp, SSPtr->JetSlots, NUM_JET_SLOTS) != 1 ||
			read_8   (fp, &SSPtr->NumLanders) != 1 ||
			read_a16 (fp, SSPtr->ElementAmounts, NUM_ELEMENT_CATEGORIES) != 1 ||

			read_str (fp, SSPtr->ShipName, SIS_NAME_SIZE) != 1 ||
			read_str (fp, SSPtr->CommanderName, SIS_NAME_SIZE) != 1 ||
			read_str (fp, SSPtr->PlanetName, SIS_NAME_SIZE) != 1 ||
			read_8   (fp, &SSPtr->Difficulty) != 1 ||
			read_8   (fp, &SSPtr->Extended) != 1 ||
			read_8   (fp, &SSPtr->Nomad) != 1 ||
			read_32s (fp, &SSPtr->Seed) != 1
		)
		return FALSE;
 	else {
		// JMS: Let's make savegames work even between different resolution modes.
		SSPtr->log_x <<= RESOLUTION_FACTOR;
		SSPtr->log_y <<= RESOLUTION_FACTOR;
		return TRUE;
	}
}

static BOOLEAN
LoadSummary (SUMMARY_DESC *SummPtr, void *fp)
{
	SDWORD magic, PrevFLoc;
	DWORD nameSize = 0;
	if (!read_32s (fp, &magic))
		return FALSE;
	if (magic == SAVEFILE_TAG)
	{
		if (read_32 (fp, &magic) != 1 || magic != SUMMARY_TAG)
			return FALSE;
		if (read_32 (fp, &magic) != 1 || magic < 160)
			return FALSE;
		nameSize = magic - 160;
	}
	else
	{
		return FALSE;
	}

	if (!LoadSisState (&SummPtr->SS, fp))
		return FALSE;

	if (
			read_8  (fp, &SummPtr->Activity) != 1 ||
			read_8  (fp, &SummPtr->Flags) != 1 ||
			read_8  (fp, &SummPtr->day_index) != 1 ||
			read_8  (fp, &SummPtr->month_index) != 1 ||
			read_16 (fp, &SummPtr->year_index) != 1 ||
			read_8  (fp, &SummPtr->MCreditLo) != 1 ||
			read_8  (fp, &SummPtr->MCreditHi) != 1 ||
			read_8  (fp, &SummPtr->NumShips) != 1 ||
			read_8  (fp, &SummPtr->NumDevices) != 1 ||
			read_a8 (fp, SummPtr->ShipList, MAX_BUILT_SHIPS) != 1 ||
			read_a8 (fp, SummPtr->DeviceList, MAX_EXCLUSIVE_DEVICES) != 1 ||
			read_8  (fp, &SummPtr->res_factor) != 1 // JMS: This'll help making saves between different resolutions compatible.		
		)
		return FALSE;
	
	if (nameSize < SAVE_NAME_SIZE)
	{
		if (read_a8 (fp, SummPtr->SaveName, nameSize) != 1)
			return FALSE;
		SummPtr->SaveName[nameSize] = 0;
	}
	else
	{
		DWORD remaining = nameSize - SAVE_NAME_SIZE + 1;
		if (read_a8 (fp, SummPtr->SaveName, SAVE_NAME_SIZE-1) != 1)
			return FALSE;
		SummPtr->SaveName[SAVE_NAME_SIZE-1] = 0;
		if (skip_8 (fp, remaining) != 1)
			return FALSE;
	}

	return TRUE;
}

static void
LoadStarDesc (STAR_DESC *SDPtr, void *fh)
{
	read_16s(fh, &SDPtr->star_pt.x);
	read_16s(fh, &SDPtr->star_pt.y);
	read_8  (fh, &SDPtr->Type);
	read_8  (fh, &SDPtr->Index);
	read_8  (fh, &SDPtr->Prefix);
	read_8  (fh, &SDPtr->Postfix);
}

static void
LoadScanInfo (uio_Stream *fh, DWORD flen)
{
	GAME_STATE_FILE *fp = OpenStateFile (STARINFO_FILE, "wb");
	if (fp)
	{
		while (flen)
		{
			DWORD val;
			read_32 (fh, &val);
			swrite_32 (fp, val);
			flen -= 4;
		}
		CloseStateFile (fp);
	}
}

static void
LoadGroupList (uio_Stream *fh, DWORD chunksize)
{
	GAME_STATE_FILE *fp = OpenStateFile (RANDGRPINFO_FILE, "rb");
	if (fp)
	{
		GROUP_HEADER h;
		BYTE LastEnc, NumGroups;
		int i;
		ReadGroupHeader (fp, &h);
		/* There's only supposed to be one of these, so group 0 should be
		 * zero here whenever we're here. We add the group list to the
		 * end here. */
		h.GroupOffset[0] = LengthStateFile (fp);
		SeekStateFile (fp, 0, SEEK_SET);
		WriteGroupHeader (fp, &h);
		SeekStateFile (fp, h.GroupOffset[0], SEEK_SET);
		read_8 (fh, &LastEnc);
		NumGroups = (chunksize - 1) / 14;
		swrite_8 (fp, LastEnc);
		swrite_8 (fp, NumGroups);
		for (i = 0; i < NumGroups; ++i)
		{
			BYTE race_outer;
			IP_GROUP ip;
			read_8  (fh, &race_outer);
			read_16 (fh, &ip.group_counter);
			read_8  (fh, &ip.race_id);
			read_8  (fh, &ip.sys_loc);
			read_8  (fh, &ip.task);
			read_8  (fh, &ip.in_system);
			read_8  (fh, &ip.dest_loc);
			read_8  (fh, &ip.orbit_pos);
			read_8  (fh, &ip.group_id);
			read_16 (fh, &ip.loc.x);
			read_16 (fh, &ip.loc.y);

			swrite_8 (fp, race_outer);
			WriteIpGroup (fp, &ip);
		}
		CloseStateFile (fp);
	}
}

static void
SetBattleGroupOffset (int encounterIndex, DWORD offset)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because SET_GAME_STATE is a #define, which stringizes
	// its first argument.
	switch (encounterIndex)
	{
		case  1: SET_GAME_STATE (SHOFIXTI_GRPOFFS,     offset); break;
		case  2: SET_GAME_STATE (ZOQFOT_GRPOFFS,       offset); break;
		case  3: SET_GAME_STATE (MELNORME0_GRPOFFS,    offset); break;
		case  4: SET_GAME_STATE (MELNORME1_GRPOFFS,    offset); break;
		case  5: SET_GAME_STATE (MELNORME2_GRPOFFS,    offset); break;
		case  6: SET_GAME_STATE (MELNORME3_GRPOFFS,    offset); break;
		case  7: SET_GAME_STATE (MELNORME4_GRPOFFS,    offset); break;
		case  8: SET_GAME_STATE (MELNORME5_GRPOFFS,    offset); break;
		case  9: SET_GAME_STATE (MELNORME6_GRPOFFS,    offset); break;
		case 10: SET_GAME_STATE (MELNORME7_GRPOFFS,    offset); break;
		case 11: SET_GAME_STATE (MELNORME8_GRPOFFS,    offset); break;
		case 12: SET_GAME_STATE (URQUAN_PROBE_GRPOFFS, offset); break;
		case 13: SET_GAME_STATE (COLONY_GRPOFFS,       offset); break;
		case 14: SET_GAME_STATE (SAMATRA_GRPOFFS,      offset); break;
		default:
			log_add (log_Warning, "SetBattleGroupOffset: invalid encounter "
					"index.\n");
			break;
	}
}

static void
LoadBattleGroup (uio_Stream *fh, DWORD chunksize)
{
	GAME_STATE_FILE *fp;
	GROUP_HEADER h;
	DWORD encounter, offset;
	BYTE current;
	int i;

	read_32 (fh, &encounter);
	read_8 (fh, &current);
	chunksize -= 5;
	if (encounter)
	{
		/* This is a defined group, so it's new */
		fp = OpenStateFile (DEFGRPINFO_FILE, "rb");
		offset = LengthStateFile (fp);
		memset (&h, 0, sizeof (GROUP_HEADER));
	}
	else
	{
		/* This is the random group. Load in what was there,
		 * as we might have already seen the Group List. */
		fp = OpenStateFile (RANDGRPINFO_FILE, "rb");
		current = FALSE;
		offset = 0;
		ReadGroupHeader (fp, &h);
	}
	if (!fp)
	{
		skip_8 (fh, chunksize);
		return;
	}
	read_16 (fh, &h.star_index);
	read_8  (fh, &h.day_index);
	read_8  (fh, &h.month_index);
	read_16 (fh, &h.year_index);
	read_8  (fh, &h.NumGroups);
	chunksize -= 7;
	/* Write out the half-finished state file so that we can use
	 * the file size to compute group offsets */
	SeekStateFile (fp, offset, SEEK_SET);
	WriteGroupHeader (fp, &h);
	for (i = 1; i <= h.NumGroups; ++i)
	{
		int j;
		BYTE icon, NumShips;
		read_8 (fh, &icon);
		read_8 (fh, &NumShips);
		chunksize -= 2;
		h.GroupOffset[i] = LengthStateFile (fp);
		SeekStateFile (fp, h.GroupOffset[i], SEEK_SET);
		swrite_8 (fp, icon);
		swrite_8 (fp, NumShips);
		for (j = 0; j < NumShips; ++j)
		{
			BYTE race_outer;
			SHIP_FRAGMENT sf;
			read_8  (fh, &race_outer);
			read_8  (fh, &sf.captains_name_index);
			read_8  (fh, &sf.race_id);
			read_8  (fh, &sf.index);
			read_16 (fh, &sf.crew_level);
			read_16 (fh, &sf.max_crew);
			read_8  (fh, &sf.energy_level);
			read_8  (fh, &sf.max_energy);
			chunksize -= 10;

			swrite_8 (fp, race_outer);
			WriteShipFragment (fp, &sf);
		}
	}
	/* Now that the GroupOffset array is properly initialized,
	 * write the header back out. */
	SeekStateFile (fp, offset, SEEK_SET);
	WriteGroupHeader (fp, &h);
	CloseStateFile (fp);
	/* And update the gamestate accordingly, if we're a defined group. */
	if (encounter)
	{
		SetBattleGroupOffset (encounter, offset);
		if (current)
		{
			GLOBAL (BattleGroupRef) = offset;
		}
	}
	/* Consistency check. */
	if (chunksize)
	{
		log_add (log_Warning, "BattleGroup chunk mis-sized!");
	}
}

BOOLEAN
LoadGame (COUNT which_game, SUMMARY_DESC *SummPtr)
{
	uio_Stream *in_fp;
	char file[PATH_MAX];
	SUMMARY_DESC loc_sd;
	COUNT num_links;
	STAR_DESC SD;
	ACTIVITY Activity;
	DWORD chunk, chunkSize;
	BOOLEAN first_group_spec = TRUE;

	sprintf (file, "uqmsave.%02u", which_game);
	in_fp = res_OpenResFile (saveDir, file, "rb");
	if (!in_fp)
		return LoadLegacyGame(which_game, SummPtr, FALSE);

	if (!LoadSummary (&loc_sd, in_fp))
	{
		res_CloseResFile (in_fp);
		return LoadLegacyGame(which_game, SummPtr, FALSE);
	}

	if (!SummPtr)
	{
		SummPtr = &loc_sd;
	}
	else
	{	// only need summary for displaying to user
		memcpy (SummPtr, &loc_sd, sizeof (*SummPtr));
		res_CloseResFile (in_fp);
		return TRUE;
	}

	GlobData.SIS_state = SummPtr->SS;

	ReinitQueue (&GLOBAL (GameClock.event_q));
	ReinitQueue (&GLOBAL (encounter_q));
	ReinitQueue (&GLOBAL (ip_group_q));
	ReinitQueue (&GLOBAL (npc_built_ship_q));
	ReinitQueue (&GLOBAL (built_ship_q));
	
	uninitEventSystem ();
	luaUqm_uninitState();
	luaUqm_initState();
	initEventSystem ();

	Activity = GLOBAL (CurrentActivity);

	if (!LoadGameState (&GlobData.Game_state, in_fp))
	{
		res_CloseResFile (in_fp);
		return FALSE;
	}
	NextActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = Activity;

	chunk = 0;
	while (TRUE)
	{
		if (read_32(in_fp, &chunk) != 1)
		{
			break;
		}
		if (read_32(in_fp, &chunkSize) != 1)
		{
			res_CloseResFile (in_fp);
			return FALSE;
		}
		switch (chunk)
		{
		case RACE_Q_TAG:
			LoadRaceQueue (in_fp, &GLOBAL (avail_race_q), chunkSize);
			break;
		case IP_GRP_Q_TAG:
			LoadGroupQueue (in_fp, &GLOBAL (ip_group_q), chunkSize);
			break;
		case ENCOUNTERS_TAG:
			num_links = chunkSize / 65;
			while (num_links--)
			{
				HENCOUNTER hEncounter;
				ENCOUNTER *EncounterPtr;

				hEncounter = AllocEncounter ();
				LockEncounter (hEncounter, &EncounterPtr);

				LoadEncounter (EncounterPtr, in_fp);

				UnlockEncounter (hEncounter);
				PutEncounter (hEncounter);
			}
			break;
		case EVENTS_TAG:
			num_links = chunkSize / 5;
#ifdef DEBUG_LOAD
			log_add (log_Debug, "EVENTS:");
#endif /* DEBUG_LOAD */
			while (num_links--)
			{
				HEVENT hEvent;
				EVENT *EventPtr;

				hEvent = AllocEvent ();
				LockEvent (hEvent, &EventPtr);

				LoadEvent (EventPtr, in_fp);

#ifdef DEBUG_LOAD
				log_add (log_Debug, "\t%u/%u/%u -- %u",
						 EventPtr->month_index,
						 EventPtr->day_index,
						 EventPtr->year_index,
						 EventPtr->func_index);
#endif /* DEBUG_LOAD */
				UnlockEvent (hEvent);
				PutEvent (hEvent);
			}
			break;
		case STAR_TAG:
			LoadStarDesc (&SD, in_fp);			
			loadGameCheats();
			break;
		case NPC_SHIP_Q_TAG:
			LoadShipQueue (in_fp, &GLOBAL (npc_built_ship_q), chunkSize);
			break;
		case SHIP_Q_TAG:
			LoadShipQueue (in_fp, &GLOBAL (built_ship_q), chunkSize);
			break;
		case SCAN_TAG:
			LoadScanInfo (in_fp, chunkSize);
			break;
		case GROUP_LIST_TAG:
			if (first_group_spec)
			{
				InitGroupInfo (TRUE);
				GLOBAL (BattleGroupRef) = 0;
				first_group_spec = FALSE;
			}
			LoadGroupList (in_fp, chunkSize);
			break;
		case BATTLE_GROUP_TAG:
			if (first_group_spec)
			{
				InitGroupInfo (TRUE);
				GLOBAL (BattleGroupRef) = 0;
				first_group_spec = FALSE;
			}
			LoadBattleGroup (in_fp, chunkSize);
			break;
		default:
			log_add (log_Debug, "Skipping chunk of tag %08X (size %u)", chunk, chunkSize);
			if (skip_8(in_fp, chunkSize) != 1)
			{
				res_CloseResFile (in_fp);
				return FALSE;
			}
			break;
		}
	}
	res_CloseResFile (in_fp);

	EncounterGroup = 0;
	EncounterRace = -1;

	ReinitQueue (&race_q[0]);
	ReinitQueue (&race_q[1]);
	CurStarDescPtr = FindStar (NULL, &SD.star_pt, 0, 0);
	if (!(NextActivity & START_ENCOUNTER)
			&& LOBYTE (NextActivity) == IN_INTERPLANETARY)
		NextActivity |= START_INTERPLANETARY;

	return TRUE;
}