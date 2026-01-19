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

#include "save.h"

#include "build.h"
#include "controls.h"
#include "starmap.h"
#include "encount.h"
#include "libs/file.h"
#include "gamestr.h"
#include "globdata.h"
#include "options.h"
#include "races.h"
#include "shipcont.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"
#include "util.h"
#include "hyper.h"
		// for SaveSisHyperState()
#include "planets/planets.h"
		// for SaveSolarSysLocation() and tests
#include "libs/inplib.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "colors.h"

// Status boolean. If for some insane reason you need to
// save games in different threads, you'll need to
// protect your calls to SaveGame with a mutex.

// It's arguably over-paranoid to check for error on
// every single write, but this preserves the older
// behavior.

static BOOLEAN io_ok = TRUE;

// This defines the order and the number of bits in which the game state
// properties are saved.
// Full dev comments preserved at the bottom of globdata.h
#define GAME_STATE_ARRAY_ENTRY(name, bits) { #name, bits },
const GameStateBitMap gameStateBitMap[] =
{
	// Core v0.8.0
	CORE_GAME_STATES (GAME_STATE_ARRAY_ENTRY)

	{ NULL, 1 }, // MegaMod v0.8.0.85
	REV1_GAME_STATES (GAME_STATE_ARRAY_ENTRY)

	{ NULL, 2 }, // MegaMod v0.8.1
	REV2_GAME_STATES (GAME_STATE_ARRAY_ENTRY)

	{ NULL, 3 }, // MegaMod v0.8.2
	REV3_GAME_STATES (GAME_STATE_ARRAY_ENTRY)

	{ NULL, 4 }, // MegaMod v0.8.3
	REV4_GAME_STATES (GAME_STATE_ARRAY_ENTRY)

	{ NULL, 5 }, // MegaMod v0.8.4
	REV5_GAME_STATES (GAME_STATE_ARRAY_ENTRY)

	{ NULL, 0 }
};
#undef GAME_STATE_ARRAY_ENTRY

// This describes the release version corresponding to each game state
// flag revision chunk.
const char* gameStateBitMapRevTag[] = {
	"Core UQM v0.8.0",
	"MegaMod v0.8.0.85",
	"MegaMod v0.8.1",
	"MegaMod v0.8.2",
	"MegaMod v0.8.3",
	"MegaMod v0.8.4"
};

// XXX: these should handle endian conversions later
static inline void
write_8 (void *fp, BYTE v)
{
	if (io_ok)
		if (WriteResFile (&v, 1, 1, fp) != 1)
			io_ok = FALSE;
}

static inline void
write_16 (void *fp, UWORD v)
{
	write_8 (fp, (BYTE)( v        & 0xff));
	write_8 (fp, (BYTE)((v >>  8) & 0xff));
}

static inline void
write_32 (void *fp, DWORD v)
{
	write_8 (fp, (BYTE)( v        & 0xff));
	write_8 (fp, (BYTE)((v >>  8) & 0xff));
	write_8 (fp, (BYTE)((v >> 16) & 0xff));
	write_8 (fp, (BYTE)((v >> 24) & 0xff));
}

static inline void
write_a8 (void *fp, const BYTE *ar, COUNT count)
{
	if (io_ok)
		if (WriteResFile (ar, 1, count, fp) != count)
			io_ok = FALSE;
}

static inline void
write_str (void *fp, const char *str, COUNT count)
{
	// no type conversion needed for strings
	write_a8 (fp, (const BYTE *)str, count);
}

static inline void
write_a16 (void *fp, const UWORD *ar, COUNT count)
{
	for ( ; count > 0; --count, ++ar)
	{
		if (!io_ok)
			break;
		write_16 (fp, *ar);
	}
}

