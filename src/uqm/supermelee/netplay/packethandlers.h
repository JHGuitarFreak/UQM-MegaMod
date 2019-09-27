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

#ifndef UQM_SUPERMELEE_NETPLAY_PACKETHANDLERS_H_
#define UQM_SUPERMELEE_NETPLAY_PACKETHANDLERS_H_

#include "packet.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define DECLARE_PACKETHANDLER(type) \
		int PacketHandler_##type(NetConnection *conn, \
				const Packet_##type *packet)

DECLARE_PACKETHANDLER(Init);
DECLARE_PACKETHANDLER(Ping);
DECLARE_PACKETHANDLER(Ack);
DECLARE_PACKETHANDLER(Ready);
DECLARE_PACKETHANDLER(Fleet);
DECLARE_PACKETHANDLER(TeamName);
DECLARE_PACKETHANDLER(Handshake0);
DECLARE_PACKETHANDLER(Handshake1);
DECLARE_PACKETHANDLER(HandshakeCancel);
DECLARE_PACKETHANDLER(HandshakeCancelAck);
DECLARE_PACKETHANDLER(SeedRandom);
DECLARE_PACKETHANDLER(InputDelay);
DECLARE_PACKETHANDLER(SelectShip);
DECLARE_PACKETHANDLER(BattleInput);
DECLARE_PACKETHANDLER(FrameCount);
DECLARE_PACKETHANDLER(Checksum);
DECLARE_PACKETHANDLER(Abort);
DECLARE_PACKETHANDLER(Reset);


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_PACKETHANDLERS_H_ */
