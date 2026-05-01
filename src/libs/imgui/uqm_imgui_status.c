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
#include "uqm/gamestr.h"

static int collapse_player;
static int collapse_lander;
static int collapse_cargo;
static int collapse_module;
static int collapse_alien;
static int collapse_flagship;

static BYTE cached_drive_slots[NUM_DRIVE_SLOTS];
static BYTE cached_jet_slots[NUM_JET_SLOTS];
static BYTE cached_module_slots[NUM_MODULE_SLOTS];
bool slots_cached = false;
static int cached_landers;
bool landers_cached = false;

#define CHILD_FLAGS ImGuiChildFlags_AutoResizeY \
		| ImGuiChildFlags_AlwaysUseWindowPadding

#define TABLE_FLAGS ImGuiTableFlags_SizingStretchSame | \
					ImGuiTableFlags_PadOuterX

enum
{
	CACHE_SLOTS = 0,
	APPLY_SLOTS = 1
};

static void module_cache (BOOLEAN apply)
{
	if (apply)
	{
		memcpy (GLOBAL_SIS (DriveSlots), cached_drive_slots, NUM_DRIVE_SLOTS);
		memcpy (GLOBAL_SIS (JetSlots), cached_jet_slots, NUM_JET_SLOTS);
		memcpy (GLOBAL_SIS (ModuleSlots), cached_module_slots, NUM_MODULE_SLOTS);
		return;
	}

	memcpy (cached_drive_slots, GLOBAL_SIS (DriveSlots), NUM_DRIVE_SLOTS);
	memcpy (cached_jet_slots, GLOBAL_SIS (JetSlots), NUM_JET_SLOTS);
	memcpy (cached_module_slots, GLOBAL_SIS (ModuleSlots), NUM_MODULE_SLOTS);
}

