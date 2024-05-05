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

#include "globdata.h"

#include "coderes.h"
#include "encount.h"
#include "starmap.h"
#include "master.h"
#include "setup.h"
#include "units.h"
#include "hyper.h"
#include "resinst.h"
#include "nameref.h"
#include "build.h"
#include "state.h"
#include "grpinfo.h"
#include "gamestr.h"
#include "libs/scriptlib.h"
#include "libs/log.h"
#include "options.h"
#include <assert.h>
#include <stdlib.h>
#include "uqmdebug.h"

#include <time.h>//required to use 'srand(time(NULL))'

#define OOPS_ALL 0 //MELNORME1_DEFINED
static void CreateRadar (void);

CONTEXT RadarContext;
FRAME PlayFrame;

GLOBDATA GlobData;


// Pre: 0 <= bits <= 32
// This function is necessary because expressions such as '(1 << bits) - 1'
// or '~(~0 << bits)' may shift by 32 bits, which is undefined (for 32 bits
// integers). This is not a hypothetical issue; 'uint8_t numBits = 32;
// printf("%u\n", (1 << numBits));' will return 1 on x86 when compiled with
// gcc (4.4.3).
static inline DWORD
bitmask32 (BYTE bits)
{
	return (bits >= 32) ? 0xffffffff : ((1U << bits) - 1);
}

// Pre: 0 <= bits <= 32
// This function is necessary because shifting by 32 bits is undefined (for
// 32 bits integers). This is not a hypothetical issue; 'uint8_t numBits =
// 32; printf("%u\n", (1 << numBits));' will return 1 on x86 when compiled
// with gcc (4.4.3).
static inline DWORD
shl32 (DWORD value, BYTE shift)
{
	return (shift >= 32) ? 0 : (value << shift);
}

// Returns the total number of bits which are needed to store a game state
// according to 'bm'.
size_t
totalBitsForGameState (const GameStateBitMap *bm, int rev)
{
	size_t totalBits = 0;
	const GameStateBitMap *bmPtr;

	for (bmPtr = bm; bmPtr; bmPtr++)
	{
		if (bmPtr->name != NULL)
			totalBits += bmPtr->numBits;
		else if (bmPtr->numBits == 0)
			break;
		else if (rev >= 0 && bmPtr->numBits > rev)
			break;
	}

	return totalBits;
}

// Returns the game state revision number that requires the specified
// number of 'bytes' according to 'bm', or -1 if no match
int
getGameStateRevByBytes (const GameStateBitMap *bm, int bytes)
{
	int rev = 0;
	size_t totalBits = 0;
	const GameStateBitMap *bmPtr;

	for (bmPtr = bm; bmPtr; bmPtr++)
	{
		if (bmPtr->name != NULL)
			totalBits += bmPtr->numBits;
		else if (bmPtr->numBits == 0)
			break;
		else if ((int)((totalBits + 7) >> 3) >= bytes)
			break;
		else
			rev = bmPtr->numBits;
	}

	if ((int)((totalBits + 7) >> 3) != bytes)
		rev = -1;

	return rev;
}

// Write 'valueBitCount' bits from 'value' into the buffer pointed to
// by '*bufPtrPtr'.
// '*restBitsPtr' is used to store the bits in which do not make up
// a byte yet. The number of bits stored is kept in '*restBitCount'.
static inline void
serialiseBits (BYTE **bufPtrPtr, DWORD *restBitsPtr, size_t *restBitCount,
		BYTE value, size_t valueBitCount)
{
	BYTE valueBitMask;

	assert (*restBitCount < 8);
	assert (valueBitCount <= 8);

	valueBitMask = (1 << valueBitCount) - 1;

	// Add the bits from 'value' to the working 'buffer' (*restBits).
	*restBitsPtr |= (value & valueBitMask) << *restBitCount;
	*restBitCount += valueBitCount;

	// Write out restBits (possibly partialy), if we have enough bits to
	// make a byte.
	if (*restBitCount >= 8)
	{
		**bufPtrPtr = *restBitsPtr & 0xff;
		*restBitsPtr >>= 8;
		(*bufPtrPtr)++;
		*restBitCount -= 8;
	}
}

