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

static int
ToCons (int opt)
{
	return (opt ? OPT_3DO : OPT_PC);
}

void
draw_engine_menu (void)
{
	static const char **pc_or_3do        = NULL;
	static const char **star_backgrounds = NULL;
	static const char **dos_3do_uqm      = NULL;
	static const char **menu_styles      = NULL;
	static const char **font_styles      = NULL;
	static const char **cutscene_style   = NULL;
	static const char **melee_style      = NULL;
	static const char **engine_style     = NULL;
	static const char **scroll_style     = NULL;
	static const char **slave_shields    = NULL;
	static const char **sphere_tint      = NULL;
	static const char **stats_display    = NULL;

	if (!pc_or_3do)
	{
		dos_3do_uqm      = ImStrArr (GEN_ENG_STR_BASE);
		menu_styles      = ImStrArr (GEN_ENG_STR_BASE +  1);
		font_styles      = ImStrArr (GEN_ENG_STR_BASE +  2);
		cutscene_style   = ImStrArr (GEN_ENG_STR_BASE +  3);
		engine_style     = ImStrArr (GEN_ENG_STR_BASE +  4);
		melee_style      = ImStrArr (GEN_ENG_STR_BASE +  5);
		pc_or_3do        = ImStrArr (GEN_ENG_STR_BASE +  6);
		scroll_style     = ImStrArr (GEN_ENG_STR_BASE +  7);
		star_backgrounds = ImStrArr (GEN_ENG_STR_BASE +  8);
		stats_display    = ImStrArr (GEN_ENG_STR_BASE +  9);
		slave_shields    = ImStrArr (GEN_ENG_STR_BASE + 10);
		sphere_tint      = ImStrArr (GEN_ENG_STR_BASE + 11);
	}

	if (!IN_MAIN_MENU)
	{
		ImGui_BeginStyledChild ("##WarningChild", ZERO_F, CHILD_FLAGS, 0, NULL);
		ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_YELLOW_COLOR),
				ImStr (TIP_WARN_STR_BASE + 2)); // Options Warning
		Spacer ();
		ImGui_EndChild ();
	}

	ImGui_BeginStyledChild ("##Column1", content_col_size, CHILD_FLAGS, 0, NULL);

	// User Interface
	ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 12));
						// User Interface
	
	// Platform UI
	UQM_ComboChar (ImStr (GEN_ENG_STR_BASE + 13), dos_3do_uqm, 3,
			(int *)&optWindowType, "mm.windowType", true);

	// Menu Style
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 14), menu_styles,
			&optWhichMenu, "config.textmenu", false);

	ImGui_NewLine ();

	UQM_ImGui_CheckBox (ImStr (GEN_ENG_STR_BASE + 15), &optDosMenus,
			"mm.dosMenus", false); // DOS Side Menu

	ImGui_NewLine ();

	// Font Style
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 16), font_styles,
			&optWhichFonts, "config.textgradients", false);

	// Cutscenes
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 17), cutscene_style,
			&optWhichIntro, "config.3domovies", true);
	
	ImGui_BeginDisabled (!IN_MAIN_MENU);

	{	// Melee Zoom
		int melee_scale = (optMeleeScale != TFB_SCALE_STEP);

		if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 18),
				&melee_scale, melee_style, 2)) // Melee Zoom
		{
			optMeleeScale = (melee_scale ?
					TFB_SCALE_TRILINEAR : TFB_SCALE_STEP);
			res_PutBoolean ("config.smoothmelee",
					(BOOLEAN)melee_scale);
			config_changed = true;
		}
		if (!IN_MAIN_MENU)
		{
			ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_RED_COLOR),
					ImStr (TIP_WARN_STR_BASE + 3));
						// Main Menu Warning
			Spacer ();
		}
	}

	ImGui_EndDisabled ();

	// Flagship Engine Color
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 19), engine_style,
			&optFlagshipColor, "mm.flagshipColor", false);

	// Screen Transitions
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 20), pc_or_3do,
			&optScrTrans, "mm.scrTransition", false);

	ImGui_NewLine ();

	if (NUM_COLUMNS > 2)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2", content_col_size, CHILD_FLAGS,
				0, NULL);
	}

	// Conversation Screen
	ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 21)); // Conversation Screen

	// Scroll Style
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 22), scroll_style,
			&optSmoothScroll, "config.smoothscroll", false);

	ImGui_NewLine ();

	ImGui_BeginDisabled (true);

	// Speech
	ImGui_Checkbox (ImStr (GEN_ENG_STR_BASE + 23), // Speech
			(bool *)&optSpeech);
	if (ImGui_IsItemHovered (ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 4));
						// Setup Menu Warning
	}

	ImGui_EndDisabled ();

	UQM_ImGui_CheckBox (ImStr (GEN_ENG_STR_BASE + 24), // Subtitles
			&optSubtitles, "config.subtitles", false);

	ImGui_NewLine ();

	// Oscilloscope Style
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 25), pc_or_3do,
			&optScopeStyle, "mm.scopeStyle", false);

	ImGui_NewLine ();

	// Star System View
	ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 26));
						// Star System View

	ImGui_BeginDisabled (!IN_MAIN_MENU);

	// Planet Style
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 27), pc_or_3do,
			&optPlanetStyle, "mm.planetStyle", false);
	if (!IN_MAIN_MENU)
	{
		ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_RED_COLOR),
				ImStr (TIP_WARN_STR_BASE + 3)); // Main Menu Warning
		Spacer ();
	}

	ImGui_EndDisabled ();
	
	// Star Background
	UQM_ComboChar (ImStr (GEN_ENG_STR_BASE + 28), star_backgrounds, 4,
			&optStarBackground, "mm.starBackground", false);

	ImGui_NewLine ();

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column3", content_col_size, CHILD_FLAGS,
				0, NULL);
	}

	// Orbit Screen
	ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 29)); // Orbit Screen

	{	// Stats Display
		int which_scan = optWhichCoarseScan;

		if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 30),
				&which_scan, stats_display, 4)) // Stats Display
		{
			optWhichCoarseScan = which_scan;
			res_PutInteger ("config.iconicscan", optWhichCoarseScan);
			config_changed = true;
		}
	}

	// Slave Shields
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 31), slave_shields,
			&optWhichShield, "config.pulseshield", false);

	// Slave Shields
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 32), pc_or_3do,
			&optScanStyle, "mm.scanStyle", false);

	{	// Sphere Style
		int sphere_style = optScanSphere;

		if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 33),
				&sphere_style, dos_3do_uqm, 3)) // Sphere Style
		{
			optScanSphere = sphere_style;
			res_PutInteger ("mm.sphereType", optScanSphere);
			mmcfg_changed = true;
		}
	}

	// Tinted Sphere Scan
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 34), sphere_tint,
			&optTintPlanSphere, "mm.tintPlanSphere", false);

	// Lander View Style
	UQM_ConsComboChar (ImStr (GEN_ENG_STR_BASE + 35), pc_or_3do,
			&optSuperPC, "mm.landerStyle", false);

	ImGui_NewLine ();

	ImGui_EndChild ();
}