void draw_status_menu (void)
{
	if (IN_MAIN_MENU)
	{
		ImGui_Text ("Status not available in the Main Menu...");
		return;
	}

	ImGui_ColumnsEx (DISPLAY_BOOL, "StatusColumns", false);

	if (DISPLAY_BOOL != 1)
		ImGui_BeginStyledChild ("##Column1", ZERO_F, CHILD_FLAGS, 0, NULL);

	// Player Status
	//if (ImGui_CollapsingHeader ("Player Status", collapse_player))
	{

		ImGui_SeparatorText ("Player Status");

		{
			DWORD curr_ru = GLOBAL_SIS (ResUnits);

			ImGui_Text ("Current R.U.:");
			ImGui_InputScalar ("##CurrentRU", ImGuiDataType_U32, &curr_ru);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& curr_ru > 0 && curr_ru < (DWORD)~0)
			{
				GLOBAL_SIS (ResUnits) = curr_ru;
			}
		}

		{
			int Credits = MAKE_WORD (
					GET_CGAME_STATE (MELNORME_CREDIT0),
					GET_CGAME_STATE (MELNORME_CREDIT1));

			ImGui_Text ("Current Credits:");
			ImGui_InputIntEx ("##CurrentCredits", &Credits,0,0,0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& Credits < (COUNT)~0 && Credits > 0)
			{
				SET_CGAME_STATE (MELNORME_CREDIT0, LOBYTE (Credits));
				SET_CGAME_STATE (MELNORME_CREDIT1, HIBYTE (Credits));
			}
		}

		ImGui_NewLine ();

		if (collapse_player == ImGuiTreeNodeFlags_None)
			collapse_player = ImGuiTreeNodeFlags_DefaultOpen;
	}
	//else if (collapse_player = ImGuiTreeNodeFlags_DefaultOpen)
	//	collapse_player = ImGuiTreeNodeFlags_None;

	// Lander Upgrades
	//if (ImGui_CollapsingHeader ("Lander Upgrades", collapse_lander))
	{
		BYTE ShieldFlags = GET_CGAME_STATE (LANDER_SHIELDS); // gs_cache.ShieldFlags;
		bool QuakeShield = ShieldFlags & (1 << EARTHQUAKE_DISASTER);
		bool BioShield = ShieldFlags & (1 << BIOLOGICAL_DISASTER);
		bool LghtngShield = ShieldFlags & (1 << LIGHTNING_DISASTER);
		bool LavaShield = ShieldFlags & (1 << LAVASPOT_DISASTER);
		bool LanderShot = GET_CGAME_STATE (IMPROVED_LANDER_SHOT); // gs_cache.LanderShot;
		bool LanderSpeed = GET_CGAME_STATE (IMPROVED_LANDER_SPEED); // gs_cache.LanderSpeed;
		bool LanderCargo = GET_CGAME_STATE (IMPROVED_LANDER_CARGO); // gs_cache.LanderCargo;

		ImGui_SeparatorText ("Lander Upgrades");

		if (ImGui_BeginTable ("##Upgrades", 2, TABLE_FLAGS))
		{
			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Quake", &QuakeShield))
			{
				ShieldFlags ^= 1 << EARTHQUAKE_DISASTER;
				SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("BIO", &BioShield))
			{
				ShieldFlags ^= 1 << BIOLOGICAL_DISASTER;
				SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Lightning", &LghtngShield))
			{
				ShieldFlags ^= 1 << LIGHTNING_DISASTER;
				SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Lava", &LavaShield))
			{
				ShieldFlags ^= 1 << LAVASPOT_DISASTER;
				SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
			}

			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Weapon", &LanderShot))
			{
				SET_CGAME_STATE (IMPROVED_LANDER_SHOT, LanderShot);
			}

			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Speed", &LanderSpeed))
			{
				SET_CGAME_STATE (IMPROVED_LANDER_SPEED, LanderSpeed);
			}

			ImGui_TableNextRow ();
			ImGui_TableNextColumn ();

			if (ImGui_Checkbox ("Cargo", &LanderCargo))
			{
				SET_CGAME_STATE (IMPROVED_LANDER_CARGO, LanderCargo);
			}

			ImGui_EndTable ();
		}

		ImGui_NewLine ();

		if (collapse_lander == ImGuiTreeNodeFlags_None)
			collapse_lander = ImGuiTreeNodeFlags_DefaultOpen;
	}
	//else if (collapse_lander = ImGuiTreeNodeFlags_DefaultOpen)
	//	collapse_lander = ImGuiTreeNodeFlags_None;

	// Cargo Status
	//if (ImGui_CollapsingHeader ("Cargo Status", collapse_cargo))
	{
		ImGui_SeparatorText ("Cargo Status");

		if (ImGui_BeginTable ("##Cargo", 2, TABLE_FLAGS))
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
				ImGui_ProgressBar (capacity_filled, ZERO_F, buf);
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

		if (collapse_cargo == ImGuiTreeNodeFlags_None)
			collapse_cargo = ImGuiTreeNodeFlags_DefaultOpen;
	}
	//else if (collapse_cargo = ImGuiTreeNodeFlags_DefaultOpen)
	//	collapse_cargo = ImGuiTreeNodeFlags_None;

	// Module Status
	//if (ImGui_CollapsingHeader ("Module Status", collapse_module))
	{
		ImGui_SeparatorText ("Module Status");

		if (ImGui_BeginTable ("##Modules", 2, TABLE_FLAGS))
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

		if (collapse_module == ImGuiTreeNodeFlags_None)
			collapse_module = ImGuiTreeNodeFlags_DefaultOpen;
	}
	//else if (collapse_module = ImGuiTreeNodeFlags_DefaultOpen)
	//	collapse_module = ImGuiTreeNodeFlags_None;

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_NextColumn ();
		ImGui_BeginStyledChild ("##Column2", ZERO_F, CHILD_FLAGS, 0, NULL);
	}

	// Alien Status
	//if (ImGui_CollapsingHeader ("Alien Status", collapse_alien))
	{
		ImGui_SeparatorText ("Alien Status");

		if (ImGui_BeginTable ("##Aliens", 2, TABLE_FLAGS))
		{
			HFLEETINFO hStarShip, hNextShip;
			FLEET_INFO *FleetPtr;
			bool ship_buildable;
			int race_state;

			const char *allied_states[3] = {
					"Dead", "Allied", "Enemy" };

#define COLUMN_FLAGS ImGuiTableColumnFlags_WidthFixed

			ImGui_TableSetupColumn ("Race", COLUMN_FLAGS);
			ImGui_TableSetupColumn ("Status", COLUMN_FLAGS);

			int Index = 0;
			for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
					hStarShip && Index < NUM_BUILDABLE_SHIPS;
					hStarShip = hNextShip, Index++)
			{
				FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);

				ImGui_PushIDInt (Index);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				ship_buildable = FleetPtr->can_build;

				if (ship_buildable)
				{
					ImGui_PushStyleColor (ImGuiCol_Button, 0xCC006600);
					ImGui_PushStyleColor (ImGuiCol_ButtonHovered, 0xCC008800);
					ImGui_PushStyleColor (ImGuiCol_ButtonActive, 0xCC004400);
				}

				if (ImGui_ButtonEx (GetStringAddress (
						SetAbsStringTableIndex (FleetPtr->race_strings, 0)), (ImVec2 ){110.0f, 0.0f }))
				{
					FleetPtr->can_build = !ship_buildable;
				}

				if (ship_buildable)
					ImGui_PopStyleColorEx (3);

				ImGui_TableNextColumn ();

				ImGui_SetNextItemWidth (95.0f);

				race_state = FleetPtr->allied_state;
				if (ImGui_ComboChar ("##AlliedState", &race_state, allied_states, 3))
				{
					FleetPtr->allied_state = race_state;
				}

				ImGui_PopID ();

				hNextShip = _GetSuccLink (FleetPtr);
				UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
			}

			ImGui_EndTable ();

			ImGui_TextWrappedColored (IV4_YELLOW_COLOR,
				"Clicking on the race name will allow you to build "
				"their ships regardless of alliance status.");
		}
	}
	//else if (collapse_alien = ImGuiTreeNodeFlags_DefaultOpen)
	//	collapse_alien = ImGuiTreeNodeFlags_None;

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_NextColumn ();
		ImGui_BeginStyledChild ("##Column3", ZERO_F, CHILD_FLAGS, 0, NULL);
	}

	// Flagship Status
	//if (ImGui_CollapsingHeader ("Flagship Status", collapse_flagship))
	{
		static bool thrusters[11] = { false };
		static bool jets[8] = { false };
		int something = 0;
		int num_m_slots = 16;
		int jt_begin = num_m_slots - 4;
		int i;
		const char *ship_modules[] =
		{
			"None", "Crew Pod",
			"Storage Bay", "Fuel Tank", "High-Eff Fuel Sys",
			"Dynamo Unit", "Shiva Furnace", "Ion-Bolt Gun",
			"Fusion Blaster", "Hellbore Cannon", "Tracking System",
			"Point Defense Laser", "Bomb Module 0", "Bomb Module 1",
			"Bomb Module 2", "Bomb Module 3", "Bomb Module 4",
			"Bomb Module 5"
		};

		ImGuiStyle *style = ImGui_GetStyle ();
		ImVec2 og_spacing = style->ItemSpacing;
		style->ItemSpacing = MAKE_IV2 (4, 4);

		ImGui_SeparatorText ("Flagship Status");

		// Current Coordinates
		{
			POINT universe;
			char buf[SIS_NAME_SIZE];

			universe = MAKE_POINT (LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
				LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)));

			snprintf (buf, sizeof buf, "%03u.%01u : %03u.%01u",
				universe.x / 10, universe.x % 10,
				universe.y / 10, universe.y % 10);

			ImGui_Text ("Coordinates:");
			ImGui_BeginDisabled (true);
			ImGui_InputText ("##Coordinates", buf, sizeof (buf), 0);
			ImGui_EndDisabled ();
		}

		//Current Location
		{
			char buf[256];

			switch (LOBYTE (GLOBAL (CurrentActivity)))
			{
			default:
			case IN_ENCOUNTER:
				buf[0] = '\0';
				break;
			case IN_LAST_BATTLE:
			case IN_INTERPLANETARY:
				GetClusterName (CurStarDescPtr, buf);
				break;
			case IN_HYPERSPACE:
				if (GET_CGAME_STATE (ARILOU_SPACE_SIDE) <= 1)
				{
					snprintf (buf, sizeof buf, "%s",
							GAME_STRING (NAVIGATION_STRING_BASE));
					// "HyperSpace"
				}
				else
				{
					POINT Log = MAKE_POINT (
						LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
						LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)));

					snprintf (buf, sizeof buf, "%s",
							GAME_STRING (NAVIGATION_STRING_BASE + 1));
					// "QuasiSpace"

					if (Log.x == ARILOU_HOME_X && Log.y == ARILOU_HOME_Y)
					{
						snprintf (buf, sizeof buf, "%s",
								GAME_STRING (STAR_STRING_BASE + 148));
						// "Falayalaralfali"
					}
				}
				break;
			}

			ImGui_Text ("Location:");
			ImGui_BeginDisabled (true);
			ImGui_InputText ("##Location", buf, sizeof (buf), 0);
			ImGui_EndDisabled ();
		}

		// Captain's Name
		{
			char CaptainsName[SIS_NAME_SIZE];

			snprintf ((char *)&CaptainsName, sizeof (CaptainsName),
				"%s", GLOBAL_SIS (CommanderName));

			ImGui_Text ("Captain's Name:");
			ImGui_InputText ("##CaptainsName", CaptainsName,
				sizeof (CaptainsName), 0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
				&& strlen (CaptainsName) < SIS_NAME_SIZE)
			{
				snprintf (GLOBAL_SIS (CommanderName),
					sizeof (GLOBAL_SIS (CommanderName)),
					"%s", CaptainsName);

				scr_refresh = true;
			}
		}

		// Ship Name
		{
			char SISName[SIS_NAME_SIZE];

			snprintf ((char *)&SISName, sizeof (SISName),
				"%s", GLOBAL_SIS (ShipName));

			ImGui_Text ("Ship Name:");
			ImGui_InputText ("##SISName", SISName, sizeof (SISName), 0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
				&& strlen (SISName) < SIS_NAME_SIZE)
			{
				snprintf (GLOBAL_SIS (ShipName),
					sizeof (GLOBAL_SIS (ShipName)),
					"%s", SISName);

				scr_refresh = true;
			}
		}

		// Landers
		{
			if (!landers_cached)
			{
				cached_landers = GLOBAL_SIS (NumLanders);
				landers_cached = true;
			}

			ImGui_Text ("Landers:");
			ImGui_InputInt ("##Landers", &cached_landers);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& cached_landers <= MAX_LANDERS && cached_landers >= 0)
			{
				GLOBAL_SIS (NumLanders) = cached_landers;

				scr_refresh = true;
				landers_cached = false;
			}
		}

		// Fuel
		{
			int CurrentFuel = GLOBAL_SIS (FuelOnBoard);
			int volume = GetFuelTankCapacity ();

			ImGui_Text ("Current Fuel:");
			if (optInfiniteFuel)
			{
				char buf[40];
				snprintf (buf, sizeof buf, "%s",
						GAME_STRING (STATUS_STRING_BASE + 2));
				ImGui_BeginDisabled (true);
				{
					ImGui_InputText ("##CurrentFuel", buf, sizeof buf, 0);
				}

				ImGui_EndDisabled ();
			}
			else
				ImGui_InputIntEx ("##CurrentFuel", &CurrentFuel,0,0,0);
			if (ImGui_IsItemDeactivatedAfterEdit ())
			{
				if (CurrentFuel > volume)
					CurrentFuel = volume;

				GLOBAL_SIS (FuelOnBoard) = CurrentFuel;

				scr_refresh = true;
			}
		}

		Spacer ();

		// Ship Modules
		if (!slots_cached)
		{
			module_cache (CACHE_SLOTS);
			slots_cached = true;
		}

		for (i = num_m_slots-1; i >= 0; i--)
		{
			char buf[40];
			int t_index = i - 1;
			int j_index = i - 4;

			if (i < jt_begin && t_index >= 0)
			{
				bool DriveSlot = cached_drive_slots[t_index] == 1;

				ImGui_PushStyleColor (ImGuiCol_CheckMark, U32_RED_COLOR);

				snprintf (buf, sizeof buf, "##thruster%d", t_index);
				if (ImGui_Checkbox (buf, &DriveSlot))
				{
					cached_drive_slots[t_index] =
							DriveSlot ? FUSION_THRUSTER : (EMPTY_SLOT + 0);
					module_cache (APPLY_SLOTS);
					scr_refresh = true;
				}

				ImGui_PopStyleColor ();
			}
			else
			{
				float checkbox_size = ImGui_GetFrameHeight ();
				ImGui_Dummy (MAKE_IV2 (checkbox_size, checkbox_size));
			}

			ImGui_SameLine ();

			if (i < jt_begin && j_index >= 0)
			{
				bool JetSlot = cached_jet_slots[j_index] == 2;

				ImGui_PushStyleColor (ImGuiCol_CheckMark, U32_GREEN_COLOR);

				snprintf (buf, sizeof buf, "##jet%d", j_index);
				if (ImGui_Checkbox (buf, &JetSlot))
				{
					cached_jet_slots[j_index] =
							JetSlot ? TURNING_JETS : (EMPTY_SLOT + 1);
					module_cache (APPLY_SLOTS);
					scr_refresh = true;
				}

				ImGui_PopStyleColor ();
			}
			else
			{
				float checkbox_size = ImGui_GetFrameHeight ();
				ImGui_Dummy (MAKE_IV2 (checkbox_size, checkbox_size));
			}

			ImGui_SameLine ();

			{
				bool gun_slots = i < 2 || i > 12;
				int ModuleSlot = cached_module_slots[i];

				if (ModuleSlot == EMPTY_SLOT + 2)
					ModuleSlot = 0;
				else
					ModuleSlot -= TURNING_JETS;

				if (gun_slots)
				{
					ImGui_PushStyleColor (ImGuiCol_FrameBg, U32_FRAMEBG_GS);
					ImGui_PushStyleColor (ImGuiCol_FrameBgHovered, U32_FRAMEBG_HOV_GS);
					ImGui_PushStyleColor (ImGuiCol_FrameBgActive, U32_FRAMEBG_ACT_GS);
					ImGui_PushStyleColor (ImGuiCol_Button, U32_BUTTON_GS);
					ImGui_PushStyleColor (ImGuiCol_ButtonHovered, U32_BUTTON_HOV_GS);
					ImGui_PushStyleColor (ImGuiCol_ButtonActive, U32_BUTTON_ACT_GS);
				}

				snprintf (buf, sizeof buf, "##module%d", i);
				if (ImGui_ComboChar (buf, &ModuleSlot, ship_modules, 18))
				{
					cached_module_slots[i] = ModuleSlot > 0 ?
							ModuleSlot + TURNING_JETS : (EMPTY_SLOT + 2);
					module_cache (APPLY_SLOTS);
					scr_refresh = true;
				}

				if (gun_slots)
					ImGui_PopStyleColorEx (6);
			}
		}

		style->ItemSpacing = og_spacing;

		Spacer ();

		// Crew
		{
			int CurrentCrew = GLOBAL_SIS (CrewEnlisted);
			int volume = GetCrewPodCapacity ();

			ImGui_Text ("Current Crew:");
			ImGui_InputIntEx ("##CurrentCrew", &CurrentCrew, 0, 0, 0);
			if (ImGui_IsItemDeactivatedAfterEdit ())
			{
				if (CurrentCrew > volume)
					CurrentCrew = volume;

				GLOBAL_SIS (CrewEnlisted) = CurrentCrew;

				scr_refresh = true;
			}
		}

		if (collapse_flagship == ImGuiTreeNodeFlags_None)
			collapse_flagship = ImGuiTreeNodeFlags_DefaultOpen;
	}
	//else if (collapse_flagship = ImGuiTreeNodeFlags_DefaultOpen)
	//	collapse_flagship = ImGuiTreeNodeFlags_None;

	if (DISPLAY_BOOL != 1)
		ImGui_EndChild ();
}