// Serialise the current game state into a newly allocated buffer,
// according to the GameStateBitMap 'bm'.
// Only the (integer) values from 'bm' are saved, in the specified order.
// This function fills in '*buf' with the newly allocated buffer, and
// '*numBytes' with its size. The caller becomes the owner of '*buf' and
// is responsible for freeing it.
BOOLEAN
serialiseGameState (const GameStateBitMap *bm, BYTE **buf,
		size_t *numBytes)
{
	size_t totalBits;
	size_t totalBytes;
	const GameStateBitMap *bmPtr;
	BYTE *result;
	BYTE *bufPtr;

	DWORD restBits = 0;
			// Bits which have not yet been stored because they did not
			// form an entire byte.
	size_t restBitCount = 0;

	// Determine the total number of bits/bytes required.
	totalBits = totalBitsForGameState (bm, -1);
	totalBytes = (totalBits + 7) / 8;

	// Allocate memory for the serialised data.
	result = HMalloc (totalBytes);
	if (result == NULL)
		return FALSE;

	bufPtr = result;
	for (bmPtr = bm; bmPtr; bmPtr++)
	{
		if (bmPtr->name != NULL)
		{
			DWORD value = getGameStateUint (bmPtr->name);
			BYTE numBits = bmPtr->numBits;

#ifdef STATE_DEBUG
			log_add (log_Debug, "Saving: GameState[\'%s\'] = %u",
					bmPtr->name, value);
#endif  /* STATE_DEBUG */

			if (value > bitmask32(numBits))
			{
				log_add (log_Error, "Warning: serialiseGameState(): the "
						"value of the property '%s' (%u) does not fit in "
						"the reserved number of bits (%d).",
						bmPtr->name, value, numBits);
			}

			// Store multi-byte values with the least significant byte 1st.
			while (numBits >= 8)
			{
				serialiseBits (&bufPtr, &restBits, &restBitCount,
						value & 0xff, 8);
				value >>= 8;
				numBits -= 8;
			}
			if (numBits > 0)
				serialiseBits (&bufPtr, &restBits, &restBitCount, value,
						numBits);
		}
		else if (bmPtr->numBits == 0)
			break;
	}

	// Pad the end up to a byte.
	if (restBitCount > 0)
		serialiseBits (&bufPtr, &restBits, &restBitCount, 0,
				8 - restBitCount);

	*buf = result;
	*numBytes = totalBytes;
	return TRUE;
}

// Read 'numBits' bits from '*bytePtr', starting at the bit offset
// '*bitPtr'. The result is returned.
// '*bitPtr' and '*bytePtr' are updated by this function.
static inline DWORD
deserialiseBits (const BYTE **bytePtr, BYTE *bitPtr, size_t numBits)
{
	assert (*bitPtr < 8);
	assert (numBits <= 8);

	if (numBits <= (size_t) (8 - *bitPtr))
	{
		// Can get the entire value from one byte.
		// We want bits *bitPtr through (excluding) *bitPtr+numBits
		DWORD result =
				((*bytePtr)[0] >> *bitPtr) & bitmask32((BYTE)numBits);

		// Update the pointers.
		if (numBits == (size_t) (8 - *bitPtr))
		{
			// The entire (rest of the) byte is read. Go to the next byte.
			(*bytePtr)++;
			*bitPtr = 0;
		}
		else
		{
			// There are still unread bits in the byte.
			*bitPtr += (BYTE)numBits;
		}
		return result;
	}
	else
	{
		// The result comes from two bytes.
		// We get the *bitPtr most significant bits from [0], as the least
		// significant bits of the result, and the (numBits - *bitPtr)
		// least significant bits from [1], as the most significant bits of
		// the result.
		DWORD result = (((*bytePtr)[0] >> *bitPtr)
				| ((*bytePtr)[1] << (8 - *bitPtr))) &
				bitmask32((BYTE)numBits);
		(*bytePtr)++;
		*bitPtr += (BYTE)numBits - 8;
		return result;
	}
}

