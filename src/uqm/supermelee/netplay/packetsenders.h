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

#ifndef UQM_SUPERMELEE_NETPLAY_PACKETSENDERS_H_
#define UQM_SUPERMELEE_NETPLAY_PACKETSENDERS_H_

#include "types.h"

#include "netconnection.h"
#include "packet.h"

#include "../../controls.h"
		// for BATTLE_INPUT_STATE
#include "../meleeship.h"
		// for MeleeShip
#include "../meleesetup.h"
		// for FleetShipIndex

#if defined(__cplusplus)
extern "C" {
#endif

void sendInit(NetConnection *conn);
void sendPing(NetConnection *conn, uint32 id);
void sendAck(NetConnection *conn, uint32 id);
void sendReady(NetConnection *conn);
void sendHandshake0(NetConnection *conn);
void sendHandshake1(NetConnection *conn);
void sendHandshakeCancel(NetConnection *conn);
void sendHandshakeCancelAck(NetConnection *conn);
void sendTeamName(NetConnection *conn, NetplaySide side,
		const char *name, size_t len);
void sendFleet(NetConnection *conn, NetplaySide side,
		const MeleeShip *ships, size_t numShips);
void sendFleetShip(NetConnection *conn, NetplaySide player,
		FleetShipIndex shipIndex, MeleeShip ship);
void sendSeedRandom(NetConnection *conn, uint32 seed);
void sendInputDelay(NetConnection *conn, uint32 delay);
void sendSelectShip(NetConnection *conn, FleetShipIndex index);
void sendBattleInput(NetConnection *conn, BATTLE_INPUT_STATE input);
void sendFrameCount(NetConnection *conn, BattleFrameCounter frameCount);
void sendChecksum(NetConnection *conn, BattleFrameCounter frameNr,
		Checksum checksum);
void sendAbort(NetConnection *conn, NetplayAbortReason reason);
void sendReset(NetConnection *conn, NetplayResetReason reason);


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_PACKETSENDERS_H_ */


