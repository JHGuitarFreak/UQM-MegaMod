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
GetColumnSize (ImVec2 *var, int num_columns)
{
	var->x = (ImGui_GetContentRegionAvail ().x -
			(style->ItemSpacing.x * 2)) / num_columns;
	var->y = 0.0f;
}

void
Spacer (void)
{
	ImGui_Dummy (MAKE_IV2 (0.0f, SCALE_IT (4.0f)));
}

void
Spacer_Column (int num_col)
{
	int i;

	for (i = 0; i < num_col; i++)
	{
		Spacer ();
		ImGui_NextColumn ();
	}
}

ImVec2
Float2Mult (ImVec2 iv2, float mul)
{
	ImVec2 result;
	result.x = iv2.x * mul;
	result.y = iv2.y * mul;
	return result;
}

void
ImGui_TextWrappedColored (ImVec4 col, const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	ImGui_PushStyleColorImVec4 (ImGuiCol_Text, col);
	ImGui_TextWrappedV (fmt, args);
	ImGui_PopStyleColor ();
	va_end (args);
}

void
ImGui_HorizontalSeparator (const char *str_id)
{
	ImGui_PushStyleColor (ImGuiCol_ChildBg, STYLE_COLOR (ImGuiCol_FrameBg));
	ImGui_BeginChild (str_id, MAKE_IV2 (0, 1), 0, 0);
	ImGui_EndChild ();
	ImGui_PopStyleColor ();
}

bool
ImGui_SizedComboChar (const char *label, int *curr_item,
		const char *const items[], int items_count)
{
	bool temp = false;
	char buf[100];
	float column_width = ImGui_GetColumnWidth (ImGui_GetColumnIndex ());
	float combo_width = column_width * 0.75f;
	float center_offset = (column_width - combo_width) * 0.5f
			- style->WindowPadding.x * 2;

	snprintf (buf, sizeof buf, "##%s", label);

	ImGui_AlignTextToFramePadding ();
	ImGui_TextUnformatted (label);
	ImGui_SetCursorPosX (ImGui_GetCursorPosX () + center_offset);
	ImGui_SetNextItemWidth (combo_width);

	ImGui_PushStyleVarImVec2 (ImGuiStyleVar_SelectableTextAlign, CENTER_IT);

	temp = ImGui_ComboChar (buf, curr_item, items, items_count);

	ImGui_PopStyleVar ();

	return temp;
}

// Code adapted from StackOverflow reply
// https://stackoverflow.com/a/70073137
void
ImGui_TextCenteredColored (ImVec4 col, const char *fmt, ...)
{
	va_list args;
	float text_indentation;
	float min_indentation;
	float win_width = ImGui_GetWindowSize ().x;
	float text_width;

	va_start (args, fmt);

	text_width = ImGui_CalcTextSize (fmt).x;
	text_indentation = (win_width - text_width) * 0.5f;
	min_indentation = 20.0f;

	if (text_indentation <= min_indentation)
	{
		text_indentation = min_indentation;
	}

	ImGui_SameLineEx (0, text_indentation);

	ImGui_PushTextWrapPos (win_width - text_indentation);

	ImGui_PushStyleColorImVec4 (ImGuiCol_Text, col);
	ImGui_TextWrappedV (fmt, args);
	ImGui_PopStyleColor ();

	ImGui_PopTextWrapPos ();

	va_end (args);
}

void
ImGui_BeginStyledChild (const char *str_id, ImVec2 size,
		ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags,
		ImVec4 *col)
{
	ImVec4 colour = MAKE_IV4 (0, 0, 0, 0);
	if (col != NULL)
		colour = *col;

	ImGui_PushStyleColorImVec4 (ImGuiCol_ChildBg, colour);
	ImGui_BeginChild (str_id, size, child_flags, window_flags);
	ImGui_PopStyleColor ();
}

ImVec4
DangerGradient (void)
{
	static DWORD c_index = 0;
	static TimeCount NextTime = 0;
	static int direction = 1;
	const size_t c_count = 10;

	if (GetTimeCounter () >= NextTime)
	{
		c_index = (c_index + direction + c_count) % c_count;
		if (c_index == 0 || c_index == c_count - 1)
			direction = -direction;

		NextTime = GetTimeCounter () + (ONE_SECOND * 3 / 40);
	}

	return MAKE_IV4 (1, (float)c_index / (float)c_count, 0, 1);
}