// Deserialise the current game state from the bit array in 'buf', which
// has size 'numBytes', according to the GameStateBitMap 'bm'.
BOOLEAN
deserialiseGameState (const GameStateBitMap *bm,
		const BYTE *buf, size_t numBytes, int rev)
{
	size_t totalBits;
	const GameStateBitMap *bmPtr;

	const BYTE *bytePtr = buf;
	BYTE bitPtr = 0;
			// Number of bits already processed from the byte pointed at by
			// bytePtr.
	BOOLEAN matchRev = TRUE;

	// Sanity check: determine the number of bits required, and check
	// whether 'numBytes' is large enough.
	totalBits = totalBitsForGameState (bm, rev);
	if (numBytes * 8 < totalBits)
	{
		log_add (log_Error, "Warning: deserialiseGameState(): Corrupt "
				"save game: state: less bytes available than expected.");
		return FALSE;
	}

	for (bmPtr = bm; bmPtr; bmPtr++)
	{
		if (bmPtr->name != NULL)
		{
			DWORD value = 0;
			BYTE numBits = bmPtr->numBits;
			BYTE bitsLeft = numBits;

			if (matchRev)
			{
				// Multi-byte values are stored with the least significant
				// byte first.
				while (bitsLeft >= 8)
				{
					DWORD bits = deserialiseBits (&bytePtr, &bitPtr, 8);
					value |= shl32 (bits, numBits - bitsLeft);
					bitsLeft -= 8;
				}
				if (bitsLeft > 0)
				{
					value |= shl32 (
							deserialiseBits (&bytePtr, &bitPtr, bitsLeft),
							numBits - bitsLeft);
				}
	
#ifdef STATE_DEBUG
				log_add (log_Debug, "Loading: GameState[\'%s\'] = %u",
						bmPtr->name, value);
#endif  /* STATE_DEBUG */
			}

			setGameStateUint (bmPtr->name, value);
		}
		else if (bmPtr->numBits == 0)
			break;
		else if (bmPtr->numBits > rev)
			matchRev = FALSE;
	}
#ifdef STATE_DEBUG
	fflush (stderr);
#endif  /* STATE_DEBUG */

	return TRUE;
}

static void
CreateRadar (void)
{
	if (RadarContext == 0)
	{
		RECT r;
		CONTEXT OldContext;

		RadarContext = CreateContext ("RadarContext");
		OldContext = SetContext (RadarContext);
		SetContextFGFrame (Screen);
		r.corner.x = RADAR_X;
		r.corner.y = RADAR_Y;
		r.extent.width = RADAR_WIDTH;
		r.extent.height = RADAR_HEIGHT;
		SetContextClipRect (&r);
		SetContext (OldContext);
	}
}

BOOLEAN
LoadSC2Data (void)
{
	if (FlagStatFrame == 0)
	{
		FlagStatFrame = CaptureDrawable (
				LoadGraphic (FLAGSTAT_MASK_PMAP_ANIM));
		if (FlagStatFrame == NULL)
			return FALSE;

		MiscDataFrame = CaptureDrawable (
				LoadGraphic (MISCDATA_MASK_PMAP_ANIM));
		if (MiscDataFrame == NULL)
			return FALSE;

		visitedStarsFrame = CaptureDrawable (
				LoadGraphic (VISITED_STARS_ANIM));
		if (visitedStarsFrame == NULL)
			return FALSE;

		FontGradFrame = CaptureDrawable (
				LoadGraphic (FONTGRAD_PMAP_ANIM));
	}

	CreateRadar ();

	if (inHQSpace())
	{
		GLOBAL (ShipStamp.origin.x) =
				GLOBAL (ShipStamp.origin.y) = -1;
	}

	return TRUE;
}

static void
copyFleetInfo (FLEET_INFO *dst, SHIP_INFO *src, FLEET_STUFF *fleet)
{
	// other leading fields are irrelevant
	dst->crew_level = src->crew_level;
	dst->max_crew = src->max_crew;
	dst->max_energy = src->max_energy;

	dst->shipIdStr = src->idStr;
	dst->race_strings = src->race_strings;
	dst->icons = src->icons;
	dst->melee_icon = src->melee_icon;

	dst->actual_strength = fleet->strength;
	dst->known_loc = fleet->known_loc;
}

