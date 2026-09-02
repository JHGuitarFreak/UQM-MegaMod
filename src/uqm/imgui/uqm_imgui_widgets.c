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

bool config_changed = false;
bool mmcfg_changed = false;
bool cheat_changed = false;
bool imcfg_changed = false;
bool res_change = false;
bool gfx_change = false;
bool scr_refresh = false;

#define DESIRED_COL_WIDTH 360.0f

int NumberOfColumns (void)
{
	int columns = (int)((DISPLAY_SIZE.x / FONT_SCALE) / DESIRED_COL_WIDTH);

	if (columns < 1)
		return 1;

	return columns;
}

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
ImGui_HoriVertSeparator (const char *str_id, bool v)
{
	ImGui_PushStyleColor (ImGuiCol_ChildBg, STYLE_COLOR (ImGuiCol_FrameBg));
	ImGui_BeginChild (str_id, MAKE_IV2 (v ? 1 : 0, v ? 0 : 1), 0, 0);
	ImGui_EndChild ();
	ImGui_PopStyleColor ();
}

bool
ImGui_SizedComboChar (const char *label, int *curr_item,
		const char *const items[], int items_count)
{
	bool temp = false;
	float column_width = ImGui_GetColumnWidth (ImGui_GetColumnIndex ());
	float combo_width = column_width * 0.86f;

	if (strncmp (label, "##", 2) != 0)
	{
		ImGui_AlignTextToFramePadding ();
		ImGui_TextUnformatted (label);
	}

	ImGui_SetNextItemWidth (combo_width);

	ImGui_PushStyleVarImVec2 (ImGuiStyleVar_SelectableTextAlign, CENTER_IT);
	ImGui_PushID (label);

	temp = ImGui_ComboChar ("", curr_item, items, items_count);

	ImGui_PopID ();
	ImGui_PopStyleVar ();

	return temp;
}

static int
ToCons (int opt)
{
	return (opt ? OPT_3DO : OPT_PC);
}

void
UQM_WhichConfig (const char *key)
{
	if (strncmp (key, "config.", 7) == 0)
		config_changed = true;
	else if (strncmp (key, "mm.", 3) == 0)
		mmcfg_changed = true;
	else if (strncmp (key, "cheat.", 6) == 0)
		cheat_changed = true;
	else if (strncmp (key, "imgui.", 6) == 0)
		imcfg_changed = true;
}

