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
static size_t
totalBitsForGameState (const GameStateBitMap *bm)
{
	size_t totalBits = 0;
	const GameStateBitMap *bmPtr;

	for (bmPtr = bm; bmPtr->name != NULL; bmPtr++)
		totalBits += bmPtr->numBits;

	return totalBits;
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
serialiseGameState (const GameStateBitMap *bm, BYTE **buf, size_t *numBytes)
{
	size_t totalBits;
	size_t totalBytes;
	const GameStateBitMap *bmPtr;
	BYTE *result;
	BYTE *bufPtr;

	DWORD restBits = 0;
			// Bits which have not yet been stored because they did not form
			// an entire byte.
	size_t restBitCount = 0;

	// Determine the total number of bits/bytes required.
	totalBits = totalBitsForGameState (bm);
	totalBytes = (totalBits + 7) / 8;

	// Allocate memory for the serialised data.
	result = HMalloc (totalBytes);
	if (result == NULL)
		return FALSE;

	bufPtr = result;
	for (bmPtr = bm; bmPtr->name != NULL; bmPtr++)
	{
		DWORD value = getGameStateUint (bmPtr->name);
		BYTE numBits = bmPtr->numBits;

#ifdef STATE_DEBUG
		log_add (log_Debug, "Saving: GameState[\'%s\'] = %u", bmPtr->name,
				value);
#endif  /* STATE_DEBUG */

		if (value > bitmask32(numBits))
		{
			log_add (log_Error, "Warning: serialiseGameState(): the value "
					"of the property '%s' (%u) does not fit in the reserved "
					"number of bits (%d).", bmPtr->name, value, numBits);
		}

		// Store multi-byte values with the least significant byte first.
		while (numBits >= 8)
		{
		
			serialiseBits (&bufPtr, &restBits, &restBitCount, value & 0xff, 8);
			value >>= 8;
			numBits -= 8;
		}
		if (numBits > 0)
			serialiseBits (&bufPtr, &restBits, &restBitCount, value, numBits);
	}

	// Pad the end up to a byte.
	if (restBitCount > 0)
		serialiseBits (&bufPtr, &restBits, &restBitCount, 0, 8 - restBitCount);

	*buf = result;
	*numBytes = totalBytes;
	return TRUE;
}

// Read 'numBits' bits from '*bytePtr', starting at the bit offset
// '*bitPtr'. The result is returned.
// '*bitPtr' and '*bytePtr' are updated by this function.
static inline DWORD
deserialiseBits (const BYTE **bytePtr, BYTE *bitPtr, size_t numBits) {
	assert (*bitPtr < 8);
	assert (numBits <= 8);

	if (numBits <= (size_t) (8 - *bitPtr))
	{
		// Can get the entire value from one byte.
		// We want bits *bitPtr through (excluding) *bitPtr+numBits
		DWORD result = ((*bytePtr)[0] >> *bitPtr) & bitmask32(numBits);

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
			*bitPtr += numBits;
		}
		return result;
	}
	else
	{
		// The result comes from two bytes.
		// We get the *bitPtr most significant bits from [0], as the least
		// significant bits of the result, and the (numBits - *bitPtr) least
		// significant bits from [1], as the most significant bits of the
		// result.
		DWORD result = (((*bytePtr)[0] >> *bitPtr)
				| ((*bytePtr)[1] << (8 - *bitPtr))) &
				bitmask32(numBits);
		(*bytePtr)++;
		*bitPtr += numBits - 8;
		return result;
	}
}

