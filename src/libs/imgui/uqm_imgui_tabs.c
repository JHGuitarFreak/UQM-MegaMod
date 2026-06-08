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

#define SUBTAB_SIZE 10

void
draw_settings_menu (void)
{
	static const char *menu_settings_lbl, *menu_cntrlr_nav, *menu_bg_lbl;
	static const char *bt_reset;

	ImGui_ColumnsEx (DISPLAY_BOOL, "SettingsColumns", false);

	if (!menu_settings_lbl)
	{
		menu_settings_lbl = ImStr ("menu_settings_lbl");
		menu_cntrlr_nav = ImStr ("menu_cntrlr_nav");
		menu_bg_lbl = ImStr ("menu_bg_lbl");
		bt_reset = ImStr ("bt_reset");
	}

	// Menu Settings
	ImGui_SeparatorText (menu_settings_lbl);

	Spacer ();

	{	// Menu Controller Navigation
		bool flags = io->ConfigFlags & ImGuiConfigFlags_NavEnableGamepad;

		if (ImGui_Checkbox (menu_cntrlr_nav, &flags))
		{
			io->ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

			if (flags)
				io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

			res_PutBoolean ("imgui.nav_gamepad", (BOOLEAN)flags);

			imcfg_changed = true;
		}
	}

	ImGui_NewLine ();

	{	// Menu Background Opacity
		ImGuiStyle *style = ImGui_GetStyle ();

		ImGui_Text (menu_bg_lbl);
		if (ImGui_Button (bt_reset)) // Reset
		{
			style->Colors[ImGuiCol_ChildBg].w = 0.8f;

			res_PutFloat ("imgui.background_opacity",
					style->Colors[ImGuiCol_ChildBg].w);

			imcfg_changed = true;
		}
		ImGui_SameLine ();
		if (ImGui_SliderFloat ("##Transparency",
				&style->Colors[ImGuiCol_ChildBg].w, 0.0f, 1.0f))
		{
			res_PutFloat ("imgui.background_opacity",
					style->Colors[ImGuiCol_ChildBg].w);

			imcfg_changed = true;
		}
	}

	ImGui_NewLine ();

	{
		static bool button_pressed = false;
		static COUNT CurSound = 0;
		char *insult[17] = 
		{ 
			"- Sound Test -", "- Sound Test -", "Baby!", "Dodo!", "Dummy!",
			"Fool!", "Idiot!", "Jerk!", "Loser!", "Moron!", "Stupid!", "Twit!",
			"Wimp!", "Worm!", "Dummy!", "Nerd!", "Nitwit!"
		};

		if (ImGui_ButtonEx (insult [CurSound], MAKE_IV2 (150.0f, 0.0f)))
		{
			CurSound = 2 + ((COUNT)TFB_Random () % (GetSoundCount (PkunkSounds) - 2));
			PlaySound (SetAbsSoundIndex (PkunkSounds, CurSound),
					NotPositional (), NULL, GAME_SOUND_PRIORITY);
		}
	}
}

static void DrawBorderAroundLastItem (void)
{
	ImDrawList *draw_list = ImGui_GetWindowDrawList ();
	ImVec2 min = ImGui_GetItemRectMin ();
	ImVec2 max = ImGui_GetItemRectMax ();
	ImDrawList_AddRectEx (draw_list, min, max, U32_FRAMEBG_ACT_GS, 4.0f, 0, 1.0f);
}