void
UQM_GetInt (const char *key, int *var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	if (res_HasKey (imgui_key))
	{
		if (!res_IsInteger (imgui_key))
		{
			res_PutInteger (imgui_key, *var);
			imcfg_changed = true;
			return;
		}

		*var = res_GetInteger (imgui_key);

		return;
	}

	res_PutInteger (imgui_key, *var);
	imcfg_changed = true;
}

void
easy_PutInteger (const char *key, int var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	res_PutInteger (imgui_key, var);
	imcfg_changed = true;
}

void
UQM_GetFloat (const char *key, float *var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	if (res_HasKey (imgui_key))
	{
		if (!res_IsFloat (imgui_key))
		{
			res_PutFloat (imgui_key, *var);
			imcfg_changed = true;
			return;
		}

		*var = res_GetFloat (imgui_key);

		return;
	}

	res_PutFloat (imgui_key, *var);
	imcfg_changed = true;
}

void
easy_PutFloat (const char *key, float var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	res_PutFloat (imgui_key, var);
	imcfg_changed = true;
}

void
UQM_GetBool (const char *key, bool *var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	if (res_HasKey (imgui_key))
	{
		if (!res_IsBoolean (imgui_key))
		{
			res_PutBoolean (imgui_key, (BOOLEAN)*var);
			imcfg_changed = true;
			return;
		}

		*var = (bool)res_GetBoolean (imgui_key);

		return;
	}

	res_PutBoolean (imgui_key, (BOOLEAN)*var);
	imcfg_changed = true;
}

void
easy_PutBool (const char *key, bool var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	res_PutBoolean (imgui_key, (BOOLEAN)var);
	imcfg_changed = true;
}

const char *
UQM_GetStr (const char *key)
{
	static char buf[PATH_MAX];
	static char error_buf[PATH_MAX];

	snprintf (buf, sizeof (buf), "imstr.%s", key);

	if (res_IsString (buf))
		return res_GetString (buf);

	snprintf (error_buf, sizeof (error_buf), "STRING_NOT_FOUND: imstr.%s", key);
	return error_buf;
}

const char **
UQM_GetStrArray (const char *key)
{
	static char buf[PATH_MAX];
	static char error_buf[PATH_MAX];
	static const char *error_array[] = { NULL, NULL };

	snprintf (buf, sizeof (buf), "imstr.%s", key);

	if (res_IsStringArray (buf))
		return res_GetStringArray (buf);

	snprintf (error_buf, sizeof (error_buf), "ARRAY_NOT_FOUND: imstr.%s", key);

	error_array[0] = error_buf;
	return error_array;

}

BINARY_RES *
UQM_GetBinary (const char *key)
{
	static char buf[PATH_MAX];

	snprintf (buf, sizeof (buf), "imstr.%s", key);

	if (res_IsBinary (buf))
		return res_GetBinary (buf);

	log_add (log_Error, "BINARY_NOT_FOUND: imstr.%s", key);
	return NULL;
}

void
UQM_BitRegister (const char *gamestate, int size)
{
	int i;
	char buf[40];
	static bool selected[32];
	int bitmask = D_GET_CGAME_STATE (gamestate);
	ImVec2 align = { 0.7f, 0.1f };

	snprintf (buf, sizeof buf, "##AutoChild%s", gamestate);
	ImGui_BeginChild (buf, ZERO_F, CARD_FLAGS, ImGuiWindowFlags_MenuBar);
	{
		if (ImGui_BeginMenuBar ())
		{
			ImGui_MenuItemEx (gamestate, NULL, false, false);
			ImGui_EndMenuBar ();
		}

		snprintf (buf, sizeof buf, "##%sTable", gamestate);

		ImGui_PushStyleVar (ImGuiStyleVar_SelectablesRounding, 0.0f);
		ImGui_PushStyleColorImVec4 (ImGuiCol_BorderShadow,
				MAKE_IV4 (0, 0, 0, 0));
		if (ImGui_BeginTable (buf, 8, ImGuiTableFlags_Borders |
			ImGuiTableFlags_NoHostExtendX))
		{
			for (i = 0; i < size; i++)
			{
				ImGui_TableNextColumn ();

				selected[i] = bitmask & (1 << i);
				snprintf (buf, sizeof buf, "%d##%s%d", selected[i],
						gamestate, i);

				if (ImGui_SelectableEx (buf, selected[i],
					ImGuiSelectableFlags_None,
					MAKE_IV2 (SCALE_IT (15.0f), SCALE_IT (22.0f))))
				{
					bitmask ^= (1 << i);

					D_SET_CGAME_STATE (gamestate, bitmask);
				}
			}
			ImGui_EndTable ();
		}
		ImGui_PopStyleColor ();
		ImGui_PopStyleVar ();

		snprintf (buf, sizeof buf, "Set All##%s", gamestate);
		if (ImGui_Button (buf))
		{
			bitmask = ~0;
			D_SET_CGAME_STATE (gamestate, bitmask);
		}
		ImGui_SameLine ();
		snprintf (buf, sizeof buf, "Clear All##%s", gamestate);
		if (ImGui_Button (buf))
		{
			bitmask = 0;
			D_SET_CGAME_STATE (gamestate, bitmask);
		}
	} ImGui_EndChild (); // ##AutoChild

	DrawBorderAroundLastItem ();
}