BOOLEAN
InitGameStructures (void)
{
	COUNT i;

	InitGlobData ();
	// Set Seed Type, then check/start StarSeed
	SET_GAME_STATE (SEED_TYPE, optSeedType);
	GLOBAL_SIS (Seed) = optCustomSeed;
#ifdef DEBUG_STARSEED
	fprintf (stderr, "Starting a NEW game with seed type %d, %s\n",
			optSeedType,
			(optSeedType == 0) ? "Default Game Mode (no seeding)" :
			(optSeedType == 1) ? "Seed Planets (SysGenRNG only)" :
			(optSeedType == 2) ? "MRQ (Melnorme, Rainbow, and Quasispace)" :
			(optSeedType == 3) ? "Seed Plot (Starseed)" : "UNKNOWN");
#endif
	// During NEW game we want to time more aggressively and reseed
	// if it takes too long to create a map with a seed.
	// The new seed will be saved to SIS and optCustomSeed
	// If non-starseed, this will give us a default plot map,
	// so we still need this.
	if (!InitStarseed (TRUE))
		return (FALSE);

	PlayFrame = CaptureDrawable (LoadGraphic (PLAYMENU_ANIM));
	
	{
		COUNT num_ships;
		SPECIES_ID s_id = ARILOU_ID;

		num_ships = LAST_MELEE_ID - s_id + 1
				+ 2; /* Yehat Rebels and Ur-Quan probe */

		InitQueue (&GLOBAL (avail_race_q), num_ships, sizeof (FLEET_INFO));
		for (i = 0; i < num_ships; ++i)
		{
			SPECIES_ID ship_ref;
			HFLEETINFO hFleet;
			FLEET_INFO *FleetPtr;

			if (i < num_ships - 2)
				ship_ref = s_id++;
			else if (i == num_ships - 2)
				ship_ref = YEHAT_ID;
			else  /* (i == num_ships - 1) */
				ship_ref = UR_QUAN_PROBE_ID;
			
			hFleet = AllocLink (&GLOBAL (avail_race_q));
			if (!hFleet)
				continue;
			FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
			FleetPtr->SpeciesID = ship_ref;

			if (i < num_ships - 1)
			{
				HMASTERSHIP hMasterShip;
				MASTER_SHIP_INFO *MasterPtr;
				
				hMasterShip = FindMasterShip (ship_ref);
				MasterPtr = LockMasterShip (&master_q, hMasterShip);
				// Grab a copy of loaded icons and strings (not owned)
				copyFleetInfo (FleetPtr, &MasterPtr->ShipInfo,
						&MasterPtr->Fleet);
				UnlockMasterShip (&master_q, hMasterShip);
				// If the game is seeded, move the fleet to starting position
				SeedFleet (FleetPtr, plot_map);
			}
			else
			{
				// Ur-Quan probe.
				RACE_DESC *RDPtr = load_ship (FleetPtr->SpeciesID,
						FALSE);
				if (RDPtr)
				{	// Grab a copy of loaded icons and strings
					copyFleetInfo (FleetPtr, &RDPtr->ship_info,
							&RDPtr->fleet);
					// avail_race_q owns these resources now
					free_ship (RDPtr, FALSE, FALSE);
				}
			}

			FleetPtr->allied_state = BAD_GUY;
			FleetPtr->known_strength = 0;
			FleetPtr->loc = FleetPtr->known_loc;
			// XXX: Hack: Rebel special case 
			if (i == YEHAT_REBEL_SHIP)
				FleetPtr->actual_strength = 0;
			FleetPtr->growth = 0;
			FleetPtr->growth_fract = 0;
			FleetPtr->growth_err_term = 255 >> 1;
			FleetPtr->days_left = 0;
			FleetPtr->func_index = ~0;
			FleetPtr->can_build = FALSE;
			if (optUnlockShips && i < LAST_MELEE_ID)
				FleetPtr->can_build = TRUE;

			UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
			PutQueue (&GLOBAL (avail_race_q), hFleet);
		}
	}

	InitSISContexts ();
	LoadSC2Data ();

	InitPlanetInfo ();
	InitGroupInfo (TRUE);

	GLOBAL (glob_flags) = NUM_READ_SPEEDS >> 1;
	
	GLOBAL_SIS (Difficulty) = optDifficulty;
	GLOBAL_SIS (Extended) = optExtended;
	GLOBAL_SIS (Nomad) = optNomad;
	GLOBAL_SIS (Seed) = optCustomSeed;

	if (DIF_HARD && !PrimeSeed && !StarSeed)
	{
		srand (time (NULL));
		GLOBAL_SIS (Seed) = (rand () % ((MAX_SEED - MIN_SEED) + MIN_SEED));
	}

	GLOBAL (ElementWorth[COMMON]) = 1;
	GLOBAL_SIS (ElementAmounts[COMMON]) = 0;
	GLOBAL (ElementWorth[CORROSIVE]) = 2;
	GLOBAL_SIS (ElementAmounts[CORROSIVE]) = 0;
	GLOBAL (ElementWorth[BASE_METAL]) = 3;
	GLOBAL_SIS (ElementAmounts[BASE_METAL]) = 0;
	GLOBAL (ElementWorth[NOBLE]) = 4;
	GLOBAL_SIS (ElementAmounts[NOBLE]) = 0;
	GLOBAL (ElementWorth[RARE_EARTH]) = 5;
	GLOBAL_SIS (ElementAmounts[RARE_EARTH]) = 0;
	GLOBAL (ElementWorth[PRECIOUS]) = 6;
	GLOBAL_SIS (ElementAmounts[PRECIOUS]) = 0;
	GLOBAL (ElementWorth[RADIOACTIVE]) = 8;
	GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) = 0;
	GLOBAL (ElementWorth[EXOTIC]) = 25;
	GLOBAL_SIS (ElementAmounts[EXOTIC]) = 0;

	switch (DIFFICULTY)
	{
		int i;
		case EASY:
			for (i = 0; i < NUM_ELEMENT_CATEGORIES; i++) {
				GLOBAL (ElementWorth[i]) *= 2;
			}
			break;
		case HARD:
			GLOBAL (ElementWorth[EXOTIC]) = 16;
			SET_GAME_STATE (CREW_PURCHASED0, LOBYTE (100));
			SET_GAME_STATE (CREW_PURCHASED1, HIBYTE (100));
			break;
	}

	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
		GLOBAL_SIS (DriveSlots[i]) = EMPTY_SLOT + 0;
	GLOBAL_SIS (DriveSlots[5]) =
			GLOBAL_SIS (DriveSlots[6]) = FUSION_THRUSTER;
	for (i = 0; i < NUM_JET_SLOTS; ++i)
		GLOBAL_SIS (JetSlots[i]) = EMPTY_SLOT + 1;
	GLOBAL_SIS (JetSlots[0]) =
			GLOBAL_SIS (JetSlots[6]) = TURNING_JETS;
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
		GLOBAL_SIS (ModuleSlots[i]) = EMPTY_SLOT + 2;
	GLOBAL_SIS (ModuleSlots[15]) = GUN_WEAPON;
	GLOBAL_SIS (ModuleSlots[2]) = CREW_POD;
	// Make crew 31 at start to align with the amount of crew lost on the
	// Tobermoon during the journey from Vela to Sol, Hard and/or Extended
	// mode only
	GLOBAL_SIS (CrewEnlisted) =
			(DIF_HARD || EXTENDED) ? 31 : CREW_POD_CAPACITY;
	GLOBAL_SIS (ModuleSlots[8]) = STORAGE_BAY;
	GLOBAL_SIS (ModuleSlots[1]) = FUEL_TANK;
	GLOBAL_SIS (FuelOnBoard) = IF_EASY(10 * FUEL_TANK_SCALE, 4338);

	if (DIF_EASY)
	{
		GLOBAL_SIS (DriveSlots[7]) =
			GLOBAL_SIS (DriveSlots[8]) = FUSION_THRUSTER;
		GLOBAL_SIS (JetSlots[1]) =
			GLOBAL_SIS (JetSlots[7]) = TURNING_JETS;
	}

	if (NOMAD)
	{
		GLOBAL_SIS (DriveSlots[3]) =
			GLOBAL_SIS (DriveSlots[4]) = FUSION_THRUSTER;
		GLOBAL_SIS (JetSlots[2]) =
			GLOBAL_SIS (JetSlots[5]) = TURNING_JETS;
		GLOBAL_SIS (FuelOnBoard) += 20 * FUEL_TANK_SCALE;
	}
 
	if (optHeadStart)
	{
		SET_GAME_STATE (FOUND_PLUTO_SPATHI, 2);
		SET_GAME_STATE (KNOW_SPATHI_PASSWORD, 1);
		SetHomeworldKnown (SPATHI_HOME);
		if (!NOMAD) 
		{
			SET_GAME_STATE (MOONBASE_ON_SHIP, 1);
			SET_GAME_STATE (MOONBASE_DESTROYED, 1);
			GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) = 1;
			GLOBAL_SIS (TotalElementMass) = 1;
		}
	}

	loadGameCheats ();

	InitQueue (&GLOBAL (built_ship_q),
			MAX_BUILT_SHIPS, sizeof (SHIP_FRAGMENT));
	InitQueue (&GLOBAL (npc_built_ship_q), MAX_SHIPS_PER_SIDE,
			sizeof (SHIP_FRAGMENT));
	InitQueue (&GLOBAL (ip_group_q), MAX_BATTLE_GROUPS,
			sizeof (IP_GROUP));
	InitQueue (&GLOBAL (encounter_q), MAX_ENCOUNTERS, sizeof (ENCOUNTER));

	GLOBAL (CurrentActivity) = IN_INTERPLANETARY | START_INTERPLANETARY;

	GLOBAL_SIS (ResUnits) = IF_EASY(0, 2500);
	GLOBAL (CrewCost) = 3;
	GLOBAL (FuelCost) = FUEL_COST_RU; // JMS: Was "20"
	GLOBAL (ModuleCost[PLANET_LANDER]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[FUSION_THRUSTER]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[TURNING_JETS]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[CREW_POD]) = 2000 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[STORAGE_BAY]) = 750 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[FUEL_TANK]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[DYNAMO_UNIT]) = 2000 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[GUN_WEAPON]) = 2000 / MODULE_COST_SCALE;

	GLOBAL_SIS (NumLanders) = IF_EASY(1, 2);

	utf8StringCopy (GLOBAL_SIS (ShipName),
			sizeof (GLOBAL_SIS (ShipName)),
			GAME_STRING (NAMING_STRING_BASE + 6)); // UNNAMED
	utf8StringCopy (GLOBAL_SIS (CommanderName),
			sizeof (GLOBAL_SIS (CommanderName)),
			GAME_STRING (NAMING_STRING_BASE + 6)); // UNNAMED

	SetRaceAllied (HUMAN_SHIP, TRUE);
	CloneShipFragment (HUMAN_SHIP, &GLOBAL (built_ship_q), 0);

	if (optHeadStart)
	{
		BYTE SpaCrew = IF_EASY(1, 30);
		AddEscortShips (SPATHI_SHIP, 1);
		// Make the Eluder escort captained by Fwiffo alone or have a full
		// compliment for Easy mode.
		SetEscortCrewComplement (SPATHI_SHIP,
				SpaCrew, NAME_OFFSET + NUM_CAPTAINS_NAMES);
		StartSphereTracking (SPATHI_SHIP);
	}

	GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (plot_map[SOL_DEFINED].star_pt.x);
	GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (plot_map[SOL_DEFINED].star_pt.y);
	CurStarDescPtr = 0;
	GLOBAL (autopilot.x) = ~0;
	GLOBAL (autopilot.y) = ~0;

	ZeroLastLoc ();
	ZeroAdvancedAutoPilot ();
	ZeroAdvancedQuasiPilot ();

	return (TRUE);
}

