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

#include "uqm/fmv.h"
#include "libs/vidlib.h"
#include "libs/graphics/widgets.h"
#include "libs/graphics/bbox.h"

int imgui_GfxFlags = (int)~0;
int imgui_SavedWidth = 0;
int imgui_SavedHeight = 0;

void draw_graphics_menu (void)
{
	static const char **resolutions = NULL;
	static const char **aspect_ratios = NULL;
	static const char **display_modes = NULL;
	static const char **scalers = NULL;

	if (!resolutions)
	{
		resolutions = ImStrArr (GEN_GFX_STR_BASE);
		aspect_ratios = ImStrArr (GEN_GFX_STR_BASE + 1);
		display_modes = ImStrArr (GEN_GFX_STR_BASE + 2);
		scalers = ImStrArr (GEN_GFX_STR_BASE + 3);
	}

	ImGui_BeginStyledChild ("##Column1", content_col_size, CHILD_FLAGS,
			0, NULL);
	{
		ImGui_SeparatorText (ImStr (GEN_GFX_STR_BASE + 4)); // Graphics Options

		{	// HD Mode
			bool res_factor = resolutionFactor > 0;

			if (ImGui_Checkbox (ImStr (GEN_GFX_STR_BASE + 5), &res_factor))
			{					// HD Mode
				if (IN_MAIN_MENU)
					GLOBAL (CurrentActivity) = 0;
				else
					GLOBAL (CurrentActivity) |= CHECK_ABORT;

				resolutionFactor = res_factor ? 2 : 0;
				optRequiresReload = TRUE;

				res_PutInteger ("config.resolutionfactor", resolutionFactor);
				config_changed = true;
			}
			if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
			{
				ImGui_BeginTooltip ();
				ImGui_TextColoredUnformatted (ColorToIV4 (BRIGHT_RED_COLOR),
						ImStr (TIP_WARN_STR_BASE + 1)); // Reload Warning
				ImGui_EndTooltip ();
			}
		}

		Spacer ();

		{	// Resolution
			if (ImGui_SizedComboChar (ImStr (GEN_GFX_STR_BASE + 6),
					(int *)&loresBlowupScale, resolutions, 7)) // Resolution
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
						imgui_SavedHeight = DOS_BOOL (240, 200) *
								(1 + loresBlowupScale);
					}
				}
				res_change = true;
			}
		}

		Spacer ();

		{	// Custom Resolution
			int cust_res[2] = { SavedWidth, SavedHeight };

			ImGui_Text (ImStr (GEN_GFX_STR_BASE + 7)); // Custom Resolution
			ImGui_InputInt2 ("##CustomResolution", cust_res, 0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
				&& cust_res[0] >= 320 && cust_res[1] >= 200)
			{
				imgui_SavedWidth = cust_res[0];
				imgui_SavedHeight = cust_res[1];
				res_change = true;
			}
		}

		Spacer ();

		{	// Aspect Ratio
			if (ImGui_SizedComboChar (ImStr (GEN_GFX_STR_BASE + 8), // Aspect Ratio
					(int *)&optKeepAspectRatio, aspect_ratios, 3))
			{
				imgui_SavedWidth = SavedWidth;
				imgui_SavedHeight = SavedHeight;
				res_change = true;
			}
		}

		Spacer ();

		{	// Display Mode
			int display_mode = 0;

			if (GfxFlags & TFB_GFXFLAGS_FULLSCREEN)
				display_mode = 2;
			else if (GfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN)
				display_mode = 1;
			else
				display_mode = 0;

			if (ImGui_SizedComboChar (ImStr (GEN_GFX_STR_BASE + 9),
					(int *)&display_mode, display_modes, 3)) // Display Mode
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

		{	// Gamma
			ImGui_Text (ImStr (GEN_GFX_STR_BASE + 10)); // Gamma
			if (ImGui_Button (ImMakeID (ImStr (GEN_SETT_STR_BASE + 5), // Reset
					ImStr (GEN_GFX_STR_BASE + 10)))) // Gamma
			{
				optGamma = 1.0f;
				setGammaCorrection (optGamma);
				res_PutInteger ("config.gamma",
					(int)(optGamma * GAMMA_SCALE + 0.5));

				config_changed = true;
			}
			ImGui_SameLine ();
			ImGui_SetNextItemWidth (ImGui_CalcItemWidth () -
				(ImGui_GetItemRectSize ().x + style->ItemSpacing.x));
			if (ImGui_SliderFloatEx ("##Gamma", &optGamma, 0.4f, 2.5f, "%.2f",
				ImGuiSliderFlags_Logarithmic))
			{
				setGammaCorrection (optGamma);
				res_PutInteger ("config.gamma",
					(int)(optGamma * GAMMA_SCALE + 0.5));

				config_changed = true;
			}
		}

		Spacer ();

		{	// Scaler
			int curr_scaler = 0;
			switch (GfxFlags & 248) // 11111000 - only scaler flags
			{
			case TFB_GFXFLAGS_SCALE_BILINEAR:   curr_scaler = 1; break;
			case TFB_GFXFLAGS_SCALE_BIADAPT:    curr_scaler = 2; break;
			case TFB_GFXFLAGS_SCALE_BIADAPTADV: curr_scaler = 3; break;
			case TFB_GFXFLAGS_SCALE_TRISCAN:    curr_scaler = 4; break;
			case TFB_GFXFLAGS_SCALE_HQXX:       curr_scaler = 5; break;
			}

			if (ImGui_SizedComboChar (ImStr (GEN_GFX_STR_BASE + 11), // Scaler
					&curr_scaler, scalers, 6))
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

		{	// Scanlines
			bool flags = GfxFlags & TFB_GFXFLAGS_SCANLINES;
								// Scanlines
			if (ImGui_Checkbox (ImStr (GEN_GFX_STR_BASE + 12), &flags))
			{
				imgui_GfxFlags = GfxFlags;
				imgui_GfxFlags &= ~TFB_GFXFLAGS_SCANLINES;

				if (flags)
					imgui_GfxFlags |= TFB_GFXFLAGS_SCANLINES;

				gfx_change = true;
			}
		}

		Spacer ();

		{	// Show FPS
			bool flags = GfxFlags & TFB_GFXFLAGS_SHOWFPS;
								// Show FPS
			if (ImGui_Checkbox (ImStr (GEN_GFX_STR_BASE + 13), &flags))
			{
				imgui_GfxFlags = GfxFlags;
				imgui_GfxFlags &= ~TFB_GFXFLAGS_SHOWFPS;

				if (flags)
					imgui_GfxFlags |= TFB_GFXFLAGS_SHOWFPS;

				gfx_change = true;
			}
		}
	} ImGui_EndChild ();
}