void
UQM_ComboChar (const char* label, const char *const items[],
		int items_count, int *option, const char *key, bool reload)
{
	int temp = *option;

	if (ImGui_SizedComboChar (label, &temp, items, items_count))
	{
		if (reload)
		{
			if (IN_MAIN_MENU)
				GLOBAL (CurrentActivity) = 0;
			else
				GLOBAL (CurrentActivity) |= CHECK_ABORT;
		}

		*option = temp;
		res_PutInteger (key, temp);

		UQM_WhichConfig (key);

		optRequiresReload = reload;
	}
	if (reload && ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
	{
		ImGui_BeginTooltip ();
		ImGui_TextColoredUnformatted (
				ColorToIV4 (BRIGHT_RED_COLOR),
				ImStr (TIP_WARN_STR_BASE + 1)); // Reload Warning
		ImGui_EndTooltip ();
	}
}

void
UQM_ConsComboChar (const char* label, const char *const items[],
		int *option, const char *key, bool reload)
{
	int temp = is3DO (*option);

	if (ImGui_SizedComboChar (label, &temp, items, 2))
	{
		if (reload)
		{
			if (IN_MAIN_MENU)
				GLOBAL (CurrentActivity) = 0;
			else
				GLOBAL (CurrentActivity) |= CHECK_ABORT;
		}

		*option = ToCons (temp);
		res_PutBoolean (key, (BOOLEAN)temp);

		UQM_WhichConfig (key);

		optRequiresReload = reload;
	}
	if (reload && ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
	{
		ImGui_BeginTooltip ();
		ImGui_TextColoredUnformatted (
				ColorToIV4 (BRIGHT_RED_COLOR),
				ImStr (TIP_WARN_STR_BASE + 1)); // Reload Warning
		ImGui_EndTooltip ();
	}
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
UQM_ImGui_CheckBox (const char *label, OPT_ENABLABLE *v, const char *key,
	bool needs_reboot)
{
	if (key == NULL)
		return;

	if (ImGui_Checkbox (label, (bool *)v) || key == NULL)
	{
		res_PutBoolean (key, *v);

		if (strncmp (key, "cheat.", 6) == 0)
			cheat_changed = true;
		else if (strncmp (key, "mm.", 3) == 0)
			mmcfg_changed = true;
		else if (strncmp (key, "config.", 7) == 0)
			config_changed = true;

		if (needs_reboot)
		{
			if (IN_MAIN_MENU)
				GLOBAL (CurrentActivity) = 0;
			else
				GLOBAL (CurrentActivity) |= CHECK_ABORT;

			optRequiresReload = TRUE;
		}
	}

	if (needs_reboot)
	{
		if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
		{
			ImGui_BeginTooltip ();
			ImGui_TextColoredUnformatted (ColorToIV4 (BRIGHT_RED_COLOR),
				ImStr (TIP_WARN_STR_BASE + 1)); // Reload Warning
			ImGui_EndTooltip ();
		}
	}
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

const char **
UQM_GetStrArray (int index)
{
	char *copy, *token;
	int count = 0;
	char **array = NULL;
	static char buf[PATH_MAX];

	copy = strdup (ImStr (index));

	if (!copy)
	{
		char error_msg[PATH_MAX];
		snprintf (error_msg, sizeof (error_msg),
				"String index %d either not found or empty", index);

		array = HMalloc (2 * sizeof (char *));
		array[0] = strdup (error_msg);
		array[1] = NULL;
		return (const char **)array;
	}

	token = strtok (copy, ",\n");

	while (token)
	{
		while (*token == ' ')
			token++;

		array = HRealloc (array, (count + 2) * sizeof (char *));
		array[count++] = strdup (token);
		token = strtok (NULL, ",\n");
	}
	HFree (copy);

	if (array == NULL)
	{
		array = HMalloc (sizeof (char *));
		count = 0;
	}

	array[count] = NULL;

	return (const char **)array;
}

BINARY_RES *
UQM_GetBinary (const char *key)
{
	static char buf[PATH_MAX];

	snprintf (buf, sizeof (buf), "%s", key);

	if (res_IsBinary (buf))
		return res_GetBinary (buf);

	log_add (log_Error, "BINARY_NOT_FOUND: %s", key);
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

	snprintf (buf, sizeof buf, "##Child%s", gamestate);
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
		
		ImGui_PushID (gamestate);

		if (ImGui_Button (ImStr (DBG_GMS_STR_BASE + 15)))
		{
			bitmask = ~0;
			D_SET_CGAME_STATE (gamestate, bitmask);
		}

		ImGui_SameLine ();

		if (ImGui_Button (ImStr (DBG_GMS_STR_BASE + 16)))
		{
			bitmask = 0;
			D_SET_CGAME_STATE (gamestate, bitmask);
		}

		ImGui_PopID ();
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
	void *font_data;
	ImFontConfig font_cfg;
	BINARY_RES *binfont = ImBinary (res);

	if (binfont != NULL)
	{
		font_data = malloc (binfont->size);
		if (!font_data)
		{
			log_add (log_Error, "Failed to allocate font data for: %s", res);
			return;
		}
		memcpy (font_data, binfont->data, binfont->size);

		InitializeFontConfig (&font_cfg, res, size);

		ImFontAtlas_AddFontFromMemoryTTF (io->Fonts, font_data,
				binfont->size, 0, &font_cfg, NULL);

		FreeBinaryData (binfont);
	}
	else
		log_add (log_Error, "Failed to load font: %s", res);
}

float
GetDefaultFontSize (void)
{
	int i;
	ImFontAtlas *atlas = io->Fonts;

	for (i = 0; i < atlas->Sources.Size; i++)
	{
		if (atlas->Sources.Data[i].DstFont == io->FontDefault)
			return atlas->Sources.Data[i].SizePixels;
	}
	return 0.0f;
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

ImVec4
UQM_ColorToImVec4 (Color color)
{
	ImVec4 temp;

	temp.x = (float)color.r / 255.0f;
	temp.y = (float)color.g / 255.0f;
	temp.z = (float)color.b / 255.0f;
	temp.w = (float)color.a / 255.0f;

	return temp;
}

ImU32
UQM_ColorToU32 (Color color)
{
	return (ImU32)(
			(color.a << 24) |
			(color.b << 16) |
			(color.g << 8) |
			color.r);
}

const char *
UQM_MakeIDFromStrings (const char *str1, const char *str2)
{
	static char buf[PATH_MAX];

	if (!str1)
		return "str1 Empty!";
	if (!str2)
		return "str2 Empty!";

	snprintf (buf, sizeof buf, "%s##%s", str1, str2);

	return buf;
}

void
UQM_SaveConfigs (void)
{
	if (!(config_changed || mmcfg_changed || cheat_changed || imcfg_changed))
		return;

	if (config_changed)
		SaveResourceIndex (configDir, "uqm.cfg", "config.", TRUE);
	if (mmcfg_changed)
		SaveResourceIndex (configDir, "megamod.cfg", "mm.", TRUE);
	if (cheat_changed)
		SaveResourceIndex (configDir, "cheats.cfg", "cheat.", TRUE);
	if (imcfg_changed)
		SaveResourceIndex (configDir, "imgui.cfg", "imgui.", TRUE);

	config_changed = mmcfg_changed = cheat_changed = imcfg_changed = false;
}

void
UQM_LoadImGuiSettings (void)
{
	int font_selection = 0;
	float ui_scale = style->FontScaleMain;
	bool nav_gamepad = 1;
	float background_opacity = style->Colors[ImGuiCol_ChildBg].w;

	ImGetInt (font_selection);
	if (font_selection < io->Fonts->Fonts.Size && font_selection >= 0)
		io->FontDefault = io->Fonts->Fonts.Data[font_selection];

	ImGetFlt (ui_scale);
	if (ui_scale >= 0.50f && ui_scale <= 3.0f)
	{
		style->FontScaleMain = ui_scale;
		UQM_ScaleAllSizes ();
	}

	ImGetFlt (background_opacity);
	if (background_opacity >= 0.0f && background_opacity <= 1.0f)
		style->Colors[ImGuiCol_ChildBg].w = background_opacity;

	ImGetBool (nav_gamepad);
	if (nav_gamepad)
		io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	else
		io->ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
}

void
ApplyResChanges (void)
{
	BOOLEAN isExclusive;
	int NewGfxFlags;
	int NewWidth = imgui_SavedWidth;
	int NewHeight = imgui_SavedHeight;

	if (!window || !renderer || !res_change)
		return;

	res_change = false;

	NewGfxFlags = GfxFlags;

	SavedWidth = NewWidth;
	SavedHeight = NewHeight;

	isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;

	if (NewWidth != WindowWidth || NewHeight != WindowHeight)
	{
		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;

		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				NewWidth, NewHeight);

		TFB_SetWindowSize (NewWidth, NewHeight);
	}

	if (NewGfxFlags != GfxFlags)
		GfxFlags = NewGfxFlags;

	if (isExclusive)
	{	// needed twice to reinitialize Exclusive Full Screen after a 
		// resolution change
		GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				WindowWidth, WindowHeight);
	}

	res_PutInteger ("config.keepaspectratio", optKeepAspectRatio);
	res_PutInteger ("config.loresBlowupScale", loresBlowupScale);
	res_PutInteger ("config.reswidth", SavedWidth);
	res_PutInteger ("config.resheight", SavedHeight);

	config_changed = true;
}

// Applies graphics changes such as Full Screen, Exclusive Full Screen,
// Scanlines, FPS Counter, and Scaler settings
void
ApplyGfxChanges (void)
{
	BOOLEAN isExclusive;
	int NewGfxFlags;

	if (!window || !renderer || !gfx_change)
		return;

	gfx_change = false;

	NewGfxFlags = imgui_GfxFlags;

	isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;

	if (NewGfxFlags != GfxFlags)
	{
		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;

		TFB_DrawScreen_ReinitVideo (GraphicsDriver, NewGfxFlags,
				WindowWidth, WindowHeight);
	}

	if (NewGfxFlags != GfxFlags)
		GfxFlags = NewGfxFlags;

	if (isExclusive)
	{	// needed twice to reinitialize Exclusive Full Screen after a 
		// resolution change
		GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				WindowWidth, WindowHeight);
	}

	res_PutBoolean ("config.scanlines", GfxFlags & TFB_GFXFLAGS_SCANLINES);
	res_PutBoolean ("config.showfps", GfxFlags & TFB_GFXFLAGS_SHOWFPS);

	{
		int fullscreen_mode = 0;

		if (GfxFlags & TFB_GFXFLAGS_FULLSCREEN)
			fullscreen_mode = 2;
		else if (GfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN)
			fullscreen_mode = 1;

		res_PutInteger ("config.fullscreen", fullscreen_mode);
	}

	{
		const char *scaler_list[6] =
				{ "no", "bilinear", "biadapt", "biadv", "triscan", "hq" };
		int curr_scaler;

		switch (GfxFlags & 248)
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

		res_PutString ("config.scaler", scaler_list[curr_scaler]);
	}

	config_changed = true;
}

// Refreshes the Flagship stats when changing Captain/Ship name,
// Landers, Fuel, Modules, and Crew.
void
FlagStatRefresh (void)
{
	if (!scr_refresh)
		return;

	scr_refresh = false;

	DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
}

void
UQM_ImGui_Style (void)
{
	ImVec4 *colors = style->Colors;
	const ImVec4 transparent = { 0 };

	ImGui_StyleColorsDark (NULL);

	colors[ImGuiCol_WindowBg] = transparent;
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

// Begin GameState cache implementation

GameStateCache gs_cache = { NULL, 0 };

GSCacheEntry *
find_cache_entry (const char *name)
{
	for (size_t i = 0; i < gs_cache.count; i++)
	{
		if (strcmp (gs_cache.entries[i].name, name) == 0)
		{
			return &gs_cache.entries[i];
		}
	}
	return NULL;
}

static GSCacheEntry *
add_cache_entry (const char *name)
{
	size_t new_count = gs_cache.count + 1;
	size_t new_size = sizeof (GSCacheEntry) * new_count;
	GSCacheEntry *new_entries = realloc (gs_cache.entries, new_size);

	if (!new_entries)
	{
		log_add (log_Warning, "Failed to realloc gs_cache");
		return NULL;
	}

	gs_cache.entries = new_entries;

	gs_cache.entries[gs_cache.count].name = name;
	gs_cache.entries[gs_cache.count].valid = false;
	gs_cache.entries[gs_cache.count].value = 0;

	return &gs_cache.entries[gs_cache.count++];
}

// Get cached gamestate if it exists, If not, create one
int
get_cached_gamestate (const char *name)
{
	GSCacheEntry *entry = find_cache_entry (name);

	if (entry)
	{
		if (!entry->valid)
		{
			entry->value = D_GET_GAME_STATE (name);
			entry->valid = true;
		}
		return entry->value;
	}

	entry = add_cache_entry (name);
	if (entry)
	{
		entry->value = D_GET_GAME_STATE (name);
		entry->valid = true;
		return entry->value;
	}

	return D_GET_GAME_STATE (name);
}

// Set cached gamestate if it exists. If not, create one
void
set_cached_gamestate (const char *name, int value)
{
	GSCacheEntry *entry = find_cache_entry (name);

	D_SET_GAME_STATE (name, value);

	if (entry)
	{
		entry->value = value;
		entry->valid = true;
	}
	else
	{
		entry = add_cache_entry (name);
		if (entry)
		{
			entry->value = value;
			entry->valid = true;
		}
	}
}

void
revalidate_game_state_cache (void)
{
	size_t i;

	for (i = 0; i < gs_cache.count; i++)
		gs_cache.entries[i].valid = false;
}