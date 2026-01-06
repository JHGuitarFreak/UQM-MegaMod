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

#define IGCF_B ImGuiChildFlags_AlwaysUseWindowPadding
#define SelectableAlign ImGuiStyleVar_SelectableTextAlign

	void UQM_ImGui_Tabs (TabState *state, ImVec2 content_size,
		ImVec2 sidebar_size, ImVec2 button_size)
{
	int num_tabs;
	int subtab_counts;
	int active_tab;
	ImGuiStyle *style = ImGui_GetStyle ();

	const char *tab_names[] =
			{ "General", "Enhancements", "Randomizer", "Dev Tools" };

	const char *subtab_names[][6] = {
			{"Graphics", "PC / 3DO", "Audio", "Controls", "Advanced", "Status"},
			{"Quality of Life", "Visuals", "Difficulty", "Cheats", NULL, NULL},
			{"Seed Settings", "Enhancements", NULL, NULL, NULL, NULL},
			{"General", "Stats", "Console", "Save Editor", NULL, NULL}
	};

	int *active_subtab[] =
	{
		&state->settings_tab,
		&state->enhancements_tab,
		&state->randomizer_tab,
		&state->devtools_tab
	};

	active_tab = state->active_tab;
	num_tabs = sizeof (tab_names) / sizeof (tab_names[0]);

	ImGui_PushStyleVarImVec2 (SelectableAlign, CENTER_TEXT);

	for (int i = 0; i < num_tabs; i++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		bool selected = (active_tab == i);

		ImGui_SameLine ();
		ImGui_Dummy ((ImVec2) { 4.0f, 0.0f });
		ImGui_SameLine ();

		text_size = ImGui_CalcTextSize (tab_names[i]);
		button_size = (ImVec2){ text_size.x + 20.0f, 20.0f };
		if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
		{
			active_tab = i;
			state->active_tab = i;
		}
	}

	ImGui_PopStyleVar ();

	ImGui_Spacing ();

	ImGui_PushStyleColor (ImGuiCol_ChildBg,
		ImGui_GetColorU32 (ImGuiCol_Separator));
	ImGui_BeginChild ("HorizontalSeparator", (ImVec2) { 0.0f, 1.0f }, 0, 0);
	ImGui_EndChild ();
	ImGui_PopStyleColor ();

	ImGui_BeginChild ("Content", content_size, IGCF_B, 0);
	ImGui_BeginChild ("Sidebar", sidebar_size, IGCF_B, 0);

	ImGui_PushStyleVarImVec2 (SelectableAlign, CENTER_TEXT);

	subtab_counts = sizeof (subtab_names[active_tab]) /
		sizeof (subtab_names[active_tab][0]);

	for (int j = 0; j < subtab_counts; j++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		float centering;
		bool selected;

		if (subtab_names[active_tab][j] == NULL)
			break;

		selected = (*active_subtab[active_tab] == j);

		text_size = ImGui_CalcTextSize (subtab_names[active_tab][j]);
		button_size = (ImVec2){ text_size.x + 20.0f, 20.0f };

		centering = ((sidebar_size.x - button_size.x) / 2)
				- style->WindowPadding.x * 2;

		if (centering > 0.0f)
		{
			ImGui_Dummy ((ImVec2) { centering, 0.0f });
			ImGui_SameLine ();
		}

		if (ImGui_SelectableEx (subtab_names[active_tab][j],
			selected, 0, button_size))
		{
			*active_subtab[active_tab] = j;
		}

		if (j >= 0)
			ImGui_Dummy ((ImVec2) { 0.0f, 8.0f });
	}

	ImGui_PopStyleVar ();
	ImGui_EndChild ();

	ImGui_SameLine ();

	ImGui_PushStyleColor (ImGuiCol_ChildBg,
		ImGui_GetColorU32 (ImGuiCol_Separator));
	ImGui_BeginChild ("VerticalSeparator", (ImVec2) { 1.0f, 0.0f }, 0, 0);
	ImGui_EndChild ();
	ImGui_PopStyleColor ();

	ImGui_SameLine ();

	ImGui_BeginChild ("ContentArea", ZERO_F, IGCF_B, 0);

	switch (active_tab)
	{
		case 0:
			switch (*active_subtab[active_tab])
			{
				case 0: draw_graphics_menu (); break;
				case 1: draw_engine_menu (); break;
				case 2: draw_audio_menu (); break;
				case 3: draw_controls_menu (); break;
				case 4: draw_adv_menu (); break;
				case 5: draw_status_menu (); break;
			}
			break;
		case 1:
			switch (*active_subtab[active_tab])
			{
				case 0: draw_qol_menu (); break;
				case 1: draw_visual_menu (); break;
				case 2: ImGui_Text ("Difficulty"); break;
				case 3: draw_cheats_menu (); break;
			}
			break;
		case 2:
			switch (*active_subtab[active_tab])
			{
				case 0: ImGui_Text ("Seed Settings"); break;
				case 1: ImGui_Text ("Enhancements"); break;
			}
			break;
		case 3:
			switch (*active_subtab[active_tab])
			{
				case 0: ImGui_Text ("General Dev Tools"); break;
				case 1: ImGui_Text ("Statistics"); break;
				case 2: ImGui_Text ("Console"); break;
				case 3: ImGui_Text ("Save Editor"); break;
			}
			break;
	}

	ImGui_EndChild ();
	ImGui_EndChild ();
}