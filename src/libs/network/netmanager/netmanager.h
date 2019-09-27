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

#ifndef LIBS_NETWORK_NETMANAGER_NETMANAGER_H_
#define LIBS_NETWORK_NETMANAGER_NETMANAGER_H_

#include "port.h"
#include "types.h"

#ifdef USE_WINSOCK
#	include "netmanager_win.h"
#else
#	include "netmanager_bsd.h"
#endif

#include "ndesc.h"

void NetManager_init(void);
void NetManager_uninit(void);
int NetManager_process(uint32 *timeoutMs);

// Only for internal use by the NetManager:
int NetManager_addDesc(NetDescriptor *nd);
void NetManager_removeDesc(NetDescriptor *nd);
void NetManager_activateReadCallback(NetDescriptor *nd);
void NetManager_deactivateReadCallback(NetDescriptor *nd);
void NetManager_activateWriteCallback(NetDescriptor *nd);
void NetManager_deactivateWriteCallback(NetDescriptor *nd);
void NetManager_activateExceptionCallback(NetDescriptor *nd);
void NetManager_deactivateExceptionCallback(NetDescriptor *nd);

#endif  /* LIBS_NETWORK_NETMANAGER_NETMANAGER_H_ */

