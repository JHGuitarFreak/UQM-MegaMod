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

#ifndef UQM_PLANETS_LANDER_H_
#define UQM_PLANETS_LANDER_H_

#include "elemdata.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "libs/timelib.h"
#include "../element.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Surface magnification shift (x4)
#define MAG_SHIFT 2

#define NUM_TEXT_FRAMES 32

// XXX: This is a private type now. Move it to lander.c?
//   We may also want to merge it with LanderInputState.
typedef struct
{
	BOOLEAN InTransit;
			// Landing on or taking of from a planet.
			// Setting it while landed will initiate takeoff.

	COUNT ElementLevel;
	COUNT MaxElementLevel;
	COUNT BiologicalLevel;
	COUNT ElementAmounts[NUM_ELEMENT_CATEGORIES];

	COUNT NumFrames;
	UNICODE AmountBuf[40];
	TEXT MineralText[3];

	Color ColorCycle[NUM_TEXT_FRAMES >> 1];

	BYTE TectonicsChance;
	BYTE WeatherChance;
	BYTE FireChance;
} PLANETSIDE_DESC;

extern MUSIC_REF LanderMusic;

extern void PlanetSide (POINT planetLoc);
extern void DoDiscoveryReport (SOUND ReadOutSounds);
extern void SetPlanetMusic (BYTE planet_type);
extern void LoadLanderData (void);
extern void FreeLanderData (void);

extern void object_animation (ELEMENT *ElementPtr);

extern void SetLanderTakeoff (void);
extern bool KillLanderCrewSeq (COUNT numKilled, DWORD period);

extern unsigned GetThermalHazardRating (int temp);

// ELEMENT.playerNr constants
enum
{
	PS_HUMAN_PLAYER,
	PS_NON_PLAYER,
};

#if defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_LANDER_H_ */