static void
SaveShipQueue (uio_Stream *fh, QUEUE *pQueue, DWORD tag)
{
	COUNT num_links;
	HSHIPFRAG hStarShip;

	num_links = CountLinks (pQueue);
	if (num_links == 0)
		return;
	write_32 (fh, tag);
	write_32 (fh, num_links * 11);
			// Size of chunk: each entry is 11 bytes long.

	hStarShip = GetHeadLink (pQueue);
	while (num_links--)
	{
		HSHIPFRAG hNextShip;
		SHIP_FRAGMENT *FragPtr;
		COUNT Index;

		FragPtr = LockShipFrag (pQueue, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		Index = FragPtr->race_id;
		// Write the number identifying this ship type.
		// See races.h; look for the enum containing NUM_AVAILABLE_RACES.
		write_16 (fh, Index);

		// Write SHIP_FRAGMENT elements
		write_8  (fh, FragPtr->captains_name_index);
		write_8  (fh, FragPtr->race_id);
		write_8  (fh, FragPtr->index);
		write_16 (fh, FragPtr->crew_level);
		write_16 (fh, FragPtr->max_crew);
		write_8  (fh, FragPtr->energy_level);
		write_8  (fh, FragPtr->max_energy);

		UnlockShipFrag (pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

static void
SaveRaceQueue (uio_Stream *fh, QUEUE *pQueue)
{
	COUNT num_links;
	HFLEETINFO hFleet;

	num_links = CountLinks (pQueue);
	if (num_links == 0)
		return;
	write_32 (fh, RACE_Q_TAG);
	// Write chunk size: 30 bytes per entry
	write_32 (fh, num_links * 30);

	hFleet = GetHeadLink (pQueue);
	while (num_links--)
	{
		HFLEETINFO hNextFleet;
		FLEET_INFO *FleetPtr;
		COUNT Index;

		FleetPtr = LockFleetInfo (pQueue, hFleet);
		hNextFleet = _GetSuccLink (FleetPtr);

		Index = GetIndexFromStarShip (pQueue, hFleet);
		// The index is the position in the queue.
		write_16 (fh, Index);

		// Write FLEET_INFO elements
		write_16 (fh, FleetPtr->allied_state);
		write_8  (fh, FleetPtr->days_left);
		write_8  (fh, FleetPtr->growth_fract);
		write_16 (fh, FleetPtr->crew_level);
		write_16 (fh, FleetPtr->max_crew);
		write_8  (fh, FleetPtr->growth);
		write_8  (fh, FleetPtr->max_energy);
		write_16 (fh, FleetPtr->loc.x);
		write_16 (fh, FleetPtr->loc.y);

		write_16 (fh, FleetPtr->actual_strength);
		write_16 (fh, FleetPtr->known_strength);
		write_16 (fh, FleetPtr->known_loc.x);
		write_16 (fh, FleetPtr->known_loc.y);
		write_8  (fh, FleetPtr->growth_err_term);
		write_8  (fh, FleetPtr->func_index);
		write_16 (fh, FleetPtr->dest_loc.x);
		write_16 (fh, FleetPtr->dest_loc.y);

		UnlockFleetInfo (pQueue, hFleet);
		hFleet = hNextFleet;
	}
}

static void
SaveGroupQueue (uio_Stream *fh, QUEUE *pQueue)
{
	HIPGROUP hGroup, hNextGroup;
	COUNT num_links;

	num_links = CountLinks (pQueue);
	if (num_links == 0)
		return;
	write_32 (fh, IP_GRP_Q_TAG);
	write_32 (fh, num_links * 13); // 13 bytes per element right now

	for (hGroup = GetHeadLink (pQueue); hGroup; hGroup = hNextGroup)
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (pQueue, hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);

		write_16 (fh, GroupPtr->group_counter);
		write_8  (fh, GroupPtr->race_id);
		write_8  (fh, GroupPtr->sys_loc);
		write_8  (fh, GroupPtr->task);
		write_8  (fh, GroupPtr->in_system); /* was crew_level */
		write_8  (fh, GroupPtr->dest_loc);
		write_8  (fh, GroupPtr->orbit_pos);
		write_8  (fh, GroupPtr->group_id); /* was max_energy */
		write_16 (fh, GroupPtr->loc.x);
		write_16 (fh, GroupPtr->loc.y);

		UnlockIpGroup (pQueue, hGroup);
	}
}

static void
SaveEncounters (uio_Stream *fh)
{
	COUNT num_links;
	HENCOUNTER hEncounter;
	num_links = CountLinks (&GLOBAL (encounter_q));
	if (num_links == 0)
		return;
	write_32 (fh, ENCOUNTERS_TAG);
	write_32 (fh, 65 * num_links);

	hEncounter = GetHeadLink (&GLOBAL (encounter_q));
	while (num_links--)
	{
		HENCOUNTER hNextEncounter;
		ENCOUNTER *EncounterPtr;
		COUNT i;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);

		write_16  (fh, EncounterPtr->transition_state);
		write_16  (fh, EncounterPtr->origin.x);
		write_16  (fh, EncounterPtr->origin.y);
		write_16  (fh, EncounterPtr->radius);
		// former STAR_DESC fields
		write_16  (fh, EncounterPtr->loc_pt.x);
		write_16  (fh, EncounterPtr->loc_pt.y);
		write_8   (fh, EncounterPtr->race_id);
		write_8   (fh, EncounterPtr->num_ships);
		write_8   (fh, EncounterPtr->flags);

		// Save each entry in the BRIEF_SHIP_INFO array
		for (i = 0; i < MAX_HYPER_SHIPS; i++)
		{
			const BRIEF_SHIP_INFO *ShipInfo = &EncounterPtr->ShipList[i];

			write_8   (fh, ShipInfo->race_id);
			write_16  (fh, ShipInfo->crew_level);
			write_16  (fh, ShipInfo->max_crew);
			write_8   (fh, ShipInfo->max_energy);
		}

		// Save the stuff after the BRIEF_SHIP_INFO array
		write_32  (fh, RES_DESCALE (EncounterPtr->log_x));
		write_32  (fh, RES_DESCALE (EncounterPtr->log_y));

		UnlockEncounter (hEncounter);
		hEncounter = hNextEncounter;
	}
}

static void
SaveEvents (uio_Stream *fh)
{
	COUNT num_links;
	HEVENT hEvent;
	num_links = CountLinks (&GLOBAL (GameClock.event_q));
	if (num_links == 0)
		return;
	write_32 (fh, EVENTS_TAG);
	write_32 (fh, num_links * 5); /* Event chunks are five bytes each */

	hEvent = GetHeadLink (&GLOBAL (GameClock.event_q));
	while (num_links--)
	{
		HEVENT hNextEvent;
		EVENT *EventPtr;

		LockEvent (hEvent, &EventPtr);
		hNextEvent = GetSuccEvent (EventPtr);

		write_8   (fh, EventPtr->day_index);
		write_8   (fh, EventPtr->month_index);
		write_16  (fh, EventPtr->year_index);
		write_8   (fh, EventPtr->func_index);

		UnlockEvent (hEvent);
		hEvent = hNextEvent;
	}
}

/* The clock state is folded in with the game state chunk. */
static void
SaveClockState (const CLOCK_STATE *ClockPtr, uio_Stream *fh)
{
	write_8   (fh, ClockPtr->day_index);
	write_8   (fh, ClockPtr->month_index);
	write_16  (fh, ClockPtr->year_index);
	write_16  (fh, ClockPtr->tick_count);
	write_16  (fh, ClockPtr->day_in_ticks);
}

/* Save out the game state chunks. There are two of these; the Global
 * State chunk is fixed size, but the Game State tag can be extended
 * by modders. */
static BOOLEAN
SaveGameState (const GAME_STATE *GSPtr, uio_Stream *fh)
{
	BYTE res_scale;

	if (LOBYTE (GSPtr->CurrentActivity) != IN_INTERPLANETARY)
		res_scale = RESOLUTION_FACTOR;
	else
		res_scale = 0;

	write_32  (fh, GLOBAL_STATE_TAG);
	write_32  (fh, 75);
	write_8   (fh, GSPtr->glob_flags);
	write_8   (fh, GSPtr->CrewCost);
	write_8   (fh, GSPtr->FuelCost);
	write_a8  (fh, GSPtr->ModuleCost, NUM_MODULES);
	write_a8  (fh, GSPtr->ElementWorth, NUM_ELEMENT_CATEGORIES);
	write_16  (fh, GSPtr->CurrentActivity);

	SaveClockState (&GSPtr->GameClock, fh);

	write_16  (fh, GSPtr->autopilot.x);
	write_16  (fh, GSPtr->autopilot.y);
	write_16  (fh, GSPtr->ip_location.x);
	write_16  (fh, GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	write_16  (fh, RES_DESCALE (GSPtr->ShipStamp.origin.x));
	write_16  (fh, RES_DESCALE (GSPtr->ShipStamp.origin.y));
	write_16  (fh, GSPtr->ShipFacing);
	write_8   (fh, GSPtr->ip_planet);
	write_8   (fh, GSPtr->in_orbit);

	/* VELOCITY_DESC velocity */
	write_16  (fh, GSPtr->velocity.TravelAngle >> res_scale);
	write_16  (fh, GSPtr->velocity.vector.width >> res_scale);
	write_16  (fh, GSPtr->velocity.vector.height >> res_scale);
	write_16  (fh, GSPtr->velocity.fract.width >> res_scale);
	write_16  (fh, GSPtr->velocity.fract.height >> res_scale);
	write_16  (fh, GSPtr->velocity.error.width >> res_scale);
	write_16  (fh, GSPtr->velocity.error.height >> res_scale);
	write_16  (fh, GSPtr->velocity.incr.width >> res_scale);
	write_16  (fh, GSPtr->velocity.incr.height >> res_scale);

	/* The Game state bits. Vanilla UQM uses 155 bytes here at
	 * present. Only the first 99 bytes are significant, though;
	 * the rest will be overwritten by the BtGp chunks. */
	write_32  (fh, GAME_STATE_TAG);
	{
		uint8 *buf = NULL;
		size_t bufSize;
		if (serialiseGameState (gameStateBitMap, &buf, &bufSize))
		{
			write_32  (fh, bufSize);
			write_a8  (fh, buf, (COUNT)bufSize);
			HFree(buf);
		}
		else
			return FALSE;
	}
	return TRUE;
}

/* This is folded into the Summary chunk */
static void
SaveSisState (const SIS_STATE *SSPtr, void *fp)
{
	write_32  (fp, RES_DESCALE (SSPtr->log_x));
	write_32  (fp, RES_DESCALE (SSPtr->log_y));
	write_32  (fp, SSPtr->ResUnits);
	write_32  (fp, SSPtr->FuelOnBoard);
	write_16  (fp, SSPtr->CrewEnlisted);
	write_16  (fp, SSPtr->TotalElementMass);
	write_16  (fp, SSPtr->TotalBioMass);
	write_a8  (fp, SSPtr->ModuleSlots, NUM_MODULE_SLOTS);
	write_a8  (fp, SSPtr->DriveSlots, NUM_DRIVE_SLOTS);
	write_a8  (fp, SSPtr->JetSlots, NUM_JET_SLOTS);
	write_8   (fp, SSPtr->NumLanders);
	write_a16 (fp, SSPtr->ElementAmounts, NUM_ELEMENT_CATEGORIES);

	write_str (fp, SSPtr->ShipName, SIS_NAME_SIZE);
	write_str (fp, SSPtr->CommanderName, SIS_NAME_SIZE);
	write_str (fp, SSPtr->PlanetName, SIS_NAME_SIZE);
	write_8   (fp, SSPtr->Difficulty);
	write_8   (fp, SSPtr->Extended);
	write_8   (fp, SSPtr->Nomad);
	write_32  (fp, SSPtr->Seed);
	write_8   (fp, SSPtr->ShipSeed);
}

/* Write out the Summary Chunk. This is variable length because of the
   savegame name */
static void
SaveSummary (const SUMMARY_DESC *SummPtr, void *fp)
{
	write_32 (fp, SUMMARY_TAG);
	write_32 (fp, 160 + strlen (SummPtr->SaveName));
	SaveSisState (&SummPtr->SS, fp);

	write_8  (fp, SummPtr->Activity);
	write_8  (fp, SummPtr->Flags);
	write_8  (fp, SummPtr->day_index);
	write_8  (fp, SummPtr->month_index);
	write_16 (fp, SummPtr->year_index);
	write_8  (fp, SummPtr->MCreditLo);
	write_8  (fp, SummPtr->MCreditHi);
	write_8  (fp, SummPtr->NumShips);
	write_8  (fp, SummPtr->NumDevices);
	write_a8 (fp, SummPtr->ShipList, MAX_BUILT_SHIPS);
	write_a8 (fp, SummPtr->DeviceList, MAX_EXCLUSIVE_DEVICES);
	write_8  (fp, SummPtr->res_factor);
	write_a8 (fp, (BYTE *) SummPtr->SaveName, (COUNT)strlen (SummPtr->SaveName));
}

/* Save the Star Description chunk. This is not to be confused with
 * the Star *Info* chunk, which records which planetary features you
 * have exploited with your lander */
static void
SaveStarDesc (const STAR_DESC *SDPtr, uio_Stream *fh)
{
	write_32 (fh, STAR_TAG);
	write_32 (fh, 8);
	write_16 (fh, SDPtr->star_pt.x);
	write_16 (fh, SDPtr->star_pt.y);
	write_8  (fh, SDPtr->Type);
	write_8  (fh, SDPtr->Index);
	write_8  (fh, SDPtr->Prefix);
	write_8  (fh, SDPtr->Postfix);
}

static void
PrepareSummary (SUMMARY_DESC *SummPtr, const char *name)
{
	SummPtr->SS = GlobData.SIS_state;

	SummPtr->Activity = LOBYTE (GLOBAL (CurrentActivity));
	switch (SummPtr->Activity)
	{
		case IN_HYPERSPACE:
			if (inQuasiSpace ())
				SummPtr->Activity = IN_QUASISPACE;
			break;
		case IN_INTERPLANETARY:
			// Get a better planet name for summary
			GetPlanetOrMoonName (SummPtr->SS.PlanetName,
					sizeof (SummPtr->SS.PlanetName));
			if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0)
				SummPtr->Activity = IN_STARBASE;
			else if (playerInPlanetOrbit ())
				SummPtr->Activity = IN_PLANET_ORBIT;
			break;
		case IN_LAST_BATTLE:
			utf8StringCopy (SummPtr->SS.PlanetName,
					sizeof (SummPtr->SS.PlanetName),
					GAME_STRING (PLANET_NUMBER_BASE + 32)); // Sa-Matra
			break;
	}

	SummPtr->MCreditLo = GET_GAME_STATE (MELNORME_CREDIT0);
	SummPtr->MCreditHi = GET_GAME_STATE (MELNORME_CREDIT1);

	{
		HSHIPFRAG hStarShip, hNextShip;

		for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)),
				SummPtr->NumShips = 0; hStarShip; hStarShip = hNextShip,
				++SummPtr->NumShips)
		{
			SHIP_FRAGMENT *StarShipPtr;

			StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
			hNextShip = _GetSuccLink (StarShipPtr);
			SummPtr->ShipList[SummPtr->NumShips] = StarShipPtr->race_id;
			UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		}
	}

	SummPtr->NumDevices = InventoryDevices (SummPtr->DeviceList,
			MAX_EXCLUSIVE_DEVICES);

	SummPtr->Flags = GET_GAME_STATE (LANDER_SHIELDS)
			| (GET_GAME_STATE (IMPROVED_LANDER_SPEED) << (4 + 0))
			| (GET_GAME_STATE (IMPROVED_LANDER_CARGO) << (4 + 1))
			| (GET_GAME_STATE (IMPROVED_LANDER_SHOT) << (4 + 2))
			| ((GET_GAME_STATE (CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	SummPtr->day_index = GLOBAL (GameClock.day_index);
	SummPtr->month_index = GLOBAL (GameClock.month_index);
	SummPtr->year_index = GLOBAL (GameClock.year_index);
	SummPtr->SaveName[SAVE_NAME_SIZE-1] = 0;
	strncpy (SummPtr->SaveName, name, SAVE_NAME_SIZE-1);
	SummPtr->res_factor = RESOLUTION_FACTOR;
}

static void
SaveProblemMessage (STAMP *MsgStamp)
{
#define MAX_MSG_LINES 1
	RECT r = {{0, 0}, {0, 0}};
	COUNT i;
	TEXT t;
	UNICODE *ppStr[MAX_MSG_LINES];

	// TODO: This should probably just use DoPopupWindow()

	ppStr[0] = GAME_STRING (SAVEGAME_STRING_BASE + 2);

	SetContextFont (StarConFont);

	t.baseline.x = t.baseline.y = 0;
	t.align = ALIGN_CENTER;
	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		RECT tr;

		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		TextRect (&t, &tr, NULL);
		if (i == 0)
			r = tr;
		else
			BoxUnion (&tr, &r, &r);
		t.baseline.y += 11;
	}
	t.baseline.x = (RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1)
			- (r.extent.width >> 1)) - r.corner.x;
	t.baseline.y = (RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1)
			- (r.extent.height >> 1)) - r.corner.y;
	r.corner.x += t.baseline.x - RES_SCALE (4);
	r.corner.y += t.baseline.y - RES_SCALE (4);
	r.extent.width += RES_SCALE (8);
	r.extent.height += RES_SCALE (8);

	*MsgStamp = SaveContextFrame (&r);

	BatchGraphics ();
	DrawStarConBox (&r, RES_SCALE (2), SHADOWBOX_MEDIUM_COLOR,
			SHADOWBOX_DARK_COLOR, TRUE, DKGRAY_COLOR, TRUE, TRANSPARENT);
	SetContextForeGroundColor (
			isPC (optWhichFonts) ? WHITE_COLOR : LTGRAY_COLOR);

	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += 11;
	}
	UnbatchGraphics ();
}

