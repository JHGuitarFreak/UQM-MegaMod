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
	const char *date_formats[] =
			{ "MMM DD.YYYY", "MM.DD.YYYY", "DD MMM.YYYY", "DD.MM.YYYY" };
	const char *fuel_ranges[] =
			{ "None", "At Destination", "To Sol", "Both" };
	const char *planet_textures[] = { "3DO", "UQM" };

	if (!IN_MAIN_MENU)
	{
		ImGui_TextWrappedColored (IV4_YELLOW_COLOR,
				"Some of the options in this part of the menu need a full "
				"screen update for them to take full effect. If in doubt "
				"either enter/leave planet orbit or leave and re-enter "
				"the current star system.");
		Spacer ();
	}

	ImGui_ColumnsEx (DISPLAY_BOOL, "VisualsColumns", false);

	// User Interface
	{
		ImGui_SeparatorText ("User Interface");

		{
			ImGui_Text ("Date Format:");
			if (ImGui_ComboChar ("##DateFormat", &optDateFormat,
					date_formats, 4))
			{
				res_PutInteger ("mm.dateFormat", optDateFormat);
				mmcfg_changed = true;
			}
		}

		Spacer ();

		UQM_ImGui_CheckBox ("Custom Border", &optCustomBorder, "mm.customBorder");
		UQM_ImGui_CheckBox ("Show Whole Fuel Value", &optWholeFuel, "mm.wholeFuel");

		Spacer ();

		{
			ImGui_Text ("Fuel Range Indicators");
			if (ImGui_ComboChar ("##FuelRange", &optFuelRange, fuel_ranges, 4))
			{
				res_PutInteger ("mm.fuelRange", optFuelRange);
				mmcfg_changed = true;
			}
		}

		Spacer ();

		UQM_ImGui_CheckBox ("SOI Colors", &optSphereColors, "mm.sphereColors");

		{
			ImGui_BeginDisabled (!IN_MAIN_MENU);

			UQM_ImGui_CheckBox ("HD Animations", &optHyperStars, "mm.hyperStars");

			if (!IN_MAIN_MENU)
			{
				ImGui_TextWrappedColored (IV4_RED_COLOR,
						"WARNING! HD Animations can only be "
						"(de)activated while in the Main Menu!");
				Spacer ();
			}

			ImGui_EndDisabled ();
		}

		UQM_ImGui_CheckBox ("Captain Names in Shipyard", &optCaptainNames, "mm.captainNames");
		UQM_ImGui_CheckBox ("Game Over Cutscenes", &optGameOver, "mm.gameOver");

		ImGui_NewLine ();
	}

	// Conversation Screen
	{
		ImGui_SeparatorText ("Conversation Screen");

		UQM_ImGui_CheckBox ("Alternate Orz Font", &optOrzCompFont, "mm.orzCompFont");
		UQM_ImGui_CheckBox ("Non-Stop Oscilloscope", &optNonStopOscill, "mm.nonStopOscill");

		ImGui_NewLine ();
	}

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	// Star System View
	{
		ImGui_SeparatorText ("Star System View");

		ImGui_Text ("Nebulae & Nebulae Brightness:");
		UQM_ImGui_CheckBox ("##Nebulae", &optNebulae, "mm.nebulae");
		ImGui_SameLine ();
		if (ImGui_SliderInt ("##NebulaeVolume", &optNebulaeVolume, 0, 50))
		{
			res_PutInteger ("mm.nebulaevol", optNebulaeVolume);
			mmcfg_changed = true;
		}

		Spacer ();

		UQM_ImGui_CheckBox ("Orbiting Planets", &optOrbitingPlanets, "mm.orbitingPlanets");

		{
			ImGui_BeginDisabled (!IN_MAIN_MENU);

			UQM_ImGui_CheckBox ("Textured Planets", &optTexturedPlanets,
					"mm.texturedPlanets");

			if (!IN_MAIN_MENU)
			{
				ImGui_TextWrappedColored (IV4_RED_COLOR,
						"WARNING! Textured Planets can only be "
						"(de)activated while in the Main Menu!");
				Spacer ();
			}

			ImGui_EndDisabled ();
		}

		UQM_ImGui_CheckBox ("Unscaled View (HD Only)", &optUnscaledStarSystem, "mm.unscaledStarSystem");
		UQM_ImGui_CheckBox ("NPC Ship Orientation", &optShipDirectionIP, "mm.shipDirectionIP");

		ImGui_NewLine ();
	}

	// Orbit Screen
	{
		ImGui_SeparatorText ("Orbit Screen");

		UQM_ImGui_CheckBox ("Hazard Colors", &optHazardColors, "mm.hazardColors");

		Spacer ();

		{
			ImGui_Text ("Planet Map Textures:");
			if (ImGui_ComboChar ("##PlanetMapTextures", (int *)&optPlanetTexture,
					planet_textures, 2))
			{
				res_PutInteger ("mm.planetTexture", optPlanetTexture);
				mmcfg_changed = true;
			}
		}

		Spacer ();

		UQM_ImGui_CheckBox ("Show Lander Upgrades", &optShowUpgrades, "mm.showUpgrades");

		ImGui_NewLine ();
	}

}