void
UQM_ImGui_Tabs (TabState *state, ImVec2 content_size,
		ImVec2 sidebar_size)
{
	int active_tab;
	ImGuiStyle *style = ImGui_GetStyle ();
	static const char **subtab_names[4] = { NULL };
	static const char **tab_names = NULL;

	if (!tab_names)
	{
		tab_names = ImStrArr ("tab_names");
		subtab_names[0] = ImStrArr ("subtab_names.0");
		subtab_names[1] = ImStrArr ("subtab_names.1");
		subtab_names[2] = ImStrArr ("subtab_names.2");
		subtab_names[3] = ImStrArr ("subtab_names.3");
	}

	int *active_subtab[] =
	{
		&state->settings_tab,
		&state->enhancements_tab,
		&state->randomizer_tab,
		&state->devtools_tab
	};

	active_tab = state->active_tab;

	DrawBorderAroundLastItem ();

	ImGui_BeginChild ("NavBar", MAKE_IV2 (0.0f, 38), IGCF_B, 0);

	// Begin NavBar
	ImGui_PushStyleVarImVec2 (SelectableAlign, CENTER_IT);

	for (int i = 0; tab_names[i] != NULL; i++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		bool selected = (active_tab == i);

		ImGui_SameLine ();

		ImGui_Dummy (MAKE_IV2 (4, 0));
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
	ImGui_EndChild ();
	DrawBorderAroundLastItem ();
	// End NavBar

	// Sidebar Begins
	ImGui_BeginChild ("Sidebar", sidebar_size, IGCF_B, 0);

	if (subtab_names[active_tab] == NULL)
	{
		ImGui_Text ("No subtabs found for this tab.");
		ImGui_EndChild ();
		return;
	}

	ImGui_PushStyleVarImVec2 (SelectableAlign, CENTER_IT);

	for (int j = 0; subtab_names[active_tab][j] != NULL; j++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		float centering;
		bool selected;

		selected = (*active_subtab[active_tab] == j);

		text_size = ImGui_CalcTextSize (subtab_names[active_tab][j]);
		button_size = (ImVec2){ text_size.x + 20.0f, 20.0f };

		centering = ((sidebar_size.x - button_size.x) / 2)
				- style->WindowPadding.x * 2;

		if (centering > 0.0f)
		{
			ImGui_Dummy (MAKE_IV2 (centering, 0));
			ImGui_SameLine ();
		}

		if (ImGui_SelectableEx (subtab_names[active_tab][j],
			selected, 0, button_size))
		{
			*active_subtab[active_tab] = j;
		}

		if (j >= 0)
			ImGui_Dummy (MAKE_IV2 (0, 8));
	}

	ImGui_PopStyleVar ();
	ImGui_EndChild ();
	DrawBorderAroundLastItem ();
	// Sidebar Ends

	ImGui_SameLine ();

	// Content Begins
	ImGui_BeginChild("Content", content_size, IGCF_B, 0);

	switch (active_tab)
	{
		case 0:
			switch (*active_subtab[active_tab])
			{
				case 0: draw_settings_menu (); break;
				case 1: draw_graphics_menu (); break;
				case 2: draw_engine_menu (); break;
				case 3: draw_audio_menu (); break;
				case 4: draw_controls_menu (); break;
				case 5: draw_adv_menu (); break;
				default:
					ImGui_Text ("Subtab %d not found.",
							*active_subtab[active_tab]);
					break;
			}
			break;
		case 1:
			switch (*active_subtab[active_tab])
			{
				case 0: draw_qol_menu (); break;
				case 1: draw_visual_menu (); break;
				case 2: ImGui_Text ("Difficulty"); break;
				case 3: draw_cheats_menu (); break;
				default:
					ImGui_Text ("Subtab %d not found.",
							*active_subtab[active_tab]);
					break;
			}
			break;
		case 2:
			switch (*active_subtab[active_tab])
			{
				case 0: ImGui_Text ("Seed Settings"); break;
				case 1: ImGui_Text ("Enhancements"); break;
				default:
					ImGui_Text ("Subtab %d not found.",
							*active_subtab[active_tab]);
					break;
			}
			break;
		case 3:
			switch (*active_subtab[active_tab])
			{
				case 0: ImGui_Text ("Dev Tools"); break;
				case 1: ImGui_Text ("Statistics"); break;
				case 2: ImGui_Text ("Save Editor"); break;
				default:
					ImGui_Text ("Subtab %d not found.",
							*active_subtab[active_tab]);
					break;
			}
			break;
	}
	ImGui_EndChild ();
	DrawBorderAroundLastItem ();
	// Content Ends

}