void
SaveProblem (void)
{
	STAMP s;
	CONTEXT OldContext;

	OldContext = SetContext (SpaceContext);
	SaveProblemMessage (&s);
	FlushGraphics ();

	WaitForAnyButton (TRUE, WAIT_INFINITE, FALSE);

	// Restore the screen under the message
	DrawStamp (&s);
	SetContext (OldContext);
	DestroyDrawable (ReleaseDrawable (s.frame));
}

static void
SaveFlagshipState (void)
{
	if (inHQSpace ())
	{
		// Player is in HyperSpace or QuasiSpace.
		SaveSisHyperState ();
	}
	else if (playerInSolarSystem ())
	{
		SaveSolarSysLocation ();
	}
}

static void
SaveStarInfo (uio_Stream *fh)
{
	GAME_STATE_FILE *fp;
	fp = OpenStateFile (STARINFO_FILE, "rb");
	if (fp)
	{
		DWORD flen = LengthStateFile (fp);
		if (flen % 4)
		{
			log_add (log_Warning, "Unexpected Star Info length! Expected "
					"an integral number of DWORDS.\n");
		}
		else
		{
			write_32 (fh, SCAN_TAG);
			write_32 (fh, flen);
			while (flen)
			{
				DWORD val;
				sread_32 (fp, &val);
				write_32 (fh, val);
				flen -= 4;
			}
		}
		CloseStateFile (fp);
	}
}

