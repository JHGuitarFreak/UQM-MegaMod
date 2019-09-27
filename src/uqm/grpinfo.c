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

#include "build.h"
#include "starmap.h"
#include "gendef.h"
#include "libs/file.h"
#include "globdata.h"
#include "intel.h"
#include "state.h"
#include "grpintrn.h"

#include "libs/mathlib.h"
#include "libs/log.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static BYTE LastEncGroup;
		// Last encountered group, saved into state files

void
ReadGroupHeader (GAME_STATE_FILE *fp, GROUP_HEADER *pGH)
{
	sread_8   (fp, &pGH->NumGroups);
	sread_8   (fp, &pGH->day_index);
	sread_8   (fp, &pGH->month_index);
	sread_8   (fp, NULL); /* padding */
	sread_16  (fp, &pGH->star_index);
	sread_16  (fp, &pGH->year_index);
	sread_a32 (fp, pGH->GroupOffset, NUM_SAVED_BATTLE_GROUPS + 1);
}

void
WriteGroupHeader (GAME_STATE_FILE *fp, const GROUP_HEADER *pGH)
{
	swrite_8   (fp, pGH->NumGroups);
	swrite_8   (fp, pGH->day_index);
	swrite_8   (fp, pGH->month_index);
	swrite_8   (fp, 0); /* padding */
	swrite_16  (fp, pGH->star_index);
	swrite_16  (fp, pGH->year_index);
	swrite_a32 (fp, pGH->GroupOffset, NUM_SAVED_BATTLE_GROUPS + 1);
}

void
ReadShipFragment (GAME_STATE_FILE *fp, SHIP_FRAGMENT *FragPtr)
{
	BYTE tmpb;

	sread_16 (fp, NULL); /* unused: was which_side */
	sread_8  (fp, &FragPtr->captains_name_index);
	sread_8  (fp, NULL); /* padding; for savegame compat */
	sread_16 (fp, NULL); /* unused: was ship_flags */
	sread_8  (fp, &FragPtr->race_id);
	sread_8  (fp, &FragPtr->index);
	// XXX: reading crew as BYTE to maintain savegame compatibility
	sread_8  (fp, &tmpb);
	FragPtr->crew_level = tmpb;
	sread_8  (fp, &tmpb);
	FragPtr->max_crew = tmpb;
	sread_8  (fp, &FragPtr->energy_level);
	sread_8  (fp, &FragPtr->max_energy);
	sread_16 (fp, NULL); /* unused; was loc.x */
	sread_16 (fp, NULL); /* unused; was loc.y */
}

void
WriteShipFragment (GAME_STATE_FILE *fp, const SHIP_FRAGMENT *FragPtr)
{
	swrite_16 (fp, 0); /* unused: was which_side */
	swrite_8  (fp, FragPtr->captains_name_index);
	swrite_8  (fp, 0); /* padding; for savegame compat */
	swrite_16 (fp, 0); /* unused: was ship_flags */
	swrite_8  (fp, FragPtr->race_id);
	swrite_8  (fp, FragPtr->index);
	// XXX: writing crew as BYTE to maintain savegame compatibility
	swrite_8  (fp, FragPtr->crew_level);
	swrite_8  (fp, FragPtr->max_crew);
	swrite_8  (fp, FragPtr->energy_level);
	swrite_8  (fp, FragPtr->max_energy);
	swrite_16 (fp, 0); /* unused; was loc.x */
	swrite_16 (fp, 0); /* unused; was loc.y */
}

void
ReadIpGroup (GAME_STATE_FILE *fp, IP_GROUP *GroupPtr)
{
	BYTE tmpb;

	sread_16 (fp, NULL); /* unused; was which_side */
	sread_8  (fp, NULL); /* unused; was captains_name_index */
	sread_8  (fp, NULL); /* padding; for savegame compat */
	sread_16 (fp, &GroupPtr->group_counter);
	sread_8  (fp, &GroupPtr->race_id);
	sread_8  (fp, &tmpb); /* was var2 */
	GroupPtr->sys_loc = LONIBBLE (tmpb);
	GroupPtr->task = HINIBBLE (tmpb);
	sread_8  (fp, &GroupPtr->in_system); /* was crew_level */
	sread_8  (fp, NULL); /* unused; was max_crew */
	sread_8  (fp, &tmpb); /* was energy_level */
	GroupPtr->dest_loc = LONIBBLE (tmpb);
	GroupPtr->orbit_pos = HINIBBLE (tmpb);
	sread_8  (fp, &GroupPtr->group_id); /* was max_energy */
	sread_16s(fp, &GroupPtr->loc.x);
	sread_16s(fp, &GroupPtr->loc.y);
}