ImFontConfig *
InitializeFontConfig (ImFontConfig *font_cfg, const char *name, float size)
{
	memset (font_cfg, 0, sizeof (ImFontConfig));
	font_cfg->FontDataOwnedByAtlas = true;
	font_cfg->OversampleH = 0;
	font_cfg->OversampleV = 0;
	font_cfg->ExtraSizeScale = 1.0f;
	font_cfg->GlyphMaxAdvanceX = ((float)3.40282346638528860e+38);
	font_cfg->RasterizerMultiply = 1.0f;
	font_cfg->RasterizerDensity = 1.0f;
	font_cfg->EllipsisChar = 0;
	font_cfg->SizePixels = size;

	snprintf (font_cfg->Name, sizeof font_cfg->Name, "%s", name);

	return font_cfg;
}

void
AddFontFromResource (const char *res, float size)
{
	ImFontConfig font_cfg;
	BINARY_RES *binfont = ImBinary (res);
	if (binfont != NULL)
	{
		InitializeFontConfig (&font_cfg, res, size);
		ImFontAtlas_AddFontFromMemoryTTF (io->Fonts, binfont->data,
			binfont->size, 0, &font_cfg, NULL);
	}
}

void
DrawBorderAroundLastItem (void)
{
	ImDrawList *draw_list = ImGui_GetWindowDrawList ();
	ImVec2 min = ImGui_GetItemRectMin ();
	ImVec2 max = ImGui_GetItemRectMax ();
	ImDrawList_AddRectEx (draw_list, min, max, U32_FRAMEBG_ACT_GS,
		style->ChildRounding, 0, style->ChildBorderSize);
}

void
ImGui_DrawFilledRect (IM_RECT *rect)
{
	ImDrawList *draw_list = ImGui_GetWindowDrawList ();

	ImDrawList_AddRectFilledEx (draw_list, rect->corner, rect->extent,
		rect->color, rect->rounding, rect->flags);
}

void
UQM_AutoChild (const char *str_id)
{
	ImGui_BeginChild (str_id, ZERO_F,
		ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX |
		ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoBackground);
}

void
UQM_GameStateCheckBox (const char *gs_name)
{
	bool game_state = D_GET_CGAME_STATE (gs_name);

	if (ImGui_Checkbox (gs_name, &game_state))
		D_SET_CGAME_STATE (gs_name, game_state);
}

void
UQM_CGameStateCheckBox (const char *gamestate, const char *label)
{
	bool on_ship = D_GET_CGAME_STATE (gamestate);

	if (ImGui_Checkbox (label, &on_ship))
		D_SET_CGAME_STATE (gamestate, on_ship);
}

void
UQM_DblColTableCheckBox (const char *gs_retrieved, const char *gs_on_ship)
{
	char buf[40];

	bool retrieved = D_GET_CGAME_STATE (gs_retrieved);
	bool on_ship = D_GET_CGAME_STATE (gs_on_ship);

	snprintf (buf, sizeof buf, "##%s", gs_retrieved);
	if (ImGui_Checkbox (buf, &retrieved))
		D_SET_CGAME_STATE (gs_retrieved, retrieved);
	ImGui_TableNextColumn ();

	if (ImGui_Checkbox (gs_retrieved, &on_ship))
		D_SET_CGAME_STATE (gs_on_ship, on_ship);

	ImGui_TableNextRow ();
	ImGui_TableNextColumn ();
}

