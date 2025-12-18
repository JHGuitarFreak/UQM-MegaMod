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
	{
		"MMM DD.YYYY",
		"MM.DD.YYYY",
		"DD MMM.YYYY",
		"DD.MM.YYYY"
	};
	const char *fuel_ranges[] =
	{
		"None",
		"At Destination",
		"To Sol",
		"Both"
	};
	const char *planet_textures[] = { "3DO", "UQM" };

	ImGui_ColumnsEx (DISPLAY_BOOL, "VisualsColumns", false);

	// User Interface
	{
		ImGui_SeparatorText ("User Interface");

		{
			ImGui_Text ("Date Format:");
			ImGui_ComboChar ("##DateFormat", &optDateFormat,
					date_formats, 4);
		}

		Spacer ();

		ImGui_Checkbox ("Custom Border", (bool *)&optCustomBorder);
		ImGui_Checkbox ("Show Whole Fuel Value", (bool *)&optWholeFuel);

		Spacer ();

		{
			ImGui_Text ("Fuel Range Indicators");
			ImGui_ComboChar ("##FuelRange", &optFuelRange, fuel_ranges, 4);
		}

		Spacer ();

		ImGui_Checkbox ("SOI Colors", (bool *)&optSphereColors);
		ImGui_Checkbox ("HD Animations", (bool *)&optHyperStars);
		ImGui_Checkbox ("Captain Names in Shipyard",
				(bool *)&optCaptainNames);
		ImGui_Checkbox ("Game Over Cutscenes", (bool *)&optGameOver);

		ImGui_NewLine ();
	}

	// Conversation Screen
	{
		ImGui_SeparatorText ("Conversation Screen");

		ImGui_Checkbox ("Alternate Orz Font", (bool *)&optOrzCompFont);
		ImGui_Checkbox ("Non-Stop Oscilloscope", (bool *)&optNonStopOscill);

		ImGui_NewLine ();
	}

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	// Star System View
	{
		ImGui_SeparatorText ("Star System View");

		ImGui_Text ("Nebulae & Nebulae Brightness:");
		ImGui_Checkbox ("##Nebulae", (bool *)&optNebulae);
		ImGui_SameLine ();
		ImGui_SliderInt ("##NebulaeVolume", &optNebulaeVolume, 0, 50);

		Spacer ();

		ImGui_Checkbox ("Orbiting Planets", (bool *)&optOrbitingPlanets);
		ImGui_Checkbox ("Textured Planets", (bool *)&optTexturedPlanets);
		ImGui_Checkbox ("Unscaled View (HD Only)",
				(bool *)&optUnscaledStarSystem);
		ImGui_Checkbox ("NPC Ship Orientation",
				(bool *)&optShipDirectionIP);

		ImGui_NewLine ();
	}

	// Orbit Screen
	{
		ImGui_SeparatorText ("Orbit Screen");

		ImGui_Checkbox ("Hazard Colors", (bool *)&optHazardColors);

		Spacer ();

		{
			ImGui_Text ("Planet Map Textures:");
			ImGui_ComboChar ("##PlanetMapTextures", (int *)&optPlanetTexture,
					planet_textures, 2);
		}

		Spacer ();

		ImGui_Checkbox ("Show Lander Upgrades", (bool *)&optShowUpgrades);

		ImGui_NewLine ();
	}

}