void
WriteIpGroup (GAME_STATE_FILE *fp, const IP_GROUP *GroupPtr)
{
	swrite_16 (fp, 0); /* unused; was which_side */
	swrite_8  (fp, 0); /* unused; was captains_name_index */
	swrite_8  (fp, 0); /* padding; for savegame compat */
	swrite_16 (fp, GroupPtr->group_counter);
	swrite_8  (fp, GroupPtr->race_id);
	assert (GroupPtr->sys_loc < 0x10 && GroupPtr->task < 0x10);
	swrite_8  (fp, MAKE_BYTE (GroupPtr->sys_loc, GroupPtr->task));
			/* was var2 */
	swrite_8  (fp, GroupPtr->in_system); /* was crew_level */
	swrite_8  (fp, 0); /* unused; was max_crew */
	assert (GroupPtr->dest_loc < 0x10 && GroupPtr->orbit_pos < 0x10);
	swrite_8  (fp, MAKE_BYTE (GroupPtr->dest_loc, GroupPtr->orbit_pos));
			/* was energy_level */
	swrite_8  (fp, GroupPtr->group_id); /* was max_energy */
	swrite_16 (fp, GroupPtr->loc.x);
	swrite_16 (fp, GroupPtr->loc.y);
}

void
InitGroupInfo (BOOLEAN FirstTime)
{
	GAME_STATE_FILE *fp;

	assert (NUM_SAVED_BATTLE_GROUPS >= MAX_BATTLE_GROUPS);

	fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
	if (fp)
	{
		GROUP_HEADER GH;

		memset (&GH, 0, sizeof (GH));
		GH.star_index = (COUNT)~0;
		WriteGroupHeader (fp, &GH);
		CloseStateFile (fp);
	}

	if (FirstTime && (fp = OpenStateFile (DEFGRPINFO_FILE, "wb")))
	{
		// Group headers cannot start with offset 0 in 'defined' group
		// info file, so bump it (because offset 0 is reserved to
		// indicate the 'random' group info file).
		swrite_8 (fp, 0);
		CloseStateFile (fp);
	}
}

void
UninitGroupInfo (void)
{
	DeleteStateFile (DEFGRPINFO_FILE);
	DeleteStateFile (RANDGRPINFO_FILE);
}

HIPGROUP
BuildGroup (QUEUE *pDstQueue, BYTE race_id)
{
	HFLEETINFO hFleet;
	FLEET_INFO *TemplatePtr;
	HLINK hGroup;
	IP_GROUP *GroupPtr;

	assert (GetLinkSize (pDstQueue) == sizeof (IP_GROUP));

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race_id);
	if (!hFleet)
		return 0;

	hGroup = AllocLink (pDstQueue);
	if (!hGroup)
		return 0;
	
	TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	GroupPtr = LockIpGroup (pDstQueue, hGroup);
	memset (GroupPtr, 0, GetLinkSize (pDstQueue));
	GroupPtr->race_id = race_id;
	GroupPtr->melee_icon = TemplatePtr->melee_icon;
	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	UnlockIpGroup (pDstQueue, hGroup);
	PutQueue (pDstQueue, hGroup);

	return hGroup;
}