static void
SaveBattleGroup (GAME_STATE_FILE *fp, DWORD encounter_id, DWORD grpoffs,
		uio_Stream *fh)
{
	GROUP_HEADER h;
	DWORD size = 12;
	int i;
	SeekStateFile (fp, grpoffs, SEEK_SET);
	ReadGroupHeader (fp, &h);
	for (i = 1; i <= h.NumGroups; ++i)
	{
		BYTE NumShips;
		SeekStateFile (fp, h.GroupOffset[i], SEEK_SET);
		sread_8 (fp, NULL);
		sread_8 (fp, &NumShips);
		size += 2 + 10 * NumShips;
	}
	write_32 (fh, BATTLE_GROUP_TAG);
	write_32 (fh, size);
	write_32 (fh, encounter_id);
	write_8  (fh,
			(grpoffs && (GLOBAL (BattleGroupRef) == grpoffs)) ? 1 : 0);
	write_16 (fh, h.star_index);
	write_8  (fh, h.day_index);
	write_8  (fh, h.month_index);
	write_16 (fh, h.year_index);
	write_8  (fh, h.NumGroups);
	for (i = 1; i <= h.NumGroups; ++i)
	{
		int j;
		BYTE b;
		SeekStateFile (fp, h.GroupOffset[i], SEEK_SET);
		sread_8 (fp, &b); // Group race icon
		write_8 (fh, b);
		sread_8 (fp, &b); // NumShips
		write_8 (fh, b);
		for (j = 0; j < b; ++j)
		{
			BYTE race_outer;
			SHIP_FRAGMENT sf;
			sread_8 (fp, &race_outer);
			ReadShipFragment (fp, &sf);
			write_8  (fh, race_outer);
			write_8  (fh, sf.captains_name_index);
			write_8  (fh, sf.race_id);
			write_8  (fh, sf.index);
			write_16 (fh, sf.crew_level);
			write_16 (fh, sf.max_crew);
			write_8  (fh, sf.energy_level);
			write_8  (fh, sf.max_energy);
		}
	}
}

