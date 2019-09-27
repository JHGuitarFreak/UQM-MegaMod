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

// This file is part of netmanager_bsd.ci and netmanager_win.ci,
// from where it is #included.

static bool
NetManager_doReadCallback(NetDescriptor *nd) {
	bool closed;

	assert(nd->readCallback != NULL);
	NetDescriptor_incRef(nd);
	(*nd->readCallback)(nd);
	closed = nd->flags.closed;
	NetDescriptor_decRef(nd);
	return closed;
}

static bool
NetManager_doWriteCallback(NetDescriptor *nd) {
	bool closed;

	assert(nd->writeCallback != NULL);
	NetDescriptor_incRef(nd);
	(*nd->writeCallback)(nd);
	closed = nd->flags.closed;
	NetDescriptor_decRef(nd);
	return closed;
}

static bool
NetManager_doExceptionCallback(NetDescriptor *nd) {
	bool closed;

	assert(nd->exceptionCallback != NULL);
	NetDescriptor_incRef(nd);
	(*nd->exceptionCallback)(nd);
	closed = nd->flags.closed;
	NetDescriptor_decRef(nd);
	return closed;
}