// Deserialise the current game state from the bit array in 'buf', which
// has size 'numBytes', according to the GameStateBitMap 'bm'.
BOOLEAN
deserialiseGameState (const GameStateBitMap *bm,
		const BYTE *buf, size_t numBytes)
{
	size_t totalBits;
	const GameStateBitMap *bmPtr;

	const BYTE *bytePtr = buf;
	BYTE bitPtr = 0;
			// Number of bits already processed from the byte pointed at by
			// bytePtr.

	// Sanity check: determine the number of bits required, and check
	// whether 'numBytes' is large enough.
	totalBits = totalBitsForGameState (bm);
	if (numBytes * 8 < totalBits)
	{
		log_add (log_Error, "Warning: deserialiseGameState(): Corrupt "
				"save game: state: less bytes available than expected.");
		return FALSE;
	}

	for (bmPtr = bm; bmPtr->name != NULL; bmPtr++)
	{
		DWORD value = 0;
		BYTE numBits = bmPtr->numBits;
		BYTE bitsLeft = numBits;

		// Multi-byte values are stored with the least significant byte
		// first.
		while (bitsLeft >= 8)
		{
			DWORD bits = deserialiseBits (&bytePtr, &bitPtr, 8);
			value |= shl32(bits, numBits - bitsLeft);
			bitsLeft -= 8;
		}
		if (bitsLeft > 0) {
			value |= shl32(deserialiseBits (&bytePtr, &bitPtr, bitsLeft),
					numBits - bitsLeft);
		}
	
#ifdef STATE_DEBUG
		log_add (log_Debug, "Loading: GameState[\'%s\'] = %u", bmPtr->name,
				value);
#endif  /* STATE_DEBUG */

		setGameStateUint (bmPtr->name, value);
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

	PlayFrame = CaptureDrawable (LoadGraphic (PLAYMENU_ANIM));
	
	{
		COUNT num_ships;
		SPECIES_ID s_id = ARILOU_ID;

		num_ships = KOHR_AH_ID - s_id + 1
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

			UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
			PutQueue (&GLOBAL (avail_race_q), hFleet);
		}
	}

	InitSISContexts ();
	LoadSC2Data ();

	InitPlanetInfo ();
	InitGroupInfo (TRUE);

	GLOBAL (glob_flags) = 0;
	
	GLOBAL_SIS (Difficulty) = optDifficulty;
	GLOBAL_SIS (Extended) = optExtended;
	GLOBAL_SIS (Nomad) = optNomad;
	GLOBAL_SIS (Seed) = optCustomSeed;

	if (DIF_HARD && !PrimeSeed) {
		srand(time(NULL));
		GLOBAL_SIS(Seed) = (rand() % ((MAX_SEED - MIN_SEED) + MIN_SEED));
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

	switch (DIFFICULTY) {
		int i;
		case EASY:
			for (i = 0; i < NUM_ELEMENT_CATEGORIES; i++) {
				GLOBAL(ElementWorth[i]) *= 2;
			}
			break;
		case HARD:
			GLOBAL(ElementWorth[EXOTIC]) = 16;
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
	GLOBAL_SIS (CrewEnlisted) =  IF_HARD(CREW_POD_CAPACITY, 31);
	GLOBAL_SIS (ModuleSlots[8]) = STORAGE_BAY;
	GLOBAL_SIS (ModuleSlots[1]) = FUEL_TANK;
	GLOBAL_SIS (FuelOnBoard) = IF_EASY(10 * FUEL_TANK_SCALE, 4338);

	if (DIF_EASY) {
		GLOBAL_SIS(DriveSlots[7]) =
			GLOBAL_SIS(DriveSlots[8]) = FUSION_THRUSTER;
		GLOBAL_SIS(JetSlots[1]) =
			GLOBAL_SIS(JetSlots[7]) = TURNING_JETS;
	}

	if (NOMAD) {
		GLOBAL_SIS(DriveSlots[3]) =
			GLOBAL_SIS(DriveSlots[4]) = FUSION_THRUSTER;
		GLOBAL_SIS(JetSlots[2]) =
			GLOBAL_SIS(JetSlots[5]) = TURNING_JETS;
		GLOBAL_SIS(FuelOnBoard) += 20 * FUEL_TANK_SCALE;
	}
 
	if (optHeadStart){
		SET_GAME_STATE (FOUND_PLUTO_SPATHI, 2);
		SET_GAME_STATE (PROBE_MESSAGE_DELIVERED, 1);
		if (!NOMAD) {
			SET_GAME_STATE(MOONBASE_ON_SHIP, 1);
			SET_GAME_STATE(MOONBASE_DESTROYED, 1);
			GLOBAL_SIS(ModuleSlots[7]) = STORAGE_BAY;
			GLOBAL_SIS(ElementAmounts[COMMON]) = 178;
			GLOBAL_SIS(ElementAmounts[CORROSIVE]) = 66;
			GLOBAL_SIS(ElementAmounts[BASE_METAL]) = 378;
			GLOBAL_SIS(ElementAmounts[PRECIOUS]) = 29;
			GLOBAL_SIS(ElementAmounts[RADIOACTIVE]) = 219;
			GLOBAL_SIS(ElementAmounts[EXOTIC]) = 5;
			GLOBAL_SIS(TotalElementMass) = 875;
		}
	}

	loadGameCheats();

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
	GLOBAL (FuelCost) = FUEL_COST_RU; // JMS: Was 20
	GLOBAL (ModuleCost[PLANET_LANDER]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[FUSION_THRUSTER]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[TURNING_JETS]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[CREW_POD]) = 2000 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[STORAGE_BAY]) = 750 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[FUEL_TANK]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[DYNAMO_UNIT]) = 2000 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[GUN_WEAPON]) = 2000 / MODULE_COST_SCALE;

	GLOBAL_SIS (NumLanders) = IF_EASY(1, 2);

	utf8StringCopy (GLOBAL_SIS (ShipName), sizeof (GLOBAL_SIS (ShipName)),
			GAME_STRING (NAMING_STRING_BASE + 2));
	utf8StringCopy (GLOBAL_SIS (CommanderName), sizeof (GLOBAL_SIS (CommanderName)),
			GAME_STRING (NAMING_STRING_BASE + 3));

	SetRaceAllied (HUMAN_SHIP, TRUE);
	CloneShipFragment (HUMAN_SHIP, &GLOBAL (built_ship_q), 0);

	if(optHeadStart){
		BYTE SpaCrew = IF_EASY(1, 30);
		AddEscortShips (SPATHI_SHIP, 1);
		/* Make the Eluder escort captained by Fwiffo alone or have a full compliment for Easy mode. */
		SetEscortCrewComplement (SPATHI_SHIP, SpaCrew, NAME_OFFSET + NUM_CAPTAINS_NAMES);
	}

	GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (SOL_X);
	GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (SOL_Y);
	CurStarDescPtr = 0;
	GLOBAL (autopilot.x) = ~0;
	GLOBAL (autopilot.y) = ~0;
	if (optHeadStart && PrimeSeed) {
		// Start at Earth when Head Start is enabled
		GLOBAL(ShipFacing) = 1;
		GLOBAL(ip_location.x) = EARTH_OUTER_X;
		GLOBAL(ip_location.y) = EARTH_OUTER_Y;
	}

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

