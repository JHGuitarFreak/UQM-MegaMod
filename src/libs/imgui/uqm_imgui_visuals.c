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
	const char *planet_textures[] =
	{
		"3DO",
		"UQM"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "VisualsColumns", false);

	ImGui_ComboChar ("Date Format", &optDateFormat, date_formats, 4);

	ImGui_Checkbox ("Show Whole Fuel Value", (bool *)&optWholeFuel);

	ImGui_ComboChar ("Fuel Range Display", &optFuelRange, fuel_ranges, 4);

	ImGui_Checkbox ("SOI Colors", (bool *)&optSphereColors);
	ImGui_Checkbox ("Animated HyperSpace Stars", (bool *)&optHyperStars);
	ImGui_Checkbox ("Captain Names in Shipyard", (bool *)&optCaptainNames);
	ImGui_Checkbox ("Alternate Orz Font", (bool *)&optOrzCompFont);
	ImGui_Checkbox ("Non-stop Oscilloscope", (bool *)&optNonStopOscill);
	ImGui_Checkbox ("Nebulae", (bool *)&optNebulae);
	ImGui_Checkbox ("Orbiting Planets", (bool *)&optOrbitingPlanets);
	ImGui_Checkbox ("Textured Planets", (bool *)&optTexturedPlanets);
	ImGui_Checkbox ("Unscaled Star System (HD Only)",
		(bool *)&optUnscaledStarSystem);
	ImGui_Checkbox ("NPC Ship Orientation", (bool *)&optShipDirectionIP);
	ImGui_Checkbox ("Hazard Colors", (bool *)&optHazardColors);

	ImGui_ComboChar ("Planet Map Textures", (int *)&optPlanetTexture,
		planet_textures, 2);

	ImGui_Checkbox ("Show Lander Upgrades", (bool *)&optShowUpgrades);

	ImGui_SliderInt ("Nebulae Volume", &optNebulaeVolume, 0, 50);
}