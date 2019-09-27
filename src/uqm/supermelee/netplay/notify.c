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

// This files contains functions that notify the other side of local
// changes.

#define NETCONNECTION_INTERNAL
#include "netplay.h"
#include "notify.h"

#include "packetsenders.h"


// Convert a local player number to a side indication relative to this
// party.
static inline NetplaySide
netSide(NetConnection *conn, int side) {
	if (side == conn->player)
		return NetplaySide_remote;

	return NetplaySide_local;
}

void
Netplay_Notify_shipSelected(NetConnection *conn, FleetShipIndex index) {
	assert(NetConnection_getState(conn) == NetState_selectShip);

	sendSelectShip(conn, index);
}

void
Netplay_Notify_battleInput(NetConnection *conn, BATTLE_INPUT_STATE input) {
	assert(NetConnection_getState(conn) == NetState_inBattle ||
			NetConnection_getState(conn) == NetState_endingBattle ||
			NetConnection_getState(conn) == NetState_endingBattle2);

	sendBattleInput(conn, input);
}

void
Netplay_Notify_setTeamName(NetConnection *conn, int player,
		const char *name, size_t len) {
	assert(NetConnection_getState(conn) == NetState_inSetup);
	assert(!conn->stateFlags.handshake.localOk);
	
	sendTeamName(conn, netSide(conn, player), name, len);
}

// On initialisation, or load.
void
Netplay_Notify_setFleet(NetConnection *conn, int player,
		const MeleeShip *fleet, size_t fleetSize) {
	assert(NetConnection_getState(conn) == NetState_inSetup);
	assert(!conn->stateFlags.handshake.localOk);

	sendFleet(conn, netSide(conn, player), fleet, fleetSize);
}

void
Netplay_Notify_setShip(NetConnection *conn, int player,
		FleetShipIndex index, MeleeShip ship) {
	assert(NetConnection_getState(conn) == NetState_inSetup);
	assert(!conn->stateFlags.handshake.localOk);

	sendFleetShip(conn, netSide(conn, player), index, ship);
}

void
Netplay_Notify_seedRandom(NetConnection *conn, uint32 seed) {
	assert(NetConnection_getState(conn) == NetState_preBattle);

	sendSeedRandom(conn, seed);
	conn->stateFlags.agreement.randomSeed = true;
}

void
Netplay_Notify_inputDelay(NetConnection *conn, uint32 delay) {
	assert(NetConnection_getState(conn) == NetState_preBattle);

	sendInputDelay(conn, delay);
}

void
Netplay_Notify_frameCount(NetConnection *conn,
		BattleFrameCounter frameCount) {
	assert(NetConnection_getState(conn) == NetState_endingBattle);

	sendFrameCount(conn, frameCount);
}

#ifdef NETPLAY_CHECKSUM
void
Netplay_Notify_checksum(NetConnection *conn, BattleFrameCounter frameNr,
		Checksum checksum) {
	assert(NetState_battleActive(NetConnection_getState(conn)));

	sendChecksum(conn, frameNr, checksum);
}
#endif  /* NETPLAY_CHECKSUM */




