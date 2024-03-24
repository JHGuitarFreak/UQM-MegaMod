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

#ifndef GENDEFAULT_H
#define GENDEFAULT_H

#include "types.h"
#include "../planets.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define SMALL_ROCKY (1 << 0)
#define LARGE_ROCKY (1 << 1)
#define ALL_ROCKY (1 << 2)

bool GenerateDefault_initNpcs (SOLARSYS_STATE *solarSys);
bool GenerateDefault_reinitNpcs (SOLARSYS_STATE *solarSys);
bool GenerateDefault_uninitNpcs (SOLARSYS_STATE *solarSys);
bool GenerateDefault_generatePlanets (SOLARSYS_STATE *solarSys);
bool GenerateDefault_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
bool GenerateDefault_generateName (const SOLARSYS_STATE *,
		const PLANET_DESC *world);
bool GenerateDefault_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
COUNT GenerateDefault_generateMinerals (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
COUNT GenerateDefault_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
COUNT GenerateDefault_generateLife (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
bool GenerateDefault_pickupMinerals (SOLARSYS_STATE *, PLANET_DESC *world,
		COUNT whichNode);
bool GenerateDefault_pickupEnergy (SOLARSYS_STATE *, PLANET_DESC *world,
		COUNT whichNode);
bool GenerateDefault_pickupLife (SOLARSYS_STATE *, PLANET_DESC *world,
		COUNT whichNode);

COUNT GenerateDefault_generateArtifact (const SOLARSYS_STATE *,
		COUNT whichNode, NODE_INFO *info);
COUNT GenerateDefault_generateRuins (const SOLARSYS_STATE *,
		COUNT whichNode, NODE_INFO *info);
bool GenerateDefault_landerReport (SOLARSYS_STATE *);
bool GenerateDefault_landerReportCycle (SOLARSYS_STATE *);
extern void GeneratePlanets (SOLARSYS_STATE *system);
extern SIZE CheckForHabitable (SOLARSYS_STATE *solarSys);
extern BYTE GenerateRockyWorld (BYTE whichType);


extern const GenerateFunctions generateDefaultFunctions;

typedef enum {
	ILWRATH_ENCOUNTER, // 14/6 ships at procyon even if ilwrath left
	PROBE_ENCOUNTER, // 4/2 probes at dreadnought wreck
	URQUAN_ENCOUNTER, // 6 dreadnoughts at vault if the shuttle is on board
	EGGCASE00_ENCOUNTER, // 4 mycon
	EGGCASE01_ENCOUNTER, // 4 mycon 
	EGGCASE02_ENCOUNTER, // 4 mycon
	THRADDASH_ENCOUNTER, // 12/6 thraddash minimum at all time
	NO_HELP_FROM_PKUNK, // no help before final battle if pkunk are absorbed before the revolt
	READY_TO_BARGAIN // druuge sell rosy sphere on second deal
} HARDMODE_ENCOUNTERS;

#if defined(__cplusplus)
}
#endif

#endif  /* GENDEFAULT_H */

