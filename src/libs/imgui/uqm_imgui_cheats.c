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

	UQM_ImGui_CheckBox ("Kohr-Stahp", &optCheatMode, "cheat.kohrStahp");
	UQM_ImGui_CheckBox ("Kohr-Ah DeCleansing", &optDeCleansing,
			"cheat.deCleansing");

	Spacer ();

	ImGui_Text ("God Modes:");
	if (ImGui_ComboChar ("##GodModes", &optGodModes, god_modes, 4))
	{
		res_PutInteger ("cheat.godModes", optGodModes);
		cheat_changed = true;
	}

	ImGui_Text ("Time Dilation:");
	if (ImGui_ComboChar ("##TimeDilation", &timeDilationScale, time_modes, 3))
	{
		res_PutInteger ("cheat.timeDilation", timeDilationScale);
		cheat_changed = true;
	}

	Spacer ();

	UQM_ImGui_CheckBox ("Bubble Warp", &optBubbleWarp, "cheat.bubbleWarp");
	UQM_ImGui_CheckBox ("Head Start", &optHeadStart, "cheat.headStart");
	UQM_ImGui_CheckBox ("Unlock All Ships", &optUnlockShips, "cheat.unlockShips");
	UQM_ImGui_CheckBox ("Infinite R.U.", &optInfiniteRU, "cheat.infiniteRU");
	UQM_ImGui_CheckBox ("Infinite Fuel", &optInfiniteFuel, "cheat.infiniteFuel");
	UQM_ImGui_CheckBox ("Infinite Credits", &optInfiniteCredits, "cheat.infiniteCredits");
	UQM_ImGui_CheckBox ("No Hyperspace Encounters", &optNoHQEncounters, "cheat.noHQEncounters");
	UQM_ImGui_CheckBox ("No Melee Obstacles", &optMeleeObstacles, "cheat.meleeObstacles");

	ImGui_NewLine ();

	ImGui_SeparatorText ("Expanded Cheats");

	ImGui_Text ("Lander Capacity:");
	ImGui_Checkbox ("##ChangeLanderCapacity", &changeLanderCapacity);
	ImGui_SameLine ();
	ImGui_InputInt ("##LanderCapacity",
		!changeLanderCapacity ? &MaxScrounged : &optLanderHold);

	//ImGui_NextColumn ();
}