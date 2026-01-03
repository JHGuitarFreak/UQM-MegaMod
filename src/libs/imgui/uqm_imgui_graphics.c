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

int imgui_GfxFlags = (int)~0;
int imgui_SavedWidth = 0;
int imgui_SavedHeight = 0;

void draw_graphics_menu (void)
{
	const char *resolutions[] =
	{
		"Native",
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

	ImGui_ColumnsEx (DISPLAY_BOOL, "GraphicsColumns", false);

	ImGui_BeginDisabled (true);
	{
		bool res_factor = resolutionFactor > 0;

		if (ImGui_Checkbox ("HD Mode", &res_factor))
		{
			// Add HD switching code here
		}
	}

	if (ImGui_IsItemHovered (ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui_SetTooltip ("This option is only here for aesthetics.");
	}
	ImGui_EndDisabled ();

	Spacer ();

	{
		ImGui_Text ("Resolution:");
		if (ImGui_ComboChar ("##Resolution",
			(int *)&loresBlowupScale, resolutions, 7))
		{
			if (loresBlowupScale < 6)
			{
				if (!loresBlowupScale)
				{	// No blowup
					imgui_SavedWidth = RES_SCALE (320);
					imgui_SavedHeight = RES_SCALE (DOS_BOOL (240, 200));
				}
				else
				{
					imgui_SavedWidth = 320 * (1 + loresBlowupScale);
					imgui_SavedHeight = DOS_BOOL (240, 200) * (1 + loresBlowupScale);
				}
			}
			res_change = true;
		}
	}

	{
		int cust_res[2] = { SavedWidth, SavedHeight };

		ImGui_Text ("Custom Resolution:");
		ImGui_InputInt2 ("##CustomResolution", cust_res, 0);
		if (ImGui_IsItemDeactivatedAfterEdit ()
				&& cust_res[0] >= 320 && cust_res[1] >= 200)
		{
			imgui_SavedWidth = cust_res[0];
			imgui_SavedHeight = cust_res[1];
			res_change = true;
		}
	}

	ImGui_Text ("Aspect Ratio:");
	if (ImGui_ComboChar ("##AspectRatio", (int *)&optKeepAspectRatio,
		aspect_ratios, 2))
	{
		imgui_SavedWidth = SavedWidth;
		imgui_SavedHeight = SavedHeight;
		res_change = true;
	}

	// Display Mode
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
			imgui_GfxFlags = GfxFlags;
			imgui_GfxFlags &= ~TFB_GFXFLAGS_FS_ANY;
			if (display_mode == 1)
				imgui_GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
			else if (display_mode == 2)
				imgui_GfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
			gfx_change = true;
		}
	}

	Spacer ();

	{
		ImGui_Text ("Gamma:");
		if (ImGui_Button ("Reset"))
		{
			optGamma = 1.0f;
			setGammaCorrection (optGamma);
			res_PutInteger ("config.gamma", (int)(optGamma * GAMMA_SCALE + 0.5));

			config_changed = true;
		}
		ImGui_SameLine ();
		if (ImGui_SliderFloatEx ("##Gamma", &optGamma, 0.4f, 2.5f, "%.2f",
			ImGuiSliderFlags_Logarithmic))
		{
			setGammaCorrection (optGamma);
			res_PutInteger ("config.gamma", (int)(optGamma * GAMMA_SCALE + 0.5));

			config_changed = true;
		}
	}

	Spacer ();

	{
		int curr_scaler;

		switch (GfxFlags & 248) // 11111000 - only scaler flags
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
			const int scaler_list[6] =
			{
				0,
				TFB_GFXFLAGS_SCALE_BILINEAR,
				TFB_GFXFLAGS_SCALE_BIADAPT,
				TFB_GFXFLAGS_SCALE_BIADAPTADV,
				TFB_GFXFLAGS_SCALE_TRISCAN,
				TFB_GFXFLAGS_SCALE_HQXX
			};

			imgui_GfxFlags = GfxFlags;
			imgui_GfxFlags &= ~TFB_GFXFLAGS_SCALE_ANY;
			imgui_GfxFlags |= scaler_list[curr_scaler];
			gfx_change = true;
		}
	}

	Spacer ();

	{
		bool flags = GfxFlags & TFB_GFXFLAGS_SCANLINES;

		if (ImGui_Checkbox ("Scanlines", &flags))
		{
			imgui_GfxFlags = GfxFlags;
			imgui_GfxFlags &= ~TFB_GFXFLAGS_SCANLINES;

			if (flags)
				imgui_GfxFlags |= TFB_GFXFLAGS_SCANLINES;

			gfx_change = true;
		}
	}

	{
		bool flags = GfxFlags & TFB_GFXFLAGS_SHOWFPS;

		if (ImGui_Checkbox ("Show FPS", &flags))
		{
			imgui_GfxFlags = GfxFlags;
			imgui_GfxFlags &= ~TFB_GFXFLAGS_SHOWFPS;

			if (flags)
				imgui_GfxFlags |= TFB_GFXFLAGS_SHOWFPS;

			gfx_change = true;
		}
	}
}