static DWORD
GetBattleGroupOffset (int encounterIndex)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because GET_GAME_STATE is a #define, which stringizes
	// its argument.
	switch (encounterIndex)
	{
		case  1: return GET_GAME_STATE (SHOFIXTI_GRPOFFS);
		case  2: return GET_GAME_STATE (ZOQFOT_GRPOFFS);
		case  3: return GET_GAME_STATE (MELNORME0_GRPOFFS);
		case  4: return GET_GAME_STATE (MELNORME1_GRPOFFS);
		case  5: return GET_GAME_STATE (MELNORME2_GRPOFFS);
		case  6: return GET_GAME_STATE (MELNORME3_GRPOFFS);
		case  7: return GET_GAME_STATE (MELNORME4_GRPOFFS);
		case  8: return GET_GAME_STATE (MELNORME5_GRPOFFS);
		case  9: return GET_GAME_STATE (MELNORME6_GRPOFFS);
		case 10: return GET_GAME_STATE (MELNORME7_GRPOFFS);
		case 11: return GET_GAME_STATE (MELNORME8_GRPOFFS);
		case 12: return GET_GAME_STATE (URQUAN_PROBE_GRPOFFS);
		case 13: return GET_GAME_STATE (COLONY_GRPOFFS);
		case 14: return GET_GAME_STATE (SAMATRA_GRPOFFS);
		default:
			log_add (log_Warning, "SetBattleGroupOffset: invalid encounter"
					" index.\n");
			return 0;
	}
}

