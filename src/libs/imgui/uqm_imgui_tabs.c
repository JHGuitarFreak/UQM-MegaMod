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

#define NUM_TABS 4
#define SUBTAB_SIZE 10

void
draw_settings_menu (void)
{
	static const char *menu_settings_lbl, *menu_cntrlr_nav, *menu_bg_lbl;
	static const char *bt_reset;

	if (DISPLAY_BOOL == 3)
		ImGui_ColumnsEx (3, "SettingsColumns", false);

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

	{
		ImFontAtlas *font_atlas;
		const char *font_names[2];
		int curr_font;
		int i;

		font_atlas = io->Fonts;

		curr_font = 0;
		for (i = 0; i < font_atlas->Fonts.Size; i++)
		{
			if (io->FontDefault == font_atlas->Fonts.Data[i])
			{
				curr_font = i;
				break;
			}
		}

		for (i = 0; i < font_atlas->Fonts.Size; i++)
			font_names[i] = ImFont_GetDebugName (font_atlas->Fonts.Data[i]);

		ImGui_Text ("FontSelector");
		if (ImGui_ComboChar ("##FontSelector", &curr_font, font_names,
				font_atlas->Fonts.Size))
		{
			io->FontDefault = font_atlas->Fonts.Data[curr_font];
		}
	}

	Spacer ();

	ImGui_Text ("UI Scale");
	ImGui_DragFloatEx ("##UIScale", &style->FontScaleMain,
			0.01f, 0.50f, 5.00f, "%.2f", 0);

	Spacer ();

	{	// Menu Controller Navigation
		bool flags = io->ConfigFlags & ImGuiConfigFlags_NavEnableGamepad;

		if (ImGui_Checkbox (menu_cntrlr_nav, &flags))
		{
			io->ConfigFlags ^= ImGuiConfigFlags_NavEnableGamepad;

			res_PutBoolean ("imgui.nav_gamepad", (BOOLEAN)flags);

			imcfg_changed = true;
		}
	}

	ImGui_NewLine ();

	{	// Menu Background Opacity
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

		if (ImGui_ButtonEx (insult[CurSound],
				MAKE_IV2 (150.0f * SCALE_IT, 0.0f)))
		{
			CurSound = 2 + ((COUNT)TFB_Random () % (
					GetSoundCount (PkunkSounds) - 2));
			PlaySound (SetAbsSoundIndex (PkunkSounds, CurSound),
					NotPositional (), NULL, GAME_SOUND_PRIORITY);
		}
	}
}

void
UQM_ImGui_Tabs (TabState *state)
{
	int i, j;
	int active_tab;
	static const char **subtab_names[NUM_TABS] = { NULL };
	static const char **tab_names = NULL;
	static ImVec2 sidebar_size = { 0, 0 };
	static float temp_height = 0;
	float scale = 20.0f * SCALE_IT;
	float max_size = 0;
	float temp_size;

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

	// Begin NavBar
	ImGui_BeginChild ("NavBar", MAKE_IV2 (0.0f, temp_height), IGCF_B, 0);

	ImGui_PushStyleVarImVec2 (SelectableAlign, CENTER_IT);

	for (i = 0; tab_names[i] != NULL; i++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		bool selected = (active_tab == i);

		ImGui_SameLine ();
		ImGui_Dummy (MAKE_IV2 (4.0f * SCALE_IT, 0));
		ImGui_SameLine ();

		text_size = ImGui_CalcTextSize (tab_names[i]);
		button_size = (ImVec2){ text_size.x + scale, scale };

		if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
		{
			active_tab = i;
			state->active_tab = i;
		}
	}
	ImGui_PopStyleVar ();

	temp_height = ImGui_GetItemRectMax ().y - 1;

	ImGui_EndChild ();
	DrawBorderAroundLastItem ();
	// NavBar Ends

	// Calculate sidebar width based on max button width
	if (subtab_names[active_tab] != NULL)
	{
		for (j = 0; subtab_names[active_tab][j] != NULL; j++)
		{
			temp_size = ImGui_CalcTextSize (subtab_names[active_tab][j]).x;

			temp_size += scale;

			if (temp_size > max_size)
				max_size = temp_size;
		}
	}
	sidebar_size.x = max_size + (scale * 2);

	// Sidebar Begins
	ImGui_BeginChild ("Sidebar", sidebar_size, IGCF_B, 0);

	if (subtab_names[active_tab] == NULL)
	{
		ImGui_Text ("No subtabs found for this tab.");
		ImGui_EndChild ();
		return;
	}

	ImGui_PushStyleVarImVec2 (SelectableAlign, CENTER_IT);

	for (j = 0; subtab_names[active_tab][j] != NULL; j++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		float centering;
		bool selected;

		selected = (*active_subtab[active_tab] == j);

		text_size = ImGui_CalcTextSize (subtab_names[active_tab][j]);
		button_size = (ImVec2){ text_size.x + scale, scale };

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
			ImGui_Dummy (MAKE_IV2 (0, 8.0f * SCALE_IT));
	}

	ImGui_PopStyleVar ();
	ImGui_EndChild ();
	DrawBorderAroundLastItem ();
	// Sidebar Ends

	ImGui_SameLine ();

	// Content Begins
	ImGui_BeginChild("Content", ZERO_F, IGCF_B, 0);

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
				case 0: draw_status_menu (); break;
				case 1: draw_devices_menu (); break;
				case 2: draw_gamestates_menu (); break;
				case 3: draw_events_menu (); break;
				default:
					ImGui_Text ("Subtab %d not found.",
							*active_subtab[active_tab]);
					break;
			}
			break;
		default:
			switch (*active_subtab[active_tab])
			{
				default:
					ImGui_Text ("Subtab %d not found.",
						*active_subtab[active_tab]);
					break;
			}
	}
	ImGui_EndChild ();
	DrawBorderAroundLastItem ();
	// Content Ends
}