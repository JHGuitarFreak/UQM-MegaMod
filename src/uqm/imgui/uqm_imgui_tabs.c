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
#define SUBTAB_SIZE 10

#if defined(_WIN32)
#define PLATFORM "Windows"
#elif defined(__linux__)
#define PLATFORM "Linux"
#elif defined(__APPLE__)
#define PLATFORM "macOS"
#else
#define PLATFORM "Unknown"
#endif

#if defined (ANDROID) || defined (__ANDROID__)
#define PLATFORM "Android"
#elif defined(__IOS__)
#define PLATFORM "iOS"
#endif

ImVec2 content_col_size;
SOUND PkunkSounds;

enum
{
	GENERAL_MENU,
	ENHANCEMENTS_MENU,
	JOURNAL_MENU,
	DEBUG_MENU,

	NUM_IMGUI_MENUS
}; // Main ImGui Tabs

void
draw_settings_menu (void)
{
	static const char **insult_factory_str = NULL;

	if (!insult_factory_str)
		insult_factory_str = ImStrArr (GEN_SETT_STR_BASE + 8);

	ImGui_BeginStyledChild ("##Column1", content_col_size, CHILD_FLAGS, 0,
			NULL);

	// Menu Settings
	ImGui_SeparatorText (ImStr (GEN_SETT_STR_BASE));

	Spacer ();

	{	// Font Selector
		int i, font_selection = 0;
		const ImFontAtlas *font_atlas = io->Fonts;
		const int num_fonts = font_atlas->Fonts.Size;
		const char **font_names = HMalloc (num_fonts * sizeof (const char *));

		for (i = 0; i < num_fonts; i++)
		{
			if (io->FontDefault == font_atlas->Fonts.Data[i])
			{
				font_selection = i;
				break;
			}
		}

		for (i = 0; i < num_fonts; i++)
		{
			font_names[i] = ImFont_GetDebugName (font_atlas->Fonts.Data[i]);
		}

		if (ImGui_SizedComboChar (ImStr (GEN_SETT_STR_BASE + 1), // Font Selector
				&font_selection, font_names, num_fonts))
		{
			io->FontDefault = font_atlas->Fonts.Data[font_selection];
			ImPutInt (font_selection);
		}

		HFree (font_names);
	}

	Spacer ();

	// UI Scale
	ImGui_Text (ImStr (GEN_SETT_STR_BASE + 4));

	if (ImGui_Button (ImMakeID (ImStr (GEN_SETT_STR_BASE + 5), // Reset
			"FontScaleMain")))
	{
		style->FontScaleMain = 1.0f;
		easy_PutFloat ("ui_scale", style->FontScaleMain);
		UQM_ScaleAllSizes ();
	}

	ImGui_SameLine ();

	ImGui_SetNextItemWidth (ImGui_CalcItemWidth () -
		(ImGui_GetItemRectSize ().x + style->ItemSpacing.x));

	if (ImGui_DragFloatEx ("##UIScale", &style->FontScaleMain,
		0.01f, 0.5f, 3.0f, "%.2f", ImGuiSliderFlags_ClampOnInput))
	{
		easy_PutFloat ("ui_scale", style->FontScaleMain);
		UQM_ScaleAllSizes ();
	} // UI Scale

	Spacer ();

	{	// Menu Controller Navigation
		bool nav_gamepad =
				io->ConfigFlags & ImGuiConfigFlags_NavEnableGamepad;

		if (ImGui_Checkbox (ImStr (GEN_SETT_STR_BASE + 2),
				&nav_gamepad))
		{
			io->ConfigFlags ^= ImGuiConfigFlags_NavEnableGamepad;
			ImPutBool (nav_gamepad);
		}
	}

	ImGui_NewLine ();

	// Menu Background Opacity
	ImGui_TextUnformatted (ImStr (GEN_SETT_STR_BASE + 3));

	if (ImGui_Button (ImMakeID (ImStr (GEN_SETT_STR_BASE + 5), // RESET
			"BGOpacity")))
	{
		style->Colors[ImGuiCol_ChildBg].w = 0.8f;
		easy_PutFloat ("background_opacity",
			style->Colors[ImGuiCol_ChildBg].w);
	}

	ImGui_SameLine ();

	ImGui_SetNextItemWidth (ImGui_CalcItemWidth () -
			(ImGui_GetItemRectSize ().x + style->ItemSpacing.x));

	if (ImGui_SliderFloat ("##Transparency",
			&style->Colors[ImGuiCol_ChildBg].w, 0.0f, 1.0f))
	{
		easy_PutFloat ("background_opacity",
			style->Colors[ImGuiCol_ChildBg].w);
	} // Menu Background Opacity

	ImGui_NewLine ();

	// Open Addons Folder...
	if (ImGui_Button (ImStr (GEN_SETT_STR_BASE + 6)))
	{
		char buf[PATH_MAX];
		snprintf (buf, sizeof buf, "file:///%s", baseContentPath);
		SDL_OpenURL (buf);
	}

	// Open Config Folder...
	if (ImGui_Button (ImStr (GEN_SETT_STR_BASE + 7)))
	{
		char buf[PATH_MAX];
		snprintf (buf, sizeof buf, "file:///%s", configDirPath);
		SDL_OpenURL (buf);
	}

	ImGui_NewLine ();

	{	// - Sound Test -
		static TimeCount NextTime = 0;
		TimeCount Now = GetTimeCounter ();
		static COUNT CurSound = 0;

		if (Now >= NextTime && CurSound > 1)
			CurSound = 0;

		if (ImGui_ButtonEx (insult_factory_str[CurSound],
				MAKE_IV2 (SCALE_IT (150.0f), 0.0f)))
		{
			if (!PkunkSounds)
				PkunkSounds = CaptureSound (LoadSound (PKUNK_SHIP_SOUNDS));

			CurSound = 2 + ((COUNT)TFB_Random () % (
					GetSoundCount (PkunkSounds) - 2));
			PlaySound (SetAbsSoundIndex (PkunkSounds, CurSound),
					NotPositional (), NULL, GAME_SOUND_PRIORITY);

			NextTime = Now + (ONE_SECOND);
		}
	}

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2", content_col_size, CH_FLAT_NAV, 0,
				NULL);
	}

	ImGui_SeparatorText (ImStr (GEN_SETT_STR_BASE + 9)); // About

	Spacer ();

	ImGui_Text ("%s v0.8.0", UQM_TITLE_S);
	ImGui_Text ("%s%s v%s", (IS_HD ? "HD " : ""),
		UQM_EXTRA_VERSION, MM_BASE_VERSION_S);

	Spacer ();

	ImGui_Text ("Dear ImGui v%s", IMGUI_VERSION);

	Spacer ();

	ImGui_Text ("Platform: %s", PLATFORM);

