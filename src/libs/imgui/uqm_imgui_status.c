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

#include "uqm/races.h"

void draw_status_menu (void)
{
	bool in_main_menu = GLOBAL (CurrentActivity) == 0;

	ImGui_ColumnsEx (DISPLAY_BOOL, "StatusColumns", false);

	// Player Status
	{
		ImGui_SeparatorText ("Player Status");

		{	// Edit Captain's Name
			char CaptainsName[SIS_NAME_SIZE];

			snprintf ((char *)&CaptainsName, sizeof (CaptainsName),
				"%s", GLOBAL_SIS (CommanderName));

			ImGui_Text ("Captain's Name:");
			ImGui_InputText ("##CaptainsName", CaptainsName,
					sizeof (CaptainsName), 0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& strlen(CaptainsName) < SIS_NAME_SIZE)
			{
				snprintf (GLOBAL_SIS (CommanderName),
						sizeof (GLOBAL_SIS (CommanderName)),
						"%s", CaptainsName);

				//DrawCaptainsName (FALSE); Not Yet
			}
		}

		{	// Edit Ship Name
			char SISName[SIS_NAME_SIZE];

			snprintf ((char *)&SISName, sizeof (SISName),
				"%s", GLOBAL_SIS (ShipName));

			ImGui_Text ("Ship Name:");
			ImGui_InputText ("##SISName", SISName, sizeof (SISName), 0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& strlen(SISName) < SIS_NAME_SIZE)
			{
				snprintf (GLOBAL_SIS (ShipName),
						sizeof (GLOBAL_SIS (ShipName)),
						"%s", SISName);

				//DrawFlagshipName (TRUE, FALSE); Not Yet
			}
		}

		{
			DWORD curr_ru = GLOBAL_SIS (ResUnits);

			ImGui_Text ("Current R.U.:");
			//ImGui_InputInt ("##CurrentRU", (int *)&GLOBAL_SIS (ResUnits));
			ImGui_InputScalar ("##CurrentRU", ImGuiDataType_U32, &curr_ru);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& curr_ru > 0 && curr_ru < (DWORD)~0)
			{
				GLOBAL_SIS (ResUnits) = curr_ru;
			}
		}

		{
			int CurrentFuel = GLOBAL_SIS (FuelOnBoard);
			int volume = FUEL_RESERVE +
				((DWORD)CountSISPieces (FUEL_TANK)
					* FUEL_TANK_CAPACITY
					+ (DWORD)CountSISPieces (HIGHEFF_FUELSYS)
					* HEFUEL_TANK_CAPACITY);

			ImGui_Text ("Current Fuel:");
			ImGui_InputInt ("##CurrentFuel", &CurrentFuel);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& CurrentFuel > 0 && CurrentFuel < (DWORD)~0)
			{
				if (CurrentFuel > volume)
					CurrentFuel = volume;

				GLOBAL_SIS (FuelOnBoard) = CurrentFuel;
			}
		}

		{
			int Credits = MAKE_WORD (GET_GAME_STATE (MELNORME_CREDIT0),
				GET_GAME_STATE (MELNORME_CREDIT1));

			ImGui_Text ("Current Credits:");
			ImGui_InputInt ("##CurrentCredits", &Credits);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& Credits < (COUNT)~0 && Credits > 0)
			{
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

				ImGui_InputInt2 (buf, element, 0);
				if (ImGui_IsItemDeactivatedAfterEdit ()
						&& element[0] < (BYTE)~0 && element[1] < (COUNT)~0)
				{
					if (GLOBAL (ElementWorth[i]) != element[0])
						GLOBAL (ElementWorth[i]) = element[0];

					if (GLOBAL_SIS (ElementAmounts[i]) != element[1])
					{
						int mass = GLOBAL_SIS (TotalElementMass);
						int cap = GetStorageBayCapacity ();
						int temp = element[1]
							- GLOBAL_SIS (ElementAmounts[i]);

						if (mass + temp <= cap)
						{
							GLOBAL_SIS (ElementAmounts[i]) = element[1];
							GLOBAL_SIS (TotalElementMass) += temp;
						}
					}
				}
			}

			{
				char buf[40];
				float capacity_filled =
						(float)GLOBAL_SIS (TotalElementMass)
						/ (float)GetStorageBayCapacity ();
				int rem_capacity = GetStorageBayCapacity ()
						- GLOBAL_SIS (TotalElementMass);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text ("Free Space");

				ImGui_TableNextColumn ();

				snprintf (buf, sizeof buf, "%d", rem_capacity);
				ImGui_ProgressBar (capacity_filled, (ImVec2) {0, 0}, buf);
			}

			{
				int BioData = GLOBAL_SIS (TotalBioMass);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text ("Bio-Data");

				ImGui_TableNextColumn ();

				ImGui_InputIntEx ("##BioData", &BioData, 0, 0, 0);
				if (ImGui_IsItemDeactivatedAfterEdit ()
						&& BioData < (COUNT)~0)
				{
					GLOBAL_SIS (TotalBioMass) = BioData;
				}
			}

			ImGui_EndTable ();
		}

		ImGui_NewLine ();
	}

	// Module Status
	{
		ImGui_SeparatorText ("Module Status");

		if (ImGui_BeginTable ("##Modules", 2, 0))
		{
			int i;
			const char *modules[NUM_PURCHASE_MODULES] =
			{
				"Lander", "Fusion Thruster", "Turning Jets", "Crew Pod",
				"Storage Bay", "Fuel Tank", "High-Eff Fuel Sys",
				"Dynamo Unit", "Shiva Furnace", "Ion-Bolt Gun",
				"Fusion Blaster", "Hellbore Cannon", "Tracking System",
				"Point Defense Laser"
			};

			for (i = 0; i < NUM_PURCHASE_MODULES; i++)
			{
				char buf[40];

				int module_cost =
						GLOBAL (ModuleCost[i]) * MODULE_COST_SCALE;

				snprintf (buf, sizeof (buf), "##%s", modules[i]);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text (modules[i]);

				ImGui_TableNextColumn ();

				ImGui_InputIntEx (buf, &module_cost, 0, 0, 0);
				if (ImGui_IsItemDeactivatedAfterEdit ()
						&& module_cost < 12750)
				{
					GLOBAL (ModuleCost[i]) =
							module_cost / MODULE_COST_SCALE;
				}
			}

			ImGui_EndTable ();
		}

		ImGui_NewLine ();
	}

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	// Alien Status
	if (!in_main_menu)
	{
		ImGui_SeparatorText ("Alien Status");

		if (ImGui_BeginTable ("##Aliens", 2, 0))
		{
			COUNT Index;
			HFLEETINFO hStarShip, hNextShip;

			const char *allied_states[3] =
					{ "Dead Guy", "Good Guy", "Bad Guy" };

			Index = 0;
			for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
					hStarShip; hStarShip = hNextShip)
			{
				FLEET_INFO *FleetPtr;
				int race_state;
				bool ship_buildable;
				char buf[40];

				if (Index == NUM_BUILDABLE_SHIPS)
					break;

				FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ImGui_Text (GetStringAddress (SetAbsStringTableIndex (
						FleetPtr->race_strings, 0)));

				ImGui_TableNextColumn ();

				race_state = FleetPtr->allied_state;
				snprintf (buf, sizeof buf, "##AlliedState%d", Index);

				if (ImGui_ComboChar (buf, &race_state, allied_states, 3))
				{
					FleetPtr->allied_state = race_state;
				}

				ImGui_SameLine ();

				ship_buildable = FleetPtr->can_build;
				snprintf (buf, sizeof buf, "##Buildable%d", Index);

				if (ImGui_Checkbox (buf, &ship_buildable))
				{
					FleetPtr->can_build = ship_buildable;
				}

				Index++;
				hNextShip = _GetSuccLink (FleetPtr);
				UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
			}

			ImGui_EndTable ();
		}

		ImGui_NewLine ();
	}
}