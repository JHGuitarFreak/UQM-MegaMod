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

void draw_visual_menu (void)
{
	static const char **date_formats    = NULL;
	static const char **fuel_ranges     = NULL;
	static const char **planet_textures = NULL;

	if (!date_formats)
	{
		date_formats    = ImStrArr (ENH_VIS_STR_BASE);
		fuel_ranges     = ImStrArr (ENH_VIS_STR_BASE + 1);
		planet_textures = ImStrArr (ENH_VIS_STR_BASE + 2);
	}

	if (!IN_MAIN_MENU)
	{
		ImGui_BeginStyledChild ("##WarningChild", ZERO_F, CHILD_FLAGS, 0, NULL);
		ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_YELLOW_COLOR),
				ImStr (TIP_WARN_STR_BASE + 10)); // Screen Update Warning
		Spacer ();
		ImGui_EndChild ();
	}
	
	ImGui_BeginStyledChild ("##UI", content_col_size, CHILD_FLAGS, 0, NULL);
	{
		{	// User Interface
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 12));

			{	// Date Format
				if (ImGui_SizedComboChar (ImStr (ENH_VIS_STR_BASE + 3),
						&optDateFormat, date_formats, 4))
				{
					res_PutInteger ("mm.dateFormat", optDateFormat);
					mmcfg_changed = true;
				}
			}

			Spacer ();

			// Custom Border
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 4), &optCustomBorder,
					"mm.customBorder", false);
			// Show Whole Fuel Value
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 5), &optWholeFuel,
					"mm.wholeFuel", false);

			Spacer ();

			{	// Fuel Range Indicators
				if (ImGui_SizedComboChar (ImStr (ENH_VIS_STR_BASE + 6),
						&optFuelRange, fuel_ranges, 4))
				{
					res_PutInteger ("mm.fuelRange", optFuelRange);
					mmcfg_changed = true;
				}
			}

			Spacer ();

			// Alternate SOI Colours
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 7),
					(OPT_ENABLABLE *)&optSphereColors, "mm.sphereColors",
					false);

			{	// HD Animations
				ImGui_BeginDisabled (!IN_MAIN_MENU);

				// HD Animations
				UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 8),
						&optHyperStars, "mm.hyperStars", false);

				if (!IN_MAIN_MENU)
				{
					ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_RED_COLOR),
							ImStr (TIP_WARN_STR_BASE + 3));
								// Main Menu Warning
					Spacer ();
				}

				ImGui_EndDisabled ();
			}

			// Captain Names in Shipyard
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 9), &optCaptainNames,
					"mm.captainNames", false);
			// Game Over Cutscenes
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 10), &optGameOver,
					"mm.gameOver", false);

			ImGui_NewLine ();
		}

		{	// Conversation Screen
			ImGui_SeparatorText (ImStr(GEN_ENG_STR_BASE + 21));

			// Alternate Orz Font
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 11), &optOrzCompFont,
					"mm.orzCompFont", false);
			// Non-Stop Oscilloscope
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 12),
					&optNonStopOscill, "mm.nonStopOscill", false);

			ImGui_NewLine ();
		}
	}

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2",
				content_col_size, CHILD_FLAGS, 0, NULL);
	}

	{
		{	// Star System View
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 26));

			// Nebulae & Nebulae Brightness
			ImGui_Text (ImStr (ENH_VIS_STR_BASE + 13));
			UQM_ImGui_CheckBox ("##Nebulae", &optNebulae, "mm.nebulae", false);
			ImGui_SameLine ();
			ImGui_SetNextItemWidth (ImGui_CalcItemWidth () -
					(ImGui_GetItemRectSize ().x + style->ItemSpacing.x));
			if (ImGui_SliderInt ("##NebulaeVolume", &optNebulaeVolume, 0, 50))
			{
				res_PutInteger ("mm.nebulaevol", optNebulaeVolume);
				mmcfg_changed = true;
			}

			Spacer ();

			// Orbiting Planets
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 14),
					&optOrbitingPlanets, "mm.orbitingPlanets", false);

			{	// Textured Planets
				ImGui_BeginDisabled (!IN_MAIN_MENU);

				UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 15),
						&optTexturedPlanets, "mm.texturedPlanets", false);

				if (!IN_MAIN_MENU)
				{
					ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_RED_COLOR),
							ImStr (TIP_WARN_STR_BASE + 3));
								// Main Menu Warning
					Spacer ();
				}

				ImGui_EndDisabled ();
			}

			// Unscaled View (HD Only)
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 16),
					&optUnscaledStarSystem, "mm.unscaledStarSystem", false);
			// NPC Ship Orientation
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 17),
					&optShipDirectionIP, "mm.shipDirectionIP", false);

			ImGui_NewLine ();
		}

		{	// Orbit Screen
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 29));

			// Hazard Colors
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 18),
					&optHazardColors, "mm.hazardColors", false);

			Spacer ();

			{	// Planet Map Textures
				if (ImGui_SizedComboChar (ImStr (ENH_VIS_STR_BASE + 19),
						(int *)&optPlanetTexture, planet_textures, 2))
				{
					res_PutBoolean ("mm.planetTexture", optPlanetTexture);
					mmcfg_changed = true;
				}
			}

			Spacer ();

			// Show Lander Upgrades
			UQM_ImGui_CheckBox (ImStr (ENH_VIS_STR_BASE + 20),
					&optShowUpgrades, "mm.showUpgrades", false);

			ImGui_NewLine ();
		}
	} ImGui_EndChild ();
}