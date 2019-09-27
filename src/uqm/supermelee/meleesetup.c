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

#define MELEESETUP_INTERNAL
#include "port.h"
#include "meleesetup.h"

#include "../master.h"
#include "libs/log.h"


///////////////////////////////////////////////////////////////////////////

// Temporary
const size_t MeleeTeam_serialSize = MELEE_FLEET_SIZE +
		sizeof (((MeleeTeam*)0)->name);

void
MeleeTeam_init (MeleeTeam *team)
{
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
		team->ships[slotI] = MELEE_NONE;

	team->name[0] = '\0';
}

void
MeleeTeam_uninit (MeleeTeam *team)
{
	(void) team;
}

MeleeTeam *
MeleeTeam_new (void)
{
	MeleeTeam *result = HMalloc (sizeof (MeleeTeam));
	MeleeTeam_init (result);
	return result;
}

void
MeleeTeam_delete (MeleeTeam *team)
{
	MeleeTeam_uninit (team);
	HFree (team);
}

int
MeleeTeam_serialize (const MeleeTeam *team, uio_Stream *stream)
{
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++) {
		if (uio_putc ((int) team->ships[slotI], stream) == EOF)
			return -1;
	}
	if (uio_fwrite ((const char *) team->name, sizeof team->name, 1,
			stream) != 1)
		return -1;

	return 0;
}

int
MeleeTeam_deserialize (MeleeTeam *team, uio_Stream *stream)
{
	FleetShipIndex slotI;

	// Sanity check on the ships.
	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		int ship = uio_getc (stream);
		if (ship == EOF)
			goto err;
		team->ships[slotI] = (MeleeShip) ship;

		if (team->ships[slotI] == MELEE_NONE)
			continue;

		if (team->ships[slotI] >= NUM_MELEE_SHIPS)
		{
			log_add (log_Warning, "Invalid ship type in loaded team (index "
					"%d, ship type is %d, max valid is %d).",
					slotI, team->ships[slotI], NUM_MELEE_SHIPS - 1);
			team->ships[slotI] = MELEE_NONE;
		}
	}
	
	if (uio_fread (team->name, sizeof team->name, 1, stream) != 1)
		goto err;

	team->name[MAX_TEAM_CHARS] = '\0';

	return 0;

err:
	MeleeTeam_delete(team);
	return -1;
}

// XXX: move this to elsewhere?
COUNT
MeleeTeam_getValue (const MeleeTeam *team)
{
	COUNT total = 0;
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		MeleeShip ship = team->ships[slotI];
		COUNT shipValue = GetShipValue (ship);
		if (shipValue == (COUNT)~0)
		{
			// Invalid ship.
			continue;
		}
		total += shipValue;
	}

	return total;
}

MeleeShip
MeleeTeam_getShip (const MeleeTeam *team, FleetShipIndex slotNr)
{
	return team->ships[slotNr];
}

void
MeleeTeam_setShip (MeleeTeam *team, FleetShipIndex slotNr, MeleeShip ship)
{
	team->ships[slotNr] = ship;
}

const MeleeShip *
MeleeTeam_getFleet (const MeleeTeam *team)
{
	return team->ships;
}

const char *
MeleeTeam_getTeamName (const MeleeTeam *team)
{
	return team->name;
}

// Returns true iff the state has actually changed.
void
MeleeTeam_setName (MeleeTeam *team, const char *name)
{
	strncpy (team->name, name, sizeof team->name - 1);
	team->name[sizeof team->name - 1] = '\0';
}

void
MeleeTeam_copy (MeleeTeam *copy, const MeleeTeam *original)
{
	*copy = *original;
}

#if 0
bool
MeleeTeam_isEqual (const MeleeTeam *team1, const MeleeTeam *team2)
{
	const MeleeShip *fleet1;
	const MeleeShip *fleet2;
	FleetShipIndex slotI;

	if (strcmp (team1->name, team2->name) != 0)
		return false;

	fleet1 = team1->ships;
	fleet2 = team2->ships;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		if (fleet1[slotI] != fleet2[slotI])
			return false;
	}

	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////

#ifdef NETPLAY
static void
MeleeSetup_initSentTeam (MeleeSetup *setup, size_t teamNr)
{
	MeleeTeam *team = &setup->sentTeams[teamNr];
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
		MeleeTeam_setShip (team, slotI, MELEE_UNSET);

	setup->haveSentTeamName[teamNr] = false;
#ifdef DEBUG
	// The actual team name should be irrelevant if haveSentTeamName is
	// set to false. In a debug build, we set it to invalid, so that
	// it is more likely that it will be noticed if it is ever used.
	MeleeTeam_setName (team, "<INVALID>");
#endif  /* DEBUG */
}
#endif  /* NETPLAY */

MeleeSetup *
MeleeSetup_new (void)
{
	size_t teamI;
	MeleeSetup *result = HMalloc (sizeof (MeleeSetup));
	if (result == NULL)
		return NULL;

	for (teamI = 0; teamI < NUM_SIDES; teamI++)
	{
		MeleeTeam_init (&result->teams[teamI]);
		result->fleetValue[teamI] = 0;
#ifdef NETPLAY
		MeleeSetup_initSentTeam (result, teamI);
#endif  /* NETPLAY */
	}
	return result;
}

void
MeleeSetup_delete (MeleeSetup *setup)
{
	HFree (setup);
}

