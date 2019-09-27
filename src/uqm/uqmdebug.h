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

#include "clock.h"
#include "planets/planets.h"
#include "races.h"
#include "libs/compiler.h"

#include <stdio.h>
 
// If a function is assigned to this, it will be called from the
// Starcon2Main thread, in the main game loop.
extern void (* volatile debugHook) (void);

// Move the Flagship to the destination of the autopilot.
// Should only be called from HS/QS.
// It can be called from debugHook directly after entering HS/QS though.
void doInstantMove (void);

// Find a player ship. Setting playerNr to non-0 is only meaningful in battle.
STARSHIP* findPlayerShip (SIZE playerNr);

// Resets the energy of the first player (the bottom one) to its maximum.
void resetEnergyBattle(void);

#if !defined(UQM_UQMDEBUG_H_) && (defined(DEBUG) || defined(USE_DEBUG_KEY))
#define UQM_UQMDEBUG_H_


// If set to true, interactive routines that are called (indirectly) in debug
// functions are a no-op.
extern BOOLEAN disableInteractivity;

// Called on the main() thread when the debug key (symbol 'Debug' in the
// keys.cfg) is pressed
void debugKeyPressed (void);
// Called on the Starcon2Main() thread when the debug key (symbol 'Debug'
// in the keys.cfg) is pressed.
void debugKeyPressedSynchronous (void);

// JMS: Called when the debug key (symbol 'Debug_2' in the keys.cfg) is pressed.
void debugKey2Pressed (void);

// JMS: Called when the debug key (symbol 'Debug_3' in the keys.cfg) is pressed.
void debugKey3Pressed (void);

// JMS: Called when the debug key (symbol 'Debug_4' in the keys.cfg) is pressed.
void debugKey4Pressed (void);

// Forward time to the next event. If skipHEE is set, the event named
// HYPERSPACE_ENCOUNTER_EVENT, which normally occurs every game day,
// is skipped. Must be called on the Starcon2Main thread.
void forwardToNextEvent (BOOLEAN skipHEE);
// Generate a list of all events in the event queue.
// Must be called on the Starcon2Main thread.
void dumpEvents (FILE *out);
// Describe one event.
void dumpEvent (FILE *out, const EVENT *eventPtr);
// Get the name of one event.
const char *eventName (BYTE func_index);

// Give the flagship a decent equipment for debugging.
void equipShip (void);
// Give the player all devices.
void giveDevices (void);

// Remove all escort ships.
void clearEscorts (void);

// Show all active spheres of influence.
void showSpheres (void);

// Make the ships of all races available for building at the shipyard.
void activateAllShips (void);

// Call a function for all stars.
void forAllStars (void (*callback) (STAR_DESC *, void *), void *arg);
// Call a function for all planets in a star system.
void forAllPlanets (STAR_DESC *star, SOLARSYS_STATE *system,
		void (*callback) (STAR_DESC *, SOLARSYS_STATE *, PLANET_DESC *,
		void *), void *arg);
// Call a function for all moons of a planet.
void forAllMoons (STAR_DESC *star, SOLARSYS_STATE *system, PLANET_DESC *planet,
		void (*callback) (STAR_DESC *, SOLARSYS_STATE *, PLANET_DESC *,
		PLANET_DESC *, void *), void *arg);

// Argument to UniverseRecurse()
typedef struct
{
	void (*systemFuncPre) (const STAR_DESC *star,
			const SOLARSYS_STATE *system, void *arg);
			// Called for each system prior to recursing to its planets.
	void (*systemFuncPost) (const STAR_DESC *star,
			const SOLARSYS_STATE *system, void *arg);
			// Called for each system after recursing to its planets.
	void (*planetFuncPre) (const PLANET_DESC *planet, void *arg);
			// Called for each planet prior to recursing to its moons.
	void (*planetFuncPost) (const PLANET_DESC *planet, void *arg);
			// Called for each planet after recursing to its moons.
	void (*moonFunc) (const PLANET_DESC *moon, void *arg);
			// Called for each moon.
	void *arg;
			// User data.
} UniverseRecurseArg;
// Recurse through all systems, planets, and moons in the universe.
// Must be called on the Starcon2Main thread.
void UniverseRecurse (UniverseRecurseArg *universeRecurseArg);

// Describe the entire universe. Must be called on the Starcon2Main thread.
void dumpUniverse (FILE *out);
// Describe the entire universe, output to a file "./PlanetInfo".
// Must be called on the Starcon2Main thread.
void dumpUniverseToFile (void);
// Describe one star system.
void dumpSystem (FILE *out, const STAR_DESC *star,
		const SOLARSYS_STATE *system);
// Get a star color as a string.
const char *bodyColorString (BYTE col);
// Get a star type as a string.
const char *starTypeString (BYTE type);
// Get a string describing special presence in the star system.
const char *starPresenceString (BYTE index);
// Get a list describing all planets in a star.
void dumpPlanets (FILE *out, const STAR_DESC *star);
// Describe one planet.
void dumpPlanet(FILE *out, const PLANET_DESC *planet);
// Describe one moon.
void dumpMoon (FILE *out, const PLANET_DESC *moon);
// Calculate the total value of all minerals on a world.
COUNT calculateMineralValue (const SOLARSYS_STATE *system,
		const PLANET_DESC *world);
// Determine how much of each mineral type is present on a world
void generateMineralIndex(const SOLARSYS_STATE *system,
		const PLANET_DESC *world, COUNT minerals[]);
// Calculate the total value of all bio on a world.
COUNT calculateBioValue (const SOLARSYS_STATE *system,
		const PLANET_DESC *world);
// Determine how much of each mineral type is present on a world
void generateBioIndex(const SOLARSYS_STATE *system,
		const PLANET_DESC *world, COUNT bio[]);

// Tally the resources for each star system.
// Must be called on the Starcon2Main thread.
void tallyResources (FILE *out);
// Tally the resources for each star system, output to a file
// "./ResourceTally". Must be called on the Starcon2Main thread.
void tallyResourcesToFile (void);


// Call a function for all planet types.
void forAllPlanetTypes (void (*callBack) (int, const PlanetFrame *,
		void *), void *arg);
// Describe one planet type.
void dumpPlanetType(FILE *out, int index, const PlanetFrame *planetFrame);
// Generate a list of all planet types.
void dumpPlanetTypes(FILE *out);
// Get a string describing a planet type.
const char *planetTypeString (int typeIndex);
// Get a string describing the size of a type of planet.
const char *worldSizeString (BYTE size);
// Get a string describing a planet type map generation algoritm.
const char *worldGenAlgoString (BYTE algo);
// Get a string describing the severity of a tectonics on a type of planet.
const char *tectonicsString (BYTE tectonics);
// Get a string describing the atmospheric pressure on a type of planet.
const char *atmosphereString (BYTE atmosphere);
// Get a string describing the density of a type of planet.
const char *densityString (BYTE density);

// Get a string describing the quality of a deposit.
const char *depositQualityString (BYTE quality);

// Resets the crew of the first player (the bottom one) to its maximum.
void resetCrewBattle(void);

// Move instantly across hyperspace/quasispace.
extern BOOLEAN instantMove;


// Dump all game strings.
void dumpStrings(FILE *out);


// Graphically and textually show all the contexts.
// Must be called on the Starcon2Main thread.
void debugContexts (void);


// To add some day:
// - a function to fast forward the game clock to a specifiable time.

#endif  /* UQM_UQMDEBUG_H_ */