void
UQM_ImGui_Style (void)
{
	ImVec4 *colors = style->Colors;
	const ImVec4 transparent = { 0 };

	ImGui_StyleColorsDark (NULL);

	colors[ImGuiCol_WindowBg] = transparent;
	colors[ImGuiCol_TableHeaderBg] = transparent;
	colors[ImGuiCol_ModalWindowDimBg] = MAKE_IV4 (0, 0, 0, 0.64f);
	colors[ImGuiCol_ChildBg] = MAKE_IV4 (0.06f, 0.06f, 0.06f, 0.80f);
	colors[ImGuiCol_Border] = transparent;
	colors[ImGuiCol_BorderShadow] = MAKE_IV4 (1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_PopupBg] = MAKE_IV4 (0.14f, 0.14f, 0.14f, 0.95f);
	colors[ImGuiCol_PlotHistogram] = MAKE_IV4 (0.48f, 0.48f, 0.48f, 0.54f);

	style->WindowBorderSize = 1;
	style->ChildBorderSize = 1;
	style->PopupBorderSize = 1;
	style->FrameBorderSize = 1;
	style->TabBorderSize = 0;
	style->WindowRounding = 6;
	style->ChildRounding = 4;
	style->FrameRounding = 3;
	style->PopupRounding = 4;
	style->ScrollbarRounding = 9;
	style->GrabRounding = 3;
	style->LogSliderDeadzone = 4;
	style->TabRounding = 4;
	style->SelectablesRounding = 4;
	style->TabBarBorderSize = 4;
	style->SelectableTextAlign = CENTER_IT;
	style->FontScaleMain = 1.0f;
}

void
UQM_ScaleAllSizes (void)
{
	style->WindowPadding.x = SCALE_IT (8.0f);
	style->WindowPadding.y = SCALE_IT (8.0f);
	style->WindowRounding = SCALE_IT (4.0f);
	style->WindowBorderSize = SCALE_IT (1.0f);
	style->WindowMinSize.x = SCALE_IT (32.0f);
	style->WindowMinSize.y = SCALE_IT (32.0f);
	style->WindowBorderHoverPadding = SCALE_IT (4.0f);
	style->ChildRounding = SCALE_IT (4.0f);
	style->ChildBorderSize = SCALE_IT (1.0f);
	style->PopupRounding = SCALE_IT (4.0f);
	style->PopupBorderSize = SCALE_IT (1.0f);
	style->FramePadding.x = SCALE_IT (4.0f);
	style->FramePadding.y = SCALE_IT (3.0f);
	style->FrameBorderSize = SCALE_IT (1.0f);
	style->FrameRounding = SCALE_IT (3.0f);
	style->ItemSpacing.x = SCALE_IT (8.0f);
	style->ItemSpacing.y = SCALE_IT (4.0f);
	style->ItemInnerSpacing.x = SCALE_IT (4.0f);
	style->ItemInnerSpacing.y = SCALE_IT (4.0f);
	style->CellPadding.x = SCALE_IT (4.0f);
	style->CellPadding.y = SCALE_IT (2.0f);
	style->IndentSpacing = SCALE_IT (21.0f);
	style->ColumnsMinSpacing = SCALE_IT (6.0f);
	style->ScrollbarSize = SCALE_IT (14.0f);
	style->ScrollbarRounding = SCALE_IT (9.0f);
	style->ScrollbarPadding = SCALE_IT (2.0f);
	style->GrabMinSize = SCALE_IT (12.0f);
	style->GrabRounding = SCALE_IT (3.0f);
	style->LogSliderDeadzone = SCALE_IT (4.0f);
	style->ImageRounding = SCALE_IT (4.0f);
	style->ImageBorderSize = SCALE_IT (1.0f);
	style->TabRounding = SCALE_IT (4.0f);
	style->SelectablesRounding = SCALE_IT (4.0f);
	style->TabMinWidthBase = SCALE_IT (1.0f);
	style->TabMinWidthShrink = SCALE_IT (80.0f);
	style->TabBarBorderSize = SCALE_IT (4.0f);
	style->TabBarOverlineSize = SCALE_IT (1.0f);
	style->TreeLinesSize = SCALE_IT (1.0f);
	style->DragDropTargetRounding = SCALE_IT (4.0f);
	style->DragDropTargetBorderSize = SCALE_IT (1.0f);
	style->DragDropTargetPadding = SCALE_IT (3.0f);
	style->ColorMarkerSize = SCALE_IT (3.0f);
	style->SeparatorSize = SCALE_IT (1.0f);
	style->SeparatorTextBorderSize = SCALE_IT (3.0f);
	style->SeparatorTextPadding.x = SCALE_IT (20.0f);
	style->SeparatorTextPadding.y = SCALE_IT (3.0f);
	style->DisplayWindowPadding.x = SCALE_IT (19.0f);
	style->DisplayWindowPadding.y = SCALE_IT (19.0f);
	style->DisplaySafeAreaPadding.x = SCALE_IT (3.0f);
	style->DisplaySafeAreaPadding.y = SCALE_IT (3.0f);
	style->MouseCursorScale = SCALE_IT (1.0f);
}