void
BuildGroups (void)
{
	BYTE Index;
	BYTE BestIndex = 0;
	COUNT BestPercent = 0;
	POINT universe;
	HFLEETINFO hFleet, hNextFleet;

	BYTE HomeWorld[] = { HOMEWORLD_LOC };
	BYTE EncounterPercent[] = { RACE_INTERPLANETARY_PERCENT };

	EncounterPercent[SLYLANDRO_SHIP] *= GET_GAME_STATE (SLYLANDRO_MULTIPLIER);

	/* make Ur-Quan encounters impossible at the ZFP homeworld,
	 * like their dialog claims */
	if (EXTENDED && CurStarDescPtr->Index == ZOQFOT_DEFINED)
		EncounterPercent[URQUAN_SHIP] = EncounterPercent[BLACK_URQUAN_SHIP] = 0;

	Index = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
	if (Index > 1 && Index < 5)
	{
		// When the Utwig and Supox are on their mission, there won't be
		// new battle groups generated for the system.
		// Note that old groups may still exist (in which case this function
		// would not even be called), but those expire after spending a week
		// outside of the star system, or when a different star system is
		// entered.
		HomeWorld[UTWIG_SHIP] = 0;
		HomeWorld[SUPOX_SHIP] = 0;
	}

	universe = CurStarDescPtr->star_pt;
	for (hFleet = GetHeadLink (&GLOBAL (avail_race_q)), Index = 0;
			hFleet; hFleet = hNextFleet, ++Index)
	{
		COUNT i, encounter_radius;
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
		hNextFleet = _GetSuccLink (FleetPtr);

		if ((encounter_radius = FleetPtr->actual_strength)
				&& (i = EncounterPercent[Index]))
		{
			SIZE dx, dy;
			DWORD d_squared;
			BYTE race_enc;

			race_enc = HomeWorld[Index];
			if (race_enc && CurStarDescPtr->Index == race_enc)
			{	// In general, there are always ships at the Homeworld for
				// the races specified in HomeWorld[] array.
				BestIndex = Index;
				BestPercent = 70;
				if (race_enc == SPATHI_DEFINED || race_enc == SUPOX_DEFINED)
					BestPercent = 2;
				// Terminate the loop!
				hNextFleet = 0;

				goto FoundHome;
			}

			if (encounter_radius == INFINITE_RADIUS)
				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
			else
				encounter_radius =
						(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;
			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
				dx = -dx;
			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
				dy = -dy;
			if ((COUNT)dx < encounter_radius
					&& (COUNT)dy < encounter_radius
					&& (d_squared = (DWORD)dx * dx + (DWORD)dy * dy) <
					(DWORD)encounter_radius * encounter_radius)
			{
				DWORD rand_val;

				// EncounterPercent is only used in practice for the Slylandro
				// Probes, for the rest of races the chance of encounter is
				// calced directly below from the distance to the Homeworld
				if (FleetPtr->actual_strength != INFINITE_RADIUS)
				{
					i = 70 - (COUNT)((DWORD)square_root (d_squared)
							* 60L / encounter_radius);
				}


				rand_val = TFB_Random ();
				if ((int)(LOWORD (rand_val) % 100) < (int)i
						&& (BestPercent == 0
						|| (HIWORD (rand_val) % (i + BestPercent)) < i))
				{
					if (FleetPtr->actual_strength == INFINITE_RADIUS)
					{	// The prevailing encounter chance is hereby limitted
						// to 4% for races with infinite SoI (currently, it
						// is only the Slylandro Probes)
						i = 4;
					}
					BestPercent = i;
					BestIndex = Index;
				}
			}
		}

FoundHome:
		UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	}

	if (BestPercent)
	{
		BYTE which_group, num_groups;
		BYTE EncounterMakeup[] =
		{
			RACE_ENCOUNTER_MAKEUP
		};

		which_group = 0;
		num_groups = ((COUNT)TFB_Random () % (BestPercent >> 1)) + 1;
		if (num_groups > MAX_BATTLE_GROUPS)
			num_groups = MAX_BATTLE_GROUPS;
		else if (num_groups < 5
				&& (Index = HomeWorld[BestIndex])
				&& CurStarDescPtr->Index == Index)
			num_groups = 5;
		do
		{
			for (Index = HINIBBLE (EncounterMakeup[BestIndex]); Index;
					--Index)
			{
				if (Index <= LONIBBLE (EncounterMakeup[BestIndex])
						|| (COUNT)TFB_Random () % 100 < 50)
					CloneShipFragment (BestIndex,
							&GLOBAL (npc_built_ship_q), 0);
			}

			PutGroupInfo (GROUPS_RANDOM, ++which_group);
			ReinitQueue (&GLOBAL (npc_built_ship_q));
		} while (--num_groups);
	}

	GetGroupInfo (GROUPS_RANDOM, GROUP_INIT_IP);
}

void
findRaceSOI(void) {
	POINT universe;
	HFLEETINFO hFleet, hNextFleet;
	BYTE Index, HomeworldID, loop;
	BYTE speciesID[4] = { 0 };

	BYTE EncounterPercent[] = { RACE_MUSIC_BOOL };
	BYTE HomeWorld[] = { HOMEWORLD_LOC };

	loop = 0;
	HomeworldID = 0;
	Index = 0;

	universe = CurStarDescPtr->star_pt;

	for (hFleet = GetHeadLink(&GLOBAL(avail_race_q)), Index = 0;
		hFleet; hFleet = hNextFleet, ++Index)
	{
		COUNT i, encounter_radius;
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
		hNextFleet = _GetSuccLink(FleetPtr);

		if ((encounter_radius = FleetPtr->actual_strength)
			&& (i = EncounterPercent[Index]))
		{
			SIZE dx, dy;
			DWORD d_squared;
			BYTE race_enc = HomeWorld[Index];

			if (race_enc && CurStarDescPtr->Index == race_enc) 
			{	
				// Finds the race homeworld
				hNextFleet = 0;
				HomeworldID = FleetPtr->SpeciesID;
				break;
			}

			if (encounter_radius == INFINITE_RADIUS)
				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
			else
				encounter_radius = (encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;

			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
				dx = -dx;

			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
				dy = -dy;

			if ((COUNT)dx < encounter_radius 
				&& (COUNT)dy < encounter_radius
				&& (d_squared = (DWORD)dx * dx + (DWORD)dy * dy) 
				< (DWORD)encounter_radius * encounter_radius) 
			{	
				// Finds the race SOI
				speciesID[++loop] = FleetPtr->SpeciesID;
			}
		}
	}

	if (HomeworldID == 0 && LOBYTE(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY) {
		if (speciesID[1] == 0)
			spaceMusicBySOI = NO_ID;
		if (speciesID[1] > 0 && speciesID[2] == 0)
			spaceMusicBySOI = speciesID[1];
		if (speciesID[1] > 0 && speciesID[2] > 0)
			spaceMusicBySOI = speciesID[1];
		if (speciesID[1] > 0 && speciesID[2] > 0 && speciesID[3] > 0)
			spaceMusicBySOI = speciesID[2];
	} else {
		spaceMusicBySOI = HomeworldID; 
	}

	if (GET_GAME_STATE(KOHR_AH_FRENZY) && LOBYTE(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY)
		spaceMusicBySOI = SA_MATRA_ID;
}

static void
FlushGroupInfo (GROUP_HEADER* pGH, DWORD offset, BYTE which_group, GAME_STATE_FILE *fp)
{
	if (which_group == GROUP_LIST)
	{
		HIPGROUP hGroup, hNextGroup;

		/* If the group list was never written before, add it */
		if (pGH->GroupOffset[0] == 0)
			pGH->GroupOffset[0] = LengthStateFile (fp);

		// XXX: npc_built_ship_q must be empty because the wipe-out
		//   procedure is actually the writing of the npc_built_ship_q
		//   out as the group in question
		assert (!GetHeadLink (&GLOBAL (npc_built_ship_q)));

		/* Weed out the groups that left the system first */
		for (hGroup = GetHeadLink (&GLOBAL (ip_group_q));
				hGroup; hGroup = hNextGroup)
		{
			BYTE in_system;
			BYTE group_id;
			IP_GROUP *GroupPtr;

			GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
			hNextGroup = _GetSuccLink (GroupPtr);
			in_system = GroupPtr->in_system;
			group_id = GroupPtr->group_id;
			UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);

			if (!in_system)
			{
				// The following 'if' is needed because GROUP_LIST is only
				// ever flushed to RANDGRPINFO_FILE, but the current group
				// may need to be updated in the DEFGRPINFO_FILE as well.
				// In that case, PutGroupInfo() will update the correct file.
				if (GLOBAL (BattleGroupRef))
					PutGroupInfo (GLOBAL (BattleGroupRef), group_id);
				else
					FlushGroupInfo (pGH, GROUPS_RANDOM, group_id, fp);
				// This will also wipe the group out in the RANDGRPINFO_FILE
				pGH->GroupOffset[group_id] = 0;
				RemoveQueue (&GLOBAL (ip_group_q), hGroup);
				FreeIpGroup (&GLOBAL (ip_group_q), hGroup);
			}
		}
	}
	else if (which_group > pGH->NumGroups)
	{	/* Group not present yet -- add it */
		pGH->NumGroups = which_group;
		pGH->GroupOffset[which_group] = LengthStateFile (fp);
	}
	
	SeekStateFile (fp, offset, SEEK_SET);
	WriteGroupHeader (fp, pGH);

#ifdef DEBUG_GROUPS
	log_add (log_Debug, "1)FlushGroupInfo(%lu): WG = %u(%lu), NG = %u, "
			"SI = %u", offset, which_group, pGH->GroupOffset[which_group],
			pGH->NumGroups, pGH->star_index);
#endif /* DEBUG_GROUPS */

	if (which_group == GROUP_LIST)
	{
		/* Write out ip_group_q as group 0 */
		HIPGROUP hGroup, hNextGroup;
		BYTE NumGroups = CountLinks (&GLOBAL (ip_group_q));

		SeekStateFile (fp, pGH->GroupOffset[0], SEEK_SET);
		swrite_8 (fp, LastEncGroup);
		swrite_8 (fp, NumGroups);

		hGroup = GetHeadLink (&GLOBAL (ip_group_q));
		for ( ; NumGroups; --NumGroups, hGroup = hNextGroup)
		{
			IP_GROUP *GroupPtr;
			
			GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
			hNextGroup = _GetSuccLink (GroupPtr);

			swrite_8 (fp, GroupPtr->race_id);

#ifdef DEBUG_GROUPS
			log_add (log_Debug, "F) type %u, loc %u<%d, %d>, task 0x%02x:%u",
					GroupPtr->race_id,
					GET_GROUP_LOC (GroupPtr),
					GroupPtr->loc.x,
					GroupPtr->loc.y,
					GET_GROUP_MISSION (GroupPtr),
					GET_GROUP_DEST (GroupPtr));
#endif /* DEBUG_GROUPS */

			WriteIpGroup (fp, GroupPtr);
			
			UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		}
	}
	else
	{
		/* Write out npc_built_ship_q as 'which_group' group */
		HSHIPFRAG hStarShip, hNextShip;
		BYTE NumShips = CountLinks (&GLOBAL (npc_built_ship_q));
		BYTE RaceType = 0;

		hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
		if (NumShips > 0)
		{
			SHIP_FRAGMENT *FragPtr;

			/* The first ship in a group defines the alien race */
			FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			RaceType = FragPtr->race_id;
			UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		}

		SeekStateFile (fp, pGH->GroupOffset[which_group], SEEK_SET);
		swrite_8 (fp, RaceType);
		swrite_8 (fp, NumShips);

		for ( ; NumShips; --NumShips, hStarShip = hNextShip)
		{
			SHIP_FRAGMENT *FragPtr;

			FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			hNextShip = _GetSuccLink (FragPtr);

			swrite_8 (fp, FragPtr->race_id);
			WriteShipFragment (fp, FragPtr);

			UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		}
	}
}

BOOLEAN
GetGroupInfo (DWORD offset, BYTE which_group)
{
	GAME_STATE_FILE *fp;
	GROUP_HEADER GH;

	if (offset != GROUPS_RANDOM && which_group != GROUP_LIST)
		fp = OpenStateFile (DEFGRPINFO_FILE, "r+b");
	else
		fp = OpenStateFile (RANDGRPINFO_FILE, "r+b");

	if (!fp)
		return FALSE;

	SeekStateFile (fp, offset, SEEK_SET);
	ReadGroupHeader (fp, &GH);
#ifdef DEBUG_GROUPS
	log_add (log_Debug, "GetGroupInfo(%lu): %u(%lu) out of %u", offset,
			which_group, GH.GroupOffset[which_group], GH.NumGroups);
#endif /* DEBUG_GROUPS */

	if (which_group == GROUP_INIT_IP)
	{
		COUNT month_index, day_index, year_index;

		ReinitQueue (&GLOBAL (ip_group_q));
#ifdef DEBUG_GROUPS
		log_add (log_Debug, "%u == %u", GH.star_index,
				(COUNT)(CurStarDescPtr - star_array));
#endif /* DEBUG_GROUPS */

		/* Check if the requested groups are valid for this star system
		 * and if they are still current (not expired) */
		day_index = GH.day_index;
		month_index = GH.month_index;
		year_index = GH.year_index;
		if (offset == GROUPS_RANDOM
				&& (GH.star_index != (COUNT)(CurStarDescPtr - star_array)
				|| !ValidateEvent (ABSOLUTE_EVENT, &month_index, &day_index,
					&year_index)))
		{
#ifdef DEBUG_GROUPS
			if (GH.star_index == CurStarDescPtr - star_array)
				log_add (log_Debug, "GetGroupInfo: battle groups out of "
						"date %u/%u/%u!", month_index, day_index,
						year_index);
#endif /* DEBUG_GROUPS */

			CloseStateFile (fp);
			/* Erase random groups (out of date) */
			fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
			memset (&GH, 0, sizeof (GH));
			GH.star_index = (COUNT)~0;
			WriteGroupHeader (fp, &GH);
			CloseStateFile (fp);
			
			return FALSE;
		}

		/* Read IP groups into ip_group_q and send them on their missions */
		for (which_group = 1; which_group <= GH.NumGroups; ++which_group)
		{
			BYTE task, group_loc;
			DWORD rand_val;
			BYTE RaceType;
			BYTE NumShips;
			HIPGROUP hGroup;
			IP_GROUP *GroupPtr;

			if (GH.GroupOffset[which_group] == 0)
				continue;

			SeekStateFile (fp, GH.GroupOffset[which_group], SEEK_SET);
			sread_8 (fp, &RaceType);
			sread_8 (fp, &NumShips);
			if (!NumShips)
				continue; /* group is dead */

			hGroup = BuildGroup (&GLOBAL (ip_group_q), RaceType);
			GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
			GroupPtr->group_id = which_group;
			GroupPtr->in_system = 1;

			rand_val = TFB_Random ();
			task = (BYTE)(LOBYTE (LOWORD (rand_val)) % ON_STATION);
			if (task == FLEE)
				task = ON_STATION;
			GroupPtr->orbit_pos = NORMALIZE_FACING (
					LOBYTE (HIWORD (rand_val)));

			group_loc = pSolarSysState->SunDesc[0].NumPlanets;
			if (group_loc == 1 && task == EXPLORE)
				task = IN_ORBIT;
			else
				group_loc = (BYTE)((HIBYTE (LOWORD (rand_val)) % group_loc) + 1);
			GroupPtr->dest_loc = group_loc;
			rand_val = TFB_Random ();
			GroupPtr->loc.x = (LOWORD (rand_val) % 10000) - 5000;
			GroupPtr->loc.y = (HIWORD (rand_val) % 10000) - 5000;
			GroupPtr->group_counter = 0;
			if (task == EXPLORE)
			{
				GroupPtr->group_counter = ((COUNT)TFB_Random () %
						MAX_REVOLUTIONS) << FACING_SHIFT;
			}
			else if (task == ON_STATION)
			{
				COUNT angle;
				POINT org;

				org = planetOuterLocation (group_loc - 1);
				angle = FACING_TO_ANGLE (GroupPtr->orbit_pos + 1);
				GroupPtr->loc.x = org.x + COSINE (angle, STATION_RADIUS);
				GroupPtr->loc.y = org.y + SINE (angle, STATION_RADIUS);
				group_loc = 0;
			}

			GroupPtr->task = task;
			GroupPtr->sys_loc = group_loc;

#ifdef DEBUG_GROUPS
			log_add (log_Debug, "battle group %u(0x%04x) strength "
					"%u, type %u, loc %u<%d, %d>, task %u",
					which_group,
					hGroup,
					NumShips,
					RaceType,
					group_loc,
					GroupPtr->loc.x,
					GroupPtr->loc.y,
					task);
#endif /* DEBUG_GROUPS */

			UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		}

		if (offset != GROUPS_RANDOM)
			InitGroupInfo (FALSE); 	/* Wipe out random battle groups */
		else if (ValidateEvent (ABSOLUTE_EVENT, /* still fresh */
					&month_index, &day_index, &year_index))
		{
			CloseStateFile (fp);
			return TRUE;
		}

		CloseStateFile (fp);
		return (GetHeadLink (&GLOBAL (ip_group_q)) != 0);
	}
	
	if (!GH.GroupOffset[which_group])
	{
		/* Group not present */
		CloseStateFile (fp);
		return FALSE;
	}


	if (which_group == GROUP_LIST)
	{
		BYTE NumGroups;
		COUNT ShipsLeftInLEG;

		// XXX: Hack: First, save the state of last encountered group, if any.
		//   The assumption here is that we read the group list immediately
		//   after an IP encounter, and npc_built_ship_q contains whatever
		//   ships are left in the encountered group (can be none).
		ShipsLeftInLEG = CountLinks (&GLOBAL (npc_built_ship_q));
		
		SeekStateFile (fp, GH.GroupOffset[0], SEEK_SET);
		sread_8 (fp, &LastEncGroup);

		if (LastEncGroup)
		{
			// The following 'if' is needed because GROUP_LIST is only
			// ever read from RANDGRPINFO_FILE, but the LastEncGroup
			// may need to be updated in the DEFGRPINFO_FILE as well.
			// In that case, PutGroupInfo() will update the correct file.
			if (GLOBAL (BattleGroupRef))
				PutGroupInfo (GLOBAL (BattleGroupRef), LastEncGroup);
			else
				FlushGroupInfo (&GH, offset, LastEncGroup, fp);
		}
		ReinitQueue (&GLOBAL (npc_built_ship_q));

		/* Read group 0 into ip_group_q */
		ReinitQueue (&GLOBAL (ip_group_q));
		/* Need a seek because Put/Flush has moved the file ptr */
		SeekStateFile (fp, GH.GroupOffset[0] + 1, SEEK_SET);
		sread_8 (fp, &NumGroups);

		while (NumGroups--)
		{
			BYTE group_id;
			BYTE RaceType;
			HSHIPFRAG hGroup;
			IP_GROUP *GroupPtr;

			sread_8 (fp, &RaceType);

			hGroup = BuildGroup (&GLOBAL (ip_group_q), RaceType);
			GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
			ReadIpGroup (fp, GroupPtr);
			group_id = GroupPtr->group_id;

#ifdef DEBUG_GROUPS
			log_add (log_Debug, "G) type %u, loc %u<%d, %d>, task 0x%02x:%u",
					RaceType,
					GroupPtr->sys_loc,
					GroupPtr->loc.x,
					GroupPtr->loc.y,
					GroupPtr->task,
					GroupPtr->dest_loc);
#endif /* DEBUG_GROUPS */

			UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);

			if (group_id == LastEncGroup && !ShipsLeftInLEG)
			{
				/* No ships left in the last encountered group, remove it */
#ifdef DEBUG_GROUPS
				log_add (log_Debug, " -- REMOVING");
#endif /* DEBUG_GROUPS */
				RemoveQueue (&GLOBAL (ip_group_q), hGroup);
				FreeIpGroup (&GLOBAL (ip_group_q), hGroup);
			}
		}

		CloseStateFile (fp);
		return (GetHeadLink (&GLOBAL (ip_group_q)) != 0);
	}
	else
	{
		/* Read 'which_group' group into npc_built_ship_q */
		BYTE NumShips;

		// XXX: Hack: The assumption here is that we only read the makeup
		//   of a particular group when initializing an encounter, which
		//   makes this group 'last encountered'. Also the state of all
		//   groups is saved here. This may make working with savegames
		//   harder in the future, as special care will have to be taken
		//   when loading a game into an encounter.
		LastEncGroup = which_group;
		// The following 'if' is needed because GROUP_LIST is only
		// ever written to RANDGRPINFO_FILE, but the group we are reading
		// may be in the DEFGRPINFO_FILE as well.
		// In that case, PutGroupInfo() will update the correct file.
		// Always calling PutGroupInfo() here would also be acceptable now.
		if (offset != GROUPS_RANDOM)
			PutGroupInfo (GROUPS_RANDOM, GROUP_LIST);
		else
			FlushGroupInfo (&GH, GROUPS_RANDOM, GROUP_LIST, fp);
		ReinitQueue (&GLOBAL (ip_group_q));

		ReinitQueue (&GLOBAL (npc_built_ship_q));
		// skip RaceType
		SeekStateFile (fp, GH.GroupOffset[which_group] + 1, SEEK_SET);
		sread_8 (fp, &NumShips);

		while (NumShips--)
		{
			BYTE RaceType;
			HSHIPFRAG hStarShip;
			SHIP_FRAGMENT *FragPtr;

			sread_8 (fp, &RaceType);

			hStarShip = CloneShipFragment (RaceType,
					&GLOBAL (npc_built_ship_q), 0);

			FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			ReadShipFragment (fp, FragPtr);
			UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		}

		CloseStateFile (fp);
		return (GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0);
	}
}

DWORD
PutGroupInfo (DWORD offset, BYTE which_group)
{
	GAME_STATE_FILE *fp;
	GROUP_HEADER GH;

	if (offset != GROUPS_RANDOM && which_group != GROUP_LIST)
		fp = OpenStateFile (DEFGRPINFO_FILE, "r+b");
	else
		fp = OpenStateFile (RANDGRPINFO_FILE, "r+b");

	if (!fp)
		return offset;

	if (offset == GROUPS_ADD_NEW)
	{
		offset = LengthStateFile (fp);
		SeekStateFile (fp, offset, SEEK_SET);
		memset (&GH, 0, sizeof (GH));
		GH.star_index = (COUNT)~0;
		WriteGroupHeader (fp, &GH);
	}

	// XXX: This is a bit dangerous. The assumption here is that we are
	//   only called to write GROUP_LIST in the GROUPS_RANDOM context,
	//   which is true right now and in which case we would seek to 0 anyway.
	//   The latter also makes guarding the seek with
	//   'if (which_group != GROUP_LIST)' moot.
	if (which_group != GROUP_LIST)
	{
		SeekStateFile (fp, offset, SEEK_SET);
		if (which_group == GROUP_SAVE_IP)
		{
			LastEncGroup = 0;
			which_group = GROUP_LIST;
		}
	}
	ReadGroupHeader (fp, &GH);

#ifdef NEVER
	// XXX: this appears to be a remnant of a slightly different group info
	//   expiration mechanism. Nowadays, the 'defined' groups never expire,
	//   and the dead 'random' groups stay in the file with NumShips==0 until
	//   the entire 'random' group header expires.
	if (GetHeadLink (&GLOBAL (npc_built_ship_q)) || GH.GroupOffset[0] == 0)
#endif /* NEVER */
	{
		COUNT month_index, day_index, year_index;

		/* The groups in this system are good for the next 7 days */
		month_index = 0;
		day_index = 7;
		year_index = 0;
		ValidateEvent (RELATIVE_EVENT, &month_index, &day_index, &year_index);
		GH.day_index = (BYTE)day_index;
		GH.month_index = (BYTE)month_index;
		GH.year_index = year_index;
	}
	GH.star_index = CurStarDescPtr - star_array;

#ifdef DEBUG_GROUPS
	log_add (log_Debug, "PutGroupInfo(%lu): %u out of %u -- %u/%u/%u",
			offset, which_group, GH.NumGroups,
			GH.month_index, GH.day_index, GH.year_index);
#endif /* DEBUG_GROUPS */

	FlushGroupInfo (&GH, offset, which_group, fp);

	CloseStateFile (fp);

	return (offset);
}