static void
SaveGroups (uio_Stream *fh)
{
	GAME_STATE_FILE *fp;
	fp = OpenStateFile (RANDGRPINFO_FILE, "rb");
	if (fp && LengthStateFile (fp) > 0)
	{
		GROUP_HEADER h;
		BYTE lastenc, count;
		int i;
		ReadGroupHeader (fp, &h);
		/* Group List */
		SeekStateFile (fp, h.GroupOffset[0], SEEK_SET);
		sread_8 (fp, &lastenc);
		sread_8 (fp, &count);
		write_32 (fh, GROUP_LIST_TAG);
		write_32 (fh, 1 + 14 * count); // Chunk size
		write_8 (fh, lastenc);
		for (i = 0; i < count; ++i)
		{
			BYTE race_outer;
			IP_GROUP ip;
			sread_8 (fp, &race_outer);
			ReadIpGroup (fp, &ip);

			write_8  (fh, race_outer);
			write_16 (fh, ip.group_counter);
			write_8  (fh, ip.race_id);
			write_8  (fh, ip.sys_loc);
			write_8  (fh, ip.task);
			write_8  (fh, ip.in_system);
			write_8  (fh, ip.dest_loc);
			write_8  (fh, ip.orbit_pos);
			write_8  (fh, ip.group_id);
			write_16 (fh, ip.loc.x);
			write_16 (fh, ip.loc.y);
		}
		SaveBattleGroup (fp, 0, 0, fh);
		CloseStateFile (fp);
	}
	fp = OpenStateFile (DEFGRPINFO_FILE, "rb");
	if (fp && LengthStateFile (fp) > 0)
	{
		int encounter_index;
		for (encounter_index = 1; encounter_index < 15; encounter_index++)
		{
			DWORD grpoffs = GetBattleGroupOffset (encounter_index);
			if (grpoffs)
			{
				SaveBattleGroup (fp, encounter_index, grpoffs, fh);
			}
		}
		CloseStateFile (fp);
	}
}