void
FreeSC2Data (void)
{
	DestroyContext (RadarContext);
	RadarContext = 0;
	DestroyDrawable (ReleaseDrawable (FontGradFrame));
	FontGradFrame = 0;
	DestroyDrawable (ReleaseDrawable (MiscDataFrame));
	MiscDataFrame = 0;
	DestroyDrawable (ReleaseDrawable (visitedStarsFrame));
	visitedStarsFrame = 0;
	DestroyDrawable (ReleaseDrawable (FlagStatFrame));
	FlagStatFrame = 0;
}

void
UninitGameStructures (void)
{
	HFLEETINFO hStarShip;

	UninitQueue (&GLOBAL (encounter_q));
	UninitQueue (&GLOBAL (ip_group_q));
	UninitQueue (&GLOBAL (npc_built_ship_q));
	UninitQueue (&GLOBAL (built_ship_q));
	UninitGroupInfo ();
	UninitPlanetInfo ();

//    FreeSC2Data ();

	// The only resources avail_race_q owns are the Ur-Quan probe's
	// so free them now
	hStarShip = GetTailLink (&GLOBAL (avail_race_q));
	if (hStarShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		DestroyDrawable (ReleaseDrawable (FleetPtr->melee_icon));
		DestroyDrawable (ReleaseDrawable (FleetPtr->icons));
		DestroyStringTable (ReleaseStringTable (FleetPtr->race_strings));
		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}

	UninitQueue (&GLOBAL (avail_race_q));
	
	DestroyDrawable (ReleaseDrawable (PlayFrame));
	PlayFrame = 0;
}

