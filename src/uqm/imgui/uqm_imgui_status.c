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
#include "uqm/gameev.h"

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

#define TABLE_FLAGS ImGuiTableFlags_SizingStretchSame

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
		memcpy (GLOBAL_SIS (ModuleSlots), cached_module_slots,
				NUM_MODULE_SLOTS);
		return;
	}

	memcpy (cached_drive_slots, GLOBAL_SIS (DriveSlots), NUM_DRIVE_SLOTS);
	memcpy (cached_jet_slots, GLOBAL_SIS (JetSlots), NUM_JET_SLOTS);
	memcpy (cached_module_slots, GLOBAL_SIS (ModuleSlots), NUM_MODULE_SLOTS);
}

void draw_status_menu (void)
{
	static const char **lander_upgrades = NULL;
	static const char **elements        = NULL;
	static const char **modules         = NULL;
	static const char **allied_states   = NULL;
	static const char **ship_modules    = NULL;

	if (!lander_upgrades)
	{
		lander_upgrades = ImStrArr (DBG_STS_STR_BASE);
		elements        = ImStrArr (DBG_STS_STR_BASE + 1);
		modules         = ImStrArr (DBG_STS_STR_BASE + 2);
		allied_states   = ImStrArr (DBG_STS_STR_BASE + 3);
		ship_modules    = ImStrArr (DBG_STS_STR_BASE + 4);
	}
	
	ImGui_BeginStyledChild ("##Column1", content_col_size, CHILD_FLAGS,
			0, NULL);
	{

		{	// Currency Status
			ImGui_SeparatorText (ImStr (DBG_STS_STR_BASE + 5));

			{	// Current R.U.
				DWORD curr_ru = GLOBAL_SIS (ResUnits);

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 6)); // Current R.U.
				ImGui_InputScalar ("##CurrentRU", ImGuiDataType_U32, &curr_ru);
				if (ImGui_IsItemDeactivatedAfterEdit ()
						&& curr_ru > 0 && curr_ru < (DWORD)~0)
				{
					GLOBAL_SIS (ResUnits) = curr_ru;
				}
			}

			{	// Current Credits
				int Credits = MAKE_WORD (
						GET_CGAME_STATE (MELNORME_CREDIT0),
						GET_CGAME_STATE (MELNORME_CREDIT1));

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 7)); // Current Credits
				ImGui_InputIntEx ("##CurrentCredits", &Credits, 0, 0, 0);
				if (ImGui_IsItemDeactivatedAfterEdit ()
					&& Credits < (COUNT)~0 && Credits > 0)
				{
					SET_CGAME_STATE (MELNORME_CREDIT0, LOBYTE (Credits));
					SET_CGAME_STATE (MELNORME_CREDIT1, HIBYTE (Credits));
				}
			}

			ImGui_NewLine ();
		}

		{	// Lander Upgrade Status
			BYTE ShieldFlags = GET_CGAME_STATE (LANDER_SHIELDS);
			bool QuakeShield = ShieldFlags & (1 << EARTHQUAKE_DISASTER);
			bool BioShield = ShieldFlags & (1 << BIOLOGICAL_DISASTER);
			bool LghtngShield = ShieldFlags & (1 << LIGHTNING_DISASTER);
			bool LavaShield = ShieldFlags & (1 << LAVASPOT_DISASTER);
			bool LanderShot = GET_CGAME_STATE (IMPROVED_LANDER_SHOT);
			bool LanderSpeed = GET_CGAME_STATE (IMPROVED_LANDER_SPEED);
			bool LanderCargo = GET_CGAME_STATE (IMPROVED_LANDER_CARGO);

			ImGui_SeparatorText (ImStr (DBG_STS_STR_BASE + 8));
								// Lander Upgrades

			if (ImGui_BeginTable ("##LanderUpgrades", 3, TABLE_FLAGS))
			{
				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				if (ImGui_Checkbox (lander_upgrades[0], &QuakeShield)) // Quake
				{
					ShieldFlags ^= 1 << EARTHQUAKE_DISASTER;
					SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
				}

				ImGui_TableNextColumn ();

				if (ImGui_Checkbox (lander_upgrades[1], &BioShield)) // BIO
				{
					ShieldFlags ^= 1 << BIOLOGICAL_DISASTER;
					SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
				}

				ImGui_TableNextColumn ();
									// Lightning
				if (ImGui_Checkbox (lander_upgrades[2], &LghtngShield))
				{
					ShieldFlags ^= 1 << LIGHTNING_DISASTER;
					SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
				}

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				if (ImGui_Checkbox (lander_upgrades[3], &LavaShield)) // Lava
				{
					ShieldFlags ^= 1 << LAVASPOT_DISASTER;
					SET_CGAME_STATE (LANDER_SHIELDS, ShieldFlags);
				}

				ImGui_TableNextColumn ();

				if (ImGui_Checkbox (lander_upgrades[4], &LanderShot)) // Weapon
					SET_CGAME_STATE (IMPROVED_LANDER_SHOT, LanderShot);

				ImGui_TableNextColumn ();

				if (ImGui_Checkbox (lander_upgrades[5], &LanderSpeed)) // Speed
					SET_CGAME_STATE (IMPROVED_LANDER_SPEED, LanderSpeed);

				ImGui_TableNextRow ();
				ImGui_TableNextColumn ();

				if (ImGui_Checkbox (lander_upgrades[6], &LanderCargo)) // Cargo
					SET_CGAME_STATE (IMPROVED_LANDER_CARGO, LanderCargo);

				ImGui_EndTable ();
			}

			ImGui_NewLine ();
		}

		{	// Cargo Status
			ImGui_SeparatorText (ImStr (DBG_STS_STR_BASE + 9));

			if (ImGui_BeginTable ("##Cargo", 2, TABLE_FLAGS))
			{
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

					ImGui_AlignTextToFramePadding ();
					ImGui_TextUnformatted (elements[i]);

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

					ImGui_AlignTextToFramePadding ();
					ImGui_Text (elements[8]); // Free Space

					ImGui_TableNextColumn ();

					snprintf (buf, sizeof buf, "%d", rem_capacity);
					ImGui_ProgressBar (capacity_filled, ZERO_F, buf);
				}

				{
					int BioData = GLOBAL_SIS (TotalBioMass);

					ImGui_TableNextRow ();
					ImGui_TableNextColumn ();

					ImGui_AlignTextToFramePadding ();
					ImGui_Text (elements[9]); // Bio-Data

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

		{	// Module Status
			ImGui_SeparatorText (ImStr (DBG_STS_STR_BASE + 10));

			if (ImGui_BeginTable ("##Modules", 2, TABLE_FLAGS))
			{
				int i;

				for (i = 0; i < NUM_PURCHASE_MODULES; i++)
				{
					char buf[40];

					int module_cost =
						GLOBAL (ModuleCost[i]) * MODULE_COST_SCALE;

					snprintf (buf, sizeof (buf), "##%s", modules[i]);

					ImGui_TableNextRow ();
					ImGui_TableNextColumn ();

					ImGui_AlignTextToFramePadding ();
					ImGui_TextUnformatted (modules[i]);

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
	}

	if (NUM_COLUMNS > 2)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2", content_col_size, CHILD_FLAGS,
				0, NULL);
	}
	{
		{	// Alien Status
			ImGui_SeparatorText (ImStr (DBG_STS_STR_BASE + 11));

			if (ImGui_BeginTable ("##Aliens", 2, TABLE_FLAGS))
			{
				HFLEETINFO hStarShip, hNextShip;
				FLEET_INFO *FleetPtr;
				int race_state;

				int Index = 0;
				for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
						hStarShip && Index < NUM_BUILDABLE_SHIPS;
						hStarShip = hNextShip, Index++)
				{
					ImGui_TableNextColumn ();

					FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q),
							hStarShip);

					ImGui_PushIDInt (Index);

					ImGui_Checkbox (GetStringAddress (
							SetAbsStringTableIndex (FleetPtr->race_strings, 0)),
							(bool *)&FleetPtr->can_build);
					if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
					{
						ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 11));
										// Build Ship Tooltip
					}

					ImGui_TableNextColumn ();

					race_state = FleetPtr->allied_state;
					if (ImGui_ComboChar ("##AlliedState", &race_state,
							allied_states, 3))
					{
						FleetPtr->allied_state = race_state;
					}

					ImGui_PopID ();

					hNextShip = _GetSuccLink (FleetPtr);
					UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
				}
				ImGui_EndTable ();
			}
		}
	}

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column3", content_col_size, CHILD_FLAGS,
				0, NULL);
	}

	{
		{	// Flagship Status
			static bool thrusters[11] = { false };
			static bool jets[8] = { false };
			int something = 0;
			int num_m_slots = 16;
			int jt_begin = num_m_slots - 4;
			int i;

			ImGuiStyle *style = ImGui_GetStyle ();
			ImVec2 og_spacing = style->ItemSpacing;
			style->ItemSpacing = MAKE_IV2 (4, 4);

			ImGui_SeparatorText (ImStr (DBG_STS_STR_BASE + 12));

			// Current Coordinates
			{
				POINT universe;
				char buf[SIS_NAME_SIZE];

				universe = MAKE_POINT (LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
					LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)));

				snprintf (buf, sizeof buf, "%03u.%01u : %03u.%01u",
					universe.x / 10, universe.x % 10,
					universe.y / 10, universe.y % 10);

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 13)); // Coordinates
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
					if (CurStarDescPtr != NULL)
						GetClusterName (CurStarDescPtr, buf);
					else
						snprintf (buf, sizeof buf,
								ImStr (DBG_STS_STR_BASE + 14)); // Unknown
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

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 15)); // Location
				ImGui_BeginDisabled (true);
				ImGui_InputText ("##Location", buf, sizeof (buf), 0);
				ImGui_EndDisabled ();
			}

			{	// Captain's Name
				char CaptainsName[SIS_NAME_SIZE];

				snprintf ((char *)&CaptainsName, sizeof (CaptainsName),
					"%s", GLOBAL_SIS (CommanderName));

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 16)); // Captain's Name
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

			{	// Ship Name
				char SISName[SIS_NAME_SIZE];

				snprintf ((char *)&SISName, sizeof (SISName),
					"%s", GLOBAL_SIS (ShipName));

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 17)); // Flagship Name
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

			{	// Landers
				if (!landers_cached)
				{
					cached_landers = GLOBAL_SIS (NumLanders);
					landers_cached = true;
				}

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 18)); // Landers
				ImGui_InputInt ("##Landers", &cached_landers);
				if (ImGui_IsItemDeactivatedAfterEdit ())
				{
					if (cached_landers < 0)
						cached_landers = 0;
					else if (cached_landers > MAX_LANDERS)
						cached_landers = MAX_LANDERS;

					GLOBAL_SIS (NumLanders) = cached_landers;

					scr_refresh = true;
					landers_cached = false;
				}
			}

			{	// Current Fuel
				int CurrentFuel = GLOBAL_SIS (FuelOnBoard);
				int volume = GetFuelTankCapacity ();

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 19), volume);
							// Current Fuel [Max %d]

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
					ImGui_InputIntEx ("##CurrentFuel", &CurrentFuel, 0, 0, 0);

				if (ImGui_IsItemDeactivatedAfterEdit ())
				{
					if (CurrentFuel > volume)
						CurrentFuel = volume;

					GLOBAL_SIS (FuelOnBoard) = CurrentFuel;

					scr_refresh = true;
				}
			}

			{	// Current Crew
				int CurrentCrew = GLOBAL_SIS (CrewEnlisted);
				int volume = GetCrewPodCapacity ();

				ImGui_Text (ImStr (DBG_STS_STR_BASE + 20), volume);
							// Current Crew [Max %d]
				ImGui_InputIntEx ("##CurrentCrew", &CurrentCrew, 0, 0, 0);
				if (ImGui_IsItemDeactivatedAfterEdit ())
				{
					if (CurrentCrew > volume)
						CurrentCrew = volume;

					GLOBAL_SIS (CrewEnlisted) = CurrentCrew;

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

			for (i = num_m_slots - 1; i >= 0; i--)
			{
				char buf[40];
				int t_index = i - 1;
				int j_index = i - 4;

				ImGui_BeginGroup ();

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

				ImGui_EndGroup ();

				ImGui_SameLine ();

				ImGui_SetNextItemWidth (ImGui_CalcItemWidth () -
					(ImGui_GetItemRectSize ().x + style->ItemSpacing.x));

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

			if (collapse_flagship == ImGuiTreeNodeFlags_None)
				collapse_flagship = ImGuiTreeNodeFlags_DefaultOpen;
		}
	}
	ImGui_EndChild ();
}

