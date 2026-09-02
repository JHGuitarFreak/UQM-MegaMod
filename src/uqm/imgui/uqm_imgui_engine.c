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
	{	
		{	// User Interface
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 12));
								// User Interface
			{
				int window_type = optWindowType;

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 13),
						&window_type, dos_3do_uqm, 3)) // Platform UI
				{
					if (IN_MAIN_MENU)
						GLOBAL (CurrentActivity) = 0;
					else
						GLOBAL (CurrentActivity) |= CHECK_ABORT;

					optWindowType = window_type;
					optRequiresReload = TRUE;

					res_PutInteger ("mm.windowType", optWindowType);
					mmcfg_changed = true;
				}
				if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
				{
					ImGui_BeginTooltip ();
					ImGui_TextColoredUnformatted (
							ColorToIV4 (BRIGHT_RED_COLOR),
							ImStr (TIP_WARN_STR_BASE + 1)); // Reload Warning
					ImGui_EndTooltip ();
				}
			}

			{
				int which_menu = is3DO (optWhichMenu);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 14),
						&which_menu, menu_styles, 2)) // Menu Style
				{
					optWhichMenu = ToCons (which_menu);
					res_PutBoolean ("config.textmenu", (BOOLEAN)which_menu);
					config_changed = true;
				}
			}

			ImGui_NewLine ();

			UQM_ImGui_CheckBox (ImStr (GEN_ENG_STR_BASE + 15), &optDosMenus,
					"mm.dosMenus", false); // DOS Side Menu

			ImGui_NewLine ();

			{
				int which_fonts = is3DO (optWhichFonts);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 16),
						&which_fonts, font_styles, 2)) // Font Style
				{
					optWhichFonts = ToCons (which_fonts);
					res_PutBoolean ("config.textgradients",
							(BOOLEAN)which_fonts);
					config_changed = true;
				}
			}

			{
				int which_intro = is3DO (optWhichIntro);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 17),
						&which_intro, cutscene_style, 2)) // Cutscenes
				{
					if (IN_MAIN_MENU)
						GLOBAL (CurrentActivity) = 0;
					else
						GLOBAL (CurrentActivity) |= CHECK_ABORT;

					optWhichIntro = ToCons (which_intro);
					optRequiresReload = TRUE;

					res_PutBoolean ("config.3domovies", (BOOLEAN)which_intro);
					config_changed = true;
				}
				if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
				{
					ImGui_BeginTooltip ();
					ImGui_TextColoredUnformatted (
							ColorToIV4 (BRIGHT_RED_COLOR),
							ImStr (TIP_WARN_STR_BASE + 1)); // Reload Warning
					ImGui_EndTooltip ();
				}
			}

			Spacer ();

			{
				ImGui_BeginDisabled (!IN_MAIN_MENU);

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

				ImGui_EndDisabled ();
			}

			{	// Flagship Engine Color
				int engine_color = is3DO (optFlagshipColor);
										// Flagship Engine Color
				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 19),
						&engine_color, engine_style, 2))
				{
					optFlagshipColor = ToCons (engine_color);
					res_PutBoolean ("mm.flagshipColor", (BOOLEAN)engine_color);
					mmcfg_changed = true;
				}
			}

			{
				int scr_trans = is3DO (optScrTrans);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 20),
						&scr_trans, pc_or_3do, 2)) // Screen Transitions
				{
					optScrTrans = ToCons (scr_trans);
					res_PutBoolean ("mm.scrTransition", (BOOLEAN)scr_trans);
					mmcfg_changed = true;
				}
			}
			ImGui_NewLine ();
		}
	
}
	if (NUM_COLUMNS > 2)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2", content_col_size, CHILD_FLAGS,
				0, NULL);
	}

	{
		{	// Conversation Screen
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 21));
								// Conversation Screen

			{
				int smooth_scroll = is3DO (optSmoothScroll);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 22),
						&smooth_scroll, scroll_style, 2)) // Scroll Style
				{
					optSmoothScroll = ToCons (smooth_scroll);
					res_PutBoolean ("config.smoothscroll",
							(BOOLEAN)smooth_scroll);
					config_changed = true;
				}
			}

			ImGui_NewLine ();

			{
				ImGui_BeginDisabled (true);

				ImGui_Checkbox (ImStr (GEN_ENG_STR_BASE + 23), // Speech
						(bool *)&optSpeech);

				if (ImGui_IsItemHovered (ImGuiHoveredFlags_AllowWhenDisabled))
				{
					ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 4));
									// Setup Menu Warning
				}

				ImGui_EndDisabled ();
			}

			UQM_ImGui_CheckBox (ImStr (GEN_ENG_STR_BASE + 24), // Subtitles
					&optSubtitles, "config.subtitles", false);

			ImGui_NewLine ();

			{
				int scope_style = is3DO (optScopeStyle);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 25),
						&scope_style, pc_or_3do, 2)) // Oscilloscope Style
				{
					optScopeStyle = ToCons (scope_style);
					res_PutBoolean ("mm.scopeStyle", (BOOLEAN)scope_style);
					mmcfg_changed = true;
				}
			}
			ImGui_NewLine ();
		}

		{	// Star System View
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 26));
								// Star System View

			{
				ImGui_BeginDisabled (!IN_MAIN_MENU);

				int planet_style = is3DO (optPlanetStyle);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 27),
						&planet_style, pc_or_3do, 2)) // Planet Style
				{
					optPlanetStyle = ToCons (planet_style);
					res_PutBoolean ("mm.planetStyle", (BOOLEAN)planet_style);
					mmcfg_changed = true;
				}

				if (!IN_MAIN_MENU)
				{
					ImGui_TextWrappedColored (ColorToIV4 (BRIGHT_RED_COLOR),
							ImStr (TIP_WARN_STR_BASE + 3));
								// Main Menu Warning
					Spacer ();
				}

				ImGui_EndDisabled ();
			}

			{
				int star_background = optStarBackground;
											// Star Background
				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 28),
						&star_background, star_backgrounds, 4))
				{
					optStarBackground = star_background;
					res_PutInteger ("mm.starBackground", optStarBackground);
					mmcfg_changed = true;
				}
			}
			ImGui_NewLine ();
		}
	}

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column3", content_col_size, CHILD_FLAGS,
				0, NULL);
	}

	{
		{	// Orbit Screen
			ImGui_SeparatorText (ImStr (GEN_ENG_STR_BASE + 29));
								// Orbit Screen
			{
				int which_scan = optWhichCoarseScan;

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 30),
						&which_scan, stats_display, 4)) // Stats Display
				{
					optWhichCoarseScan = which_scan;
					res_PutInteger ("config.iconicscan", optWhichCoarseScan);
					config_changed = true;
				}
			}

			{
				int which_shield = is3DO (optWhichShield);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 31),
						&which_shield, slave_shields, 2)) // Slave Shields
				{
					optWhichShield = ToCons (which_shield);
					res_PutBoolean ("config.pulseshield", (BOOLEAN)which_shield);
					config_changed = true;
				}
			}

			{
				int scan_style = is3DO (optScanStyle);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 32),
						&scan_style, pc_or_3do, 2)) // Scanning Style
				{
					optScanStyle = ToCons (scan_style);
					res_PutBoolean ("mm.scanStyle", (BOOLEAN)scan_style);
					mmcfg_changed = true;
				}
			}

			{
				int sphere_style = optScanSphere;

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 33),
						&sphere_style, dos_3do_uqm, 3)) // Sphere Style
				{
					optScanSphere = sphere_style;
					res_PutInteger ("mm.sphereType", optScanSphere);
					mmcfg_changed = true;
				}
			}

			{
				int tint_sphere = is3DO (optTintPlanSphere);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 34),
						&tint_sphere, sphere_tint, 2)) // Tinted Sphere Scan
				{
					optTintPlanSphere = ToCons (tint_sphere);
					res_PutBoolean ("mm.tintPlanSphere", (BOOLEAN)tint_sphere);
					mmcfg_changed = true;
				}
			}

			{
				int super_pc = is3DO (optSuperPC);

				if (ImGui_SizedComboChar (ImStr (GEN_ENG_STR_BASE + 35),
						&super_pc, pc_or_3do, 2)) // Lander View Style
				{
					optSuperPC = ToCons (super_pc);
					res_PutBoolean ("mm.landerStyle", (BOOLEAN)super_pc);
					mmcfg_changed = true;
				}
			}
			ImGui_NewLine ();
		}
	}
	ImGui_EndChild ();
}