void
InitGlobData (void)
{
	COUNT i;

	i = GLOBAL (glob_flags);
	memset (&GlobData, 0, sizeof (GlobData));
	GLOBAL (glob_flags) = (BYTE)i;

	GLOBAL (DisplayArray) = DisplayArray;
}


// For debugging purposes, generate a bunch of seeds and then
// calculate how many fall into each time category.
void
SeedDEBUG ()
{
#define SAMPLE_SIZE 1000
#define START 123000
	SDWORD save = optCustomSeed;
	COUNT histogram[100] = {[0 ... 99] = 0};
	COUNT decisec;
	clock_t start_clock;
	BOOLEAN myRNG = false;

	if (!StarGenRNG)
	{
		fprintf(stderr, "****Seed Debug Creating a STAR GEN RNG****\n");
		StarGenRNG = RandomContext_New ();
		myRNG = true;
	}
	RandomContext_SeedRandom (StarGenRNG, 123456);
	for (optCustomSeed = START; optCustomSeed < (START + SAMPLE_SIZE);
			optCustomSeed++)
	{
		start_clock = clock();
		fprintf (stderr, "\n\n\nStarting seed %d... ", optCustomSeed);
		InitPlot (plot_map);
		fprintf (stderr, "seeding stars %d... ", optCustomSeed);
		SeedStarmap (star_array);
		fprintf (stderr, "seeding plots %d... ", optCustomSeed);
		if (SeedPlot (plot_map, star_array) < NUM_PLOTS)
		{
			fprintf (stderr, "Failed to seed %d. ", optCustomSeed);
			decisec = 98;
		}
		else
			decisec = (double)(clock() - start_clock) / 100000;
		fprintf (stderr, "Complete %6.6f seconds.\n",
				(double)(clock() - start_clock) / 1000000);
		if (decisec > 99)
			decisec = 99;
		histogram[decisec]++;
	}
	for (decisec = 0; decisec < 100; decisec++)
	{
		if (decisec % 10 == 0) fprintf(stderr, "\n");
		fprintf(stderr, "%3d ", histogram[decisec]);
	}
	optCustomSeed = save;
	if (StarGenRNG && myRNG)
	{
		RandomContext_Delete (StarGenRNG);
		StarGenRNG = NULL;
	}
}

