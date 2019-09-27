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

#ifndef UQM_SUPERMELEE_NETPLAY_NOTIFY_H_
#define UQM_SUPERMELEE_NETPLAY_NOTIFY_H_

#include "netplay.h"
		// for NETPLAY_CHECKSUM
#include "netconnection.h"
#include "../../controls.h"
		// for BATTLE_INPUT_STATE
#ifdef NETPLAY_CHECKSUM
#	include "checksum.h"
#endif
#include "../meleeship.h"
		// for MeleeShip
#include "../meleesetup.h"
		// for FleetShipIndex

#if defined(__cplusplus)
extern "C" {
#endif

void Netplay_Notify_shipSelected(NetConnection *conn, FleetShipIndex index);
void Netplay_Notify_battleInput(NetConnection *conn,
		BATTLE_INPUT_STATE input);
void Netplay_Notify_setTeamName(NetConnection *conn, int player,
		const char *name, size_t len);
void Netplay_Notify_setFleet(NetConnection *conn, int player,
		const MeleeShip *fleet, size_t fleetSize);
void Netplay_Notify_setShip(NetConnection *conn, int player,
		FleetShipIndex index, MeleeShip ship);
void Netplay_Notify_seedRandom(NetConnection *conn, uint32 seed);
void Netplay_Notify_inputDelay(NetConnection *conn, uint32 delay);
void Netplay_Notify_frameCount(NetConnection *conn,
		BattleFrameCounter frameCount);
#ifdef NETPLAY_CHECKSUM
void Netplay_Notify_checksum(NetConnection *conn,
		BattleFrameCounter frameCount, Checksum checksum);
#endif


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_NOTIFY_H_ */

