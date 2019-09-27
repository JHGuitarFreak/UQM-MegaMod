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

#include "netplay.h"
#include "netstate.h"

#include <assert.h>
#include <stdlib.h>


#define DEFINE_NETSTATEDATA(stateName) \
	{ \
		/* .name = */    #stateName, \
	}
NetStateData netStateData[] = {
	DEFINE_NETSTATEDATA(unconnected),
	DEFINE_NETSTATEDATA(connecting),
	DEFINE_NETSTATEDATA(init),
	DEFINE_NETSTATEDATA(inSetup),
	DEFINE_NETSTATEDATA(preBattle),
	DEFINE_NETSTATEDATA(interBattle),
	DEFINE_NETSTATEDATA(selectShip),
	DEFINE_NETSTATEDATA(inBattle),
	DEFINE_NETSTATEDATA(endingBattle),
	DEFINE_NETSTATEDATA(endingBattle2),
};

void
NetConnectionStateData_release(NetConnectionStateData *stateData) {
	assert(stateData->releaseFunction != NULL);
	stateData->releaseFunction(stateData);
}