// Initialize the plot map, star array, and quasi portal map.
// This is called during either new or load, whether or not the new game is
// a seeded game as we will need to reset global variables regardless.
//
// Assumes the space for global arrays already allocated for:
// star_array - a copy of the starmap (starmap_array)
// plot_map - a plot array
// portal_map - a quasispace portal array
BOOLEAN
InitStarseed (BOOLEAN newgame)
{
	COUNT i;
#ifdef DEBUG_STARSEED_TRACE_V
	SeedDEBUG ();
	fprintf (stderr, "CurrentActivity %d\n", GLOBAL (CurrentActivity));
#endif
	DefaultStarmap (star_array);
	if (!StarGenRNG)
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr, "Init Starseed creating a STAR GEN RNG.\n");
#endif
		StarGenRNG = RandomContext_New ();
		RandomContext_SeedRandom (StarGenRNG, optCustomSeed);
	}
	if (!StarSeed)
	{
		// Here we will split off, provide default plotmap, return
		// This makes it easier to integrate seemlessly old vs new
		DefaultPlot (plot_map, star_array);
		DefaultQuasispace (portal_map);
		// Done with StarGenRNG for now; will create later if moving fleets
		if (StarGenRNG)
			RandomContext_Delete (StarGenRNG);
		StarGenRNG = NULL;
		return TRUE;
	}
	if (optSeedType == OPTVAL_MRQ)
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr, "Setting MQR shuffle.\n");
#endif
		DefaultPlot (plot_map, star_array);
		InitMelnormeRainbow (plot_map);
	}
	else // optSeedType == OPTVAL_STAR
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr, "Setting full plot shuffle.\n");
#endif
		InitPlot (plot_map);
	}
	fprintf (stderr, "Starting map generation using seed %d.\n",
			optCustomSeed);
	SeedStarmap (star_array);
	if (GLOBAL (CurrentActivity) || !newgame)
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr, "*+*+* LOADING *+*+*\n");
#endif
		if (SeedPlot (plot_map, star_array) != NUM_PLOTS)
		{
			fprintf (stderr, "Seed Plot Failed.\n");
			if (StarGenRNG)
				RandomContext_Delete (StarGenRNG);
			StarGenRNG = NULL;
			return FALSE;
		}
		if (!SeedQuasispace (portal_map, plot_map, star_array))
		{
			fprintf (stderr, "Seed Quasisapce Failed.\n");
			if (StarGenRNG)
				RandomContext_Delete (StarGenRNG);
			StarGenRNG = NULL;
			return FALSE;
		}
	}
	else
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr, "*+*+* NEW GAME *+*+*\n");
#endif
		i = 0;
		// if it fails to seed the plot we need to roll the starmap too
		// otherwise load game will not be correct, it will have other
		// seed's stars and this seed's plot.  Boo.
		while (SeedPlot (plot_map, star_array) != NUM_PLOTS && i < 100)
		{
			fprintf (stderr, "Seed %d failed (%d).\n", optCustomSeed++, ++i);
			SeedStarmap (star_array);
		}
		if (i >= 100)
		{
			fprintf (stderr, "Seed Plot Failed.\n");
			if (StarGenRNG)
				RandomContext_Delete (StarGenRNG);
			StarGenRNG = NULL;
			return FALSE;
		}
		if (!SeedQuasispace (portal_map, plot_map, star_array))
		{
			fprintf (stderr, "Seed Quasisapce Failed.\n");
			if (StarGenRNG)
				RandomContext_Delete (StarGenRNG);
			StarGenRNG = NULL;
			return FALSE;
		}
	}
	if (OOPS_ALL > 0)
		for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
			if (!star_array[i].Index)
				star_array[i].Index = OOPS_ALL;
	// In case the seed changed above, reset SIS
	GLOBAL_SIS (Seed) = optCustomSeed;
