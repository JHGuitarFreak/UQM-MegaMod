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

#include "notifyall.h"

#include "netmelee.h"
#include "notify.h"

// Notify the network connections of a team name change.
void
Netplay_NotifyAll_setTeamName (MELEE_STATE *pMS, size_t playerNr)
{
	const char *name;
	size_t len;
	size_t playerI;

	name = MeleeSetup_getTeamName (pMS->meleeSetup, playerNr);
	len = strlen (name);
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		if (NetConnection_getState (conn) != NetState_inSetup)
			continue;

		Netplay_Notify_setTeamName (conn, playerNr, name, len);
	}
}

// Notify the network connections of the configuration of a fleet.
void
Netplay_NotifyAll_setFleet (MELEE_STATE *pMS, size_t playerNr)
{
	MeleeSetup *setup = pMS->meleeSetup;
	const MeleeShip *ships = MeleeSetup_getFleet (setup, playerNr);
	size_t playerI;

	for (playerI = 0; playerI < NUM_PLAYERS; playerI++) {
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		if (NetConnection_getState (conn) != NetState_inSetup)
			continue;

		Netplay_Notify_setFleet (conn, playerNr, ships, MELEE_FLEET_SIZE);
	}
}

// Notify the network of a change in the configuration of a fleet.
void
Netplay_NotifyAll_setShip (MELEE_STATE *pMS, size_t playerNr, size_t index)
{
	MeleeSetup *setup = pMS->meleeSetup;
	MeleeShip ship = MeleeSetup_getShip (setup, playerNr, index);

	size_t playerI;
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		if (NetConnection_getState (conn) != NetState_inSetup)
			continue;

		Netplay_Notify_setShip (conn, playerNr, index, ship);
	}
}

static bool
Netplay_NotifyAll_inputDelayCallback(NetConnection *conn, void *arg) {
	const size_t *delay = (size_t *) arg;
	Netplay_Notify_inputDelay(conn, *delay);
	return true;
}

bool
Netplay_NotifyAll_inputDelay(size_t delay) {
	return forEachConnectedPlayer(Netplay_NotifyAll_inputDelayCallback,
			&delay);
}

#ifdef NETPLAY_CHECKSUM
void
Netplay_NotifyAll_checksum(BattleFrameCounter frameNr, Checksum checksum) {
	COUNT player;

	for (player = 0; player < NUM_PLAYERS; player++)
	{
		NetConnection *conn = netConnections[player];
		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected(conn))
			continue;

		Netplay_Notify_checksum(conn, frameNr, checksum);
	}
}
#endif  /* NETPLAY_CHECKSUM */

void
Netplay_NotifyAll_battleInput(BATTLE_INPUT_STATE input) {
	COUNT player;

	for (player = 0; player < NUM_PLAYERS; player++)
	{
		NetConnection *conn = netConnections[player];
		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected(conn))
			continue;

		Netplay_Notify_battleInput(conn, input);
	}
}

