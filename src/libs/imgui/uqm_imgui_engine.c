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
	const char *stats_display[] =
	{
		"Text",
		"Pictograms",
		"Text (6014)",
		"Pictograms (6014)"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "EngineColumns", false);

	// User Interface
	{
		ImGui_SeparatorText ("User Interface");

		{
			ImGui_Text ("Platform UI:");
			if (ImGui_ComboChar ("##PlatformUI", (int *)&optWindowType,
					dos_3do_uqm, 3))
			{
				// Add switching code here
			}
		}

		{
			int which_menu = is3DO (optWhichMenu);
			ImGui_Text ("Menu Style:");
			if (ImGui_ComboChar ("##MenuStyle", &which_menu,
					menu_styles, 2))
			{
				// Add switching code here
			}
		}

		Spacer ();

		ImGui_Checkbox ("DOS Side Menu", (bool *)&optDosMenus);

		Spacer ();

		{
			int which_fonts = is3DO (optWhichFonts);
			ImGui_Text ("Font Style:");
			if (ImGui_ComboChar ("##FontStyle", &which_fonts,
					font_styles, 2))
			{
				// Add switching code here
			}
		}

		{
			int which_intro = is3DO (optWhichIntro);
			ImGui_Text ("Cutscenes:");
			if (ImGui_ComboChar ("##Cutscenes", &which_intro,
					cutscene_style, 2))
			{
				// Add switching code here
			}
		}

		{
			int melee_scale = (optMeleeScale != TFB_SCALE_STEP);
			ImGui_Text ("Melee Zoom:");
			if (ImGui_ComboChar ("##MeleeZoom", &melee_scale,
					melee_style, 2))
			{
				// Add switching code here
			}
		}

		{
			int engine_color = is3DO (optFlagshipColor);
			ImGui_Text ("Flagship Engine Color:");
			if (ImGui_ComboChar ("##EngineColor", &engine_color,
					engine_style, 2))
			{
				// Add switching code here
			}
		}

		{
			int scr_trans = is3DO (optScrTrans);
			ImGui_Text ("Screen Transitions:");
			if (ImGui_ComboChar ("##ScreenTransitions", &scr_trans,
					pc_or_3do, 2))
			{
				// Add switching code here
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
				// Add switching code here
			}
		}

		Spacer ();

			ImGui_Checkbox ("Speech", (bool *)&optSpeech);
			ImGui_Checkbox ("Subtitles", (bool *)&optSubtitles);

		Spacer ();

		{
			int scope_style = is3DO (optScopeStyle);
			ImGui_Text ("Oscilloscope Style:");
			if (ImGui_ComboChar ("##OscilloscopelStyle", &scope_style,
					pc_or_3do, 2))
			{
				// Add switching code here
			}
		}

		ImGui_NewLine ();
	}

	// Star System View
	{
		ImGui_SeparatorText ("Star System View");

		{
			int planet_style = is3DO (optPlanetStyle);
			ImGui_Text ("Planet Style:");
			if (ImGui_ComboChar ("##PlanetStyle", &planet_style,
					pc_or_3do, 2))
			{
				// Add switching code here
			}
		}

		{
			ImGui_Text ("Star Background:");
			ImGui_ComboChar ("##StarBackground", &optStarBackground,
				star_backgrounds, 4);
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
				// Add switching code here
			}
		}

		{
			int which_shield = is3DO (optWhichShield);
			ImGui_Text ("Slave Shields:");
			if (ImGui_ComboChar ("##SlaveShields", &which_shield,
					slave_shields, 2))
			{
				// Add switching code here
			}
		}

		{
			int scan_style = is3DO (optScanStyle);
			ImGui_Text ("Scanning Style:");
			if (ImGui_ComboChar ("##ScanningStyle", &scan_style,
					pc_or_3do, 2))
			{
				// Add switching code here
			}
		}

		{
			ImGui_Text ("Sphere Style:");
			if (ImGui_ComboChar ("##SphereStyle", &optScanSphere,
					dos_3do_uqm, 3))
			{
				// Add switching code here
			}
		}

		{
			int tint_sphere = is3DO (optTintPlanSphere);
			ImGui_Text ("Sphere Scan Overlay:");
			if (ImGui_ComboChar ("##TintSphere", &tint_sphere,
					pc_or_3do, 2))
			{
				// Add switching code here
			}
		}

		{
			int super_pc = is3DO (optSuperPC);
			ImGui_Text ("Lander View Style:");
			if (ImGui_ComboChar ("##LanderView", &super_pc,
					pc_or_3do, 2))
			{
				// Add switching code here
			}
		}

		ImGui_NewLine ();
	}
}