#ifdef DEBUG_STARSEED
	fprintf (stderr, "Done seeding %d.\n", optCustomSeed);
#endif
	// Done with StarGenRNG for now; will create later if moving fleets
	if (StarGenRNG)
		RandomContext_Delete (StarGenRNG);
	StarGenRNG = NULL;
	return (TRUE);
}

BOOLEAN
inFullGame (void)
{
	ACTIVITY act = LOBYTE (GLOBAL (CurrentActivity));
	return (act == IN_LAST_BATTLE || act == IN_ENCOUNTER ||
			act == IN_HYPERSPACE || act == IN_INTERPLANETARY ||
			act == WON_LAST_BATTLE);
}

BOOLEAN
inSuperMelee (void)
{
	return (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE);
			// TODO: && !inMainMenu ()
}

#if 0
BOOLEAN
inBattle (void)
{
	// TODO: IN_BATTLE is also set while in HyperSpace/QuasiSpace.
	return ((GLOBAL (CurrentActivity) & IN_BATTLE) != 0);
}
#endif

#if 0
// Disabled for now as there are similar functions in uqm/planets/planets.h
// Pre: inFullGame()
BOOLEAN
inInterPlanetary (void)
{
	assert (inFullGame ());
	return (pSolarSysState != NULL);
}

// Pre: inFullGame()
BOOLEAN
inSolarSystem (void)
{
	assert (inFullGame ());
	return (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY);
}

// Pre: inFullGame()
BOOLEAN
inOrbit (void)
{
	assert (inFullGame ());
	return (pSolarSysState != NULL) &&
			(pSolarSysState->pOrbitalDesc != NULL);
}
#endif

// In HyperSpace or QuasiSpace
// Pre: inFullGame()
BOOLEAN
inHQSpace (void)
{
	//assert (inFullGame ());
	return (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE);
			// IN_HYPERSPACE is also set for QuasiSpace
}

// In HyperSpace
// Pre: inFullGame()
BOOLEAN
inHyperSpace (void)
{
	//assert (inFullGame ());
	return (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE) &&
				(GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1);
			// IN_HYPERSPACE is also set for QuasiSpace
}

// In QuasiSpace
// Pre: inFullGame()
BOOLEAN
inQuasiSpace (void)
{
	//assert (inFullGame ());
	return (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE) &&
				(GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1);
			// IN_HYPERSPACE is also set for QuasiSpace
}

OPT_CONSOLETYPE
isPC (int optWhich)
{
	return optWhich == OPT_PC;
}

OPT_CONSOLETYPE
is3DO (int optWhich)
{
	return optWhich == OPT_3DO;
}

// Does not work with UTF encoding!
// Basic function for replacing all instances of character "find"
// with character "replace"
// Returns the number of replaced characters on success
// -1 if any input is NULL, and 0 if 'find' isn't found
int
replaceChar (char *pStr, const char find, const char replace)
{
	int count = 0;
	size_t i;
	size_t len = strlen (pStr);

	if (!pStr || !find || !replace)
		return -1; // pStr, find, or replace is NULL

	if (utf8StringPos (pStr, find) == -1)
		return 0; // 'find' not found

	for (i = 0; i < len; i++)
	{
		if (pStr[i] == find)
		{
			pStr[i] = replace;
			if (pStr[i] == replace)
				count++;
		}
	}

	return count;
}
