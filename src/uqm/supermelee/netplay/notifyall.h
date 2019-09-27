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

#ifndef NOTIFYALL_H
#define NOTIFYALL_H

#include "../../battle.h"
#include "../../battlecontrols.h"
#include "../melee.h"
#ifdef NETPLAY_CHECKSUM
#	include "checksum.h"
#endif  /* NETPLAY_CHECKSUM */

#if defined(__cplusplus)
extern "C" {
#endif

void Netplay_NotifyAll_setTeamName (MELEE_STATE *pMS, size_t playerNr);
void Netplay_NotifyAll_setFleet (MELEE_STATE *pMS, size_t playerNr);
void Netplay_NotifyAll_setShip (MELEE_STATE *pMS, size_t playerNr,
		size_t index);

bool Netplay_NotifyAll_inputDelay(size_t delay);
#ifdef NETPLAY_CHECKSUM
void Netplay_NotifyAll_checksum(BattleFrameCounter frameNr,
		Checksum checksum);
#endif  /* NETPLAY_CHECKSUM */
void Netplay_NotifyAll_battleInput(BATTLE_INPUT_STATE input);


#if defined(__cplusplus)
}
#endif

#endif  /* NOTIFYALL_H */

