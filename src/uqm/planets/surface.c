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

#include "lifeform.h"
#include "planets.h"
#include "libs/mathlib.h"
#include "libs/log.h"


//#define DEBUG_SURFACE

const BYTE *Elements;
const PlanetFrame *PlanData;

static COUNT
CalcMineralDeposits (const SYSTEM_INFO *SysInfoPtr, COUNT which_deposit,
		NODE_INFO *info)
{
	BYTE j;
	COUNT num_deposits;
	const ELEMENT_ENTRY *eptr;

	eptr = &SysInfoPtr->PlanetInfo.PlanDataPtr->UsefulElements[0];
	num_deposits = 0;
	j = NUM_USEFUL_ELEMENTS;
	do
	{
		BYTE num_possible;

		num_possible = LOBYTE (RandomContext_Random (SysGenRNG))
				% (DEPOSIT_QUANTITY (eptr->Density) + 1);
		while (num_possible--)
		{
#define MEDIUM_DEPOSIT_THRESHOLD 150
#define LARGE_DEPOSIT_THRESHOLD 225
			COUNT deposit_quality_fine;
			COUNT deposit_quality_gross;

			// JMS: For making the mineral blip smaller in case it is partially scavenged.
			SDWORD temp_deposit_quality;

			deposit_quality_fine = (LOWORD (RandomContext_Random (SysGenRNG)) % 100)
					+ (
					DEPOSIT_QUALITY (eptr->Density)
					+ SysInfoPtr->StarSize
					) * 50;

			// JMS: This makes the mineral blip smaller in case it is partially scavenged.
			if (which_deposit < 32)
				temp_deposit_quality = deposit_quality_fine - ((SysInfoPtr->PlanetInfo.PartiallyScavengedList[MINERAL_SCAN][which_deposit]) * 10);
			// JMS: In case which_deposit >= 32 (most likely 65535), it means that this
			// function is being called only to count the number of deposit nodes on the
			// surface. In that case we don't need to use the PartiallyScavengedList
			// since the amount of minerals in that node is not stored yet.
			// (AND we cannot use the list since accessing element 65535 would crash the game ;)
			else
				temp_deposit_quality = deposit_quality_fine;
			
			if (temp_deposit_quality < 0)
				temp_deposit_quality = 0;
			
			if (temp_deposit_quality < MEDIUM_DEPOSIT_THRESHOLD)
				deposit_quality_gross = 0;
			else if (temp_deposit_quality < LARGE_DEPOSIT_THRESHOLD)
				deposit_quality_gross = 1;
			else
				deposit_quality_gross = 2;

			GenerateRandomLocation (&info->loc_pt);

			info->density = MAKE_WORD (
					deposit_quality_gross, deposit_quality_fine / 10 + 1);
			info->type = eptr->ElementType;
#ifdef DEBUG_SURFACE
			log_add (log_Debug, "\t\t%d units of %Fs",
					info->density,
					Elements[eptr->ElementType].name);
#endif /* DEBUG_SURFACE */
			if (num_deposits >= which_deposit
					|| ++num_deposits == sizeof (DWORD) * 8)
			{	// reached the maximum or the requested node
				return num_deposits;
			}
		}
		++eptr;
	} while (--j);

	return num_deposits;
}

// Returns:
//   for whichLife==~0 : the number of nodes generated
//   for whichLife<32  : the index of the last node (no known usage exists)
// Sets the SysGenRNG to the required state first.
COUNT
GenerateMineralDeposits (const SYSTEM_INFO *SysInfoPtr, COUNT whichDeposit,
		NODE_INFO *info)
{
	NODE_INFO temp_info;
	if (!info) // user not interested in info but we need space for it
		info = &temp_info;
	RandomContext_SeedRandom (SysGenRNG,
			SysInfoPtr->PlanetInfo.ScanSeed[MINERAL_SCAN]);
	return CalcMineralDeposits (SysInfoPtr, whichDeposit, info);
}

static COUNT
CalcLifeForms (const SYSTEM_INFO *SysInfoPtr, COUNT which_life,
		NODE_INFO *info)
{
	COUNT num_life_forms;

	num_life_forms = 0;
	if (PLANSIZE (SysInfoPtr->PlanetInfo.PlanDataPtr->Type) != GAS_GIANT)
	{
#define MIN_LIFE_CHANCE 10
		SIZE life_var;

		life_var = RandomContext_Random (SysGenRNG) & 1023;
		if (life_var < SysInfoPtr->PlanetInfo.LifeChance
				|| (SysInfoPtr->PlanetInfo.LifeChance < MIN_LIFE_CHANCE
				&& life_var < MIN_LIFE_CHANCE))
		{
			BYTE num_types;

			num_types = 1 + LOBYTE (RandomContext_Random (SysGenRNG))
					% MAX_LIFE_VARIATION;
			do
			{
				BYTE index, num_creatures;
				UWORD rand_val;

				rand_val = RandomContext_Random (SysGenRNG);
				index = LOBYTE (rand_val) % NUM_CREATURE_TYPES;
				num_creatures = 1 + HIBYTE (rand_val) % 10;
				do
				{
					GenerateRandomLocation (&info->loc_pt);
					info->type = index;
					info->density = 0;

					if (num_life_forms >= which_life
							|| ++num_life_forms == sizeof (DWORD) * 8)
					{	// reached the maximum or the requested node
						return num_life_forms;
					}
				} while (--num_creatures);
			} while (--num_types);
		}
#ifdef DEBUG_SURFACE
		else
		{
			log_add (log_Debug, "It's dead, Jim! (%d >= %d)", life_var,
				SysInfoPtr->PlanetInfo.LifeChance);
		}
#endif /* DEBUG_SURFACE */
	}

	return num_life_forms;
}

