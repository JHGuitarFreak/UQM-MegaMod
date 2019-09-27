/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UQM_SUPERMELEE_NETPLAY_NETSTATE_H_
#define UQM_SUPERMELEE_NETPLAY_NETSTATE_H_

#include "port.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NetConnectionStateData NetConnectionStateData;

// State of a NetConnection.
typedef enum {
	NetState_unconnected,    /* No connection initiated */
	NetState_connecting,     /* Connection being setup */
	NetState_init,           /* Initialising the connection */
	NetState_inSetup,        /* In the network game setup */
	NetState_preBattle,      /* Pre-battle initialisations */
	NetState_interBattle,    /* Negotiations between battles. */
	NetState_selectShip,     /* Selecting a ship in battle */
	NetState_inBattle,       /* Battle has started */
	NetState_endingBattle,   /* Both sides are prepared to end */
	NetState_endingBattle2,  /* Waiting for the final synchronisation */
} NetState;

#if defined(__cplusplus)
}
#endif

#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	const char *name;
} NetStateData;
extern NetStateData netStateData[];

typedef void (*NetConnectionStateData_ReleaseFunction)(
		NetConnectionStateData *stateData);

#define NETCONNECTION_STATE_DATA_COMMON \
		NetConnectionStateData_ReleaseFunction releaseFunction;

struct
NetConnectionStateData {
	NETCONNECTION_STATE_DATA_COMMON
};

void NetConnectionStateData_release(NetConnectionStateData *stateData);

static inline bool
NetState_battleActive(NetState state) {
	return state == NetState_inBattle || state == NetState_endingBattle ||
			state == NetState_endingBattle2;
}


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_NETSTATE_H_ */