#ifdef NETPLAY
void
MeleeSetup_resetSentTeams (MeleeSetup *setup)
{
	size_t teamI;

	for (teamI = 0; teamI < NUM_SIDES; teamI++)
		MeleeSetup_initSentTeam (setup, teamI);
}
#endif  /* NETPLAY */

// Returns true iff the state has actually changed.
bool
MeleeSetup_setShip (MeleeSetup *setup, size_t teamNr, FleetShipIndex slotNr,
		MeleeShip ship)
{
	MeleeTeam *team = &setup->teams[teamNr];
	MeleeShip oldShip = MeleeTeam_getShip (team, slotNr);

	if (ship == oldShip)
		return false;

	if (oldShip != MELEE_NONE)
		setup->fleetValue[teamNr] -= GetShipCostFromIndex (oldShip);

	MeleeTeam_setShip (team, slotNr, ship);

	if (ship != MELEE_NONE)
		setup->fleetValue[teamNr] += GetShipCostFromIndex (ship);
		
	return true;
}

MeleeShip
MeleeSetup_getShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr)
{
	return MeleeTeam_getShip (&setup->teams[teamNr], slotNr);
}

const MeleeShip *
MeleeSetup_getFleet (const MeleeSetup *setup, size_t teamNr)
{
	return MeleeTeam_getFleet (&setup->teams[teamNr]);
}

// Returns true iff the state has actually changed.
bool
MeleeSetup_setTeamName (MeleeSetup *setup, size_t teamNr,
		const char *name)
{
	MeleeTeam *team = &setup->teams[teamNr];
	const char *oldName = MeleeTeam_getTeamName (team);

	if (strcmp (oldName, name) == 0)
		return false;

	MeleeTeam_setName (team, name);
	return true;
}

// NB. This function returns a pointer to a static buffer, which is
// overwritten by calls to MeleeSetup_setTeamName().
const char *
MeleeSetup_getTeamName (const MeleeSetup *setup, size_t teamNr)
{
	return MeleeTeam_getTeamName (&setup->teams[teamNr]);
}

COUNT
MeleeSetup_getFleetValue (const MeleeSetup *setup, size_t teamNr)
{
	return setup->fleetValue[teamNr];
}

int
MeleeSetup_deserializeTeam (MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream)
{
	MeleeTeam *team = &setup->teams[teamNr];
	int ret = MeleeTeam_deserialize (team, stream);
	if (ret == 0)
		setup->fleetValue[teamNr] = MeleeTeam_getValue (team);
	return ret;
}

int
MeleeSetup_serializeTeam (const MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream)
{
	const MeleeTeam *team = &setup->teams[teamNr];
	return MeleeTeam_serialize (team, stream);
}

#ifdef NETPLAY
MeleeShip
MeleeSetup_getSentShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr)
{
	return MeleeTeam_getShip (&setup->sentTeams[teamNr], slotNr);
}

// Returns NULL if there is no team name set. This is not the same
// as when an empty (zero-length) team name is set.
// NB. This function returns a pointer to a static buffer, which is
// overwritten by calls to MeleeSetup_setSentTeamName().
const char *
MeleeSetup_getSentTeamName (const MeleeSetup *setup, size_t teamNr)
{
	if (!setup->haveSentTeamName[teamNr])
		return NULL;

	return MeleeTeam_getTeamName (&setup->sentTeams[teamNr]);
}

// Returns true iff the state has actually changed.
bool
MeleeSetup_setSentShip (MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship)
{
	MeleeTeam *team = &setup->sentTeams[teamNr];
	MeleeShip oldShip = MeleeTeam_getShip (team, slotNr);

	if (ship == oldShip)
		return false;

	MeleeTeam_setShip (team, slotNr, ship);
	return true;
}

// Returns true iff the state has actually changed.
// 'name' can be NULL to indicate that no team name set. This is not the same
// as when an empty (zero-length) team name is set.
bool
MeleeSetup_setSentTeamName (MeleeSetup *setup, size_t teamNr,
		const char *name)
{
	bool haveSentName = setup->haveSentTeamName[teamNr];

	if (name == NULL)
	{
		if (!haveSentName)
		{
			// Had not sent a team name, and still haven't.
			return false;
		}

#ifdef DEBUG
		{
			// The actual team name should be irrelevant if haveSentTeamName
			// is set to false. In a debug build, we set it to invalid, so
			// that it is more likely that it will be noticed if it is ever
			// used.
			MeleeTeam *team = &setup->sentTeams[teamNr];
			MeleeTeam_setName (team, "<INVALID>");
		}
#endif
	}
	else
	{
		MeleeTeam *team = &setup->sentTeams[teamNr];

		if (haveSentName)
		{
			// Have sent a team name. Check whether it has actually changed.
			const char *oldName = MeleeTeam_getTeamName (team);
			if (strcmp (oldName, name) == 0)
				return false;  // Team name has not changed.
		}

		MeleeTeam_setName (team, name);
	}
		
	setup->haveSentTeamName[teamNr] = (name != NULL);

	return true;
}

#if 0
bool
MeleeSetup_isTeamSent (MeleeSetup *setup, size_t teamNr)
{
	MeleeTeam *localTeam = &setup->teams[teamNr];
	MeleeTeam *sentTeam = &setup->sentTeams[teamNr];

	return MeleeTeam_isEqual (localTeam, sentTeam);
}
#endif

#endif  /* NETPLAY */

///////////////////////////////////////////////////////////////////////////


