//Copyright Paul Reiche, Fred Ford. 1992-2002
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

#include "uqm_imgui.h"

void draw_status_menu (void)
{
	ImGui_ColumnsEx (DISPLAY_BOOL, "StatusColumns", false);

	{	// Edit Captain's Name
		char CaptainsName[SIS_NAME_SIZE];

		snprintf ((char *)&CaptainsName, sizeof (CaptainsName),
			"%s", GLOBAL_SIS (CommanderName));

		ImGui_Text ("Captain's Name:");
		if (ImGui_InputText ("##CaptainsName", CaptainsName,
			sizeof (CaptainsName), 0))
		{
			snprintf (GLOBAL_SIS (CommanderName),
				sizeof (GLOBAL_SIS (CommanderName)),
				"%s", CaptainsName);
		}
	}

	{	// Edit Ship Name
		char SISName[SIS_NAME_SIZE];

		snprintf ((char *)&SISName, sizeof (SISName),
			"%s", GLOBAL_SIS (ShipName));

		ImGui_Text ("Ship Name:");
		if (ImGui_InputText ("##SISName", SISName, sizeof (SISName), 0))
		{
			snprintf (GLOBAL_SIS (ShipName),
				sizeof (GLOBAL_SIS (ShipName)),
				"%s", SISName);
		}
	}

	ImGui_Text ("Current R.U.:");
	ImGui_InputInt ("##CurrentRU", (int *)&GLOBAL_SIS (ResUnits));

	{
		int CurrentFuel = GLOBAL_SIS (FuelOnBoard);
		int volume = FUEL_RESERVE +
			((DWORD)CountSISPieces (FUEL_TANK)
				* FUEL_TANK_CAPACITY
				+ (DWORD)CountSISPieces (HIGHEFF_FUELSYS)
				* HEFUEL_TANK_CAPACITY);

		ImGui_Text ("Current Fuel:");
		if (ImGui_InputInt ("##CurrentFuel", &CurrentFuel))
		{
			if (CurrentFuel > volume)
				CurrentFuel = volume;
			if (CurrentFuel < 0)
				CurrentFuel = 0;

			GLOBAL_SIS (FuelOnBoard) = CurrentFuel;
		}
	}

	{
		int Credits = MAKE_WORD (GET_GAME_STATE (MELNORME_CREDIT0),
			GET_GAME_STATE (MELNORME_CREDIT1));

		ImGui_Text ("Current Credits:");
		if (ImGui_InputInt ("##CurrentCredits", &Credits))
		{
			if (Credits > (uint16)~0)
				Credits = (uint16)~0;
			if (Credits < 0)
				Credits = 0;

			SET_GAME_STATE (MELNORME_CREDIT0, LOBYTE (Credits));
			SET_GAME_STATE (MELNORME_CREDIT1, HIBYTE (Credits));
		}
	}
}