// Returns:
//   for whichLife==~0 : the number of lifeforms generated
//   for whichLife<32  : the index of the last lifeform (no known usage exists)
// Sets the SysGenRNG to the required state first.
COUNT
GenerateLifeForms (const SYSTEM_INFO *SysInfoPtr, COUNT whichLife,
		NODE_INFO *info)
{
	NODE_INFO temp_info;
	if (!info) // user not interested in info but we need space for it
		info = &temp_info;
	RandomContext_SeedRandom (SysGenRNG,
			SysInfoPtr->PlanetInfo.ScanSeed[BIOLOGICAL_SCAN]);
	return CalcLifeForms (SysInfoPtr, whichLife, info);
}

// Returns:
//   for whichLife==~0 : the number of lifeforms generated
//   for whichLife<32  : the index of the last lifeform (no known usage exists)
// Sets the SysGenRNG to the required state first.
// lifeTypes[] is terminated with -1
COUNT
GeneratePresetLife (const SYSTEM_INFO *SysInfoPtr, const SBYTE *lifeTypes,
		COUNT whichLife, NODE_INFO *info)
{
	COUNT i;
	NODE_INFO temp_info;

	if (!info) // user not interested in info but we need space for it
		info = &temp_info;

	// This function may look unnecessarily complicated, but it must be
	// kept this way to preserve the universe. That is done by preserving
	// the order and number of Random() calls.

	RandomContext_SeedRandom (SysGenRNG,
			SysInfoPtr->PlanetInfo.ScanSeed[BIOLOGICAL_SCAN]);

	for (i = 0; lifeTypes[i] >= 0; ++i)
	{
		GenerateRandomLocation (&info->loc_pt);
		info->type = lifeTypes[i];
		// density is irrelevant for bio nodes
		info->density = 0;

		if (i >= whichLife)
			break;
	}
	
	return i;
}

static int
widthHeightPicker (BOOLEAN is_width)
{
	switch (optStarBackground)
	{
	case 0:
		return (is_width ? PC_MAP_WIDTH : PC_MAP_HEIGHT);
	case 1:
		return (is_width ? THREEDO_MAP_WIDTH : THREEDO_MAP_HEIGHT);
	case 2:
	default:
		return (is_width ? (UQM_MAP_WIDTH - 1) : UQM_MAP_HEIGHT);
	}
}

static COORD
scaleMapDimensions (BOOLEAN is_width, COORD value)
{
	float percentage;
	int widthOrHeight = is_width ?
		UQM_MAP_WIDTH : UQM_MAP_HEIGHT;

	if (widthOrHeight == widthHeightPicker (is_width))
		percentage = 1;
	else
		percentage = scaleThingUp (widthOrHeight,
			widthHeightPicker (is_width));

	return (COORD)(value * percentage);
}

void
GenerateRandomLocation (POINT *loc)
{
	UWORD rand_val;

	rand_val = RandomContext_Random (SysGenRNG);
	if (optSuperPC != OPT_PC)
	{
		loc->x = RES_SCALE(
			scaleMapDimensions (
				TRUE, 8 + LOBYTE (rand_val)
				% (widthHeightPicker (TRUE) - (8 << 1))
			));
		loc->y = RES_SCALE(
			scaleMapDimensions (
				FALSE, 8 + HIBYTE (rand_val)
				% (widthHeightPicker (FALSE) - (8 << 1))
			));
	}
	else
	{
		loc->x = RES_SCALE(8 + LOBYTE (rand_val) % (PC_MAP_WIDTH - (8 << 1)));
		loc->y = RES_SCALE(
			scaleMapDimensions (
				FALSE, 8 + HIBYTE (rand_val)
				% (widthHeightPicker (FALSE) - (8 << 1))
			));
	}
}

// Returns:
//   for whichNode==~0 : the number of nodes generated
//   for whichNode<32  : the index of the last node (no known usage exists)
// Sets the SysGenRNG to the required state first.
COUNT
GenerateRandomNodes (const SYSTEM_INFO *SysInfoPtr, COUNT scan, COUNT numNodes,
		COUNT type, COUNT whichNode, NODE_INFO *info)
{
	COUNT i;
	NODE_INFO temp_info;
	const ELEMENT_ENTRY *eptr;

	eptr = &SysInfoPtr->PlanetInfo.PlanDataPtr->UsefulElements[0];

	if (!info) // user not interested in info but we need space for it
		info = &temp_info;

	RandomContext_SeedRandom (SysGenRNG,
			SysInfoPtr->PlanetInfo.ScanSeed[scan]);

	for (i = 0; i < numNodes; ++i)
	{
#define MEDIUM_DEPOSIT_THRESHOLD 150
#define LARGE_DEPOSIT_THRESHOLD 225
		COUNT deposit_quality_fine;
		COUNT deposit_quality_gross;

		GenerateRandomLocation (&info->loc_pt);
		// type is irrelevant for energy nodes
		info->type = type;

		if (scan == MINERAL_SCAN)
		{
			deposit_quality_fine = (LOWORD (RandomContext_Random (SysGenRNG)) % 100)
				+ (
					DEPOSIT_QUALITY (eptr->Density)
					+ SysInfoPtr->StarSize
					) * 50;
			if (deposit_quality_fine < MEDIUM_DEPOSIT_THRESHOLD)
				deposit_quality_gross = 0;
			else if (deposit_quality_fine < LARGE_DEPOSIT_THRESHOLD)
				deposit_quality_gross = 1;
			else
				deposit_quality_gross = 2;

			info->density = MAKE_WORD (
				deposit_quality_gross, deposit_quality_fine / 10 + 1);
		}
		else	// density is irrelevant for energy and bio nodes
			info->density = 0;

		if (i >= whichNode)
			break;
	}
	
	return i;
}
