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

void draw_cheats_menu (void)
{
	int MaxScrounged = MAX_SCROUNGED;

	const char *god_modes[] =
	{
		"None",
		"Infinite Energy",
		"Invulnerable",
		"Full God Mode"
	};
	const char *time_modes[] =
	{
		"Normal",
		"Slow (x6)",
		"Fast (x5)"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "CheatColumns", false); // For taming width

	ImGui_SeparatorText ("Basic Cheats");

	ImGui_Checkbox ("Kohr-Stahp", (bool *)&optCheatMode);
	ImGui_Checkbox ("Kohr-Ah DeCleansing", (bool *)&optDeCleansing);

	ImGui_Text ("God Modes:");
	ImGui_ComboChar ("##GodModes", &optGodModes, god_modes, 4);
	ImGui_Text ("Time Dilation:");
	ImGui_ComboChar ("##TimeDilation", &timeDilationScale, time_modes, 3);

	ImGui_Checkbox ("Bubble Warp", (bool *)&optBubbleWarp);
	ImGui_Checkbox ("Head Start", (bool *)&optHeadStart);
	ImGui_Checkbox ("Unlock All Ships", (bool *)&optUnlockShips);
	ImGui_Checkbox ("Infinite R.U.", (bool *)&optInfiniteRU);
	ImGui_Checkbox ("Infinite Fuel", (bool *)&optInfiniteFuel);
	ImGui_Checkbox ("Infinite Credits", (bool *)&optInfiniteCredits);
	ImGui_Checkbox ("No Hyperspace Encounters", (bool *)&optNoHQEncounters);
	ImGui_Checkbox ("No Melee Obstacles", (bool *)&optMeleeObstacles);

	ImGui_SeparatorText ("Expanded Cheats");

	ImGui_Text ("Lander Capacity:");
	ImGui_Checkbox ("##ChangeLanderCapacity", &changeLanderCapacity);
	ImGui_SameLine ();
	ImGui_InputInt ("##LanderCapacity",
		!changeLanderCapacity ? &MaxScrounged : &optLanderHold);

	//ImGui_NextColumn ();
}