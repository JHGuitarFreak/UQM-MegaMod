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

#include "libs/inplib.h"
#include "libs/input/sdl/vcontrol.h"

static void Control_Tabs (void);

void draw_controls_menu (void)
{
	const char *control_display[] = { "Keyboard", "Xbox", "PlayStation" };
	const char *player_controls[] =
	{
		"Control Set 1",
		"Control Set 2",
		"Control Set 3",
		"Control Set 4",
		"Control Set 5",
		"Control Set 6",
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "ControlsColumns", false);

	// Control Options
	{
		ImGui_SeparatorText ("Control Options");

		ImGui_Text ("Control Display:");
		ImGui_ComboChar ("##ControlDisplay", (int *)&optControllerType,
				control_display, 3);

		Spacer ();

		ImGui_Text ("Player 1:");
		ImGui_ComboChar ("##BottomPlayer", (int *)&PlayerControls[0],
				player_controls, 6);

		ImGui_Text ("Player 2:");
		ImGui_ComboChar ("##TopPlayer", (int *)&PlayerControls[1],
				player_controls, 6);

		ImGui_NewLine ();
	}

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	Control_Tabs ();
}

static void PlayerOneControls (void)
{
	// Bullshit goes here
}

static void PlayerOneTab (void)
{
	if (ImGui_BeginTabItem ("Player 1", NULL, 0))
	{
		ImGui_NewLine ();

		if (ImGui_Button ("Clear All"))
		{
			// Add switching code here
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Set Defaults"))
		{
			// Add switching code here
		}

		ImGui_NewLine ();

		PlayerOneControls ();

		ImGui_EndTabItem ();
	}
}

static void PlayerTwoTab (void)
{
	if (ImGui_BeginTabItem ("Player 2", NULL, 0))
	{
		ImGui_NewLine ();

		if (ImGui_Button ("Clear All"))
		{
			// Add switching code here
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Set Defaults"))
		{
			// Add switching code here
		}

		ImGui_NewLine ();

		ImGui_EndTabItem ();
	}
}

static void Control_Tabs (void)
{
	ImGui_SeparatorText ("Edit Controls");

	if (ImGui_BeginTabBar ("ControlTabs", 0))
	{
		PlayerOneTab ();
		PlayerTwoTab ();

		ImGui_EndTabBar ();
	}
}