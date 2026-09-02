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
	static const char **god_modes  = NULL;
	static const char **time_modes = NULL;

	if (!god_modes)
	{
		god_modes  = ImStrArr (ENH_CHT_STR_BASE);
		time_modes = ImStrArr (ENH_CHT_STR_BASE + 1);
	}

	ImGui_BeginStyledChild ("##CheatsColumn", content_col_size,
			CHILD_FLAGS, 0, NULL);

	// Basic Cheats
	ImGui_SeparatorText (ImStr (ENH_CHT_STR_BASE + 2));

	// Kohr-Stahp
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 3), &optCheatMode,
			"cheat.kohrStahp", false);
	// Kohr-Ah DeCleansing
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 4), &optDeCleansing,
			"cheat.deCleansing", false);

	Spacer ();

	// God Modes
	if (ImGui_SizedComboChar (ImStr (ENH_CHT_STR_BASE + 5), &optGodModes,
			god_modes, 4))
	{
		res_PutInteger ("cheat.godModes", optGodModes);
		cheat_changed = true;
	}

	// Time Dilation
	if (ImGui_SizedComboChar (ImStr (ENH_CHT_STR_BASE + 6),
			&timeDilationScale, time_modes, 3))
	{
		if (!IN_MAIN_MENU)
		{
			LockGameClock ();
			if (GLOBAL (CurrentActivity) & IN_INTERPLANETARY)
				SetGameClockRate (INTERPLANETARY_CLOCK_RATE);
			else if (GLOBAL (CurrentActivity) & IN_HYPERSPACE)
				SetGameClockRate (HYPERSPACE_CLOCK_RATE);
			UnlockGameClock ();
		}

		res_PutInteger ("cheat.timeDilation", timeDilationScale);
		cheat_changed = true;
	}

	Spacer ();

	// Bubble Warp
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 7), &optBubbleWarp,
			"cheat.bubbleWarp", false);
	// Head Start
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 8), &optHeadStart,
			"cheat.headStart", false);
	// Unlock All Ships
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 9), &optUnlockShips,
			"cheat.unlockShips", false);
	// Infinite R.U.
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 10), &optInfiniteRU,
			"cheat.infiniteRU", false);
	// Infinite Fuel
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 11), &optInfiniteFuel,
			"cheat.infiniteFuel", false);
	// Infinite Credits
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 12), &optInfiniteCredits,
			"cheat.infiniteCredits", false);
	// No Hyperspace Encounters
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 13), &optNoHQEncounters,
			"cheat.noHQEncounters", false);
	// No Melee Obstacles
	UQM_ImGui_CheckBox (ImStr (ENH_CHT_STR_BASE + 14), &optMeleeObstacles,
			"cheat.meleeObstacles", false);

	ImGui_NewLine ();

	// Expanded Cheats
	ImGui_SeparatorText (ImStr (ENH_CHT_STR_BASE + 15));

	// Lander Capacity
	ImGui_Text (ImStr (ENH_CHT_STR_BASE + 16));

	ImGui_Checkbox ("##ChangeLanderCapacity", &changeLanderCapacity);

	ImGui_SameLine ();

	ImGui_SetNextItemWidth (ImGui_CalcItemWidth () -
			(ImGui_GetItemRectSize ().x + style->ItemSpacing.x));

	ImGui_InputInt ("##LanderCapacity",
			!changeLanderCapacity ? &MaxScrounged : &optLanderHold);

	ImGui_NewLine (); // Lander Capacity

	// Ships GTFO!
	if (ImGui_Button (ImStr (ENH_CHT_STR_BASE + 17)))
		ShipGTFO = true;

	ImGui_NewLine ();

	ImGui_EndChild (); // ##CheatsColumn
}