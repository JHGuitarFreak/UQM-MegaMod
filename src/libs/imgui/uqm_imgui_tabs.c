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

static void
GeneralTab (TabState *state, ImVec2 content_size, ImVec2 sidebar_size,
	ImVec2 button_size)
{
	BYTE i;

	const char *tab_names[] = {
		"Graphics",
		"PC / 3DO",
		"Sound",
		"Music",
		"Controls",
		"Status"
	};

	if (ImGui_BeginTabItem ("General", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("GeneralContent", content_size,
			ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("GeneralSidebar", sidebar_size,
			ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
			ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->settings_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->settings_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("GeneralContentArea", ZERO_F,
			ImGuiChildFlags_Borders, 0);

		switch (state->settings_tab)
		{
			case 0:
				draw_graphics_menu ();
				break;
			case 1:
				draw_engine_menu ();
				break;
			case 2:
				break;
			case 3:
				break;
			case 4:
				break;
			case 5:
				draw_status_menu ();
				break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

static void
EnhancementsTab (TabState *state, ImVec2 content_size, ImVec2 sidebar_size,
	ImVec2 button_size)
{
	BYTE i;
	const char *tab_names[] = {
		"Quality of Life",
		"Skips & Speed-ups",
		"Visuals",
		"Fixes",
		"Difficulty",
		"Cheats"
		//		"Timers"
	};

	if (ImGui_BeginTabItem ("Enhancements", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("EnhancementsContent", content_size,
			ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("EnhancementsSidebar", sidebar_size,
			ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
			ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->enhancements_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->enhancements_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("EnhancementsContentArea", ZERO_F,
			ImGuiChildFlags_Borders, 0);

		switch (state->enhancements_tab)
		{
			case 0:
				//ImGui_Text ("Quality of Life");
				draw_qol_menu ();
				break;
			case 1:
				ImGui_Text ("Skips & Speed-ups");
				break;
			case 2:
				//ImGui_Text ("Visuals");
				draw_visual_menu ();
				break;
			case 3:
				ImGui_Text ("Fixes");
				break;
			case 4:
				ImGui_Text ("Difficulty");
				break;
			case 5:
				draw_cheats_menu ();
				break;
				//case 6:
				//	ImGui_Text ("Timers");
				//	break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

static void
RandomizerTab (TabState *state, ImVec2 content_size, ImVec2 sidebar_size,
	ImVec2 button_size)
{
	BYTE i;
	const char *tab_names[] = {
		"Seed Settings",
		"Enhancements"
	};

	if (ImGui_BeginTabItem ("Randomizer", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("RandomizerContent", content_size,
			ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("RandomizerSidebar", sidebar_size,
			ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
			ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->randomizer_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->randomizer_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("RandomizerContentArea", ZERO_F,
			ImGuiChildFlags_Borders, 0);

		switch (state->randomizer_tab)
		{
			case 0:
				ImGui_Text ("Seed Settings");
				break;
			case 1:
				ImGui_Text ("Enhancements");
				break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

static void
DevToolsTab (TabState *state, ImVec2 content_size, ImVec2 sidebar_size,
	ImVec2 button_size)
{
	BYTE i;
	const char *tab_names[] = {
		"General",
		"Stats",
		"Console",
		"Save Editor"
	};

	if (ImGui_BeginTabItem ("Dev Tools", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("DevToolsContent", content_size,
			ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("DevToolsSidebar", sidebar_size,
			ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
			ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->devtools_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->devtools_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("DevToolsContentArea", ZERO_F,
			ImGuiChildFlags_Borders, 0);

		switch (state->devtools_tab)
		{
			case 0:
				ImGui_Text ("General Dev Tools");
				break;
			case 1:
				ImGui_Text ("Statistics");
				break;
			case 2:
				ImGui_Text ("Console");
				break;
			case 3:
				ImGui_Text ("Save Editor");
				break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

void UQM_ImGui_Tabs (TabState *state, ImVec2 content_size,
		ImVec2 sidebar_size, ImVec2 button_size)
{
	if (ImGui_BeginTabBar ("MainTabs", 0))
	{
		GeneralTab (state, content_size, sidebar_size, button_size);
		EnhancementsTab (state, content_size, sidebar_size, button_size);
		RandomizerTab (state, content_size, sidebar_size, button_size);
		DevToolsTab (state, content_size, sidebar_size, button_size);

		ImGui_EndTabBar ();
	}
}