#ifdef DEBUG
	Spacer ();

	ImGui_Text ("Build Date: %s %s", __DATE__, __TIME__);
	Spacer ();

#	ifdef _MSC_VER
	ImGui_Text ("MSC_VER: %d", _MSC_VER);
	ImGui_Text ("MSC_FULL_VER: %d", _MSC_FULL_VER);
	ImGui_Text ("MSC_BUILD: %d", _MSC_BUILD);
#	endif // _MSC_VER

#	ifdef __MINGW32__
	ImGui_Text ("MINGW32_VERSION: %d.%d", __MINGW32_MAJOR_VERSION,
		__MINGW32_MINOR_VERSION);
#	endif // __MINGW32__

#	ifdef __MINGW64__
	ImGui_Text ("MINGW64_VERSION: %d.%d", __MINGW32_MAJOR_VERSION,
		__MINGW32_MINOR_VERSION);
#	endif // __MINGW32__

#	ifdef __GNUC__
	ImGui_Text ("GCC_VERSION: %d.%d.%d", __GNUC__, __GNUC_MINOR__,
		__GNUC_PATCHLEVEL__);
#	endif // __GNUC__

#	ifdef __clang__
	ImGui_Text ("CLANG_VERSION: %d.%d.%d", __clang_major__,
		__clang_minor__, __clang_patchlevel__);
#	endif // __clang__

	ImGui_NewLine ();

	if (ImGui_Button (ImStr (GEN_SETT_STR_BASE + 10))) // Show ImGui Demo Window
	{
		show_demo = !show_demo;
	}
#endif // DEBUG

	ImGui_EndChild ();
}

