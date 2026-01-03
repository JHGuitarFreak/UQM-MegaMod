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
	const char *pc_or_3do[] = { "DOS", "3DO"};
	const char *star_backgrounds[] = { "DOS", "3DO", "UQM", "HD-mod" };
	const char *dos_3do_uqm[] = { "DOS", "3DO", "UQM" };
	const char *menu_styles[] = { "Text", "Pictographic" };
	const char *font_styles[] = { "Gradients", "Flat" };
	const char *cutscene_style[] = { "Slides", "Movie" };
	const char *melee_style[] = { "Stepped", "Smooth" };
	const char *engine_style[] = { "Green Engine", "Red Engine" };
	const char *scroll_style[] = { "Per-Page", "Smooth" };
	const char *slave_shields[] = { "Static", "Pulsating" };
	const char *sphere_tint[] = { "Shaded", "Plain" };
	const char *stats_display[] =
	{
		"Text",
		"Pictograms",
		"Text (6014)",
		"Pictograms (6014)"
	};

	if (!IN_MAIN_MENU)
	{
		ImGui_TextWrappedColored (IV4_YELLOW_COLOR,
				"Some of the options in this part of the menu need a full "
				"screen update for them to take full effect. If in doubt "
				"enter/leave planet orbit, leave and re-enter "
				"the current star system, or enter and leave the Starbase");
		Spacer ();
	}

	ImGui_ColumnsEx (DISPLAY_BOOL, "EngineColumns", false);

	// User Interface
	{
		ImGui_SeparatorText ("User Interface");

		{
			ImGui_BeginDisabled (true);

			ImGui_Text ("Platform UI:");
			if (ImGui_ComboChar ("##PlatformUI", (int *)&optWindowType,
					dos_3do_uqm, 3))
			{
				// Add switching code here
			}

			ImGui_TextWrappedColored (IV4_RED_COLOR,
					"WARNING! The Platform UI option can not be changed "
					"in the GUI at this time. To change this option you "
					"must use the Setup Menu.");
			Spacer ();

			ImGui_EndDisabled ();
		}

		{
			int which_menu = is3DO (optWhichMenu);
			ImGui_Text ("Menu Style:");
			if (ImGui_ComboChar ("##MenuStyle", &which_menu,
					menu_styles, 2))
			{
				optWhichMenu = ToCons (which_menu);
				res_PutBoolean ("config.textmenu", (BOOLEAN)which_menu);
				config_changed = true;
			}
		}

		Spacer ();

		UQM_ImGui_CheckBox ("DOS Side Menu", &optDosMenus, "mm.dosMenus");

		Spacer ();

		{
			int which_fonts = is3DO (optWhichFonts);
			ImGui_Text ("Font Style:");
			if (ImGui_ComboChar ("##FontStyle", &which_fonts,
					font_styles, 2))
			{
				optWhichFonts = ToCons (which_fonts);
				res_PutBoolean ("config.textgradients", (BOOLEAN)which_fonts);
				config_changed = true;
			}
		}

		{
			ImGui_BeginDisabled (true);

			int which_intro = is3DO (optWhichIntro);
			ImGui_Text ("Cutscenes:");
			if (ImGui_ComboChar ("##Cutscenes", &which_intro,
					cutscene_style, 2))
			{
				// Add switching code here
			}

			ImGui_TextWrappedColored (IV4_RED_COLOR,
					"WARNING! The Cutscene option can not be changed in "
					"the GUI at this time. To change this option you must "
					"use the Setup Menu.");
			Spacer ();

			ImGui_EndDisabled ();
		}

		Spacer ();

		{
			ImGui_BeginDisabled (!IN_MAIN_MENU);

			int melee_scale = (optMeleeScale != TFB_SCALE_STEP);
			ImGui_Text ("Melee Zoom:");
			if (ImGui_ComboChar ("##MeleeZoom", &melee_scale,
					melee_style, 2))
			{
				optMeleeScale = (melee_scale ? TFB_SCALE_TRILINEAR : TFB_SCALE_STEP);
				res_PutBoolean ("config.smoothmelee", (BOOLEAN)melee_scale);
				config_changed = true;
			}

			if (!IN_MAIN_MENU)
			{
				ImGui_TextWrappedColored (IV4_RED_COLOR,
						"WARNING! Melee Zoom can only be "
						"changed while in the Main Menu!");
				Spacer ();
			}

			ImGui_EndDisabled ();
		}

		{
			int engine_color = is3DO (optFlagshipColor);
			ImGui_Text ("Flagship Engine Color:");
			if (ImGui_ComboChar ("##EngineColor", &engine_color,
					engine_style, 2))
			{
				optFlagshipColor = ToCons (engine_color);
				res_PutBoolean ("mm.flagshipColor", (BOOLEAN)engine_color);
				mmcfg_changed = true;
			}
		}

		{
			int scr_trans = is3DO (optScrTrans);
			ImGui_Text ("Screen Transitions:");
			if (ImGui_ComboChar ("##ScreenTransitions", &scr_trans,
					pc_or_3do, 2))
			{
				optScrTrans = ToCons (scr_trans);
				res_PutBoolean ("mm.scrTransition", (BOOLEAN)scr_trans);
				mmcfg_changed = true;
			}
		}

		ImGui_NewLine ();
	}

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	// Conversation Screen
	{
		ImGui_SeparatorText ("Conversation Screen");

		{
			int smooth_scroll = is3DO (optSmoothScroll);
			ImGui_Text ("Scroll Style:");
			if (ImGui_ComboChar ("##ScrollStyle", &smooth_scroll,
					scroll_style, 2))
			{
				optSmoothScroll = ToCons (smooth_scroll);
				res_PutBoolean ("config.smoothscroll", (BOOLEAN)smooth_scroll);
				config_changed = true;
			}
		}

		Spacer ();

		{
			ImGui_BeginDisabled (true);

			ImGui_Checkbox ("Speech", (bool *)&optSpeech);

			if (ImGui_IsItemHovered (ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui_SetTooltip (
					"This option can only be changed in the Setup Menu.");
			}

			ImGui_EndDisabled ();
		}

		UQM_ImGui_CheckBox ("Subtitles", &optSubtitles, "config.subtitles");

		Spacer ();

		{
			int scope_style = is3DO (optScopeStyle);
			ImGui_Text ("Oscilloscope Style:");
			if (ImGui_ComboChar ("##OscilloscopelStyle", &scope_style,
					pc_or_3do, 2))
			{
				optScopeStyle = ToCons (scope_style);
				res_PutBoolean ("mm.scopeStyle", (BOOLEAN)scope_style);
				mmcfg_changed = true;
			}
		}

		ImGui_NewLine ();
	}

	// Star System View
	{
		ImGui_SeparatorText ("Star System View");

		{
			ImGui_BeginDisabled (!IN_MAIN_MENU);

			int planet_style = is3DO (optPlanetStyle);
			ImGui_Text ("Planet Style:");
			if (ImGui_ComboChar ("##PlanetStyle", &planet_style,
					pc_or_3do, 2))
			{
				optPlanetStyle = ToCons (planet_style);
				res_PutBoolean ("mm.planetStyle", (BOOLEAN)planet_style);
				mmcfg_changed = true;
			}

			if (!IN_MAIN_MENU)
			{
				ImGui_TextWrappedColored (IV4_RED_COLOR,
						"WARNING! Planet Style can only be "
						"changed while in the Main Menu!");
				Spacer ();
			}

			ImGui_EndDisabled ();
		}

		{
			ImGui_Text ("Star Background:");
			if (ImGui_ComboChar ("##StarBackground", &optStarBackground,
				star_backgrounds, 4))
			{
				res_PutInteger ("mm.starBackground", optStarBackground);
				mmcfg_changed = true;
			}
		}

		ImGui_NewLine ();
	}

	if (DISPLAY_BOOL != 1)
		ImGui_NextColumn ();

	// Orbit Screen
	{
		ImGui_SeparatorText ("Orbit Screen");

		{
			ImGui_Text ("Stats Display:");
			if (ImGui_ComboChar ("##StatsStyle", &optWhichCoarseScan,
					stats_display, 4))
			{
				res_PutInteger ("config.iconicscan", optWhichCoarseScan);
				config_changed = true;
			}
		}

		{
			int which_shield = is3DO (optWhichShield);
			ImGui_Text ("Slave Shields:");
			if (ImGui_ComboChar ("##SlaveShields", &which_shield,
					slave_shields, 2))
			{
				optWhichShield = ToCons (which_shield);
				res_PutBoolean ("config.pulseshield", (BOOLEAN)which_shield);
				config_changed = true;
			}
		}

		{
			int scan_style = is3DO (optScanStyle);
			ImGui_Text ("Scanning Style:");
			if (ImGui_ComboChar ("##ScanningStyle", &scan_style,
					pc_or_3do, 2))
			{
				optScanStyle = ToCons (scan_style);
				res_PutBoolean ("mm.scanStyle", (BOOLEAN)scan_style);
				mmcfg_changed = true;
			}
		}

		{
			ImGui_Text ("Sphere Style:");
			if (ImGui_ComboChar ("##SphereStyle", &optScanSphere,
					dos_3do_uqm, 3))
			{
				res_PutInteger ("mm.sphereType", optScanSphere);
				mmcfg_changed = true;
			}
		}

		{
			int tint_sphere = is3DO (optTintPlanSphere);
			ImGui_Text ("Tinted Sphere Scan:");
			if (ImGui_ComboChar ("##TintSphere", &tint_sphere,
					sphere_tint, 2))
			{
				optTintPlanSphere = ToCons (tint_sphere);
				res_PutBoolean ("mm.tintPlanSphere", (BOOLEAN)tint_sphere);
				mmcfg_changed = true;
			}
		}

		{
			int super_pc = is3DO (optSuperPC);
			ImGui_Text ("Lander View Style:");
			if (ImGui_ComboChar ("##LanderView", &super_pc,
					pc_or_3do, 2))
			{
				optSuperPC = ToCons (super_pc);
				res_PutBoolean ("mm.landerStyle", (BOOLEAN)super_pc);
				mmcfg_changed = true;
			}
		}

		ImGui_NewLine ();
	}
}