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

	// Player Status
	{
		ImGui_SeparatorText ("Player Status");

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

		ImGui_NewLine ();
	}

	// Lander Upgrades
	{
		BYTE ShieldFlags = GET_GAME_STATE (LANDER_SHIELDS);
		bool QuakeShield = ShieldFlags & (1 << EARTHQUAKE_DISASTER);
		bool BioShield = ShieldFlags & (1 << BIOLOGICAL_DISASTER);
		bool LghtngShield = ShieldFlags & (1 << LIGHTNING_DISASTER);
		bool LavaShield = ShieldFlags & (1 << LAVASPOT_DISASTER);
		bool LanderShot = GET_GAME_STATE (IMPROVED_LANDER_SHOT);
		bool LanderSpeed = GET_GAME_STATE (IMPROVED_LANDER_SPEED);
		bool LanderCargo = GET_GAME_STATE (IMPROVED_LANDER_CARGO);

		ImGui_SeparatorText ("Lander Upgrades");

		if (ImGui_BeginTable ("##Upgrades", 2, 0))
		{
			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Quake", &QuakeShield))
			{
				ShieldFlags ^= 1 << EARTHQUAKE_DISASTER;
				SET_GAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("BIO", &BioShield))
			{
				ShieldFlags ^= 1 << BIOLOGICAL_DISASTER;
				SET_GAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Lightning", &LghtngShield))
			{
				ShieldFlags ^= 1 << LIGHTNING_DISASTER;
				SET_GAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Lava", &LavaShield))
			{
				ShieldFlags ^= 1 << LAVASPOT_DISASTER;
				SET_GAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Weapon", &LanderShot))
			{
				SET_GAME_STATE (IMPROVED_LANDER_SHOT, LanderShot);
			}

			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Speed", &LanderSpeed))
			{
				SET_GAME_STATE (IMPROVED_LANDER_SPEED, LanderSpeed);
			}

			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Cargo", &LanderCargo))
			{
				SET_GAME_STATE (IMPROVED_LANDER_CARGO, LanderCargo);
			}

			ImGui_EndTable ();
		}

		ImGui_NewLine ();
	}

	// Cargo Status
	{
		ImGui_SeparatorText ("Cargo Status");

		if (ImGui_BeginTable ("##Cargo", 2, 0))
		{
			const char *elements[8] =
			{
				"Common", "Corrosive", "Base Metal", "Noble",
				"Rare Earth", "Precious", "Radioactive", "Exotic"
			};
			int rem_capacity = GetStorageBayCapacity ()
					- GLOBAL_SIS (TotalElementMass);
			int i;

			for (i = 0; i < NUM_ELEMENT_CATEGORIES; i++)
			{
				char buf[40];

				int element[2] =
				{
					GLOBAL (ElementWorth[i]),
					GLOBAL_SIS (ElementAmounts[i])
				};

				snprintf (buf, sizeof (buf), "##%s", elements[i]);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text (elements[i]);

				ImGui_TableNextColumn ();

				if (ImGui_InputInt2 (buf, element, 0))
				{
					if (GLOBAL (ElementWorth[i]) != element[0])
						GLOBAL (ElementWorth[i]) = element[0];

					if (GLOBAL_SIS (ElementAmounts[i]) != element[1])
					{
						int temp = element[1]
								- GLOBAL_SIS (ElementAmounts[i]);

						GLOBAL_SIS (ElementAmounts[i]) = element[1];
						GLOBAL_SIS (TotalElementMass) += temp;
					}
				}
			}

			{
				char buf[40];
				float total_capacity = (float)GLOBAL_SIS (TotalElementMass)
						/ (float)GetStorageBayCapacity ();

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text ("Free Space");

				ImGui_TableNextColumn ();

				snprintf (buf, sizeof buf, "%d", rem_capacity);
				ImGui_ProgressBar (total_capacity, (ImVec2) {0, 0}, buf);
			}

			{
				int BioData = GLOBAL_SIS (TotalBioMass);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text ("Bio-Data");

				ImGui_TableNextColumn ();

				if (ImGui_InputIntEx ("##BioData", &BioData, 0, 0, 0))
				{
					GLOBAL_SIS (TotalBioMass) = BioData;
				}
			}

			ImGui_EndTable ();
		}

		ImGui_NewLine ();
	}
}