// This function first writes to a memory file, and then writes the whole
// lot to the actual save file at once.
BOOLEAN
SaveGame (COUNT which_game, SUMMARY_DESC *SummPtr, const char *name)
{
	uio_Stream *out_fp;
	POINT pt;
	STAR_DESC SD;
	char file[PATH_MAX];
	if (CurStarDescPtr)
		SD = *CurStarDescPtr;
	else
		memset (&SD, 0, sizeof (SD));

	// XXX: Backup: SaveFlagshipState() overwrites ip_location
	pt = GLOBAL (ip_location);
	SaveFlagshipState ();
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& !(GLOBAL (CurrentActivity)
			& (START_ENCOUNTER | START_INTERPLANETARY)))
		PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);

	// Write the memory file to the actual savegame file.
	sprintf (file, "uqmsave.%02u", which_game);
	if ((out_fp = res_OpenResFile (saveDir, file, "wb")))
	{
		io_ok = TRUE;
		write_32 (out_fp, MMV4_TAG);

		PrepareSummary (SummPtr, name);
		SaveSummary (SummPtr, out_fp);

		if (!SaveGameState (&GlobData.Game_state, out_fp))
			io_ok = FALSE;

		// XXX: Restore
		GLOBAL (ip_location) = pt;
		// Only relevant when loading a game and must be cleaned
		GLOBAL (in_orbit) = 0;

		SaveRaceQueue (out_fp, &GLOBAL (avail_race_q));
		// START_INTERPLANETARY is only set when saving from Homeworld
		//   encounter screen. When the game is loaded, the
		//   GenerateOrbitalFunction for the current star system
		//   create the encounter anew and populate the npc queue.
		if (!(GLOBAL (CurrentActivity) & START_INTERPLANETARY))
		{
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
				SaveShipQueue (out_fp, &GLOBAL (npc_built_ship_q),
						NPC_SHIP_Q_TAG);
			else if (LOBYTE (GLOBAL (CurrentActivity))
					== IN_INTERPLANETARY)
				// XXX: Technically, this queue does not need to be
				//   saved/loaded at all. IP groups will be reloaded
				//   from group state files. But the original code did,
				//   and so will we until we can prove we do not need to.
				SaveGroupQueue (out_fp, &GLOBAL (ip_group_q));
		}
		SaveShipQueue (out_fp, &GLOBAL (built_ship_q), SHIP_Q_TAG);
		SaveShipQueue (out_fp, &GLOBAL (stowed_ship_q), STOWED_Q_TAG);

		// Save the game event chunk
		SaveEvents (out_fp);

		// Save the encounter chunk (black globes in HS/QS)
		SaveEncounters (out_fp);

		// Save out the data that used to be in state files
		SaveStarInfo (out_fp);
		SaveGroups (out_fp);

		// Save out the Star Descriptor
		SaveStarDesc (&SD, out_fp);
		
		res_CloseResFile (out_fp);
		if (!io_ok)
		{
			DeleteResFile(saveDir, file);
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}
