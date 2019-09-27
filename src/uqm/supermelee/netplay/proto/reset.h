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

#ifndef UQM_SUPERMELEE_NETPLAY_PROTO_RESET_H_
#define UQM_SUPERMELEE_NETPLAY_PROTO_RESET_H_

#include "../netconnection.h"
#include "../packet.h"

#if defined(__cplusplus)
extern "C" {
#endif

void Netplay_setResetCallback(NetConnection *conn,
		NetConnection_ResetCallback callback, void *resetArg);
void Netplay_localReset(NetConnection *conn, NetplayResetReason reason);
void Netplay_remoteReset(NetConnection *conn, NetplayResetReason reason);
bool Netplay_isLocalReset(const NetConnection *conn);
bool Netplay_isRemoteReset(const NetConnection *conn);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_PROTO_RESET_H_ */

