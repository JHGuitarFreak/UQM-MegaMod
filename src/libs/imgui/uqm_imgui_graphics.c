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

void draw_graphics_menu (void)
{
	const char *resolutions[] =
	{
		"Default",
		"640x480",
		"960x720",
		"1280x960",
		"1600x1200",
		"1920x1440",
		"Custom"
	};
	const char *aspect_ratios[] = { "Any", "Force 4:3" };
	const char *display_modes[] =
	{
		"Windowed",
		"Exclusive Fullscreen",
		"Borderless Fullscreen"
	};
	const char *scalers[] =
	{
		"None",
		"Bilinear",
		"Adapt. Bilinear",
		"Adv. Bilinear",
		"Triscan (Scale2x)",
		"HQ (2x)"
	};

	bool in_main_menu = GLOBAL (CurrentActivity) == 0;

	ImGui_ColumnsEx (DISPLAY_BOOL, "GraphicsColumns", false);

	ImGui_BeginDisabled (in_main_menu);
	{
		bool res_factor = resolutionFactor > 0;

		if (ImGui_Checkbox ("HD Mode", &res_factor))
		{
			// Add HD switching code here
		}
	}

	if (ImGui_IsItemHovered (ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui_SetTooltip ("No can change!");
	}
	ImGui_EndDisabled ();

	Spacer ();

	ImGui_Text ("Resolution:");
	if (ImGui_ComboChar ("##Resolution",
		(int *)&loresBlowupScale, resolutions, 7))
	{
		// Add resolution switch code here
	}

	{	// Edit Captain's Name
		char CustomResolution[SIS_NAME_SIZE];

		snprintf ((char *)&CustomResolution, sizeof (CustomResolution),
			"%dx%d", SavedWidth, SavedHeight);

		ImGui_Text ("Custom Resolution:");
		if (ImGui_InputText ("##CustomeResolution", CustomResolution,
			sizeof (CustomResolution), 0))
		{
			// Add custom resolution code here
		}
	}

	ImGui_Text ("Aspect Ratio:");
	if (ImGui_ComboChar ("##AspectRatio", (int *)&optKeepAspectRatio,
		aspect_ratios, 2))
	{
		// Add aspect ratio code here
	}

	{
		int display_mode = 0;

		if (GfxFlags & TFB_GFXFLAGS_FULLSCREEN)
			display_mode = 2;
		else if (GfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN)
			display_mode = 1;
		else
			display_mode = 0;

		ImGui_Text ("Display Mode:");
		if (ImGui_ComboChar ("##DisplayMode", (int *)&display_mode,
			display_modes, 3))
		{
			// Add display mode code here
		}
	}

	Spacer ();

	{
		ImGui_Text ("Gamma:");
		if (ImGui_Button ("Reset"))
		{
			optGamma = 1.0f;
			setGammaCorrection (optGamma);
		}
		ImGui_SameLine ();
		if (ImGui_SliderFloatEx ("##Gamma", &optGamma, 0.4f, 2.5f, "%.2f",
			ImGuiSliderFlags_Logarithmic))
		{
			setGammaCorrection (optGamma);
		}
	}

	Spacer ();

	{
		int flags, curr_scaler;

		flags = GfxFlags & 248; // 11111000 - only scaler flags

		switch (flags)
		{
			case TFB_GFXFLAGS_SCALE_BILINEAR:
				curr_scaler = 1;
				break;
			case TFB_GFXFLAGS_SCALE_BIADAPT:
				curr_scaler = 2;
				break;
			case TFB_GFXFLAGS_SCALE_BIADAPTADV:
				curr_scaler = 3;
				break;
			case TFB_GFXFLAGS_SCALE_TRISCAN:
				curr_scaler = 4;
				break;
			case TFB_GFXFLAGS_SCALE_HQXX:
				curr_scaler = 5;
				break;
			default:
				curr_scaler = 0;
				break;
		}

		ImGui_Text ("Scaler:");
		if (ImGui_ComboChar ("##Scaler", &curr_scaler,
			scalers, 6))
		{
			// Add scaler code here
		}
	}

	Spacer ();

	{
		bool flags = GfxFlags & TFB_GFXFLAGS_SCANLINES;

		if (ImGui_Checkbox ("Scanlines", &flags))
		{
			// Add scanline code here
		}
	}

	{
		bool flags = GfxFlags & TFB_GFXFLAGS_SHOWFPS;

		if (ImGui_Checkbox ("Show FPS", &flags))
		{
			// Add show FPS code here
		}
	}
}