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

#ifndef MELEESETUP_H
#define MELEESETUP_H

typedef struct MeleeTeam MeleeTeam;
typedef struct MeleeSetup MeleeSetup;

#ifdef MELEESETUP_INTERNAL
#	define MELEETEAM_INTERNAL
#endif  /* MELEESETUP_INTERNAL */

#include "libs/compiler.h"

typedef COUNT FleetShipIndex;

#include "libs/uio.h"
#include "melee.h"
#include "meleeship.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef MELEETEAM_INTERNAL
struct MeleeTeam
{
	MeleeShip ships[MELEE_FLEET_SIZE];
	char name[MAX_TEAM_CHARS + 1 + 24];
			/* The +1 is for the terminating \0; the +24 is in case some
			 * default name in starcon.txt is unknowingly mangled. */
			// XXX: SvdB: Why would it be mangled? Why don't we just reject
			//            it if it is? Is this so that we have some space
			//            for multibyte UTF-8 chars?
};
#endif  /* MELEETEAM_INTERNAL */

#ifdef MELEESETUP_INTERNAL
struct MeleeSetup
{
	MeleeTeam teams[NUM_SIDES];
	COUNT fleetValue[NUM_SIDES];
#ifdef NETPLAY
	MeleeTeam sentTeams[NUM_SIDES];
			// The last sent (parts of) teams.
			// Used in the Update protocol. See doc/devel/netplay/protocol
			// XXX: this may actually be deallocated when the battle starts.
	bool haveSentTeamName[NUM_SIDES];
			// Whether we have sent a team name this 'turn'.
			// Used in the Update protocol. See doc/devel/netplay/protocol
			// (also for the term 'turn').
#endif
};

#endif  /* MELEESETUP_INTERNAL */

extern const size_t MeleeTeam_serialSize;

void MeleeTeam_init (MeleeTeam *team);
void MeleeTeam_uninit (MeleeTeam *team);
MeleeTeam *MeleeTeam_new (void);
void MeleeTeam_delete (MeleeTeam *team);
#ifdef NETPLAY
void MeleeSetup_resetSentTeams (MeleeSetup *setup);
#endif  /* NETPLAY */
int MeleeTeam_serialize (const MeleeTeam *team, uio_Stream *stream);
int MeleeTeam_deserialize (MeleeTeam *team, uio_Stream *stream);
COUNT MeleeTeam_getValue (const MeleeTeam *team);
MeleeShip MeleeTeam_getShip (const MeleeTeam *team, FleetShipIndex slotNr);
void MeleeTeam_setShip (MeleeTeam *team, FleetShipIndex slotNr,
		MeleeShip ship);
const MeleeShip *MeleeTeam_getFleet (const MeleeTeam *team);
const char *MeleeTeam_getTeamName (const MeleeTeam *team);
void MeleeTeam_setName (MeleeTeam *team, const char *name);
void MeleeTeam_copy (MeleeTeam *copy, const MeleeTeam *original);
#if 0
bool MeleeTeam_isEqual (const MeleeTeam *team1, const MeleeTeam *team2);
#endif

#ifdef NETPLAY
MeleeShip MeleeSetup_getSentShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr);
const char *MeleeSetup_getSentTeamName (const MeleeSetup *setup,
		size_t teamNr);
bool MeleeSetup_setSentShip (MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship);
bool MeleeSetup_setSentTeamName (MeleeSetup *setup, size_t teamNr,
		const char *name);
#if 0
bool MeleeSetup_isTeamSent (MeleeSetup *setup, size_t teamNr);
#endif
#endif  /* NETPLAY */

MeleeSetup *MeleeSetup_new (void);
void MeleeSetup_delete (MeleeSetup *setup);

bool MeleeSetup_setShip (MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship);
MeleeShip MeleeSetup_getShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr);
bool MeleeSetup_setFleet (MeleeSetup *setup, size_t teamNr,
		const MeleeShip *fleet);
const MeleeShip *MeleeSetup_getFleet (const MeleeSetup *setup, size_t teamNr);
bool MeleeSetup_setTeamName (MeleeSetup *setup, size_t teamNr,
		const char *name);
const char *MeleeSetup_getTeamName (const MeleeSetup *setup,
		size_t teamNr);
COUNT MeleeSetup_getFleetValue (const MeleeSetup *setup, size_t teamNr);
int MeleeSetup_deserializeTeam (MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream);
int MeleeSetup_serializeTeam (const MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream);


void MeleeState_setShip (MELEE_STATE *pMS, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship);
void MeleeState_setFleet (MELEE_STATE *pMS, size_t teamNr,
		const MeleeShip *fleet);
void MeleeState_setTeamName (MELEE_STATE *pMS, size_t teamNr,
		const char *name);
void MeleeState_setTeam (MELEE_STATE *pMS, size_t teamNr,
		const MeleeTeam *team);

#if defined(__cplusplus)
}
#endif

#endif  /* MELEESETUP_H */

