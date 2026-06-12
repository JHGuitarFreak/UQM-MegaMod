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

#define CHILD_FLAGS ImGuiChildFlags_AutoResizeY \
		| ImGuiChildFlags_AlwaysUseWindowPadding

#if defined(_WIN32)
#define PLATFORM "Windows"
#elif defined(__linux__)
#define PLATFORM "Linux"
#elif defined(__APPLE__)
#define PLATFORM "macOS"
#elif defined (ANDROID) || defined (__ANDROID__)
#define PLATFORM "Android"
#elif defined(__IOS__)
#define PLATFORM "iOS"
#else
#define PLATFORM "Unknown"
#endif

void
draw_settings_menu (void)
{
	static const char *menu_settings_lbl, *menu_cntrlr_nav, *menu_bg_lbl;
	static const char *bt_reset;

	if (DISPLAY_BOOL != 1)
		ImGui_ColumnsEx (2, "SettingsColumns", false);

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
			0.01f, 0.50f, 4.00f, "%.2f", 0);

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

	ImGui_NewLine ();

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	ImGui_SeparatorText ("About");

	Spacer ();

	{
		ImGui_Text ("%s v0.8.0", UQM_TITLE_S);
		ImGui_Text ("%s%s v%s", (IS_HD ? "HD " : ""),
			UQM_EXTRA_VERSION, MM_BASE_VERSION_S);

		Spacer ();

		ImGui_Text ("Platform: %s", PLATFORM);

		Spacer ();

		ImGui_Text ("Build Date: %s %s", __DATE__, __TIME__);
		Spacer ();

#ifdef _MSC_VER
		ImGui_Text ("MSC_VER: %d", _MSC_VER);
		ImGui_Text ("MSC_FULL_VER: %d", _MSC_FULL_VER);
		ImGui_Text ("MSC_BUILD: %d", _MSC_BUILD);
#endif // _MSC_VER

#ifdef __MINGW32__
		ImGui_Text ("MINGW32_VERSION: %d.%d", __MINGW32_MAJOR_VERSION,
				__MINGW32_MINOR_VERSION);
#endif // __MINGW32__

#ifdef __MINGW64__
		ImGui_Text ("MINGW64_VERSION: %d.%d", __MINGW32_MAJOR_VERSION,
				__MINGW32_MINOR_VERSION);
#endif // __MINGW32__

#ifdef __GNUC__
		ImGui_Text ("GCC_VERSION: %d.%d.%d", __GNUC__, __GNUC_MINOR__,
				__GNUC_PATCHLEVEL__);
#endif // __GNUC__

#ifdef __clang__
		ImGui_Text ("CLANG_VERSION: %d.%d.%d", __clang_major__,
				__clang_minor__, __clang_patchlevel__);
#endif // __clang__

		ImGui_NewLine ();

		ImGui_Text ("ImGui v%s", IMGUI_VERSION);

		{
			if (ImGui_Button ("Show ImGui Demo Window"))
			{
				show_demo = !show_demo;
			}
		}

	}

	if (DISPLAY_BOOL != 1)
		ImGui_Columns ();
}

void
UQM_ImGui_Tabs (TabState *state)
{
	int i, j;
	int active_tab;
	static const char **subtab_names[NUM_TABS] = { NULL };
	static const char **tab_names = NULL;
	static float temp_width = 0;
	static float temp_height = 0;
	float scale = 20.0f * SCALE_IT;

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

	// Sidebar Begins
	ImGui_BeginChild ("Sidebar", MAKE_IV2 (temp_width, 0.0f), IGCF_B, 0);

	if (subtab_names[active_tab] == NULL)
	{
		ImGui_Text ("No subtabs found for this tab.");
		ImGui_EndChild ();
		return;
	}

	ImGui_BeginGroup ();

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

		centering = ((temp_width - button_size.x) / 2)
				- style->WindowPadding.x * 2;

		ImGui_Dummy (MAKE_IV2 (0, 8.0f * SCALE_IT));

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
	}

	ImGui_PopStyleVar ();

	ImGui_EndGroup ();

	temp_width = ImGui_GetItemRectMax ().x + scale;

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