void
UQM_ImGui_Tabs (TabState *state)
{
	int i, j;
	int active_tab;
	static const char **subtab_names[NUM_IMGUI_MENUS] = { NULL };
	static const char **tab_names = NULL;
	static float temp_width = 0;
	static float temp_height = 0;
	float scale = SCALE_20F;

	const char *subtab_not = ImStr (NAV_TAB_STR_BASE + 6);
			// Subtab %d not found.
	const char *no_main_menu = ImStr (NAV_TAB_STR_BASE + 7);
			// Not available in the Main Menu...

	if (!tab_names)
	{
		tab_names       = ImStrArr (NAV_TAB_STR_BASE);
		subtab_names[0] = ImStrArr (NAV_TAB_STR_BASE + 1);
		subtab_names[1] = ImStrArr (NAV_TAB_STR_BASE + 2);
		subtab_names[2] = ImStrArr (NAV_TAB_STR_BASE + 3);
		subtab_names[3] = ImStrArr (NAV_TAB_STR_BASE + 4);
	}

	int *active_subtab[] =
	{
		&state->settings_tab,
		&state->enhancements_tab,
		&state->journal_tab,
		&state->devtools_tab
	};

	active_tab = state->active_tab;

	// Without this the border doesn't draw correctly around the navbar
	DrawBorderAroundLastItem ();

	// Begin NavBar
	ImGui_BeginChild ("NavBar", MAKE_IV2 (0.0f, temp_height), IGCF_B, 0);

	for (i = 0; i < NUM_IMGUI_MENUS; i++)
	{
		ImVec2 text_size;
		ImVec2 button_size;
		bool selected = (active_tab == i);

		ImGui_SameLine ();
		ImGui_Dummy (MAKE_IV2 (SCALE_IT (4.0f), 0));
		ImGui_SameLine ();

		text_size = ImGui_CalcTextSize (tab_names[i]);
		button_size = (ImVec2){ text_size.x + scale, scale };

		if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
		{
			active_tab = i;
			state->active_tab = i;
		}
#ifndef DEBUG
		if (i == DEBUG_MENU &&
				ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
		{
			ImGui_BeginTooltip ();
			ImGui_TextColoredUnformatted (ColorToIV4 (BRIGHT_RED_COLOR),
					ImStr (TIP_WARN_STR_BASE)); // Debug Warning
			ImGui_EndTooltip ();
		}
#endif
	}

	temp_height = ImGui_GetItemRectMax ().y - 1;
	ImGui_EndChild (); // NavBar

	DrawBorderAroundLastItem ();
	// NavBar Ends

	if (active_tab != JOURNAL_MENU)
	{	// Sidebar Begins
		ImGui_BeginChild ("Sidebar", MAKE_IV2 (temp_width, 0.0f), IGCF_B, 0);

		if (subtab_names[active_tab] == NULL)
		{
			ImGui_Text (ImStr (NAV_TAB_STR_BASE + 5));
			ImGui_EndChild ();
			return;
		}

		ImGui_BeginGroup ();

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

			ImGui_Dummy (MAKE_IV2 (0, SCALE_IT (8.0f)));

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

		ImGui_EndGroup ();

		temp_width = ImGui_GetItemRectMax ().x + scale;

		ImGui_EndChild (); // Sidebar
		DrawBorderAroundLastItem ();

		ImGui_SameLine ();
	} // Sidebar Ends

	// Content Begins
	ImGui_BeginChild ("Content", ZERO_F, IGCF_B, 0);
	ImGui_BeginStyledChild ("##PagePadding", ZERO_F, CH_FLAT_NAV, 0, NULL);

	GetColumnSize (&content_col_size, NUM_COLUMNS);

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
			ImGui_Text (subtab_not, *active_subtab[active_tab]);
			break;
		}
		break;
	case 1:
		switch (*active_subtab[active_tab])
		{
		case 0: draw_qol_menu (); break;
		case 1: draw_visual_menu (); break;
		case 2: draw_cheats_menu (); break;
		default:
			ImGui_Text (subtab_not, *active_subtab[active_tab]);
			break;
		}
		break;
	case 2:
		if (IN_MAIN_MENU)
		{
			ImGui_Text (no_main_menu);
			break;
		}
		else
		{
			switch (*active_subtab[active_tab])
			{
			case 0: draw_journal_menu (temp_height); break;
			default:
				ImGui_Text (subtab_not, *active_subtab[active_tab]);
				break;
			}
			break;
		}
	case 3:
		if (IN_MAIN_MENU)
		{
			ImGui_Text (no_main_menu);
			break;
		}
		else
		{
			switch (*active_subtab[active_tab])
			{
			case 0: draw_status_menu (); break;
			case 1: draw_devices_menu (); break;
			case 2: draw_gamestates_menu (); break;
			case 3: draw_events_menu (); break;
			case 4: draw_stars_menu (); break;
			default:
				ImGui_Text (subtab_not, *active_subtab[active_tab]);
				break;
			}
			break;
		}
	default:
		switch (*active_subtab[active_tab])
		{
		default:
			ImGui_Text (subtab_not, *active_subtab[active_tab]);
			break;
		}
	}

	ImGui_EndChild (); // ##PagePadding
	ImGui_EndChild (); // Content
	DrawBorderAroundLastItem ();
	// Content Ends
}