void
draw_devices_menu (void)
{
	static const char **ultron_status = NULL;

	if (!ultron_status)
		ultron_status = ImStrArr (DBG_DVC_STR_BASE);

	ImGui_BeginStyledChild ("##DevicesColumn", content_col_size,
		CHILD_FLAGS, 0, NULL);
	{
		ImGui_SeparatorText (ImStr (DBG_DVC_STR_BASE + 1)); // Devices

		if (ImGui_BeginTable ("##EventManipulatorTable", 2,
				ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
		{							// Retrieved
			ImGui_TableSetupColumn (ImStr (DBG_DVC_STR_BASE + 2), 0);
									// On-Board
			ImGui_TableSetupColumn (ImStr (DBG_DVC_STR_BASE + 3), 0);
			ImGui_TableHeadersRow ();
			ImGui_TableNextColumn ();

			DBL_COL_CHECKBOX (PORTAL_SPAWNER, PORTAL_SPAWNER_ON_SHIP);
			DBL_COL_CHECKBOX (TALKING_PET, TALKING_PET_ON_SHIP);
			DBL_COL_CHECKBOX (UTWIG_BOMB, UTWIG_BOMB_ON_SHIP);
			DBL_COL_CHECKBOX (SUN_DEVICE, SUN_DEVICE_ON_SHIP);
			DBL_COL_CHECKBOX (ROSY_SPHERE, ROSY_SPHERE_ON_SHIP);
			DBL_COL_CHECKBOX (AQUA_HELIX, AQUA_HELIX_ON_SHIP);
			DBL_COL_CHECKBOX (CLEAR_SPINDLE, CLEAR_SPINDLE_ON_SHIP);
			DBL_COL_CHECKBOX (SHOFIXTI_MAIDENS, MAIDENS_ON_SHIP);
			DBL_COL_CHECKBOX (UMGAH_BROADCASTERS, UMGAH_BROADCASTERS_ON_SHIP);
			DBL_COL_CHECKBOX (BURVIXESE_BROADCASTERS, BURV_BROADCASTERS_ON_SHIP);
			DBL_COL_CHECKBOX (TAALO_PROTECTOR, TAALO_PROTECTOR_ON_SHIP);
			DBL_COL_CHECKBOX (SYREEN_SHUTTLE, SYREEN_SHUTTLE_ON_SHIP);
			DBL_COL_CHECKBOX (VUX_BEAST, VUX_BEAST_ON_SHIP);
			DBL_COL_CHECKBOX (PORTAL_KEY, PORTAL_KEY_ON_SHIP);
			{
				bool retrieved = GET_CGAME_STATE (MOONBASE_DESTROYED);
				bool on_ship = GET_CGAME_STATE (MOONBASE_ON_SHIP);

				if (ImGui_Checkbox ("##MOONBASE_DESTROYED", &retrieved))
					SET_CGAME_STATE (MOONBASE_DESTROYED, retrieved);
				ImGui_TableNextColumn ();

				if (ImGui_Checkbox ("MOONBASE", &on_ship))
					SET_CGAME_STATE (MOONBASE_ON_SHIP, on_ship);
			}
			ImGui_EndTable ();
		}

		ImGui_Columns ();

		Spacer ();

		{
			int ultron_state = GET_CGAME_STATE (ULTRON_CONDITION);
			ImGui_Text ("ULTRON_CONDITION");
			if (ImGui_ComboChar ("##ULTRON_CONDITION", &ultron_state,
					ultron_status, 6))
			{
				SET_CGAME_STATE (ULTRON_CONDITION, ultron_state);
			}
		}

		Spacer ();

		CGAME_STATE_CHECKBOX (EGG_CASE0_ON_SHIP, "EGG_CASE0");
		CGAME_STATE_CHECKBOX (EGG_CASE1_ON_SHIP, "EGG_CASE1");
		CGAME_STATE_CHECKBOX (EGG_CASE2_ON_SHIP, "EGG_CASE2");
		CGAME_STATE_CHECKBOX (DESTRUCT_CODE_ON_SHIP, "DESTRUCT_CODE");
		CGAME_STATE_CHECKBOX (WIMBLIS_TRIDENT_ON_SHIP, "WIMBLIS_TRIDENT");
		CGAME_STATE_CHECKBOX (GLOWING_ROD_ON_SHIP, "GLOWING_ROD");
	} ImGui_EndChild ();
}

typedef struct
{
	int year_index, month_index, day_index;
	bool paused, blocked;
} CachedEvent;

static CachedEvent events[NUM_EVENTS] = { 0 };

int
DaysUntilEvent (CachedEvent event)
{
	int i, days = 0;
	int year_index, month_index, day_index;
	CLOCK_STATE GameClock = GLOBAL (GameClock);

	year_index = event.year_index;
	month_index = event.month_index;
	day_index = event.day_index;

	for (i = GameClock.year_index; i < year_index; i++)
		days += IsLeapYear (i) ? 366 : 365;

	for (i = 1; i < month_index; i++)
		days += DaysInMonth (i, year_index);

	days += day_index;

	for (i = 1; i < GameClock.month_index; i++)
		days -= DaysInMonth (i, GameClock.year_index);

	days -= GameClock.day_index;

	if (event.paused && days < 0)
		return 0;

	return days;
}

void
PauseEvent (int func_index)
{
	HEVENT hEvent = GetHeadEvent ();
	while (hEvent)
	{
		EVENT *EventPtr;
		LockEvent (hEvent, &EventPtr);
		if (EventPtr->func_index == func_index)
		{
			RemoveEvent (hEvent);
			UnlockEvent (hEvent);
			FreeEvent (hEvent);
			break;
		}
		UnlockEvent (hEvent);
		hEvent = GetSuccEvent (EventPtr);
	}
}

void
ResumeEvent (int func_index)
{
	int days = DaysUntilEvent (events[func_index]);
	AddEvent (RELATIVE_EVENT, 0, days > 1 ? days : 2, 0, func_index);
}

static void
GetEvents (int delay)
{
	static TimeCount NextTime = 0;
	TimeCount Now = GetTimeCounter ();

	if (Now >= NextTime)
	{
		HEVENT hEvent;

		NextTime = Now + (ONE_SECOND * delay);

		LockGameClock ();

		hEvent = GetHeadEvent ();

		while (hEvent != 0)
		{
			EVENT *EventPtr;
			CachedEvent *event;
			LockEvent (hEvent, &EventPtr);

			event = &events[EventPtr->func_index];

			if (event->year_index != EventPtr->year_index ||
				event->month_index != EventPtr->month_index ||
				event->day_index != EventPtr->day_index)
			{
				event->year_index = EventPtr->year_index;
				event->month_index = EventPtr->month_index;
				event->day_index = EventPtr->day_index;
			}

			HEVENT nextEvent = GetSuccEvent (EventPtr);
			UnlockEvent (hEvent);
			hEvent = nextEvent;
		}

		UnlockGameClock ();
	}
}

#define NUM_EVENT_COL (style->FontScaleMain > 2.0f ? 2 : 3)

void
draw_events_menu (void)
{
	int i, col;
	static EVENT tmp_evnt;
	float frame_padding;
	static int delay = 1;
	bool block_btn = false;
	static float widget_width = 0;
	static const char **resume_pause = NULL;

	if (!resume_pause)
		resume_pause = ImStrArr (DBG_EVT_STR_BASE + 14);

	GetEvents (delay);

	UQM_AutoChild ("##EventManipulation");
	{
		ImGui_SeparatorText (ImStr (DBG_EVT_STR_BASE)); // Event Manipulation

		Spacer ();

		ImGui_PushStyleColorImVec4 (ImGuiCol_TableHeaderBg, MAKE_IV4 (0,0,0,0));
		if (ImGui_BeginTable ("##EventManipulatorTable", 5, 0))
		{
			ImGui_TableSetupColumn ("", ImGuiTableColumnFlags_NoHeaderLabel);
			ImGui_TableSetupColumn (ImStr (DBG_EVT_STR_BASE + 1), 0); // Event Index
			ImGui_TableSetupColumn (ImStr (DBG_EVT_STR_BASE + 2), 0); // Years
			ImGui_TableSetupColumn (ImStr (DBG_EVT_STR_BASE + 3), 0); // Months
			ImGui_TableSetupColumn (ImStr (DBG_EVT_STR_BASE + 4), 0); // Days
			ImGui_TableHeadersRow ();

			ImGui_TableNextColumn ();

			if (ImGui_Button (ImStr (DBG_EVT_STR_BASE + 5))) // Add Event
			{
				AddEvent (RELATIVE_EVENT, tmp_evnt.month_index,
						tmp_evnt.day_index, tmp_evnt.year_index,
						tmp_evnt.func_index);
			}

			ImGui_TableNextColumn ();

			frame_padding = style->FramePadding.x * 2.0f;
			widget_width = ImGui_CalcTextSize (eventNames[9]).x
					+ frame_padding + SCALE_20F;

			ImGui_SetNextItemWidth (widget_width);
			ImGui_ComboChar ("##EventCombo", (int *)&tmp_evnt.func_index,
					eventNames, NUM_EVENTS);

			ImGui_TableNextColumn ();

			widget_width = ImGui_CalcTextSize ("999999").x + frame_padding;

			ImGui_SetNextItemWidth (widget_width);
			ImGui_InputScalar ("##Year", ImGuiDataType_U16,
					&tmp_evnt.year_index);

			ImGui_TableNextColumn ();

			ImGui_SetNextItemWidth (widget_width);
			ImGui_InputScalar ("##Month", ImGuiDataType_U8,
					&tmp_evnt.month_index);

			ImGui_TableNextColumn ();

			ImGui_SetNextItemWidth (widget_width);
			ImGui_InputScalar ("##Day", ImGuiDataType_U8,
					&tmp_evnt.day_index);

			ImGui_EndTable ();
		}
		ImGui_PopStyleColor ();

	} ImGui_EndChild (); // ##EventManipulation

	ImGui_NewLine ();

	UQM_AutoChild ("##EventStatus");
	{
		ImGui_SeparatorText (ImStr (DBG_EVT_STR_BASE + 6)); // Event Status

		Spacer ();

		ImGui_AlignTextToFramePadding ();
		ImGui_Text (ImStr (DBG_EVT_STR_BASE + 7)); // Refresh delay: 
		ImGui_SameLine ();
		ImGui_SliderInt (ImStr (DBG_EVT_STR_BASE + 8), &delay, 1, 10);

		Spacer ();

		ImGui_Text ("%s: %04d/%02d/%02d",
				ImStr (DBG_EVT_STR_BASE + 9), // Current Date
				GLOBAL (GameClock).year_index,
				GLOBAL (GameClock).month_index,
				GLOBAL (GameClock).day_index);

		col = 0;
		for (i = 0; i < NUM_EVENTS; i++)
		{
			if (events[i].blocked)
			{
				char buf[60];

				if (!block_btn)
				{
					ImGui_NewLine ();
					ImGui_SeparatorText (ImStr (DBG_EVT_STR_BASE + 10));
										// Blocked Events
					Spacer ();
					block_btn = true;
				}

				if (col > 0 && col % NUM_EVENT_COL != 0)
					ImGui_SameLine ();

				snprintf (buf, sizeof buf, "%s##%d",
						eventIdNumToStr (i), i);

				if (ImGui_Button (buf))
					events[i].blocked = false;

				col++;
			}
		}

		ImGui_NewLine ();

		ImGui_Separator ();

		Spacer ();

		col = 0;
		for (i = 0; i < NUM_EVENTS; i++)
		{
			int days_left;

			if (events[i].blocked)
				continue;

			days_left = DaysUntilEvent (events[i]);

			if (days_left >= 0 || events[i].paused)
			{
				static bool paused[NUM_EVENTS];

				if (col > 0 && col % NUM_EVENT_COL != 0)
					ImGui_SameLine ();
				else
					Spacer ();

				ImGui_PushID (eventIdNumToStr (i));

				ImGui_BeginChild ("Card", ZERO_F, CARD_FLAGS,
						ImGuiWindowFlags_MenuBar);
				{
					if (ImGui_BeginMenuBar ())
					{
						char buf[60];

						snprintf (buf, 30, "%s: %02d",
								ImStr (DBG_EVT_STR_BASE + 11), i);
									// EventID
						ImGui_MenuItemEx (buf, NULL, false, false);
						ImGui_EndMenuBar ();
					}

					if (paused[i])
						ImGui_PushStyleColor (ImGuiCol_Text, U32_RED_COLOR);

					ImGui_Text ("%s", eventIdNumToStr (i));

					Spacer ();

					ImGui_Text ("%04d/%02d/%02d",
							events[i].year_index,
							events[i].month_index,
							events[i].day_index);

					Spacer ();

					ImGui_Text (ImStr (DBG_EVT_STR_BASE + 12), days_left);
							// Days Left: %d

					if (paused[i])
						ImGui_PopStyleColor ();

					Spacer ();

					ImGui_Separator ();

					Spacer ();

					if (ImGui_Button (ImStr (DBG_EVT_STR_BASE + 13))) // Block
						events[i].blocked = !events[i].blocked;

					ImGui_SameLine ();

					if (ImGui_Button (resume_pause[events[i].paused]))
					{				// Pause, Resume
						events[i].paused = !events[i].paused;

						if (events[i].paused)
							PauseEvent (i);
						else
							ResumeEvent (i);
					}

					Spacer ();

				} ImGui_EndChild (); // Card

				ImGui_PopID ();
			
				DrawBorderAroundLastItem ();

				paused[i] = events[i].paused;

				col++;
			}
		}
	} ImGui_EndChild (); // ##EventStatus
}

#define STAR_MAX (NUM_SOLAR_SYSTEMS + NUM_HYPER_VORTICES + 3)

static int
FindLongestString (int string_base, int string_count)
{
	float max_width = 0.0f;
	int longest_index = -1;

	for (int i = 0; i < string_count; i++)
	{
		const char *text = GAME_STRING (string_base + i);
		float text_size = ImGui_CalcTextSize (text).x;

		if (text_size > max_width)
		{
			max_width = text_size;
			longest_index = i;
		}
	}
	return longest_index;
}

void draw_stars_menu (void)
{
	int i;
	static bool by_cur_star = false;
	static bool by_presence = false;
	static bool filter_type_bool = false;
	static bool filter_colour_bool = false;
	static int filter_type = 0;
	static int filter_colour = 0;
	static int min_x = 0, max_x = MAX_X_UNIVERSE;
	static int min_y = 0, max_y = MAX_Y_UNIVERSE;
	static int longest_postfix = -1;
	static int longest_prefix = -1;
	const char *star_type[NUM_STAR_TYPES] =
			{ "Dwarf", "Giant", "Super Giant" };
	const char *star_colour[NUM_STAR_COLORS] =
			{ "Blue", "Green", "Orange", "Red", "White", "Yellow" };

	ImGui_BeginStyledChild ("##StarFilters", ZERO_F, CH_FLAT_NAV, 0, NULL);
	{
		ImGui_SeparatorText ("Filters");

		Spacer ();

		ImGui_AlignTextToFramePadding ();
		ImGui_Text ("Filter by Presence");
		ImGui_SameLine ();
		ImGui_Checkbox ("##FilterPresence", &by_presence);

		Spacer ();

		ImGui_AlignTextToFramePadding ();
		ImGui_Text ("Show only Type");
		ImGui_SameLine ();
		ImGui_Checkbox ("##FilterTypeBool", &filter_type_bool);
		ImGui_SameLine ();
		ImGui_ComboChar ("##FilterTypeCombo", &filter_type, star_type, 
				NUM_STAR_TYPES);

		Spacer ();

		ImGui_AlignTextToFramePadding ();
		ImGui_Text ("Show only Colour");
		ImGui_SameLine ();
		ImGui_Checkbox ("##FilterColourBool", &filter_colour_bool);
		ImGui_SameLine ();
		ImGui_ComboChar ("##FilterColourCombo", &filter_colour,
				star_colour, NUM_STAR_COLORS);

		Spacer ();

		ImGui_AlignTextToFramePadding ();
		ImGui_Text ("Current Star System");
		ImGui_SameLine ();
		ImGui_Checkbox ("##CurStar", &by_cur_star);

		Spacer ();

		ImGui_AlignTextToFramePadding ();
		ImGui_Text ("X/Y Range");
		ImGui_SameLine ();
		ImGui_SliderIntRange ("##XRange", &min_x, &max_x, 0, 9999, "%d", 0, 0);
		ImGui_SameLine ();
		ImGui_SliderIntRange ("##YRange", &min_y, &max_y, 0, 9999, "%d", 0, 0);

		if (CurStarDescPtr == NULL)
			by_cur_star = 0;

	} ImGui_EndChild (); // ##StarFilters

	ImGui_BeginStyledChild ("##StarTable", ZERO_F, CH_FLAT_NAV, 0, NULL);
	{
		ImGui_SeparatorText ("Star Table");

		Spacer ();

		if (ImGui_BeginTable ("##StarArrayTable", 8,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_NoHostExtendX |
			ImGuiTableFlags_SizingStretchSame))
		{
			ImGui_TableSetupColumnEx ("Idx",
					ImGuiTableColumnFlags_WidthFixed,
					ImGui_CalcTextSize ("999").x +
						style->FramePadding.x, 0);

			if (longest_prefix == -1)
				longest_prefix = FindLongestString (STAR_NUMBER_BASE,
						STAR_NUMBER_COUNT - 1);

			ImGui_TableSetupColumnEx ("Prefix",
					ImGuiTableColumnFlags_WidthFixed,
					ImGui_CalcTextSize (GAME_STRING (
							STAR_NUMBER_BASE + longest_prefix)).x +
							style->FramePadding.x, 1);

			if (longest_postfix == -1)
				longest_postfix = FindLongestString (STAR_STRING_BASE,
						STAR_STRING_COUNT - 17);

			ImGui_TableSetupColumnEx ("Postfix",
					ImGuiTableColumnFlags_WidthFixed,
					ImGui_CalcTextSize (GAME_STRING (
						STAR_STRING_BASE + longest_postfix)).x +
						style->FramePadding.x, 2);
			
			ImGui_TableSetupColumnEx ("X",
					ImGuiTableColumnFlags_WidthFixed,
					ImGui_CalcTextSize ("99999").x +
						style->FramePadding.x * 2, 3);

			ImGui_TableSetupColumnEx ("Y",
					ImGuiTableColumnFlags_WidthFixed,
					ImGui_CalcTextSize ("99999").x +
						style->FramePadding.x * 2, 4);

			ImGui_TableSetupColumn ("Type", 0);
			ImGui_TableSetupColumn ("Colour", 0);
			ImGui_TableSetupColumn ("Presence", 0);
			ImGui_TableHeadersRow ();

			for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
			{
				if (by_cur_star && CurStarDescPtr &&
					!(CurStarDescPtr->star_pt.x ==
						star_array[i].star_pt.x &&
						CurStarDescPtr->star_pt.y ==
						star_array[i].star_pt.y))
					continue;
				if (filter_type_bool &&
						STAR_TYPE (star_array[i].Type) != filter_type)
					continue;
				if (filter_colour_bool &&
						STAR_COLOR (star_array[i].Type) != filter_colour)
					continue;
				if (by_presence && star_array[i].Index == 0)
					continue;
				if (!(star_array[i].star_pt.x >= min_x &&
					star_array[i].star_pt.x <= max_x))
					continue;
				if (!(star_array[i].star_pt.y >= min_y &&
					star_array[i].star_pt.y <= max_y))
					continue;

				ImGui_TableNextColumn ();

				ImGui_AlignTextToFramePadding ();
				ImGui_Text ("%03d", i);

				ImGui_TableNextColumn ();

				if (star_array[i].Prefix > 0)
				{
					ImGui_AlignTextToFramePadding ();
					ImGui_Text ("%s",
							GAME_STRING (STAR_NUMBER_BASE +
								star_array[i].Prefix - 1));
				}

				ImGui_TableNextColumn ();

				ImGui_AlignTextToFramePadding ();
				ImGui_Text ("%s",
						GAME_STRING (STAR_STRING_BASE +
						star_array[i].Postfix));

				ImGui_TableNextColumn ();

				{
					char buf[40];
					int star_ptx = star_array[i].star_pt.x;

					snprintf (buf, sizeof buf, "##star_ptx%d", i);
					ImGui_SetNextItemWidth (-1);
					ImGui_InputIntEx (buf, &star_ptx, 0, 0, 0);
					if (ImGui_IsItemDeactivatedAfterEdit ()
						&& star_ptx >= 0 && star_ptx <= 9999)
					{
						star_array[i].star_pt.x = star_ptx;
					}
				}

				ImGui_TableNextColumn ();

				{
					char buf[40];
					int star_pty = star_array[i].star_pt.y;

					snprintf (buf, sizeof buf, "##star_pty%d", i);
					ImGui_SetNextItemWidth (-1);
					ImGui_InputIntEx (buf, &star_pty, 0, 0, 0);
					if (ImGui_IsItemDeactivatedAfterEdit ()
						&& star_pty >= 0 && star_pty <= 9999)
					{
						star_array[i].star_pt.y = star_pty;
					}
				}

				ImGui_TableNextColumn ();

				{
					char buf[40];
					static int cur_star_type = 0;
					cur_star_type = STAR_TYPE (star_array[i].Type);

					snprintf (buf, sizeof buf, "##StarType%d", i);

					ImGui_SetNextItemWidth (-1);
					if (ImGui_ComboChar (buf,
						&cur_star_type, star_type, NUM_STAR_TYPES))
					{
						star_array[i].Type = MAKE_STAR (cur_star_type,
								STAR_COLOR (star_array[i].Type), -1);
					}
				}

				ImGui_TableNextColumn ();

				{
					char buf[40];
					static int cur_star_colour = 0;
					cur_star_colour = STAR_COLOR (star_array[i].Type);

					snprintf (buf, sizeof buf, "##StarColour%d", i);

					ImGui_SetNextItemWidth (-1);
					if (ImGui_ComboChar (buf,
						&cur_star_colour, star_colour, NUM_STAR_COLORS))
					{
						star_array[i].Type = MAKE_STAR (
								STAR_TYPE (star_array[i].Type),
								cur_star_colour, -1);
					}
				}

				ImGui_TableNextColumn ();

				{
					char buf[40];
					static int cur_presence = 0;
					cur_presence = star_array[i].Index;

					snprintf (buf, sizeof buf, "##Presence%d", i);

					ImGui_SetNextItemWidth (-1);
					if (ImGui_ComboChar (buf,
						&cur_presence, presence_names, NUM_PLOTS))
					{
						star_array[i].Index = cur_presence;
					}
				}
			}
			ImGui_EndTable ();
		}
	} ImGui_EndChild (); // ##StarTable
}