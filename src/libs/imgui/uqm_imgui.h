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

#ifndef UQM_IMGUI_H
#define UQM_IMGUI_H

#include "dcimgui/dcimgui.h"
#include "dcimgui/dcimgui_impl_sdl2.h"
#include "dcimgui/dcimgui_impl_sdlrenderer2.h"
#include "options.h"
#include "types.h"
#include "libs/graphics/gfx_common.h"
#include "uqm/globdata.h"
#include "uqm/planets/planets.h"
#include "uqm/setupmenu.h"
#include "libs/log/uqmlog.h"
#include "libs/reslib.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/inplib.h"
#include "libs/input/sdl/vcontrol.h"
#include "libs/input/sdl/keynames.h"
#include "libs/input/input_common.h"
#include "uqm/controls.h"
#include "uqm/gamestr.h"
#include "uqm/hyper.h"
#include "uqm/starmap.h"
#include "uqm/sis.h"
#include "uqm/sounds.h"
#include "uqm/nameref.h"
#include "uqm/ships/pkunk/resinst.h"

#include "libs/graphics/sdl/sdl_common.h"

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHILD_FLAGS ImGuiChildFlags_AutoResizeY | \
		ImGuiChildFlags_AlwaysUseWindowPadding

#define CH_FLAT_NAV CHILD_FLAGS | ImGuiChildFlags_NavFlattened

// ImGui Menus
void draw_graphics_menu (void);
void draw_engine_menu (void);
void draw_audio_menu (void);
void draw_controls_menu (void);
void draw_status_menu (void);
void draw_devices_menu (void);
void draw_gamestates_menu (void);
void draw_events_menu (void);
void draw_stars_menu (void);

void draw_visual_menu (void);
void draw_cheats_menu (void);
void draw_qol_menu (void);
void draw_adv_menu (void);

// ImGui main
extern bool menu_visible;
extern bool config_changed;
extern bool mmcfg_changed;
extern bool cheat_changed;
extern bool imcfg_changed;
extern ImGuiIO *io;
extern bool show_demo;

typedef struct
{
	const char *name;
	DWORD value;
	bool valid;
} GSCacheEntry;

typedef struct
{
	GSCacheEntry *entries;
	size_t count;
} GameStateCache;

extern GameStateCache gs_cache;

extern int get_cached_gamestate (const char *name);
extern void set_cached_gamestate (const char *name, int value);
extern void revalidate_game_state_cache (void);

#define SET_CGAME_STATE(SName,val) \
	set_cached_gamestate (#SName,val)
#define GET_CGAME_STATE(SName) \
	get_cached_gamestate (#SName)

#define D_SET_CGAME_STATE(SName,val) \
	set_cached_gamestate (SName,val)
#define D_GET_CGAME_STATE(SName) \
	get_cached_gamestate (SName)

bool UQM_ImGui_ProcessEvent (SDL_Event *event);
void UQM_ImGui_Render ();
void UQM_ImGui_Shutdown (void);
void UQM_ImGui_ToggleMenu (void);
int UQM_ImGui_WantCaptureInput (void);
void ApplyResChanges (void);
void ApplyGfxChanges (void);
void FlagStatRefresh (void);

extern SOUND PkunkSounds;

// ImGui Graphics
extern int imgui_GfxFlags;
extern int imgui_SavedWidth;
extern int imgui_SavedHeight;
extern bool res_change;
extern bool gfx_change;

//ImGui Status
extern bool scr_refresh;
extern bool slots_cached;

// ImGui Tabs
typedef struct
{
	int active_tab;
	int settings_tab;
	int enhancements_tab;
	int randomizer_tab;
	int devtools_tab;
} TabState;

void UQM_ImGui_Tabs (TabState *state);

// Imgui Controls
typedef struct
{
	BOOLEAN active;
	int action;
	int binding;
	int template_id;
	VCONTROL_GESTURE old_g, new_g;
	BOOLEAN show_popup, has_error;
	char error_message[128];
	char conflict_action[64];
} REBIND_STATE;

extern REBIND_STATE rebind_state;

typedef struct flight_bindings
{
	char action[40];
	VCONTROL_GESTURE binding[MAX_FLIGHT_ALTERNATES];
} FLIGHT_BINDINGS;

extern FLIGHT_BINDINGS curr_fl_bindings[2][NUM_KEYS];
extern FLIGHT_BINDINGS def_fl_bindings[2][NUM_KEYS];

bool ProcessControlEvents (SDL_Event *event);

// Helpers
extern ImGuiStyle *style;

#define FONT_SCALE style->FontScaleMain
#define SCALE_IT(a)((float)a * style->FontScaleMain)
#define DISPLAY_SIZE io->DisplaySize
#define DISPLAY_BOOL ((DISPLAY_SIZE.x > 640.0f && FONT_SCALE <= 1.5) ? 3 : 1)
#define IN_MAIN_MENU (GLOBAL (CurrentActivity) == 0)

#define SCALE_20F SCALE_IT(20.0f)

#define MAKE_IV2(x,y) ((ImVec2){ (x), (y) })
#define MAKE_IV4(w,x,y,z) ((ImVec4){ (w), (x), (y), (z) })

static inline void
Spacer (void)
{
	ImGui_Dummy (MAKE_IV2 (0.0f, SCALE_IT (4.0f)));
}

static inline void
Spacer_Column (int num_col)
{
	int i;

	for (i = 0; i < num_col; i++)
	{
		Spacer ();
		ImGui_NextColumn ();
	}
}

#define IMGUI_SPACER ImGui_Dummy (MAKE_IV2 (0, SCALE_IT (4.0f))
#define CENTER_IT MAKE_IV2 (0.5f, 0.5f)
#define ZERO_F    MAKE_IV2 (0.0f, 0.0f)

static inline ImVec2
Float2Mult (ImVec2 iv2, float mul)
{
	ImVec2 result;
	result.x = iv2.x * mul;
	result.y = iv2.y * mul;
	return result;
}

void UQM_ImGui_CheckBox (const char *label, OPT_ENABLABLE *v, const char *key,
		bool needs_reboot);
bool ImGui_SizedComboChar (const char *label, int *curr_item,
		const char *const items[], int items_count);
void ImGui_TextWrappedColored (ImVec4 col, const char *fmt, ...);
void ImGui_HorizontalSeparator (const char *str_id);
void ImGui_BeginStyledChild (const char *str_id, ImVec2 size,
		ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags,
		ImVec4 *col);

// Colors
#define STYLE_COLOR(a) ImGui_GetColorU32 (a)

#define IV4_RED_COLOR    ((ImVec4){ 1.0f, 0.0f, 0.0f, 1.0f })
#define IV4_YELLOW_COLOR ((ImVec4){ 1.0f, 1.0f, 0.0f, 1.0f })
#define IV4_WHITE_COLOR  ((ImVec4){ 1.0f, 1.0f, 1.0f, 1.0f })
#define IV4_GREEN_COLOR  ((ImVec4){ 0.0f, 1.0f, 0.0f, 1.0f })
#define IV4_BLUE_COLOR   ((ImVec4){ 0.0f, 0.0f, 1.0f, 1.0f })

#define U32_RED_COLOR   0xAF0000FF
#define U32_GREEN_COLOR 0xAF00FF00

#define U32_FRAMEBG_GS     0x8A464646
#define U32_FRAMEBG_HOV_GS 0x66898989
#define U32_FRAMEBG_ACT_GS 0xAB898989

#define U32_BUTTON_GS     0x66898989
#define U32_BUTTON_HOV_GS 0xFF898989
#define U32_BUTTON_ACT_GS 0xFF404040

static inline ImVec4
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

static inline void
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
#define ImGetInt(SKey) UQM_GetInt(#SKey, &SKey)

static inline void
easy_PutInteger (const char *key, int var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	res_PutInteger (imgui_key, var);
	imcfg_changed = true;
}
#define ImPutInt(SKey) easy_PutInteger(#SKey, SKey)

static inline void
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
#define ImGetFlt(SKey) UQM_GetFloat(#SKey, &SKey)

static inline void
easy_PutFloat (const char *key, float var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	res_PutFloat (imgui_key, var);
	imcfg_changed = true;
}
#define ImPutFlt(SKey) easy_PutFloat(#SKey, SKey)

static inline void
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
#define ImGetBool(SKey) UQM_GetBool(#SKey, &SKey)

static inline void
easy_PutBool (const char *key, bool var)
{
	char imgui_key[40];

	snprintf (imgui_key, sizeof imgui_key, "imgui.%s", key);

	res_PutBoolean (imgui_key, (BOOLEAN)var);
	imcfg_changed = true;
}
#define ImPutBool(SKey) easy_PutBool(#SKey, SKey)

static inline const char *
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
#define ImStr(key) UQM_GetStr(key)

static inline const char **
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
#define ImStrArr(key) UQM_GetStrArray(key)

static inline BINARY_RES *
UQM_GetBinary (const char *key)
{
	static char buf[PATH_MAX];

	snprintf (buf, sizeof (buf), "imstr.%s", key);

	if (res_IsBinary (buf))
		return res_GetBinary (buf);

	log_add (log_Error, "BINARY_NOT_FOUND: imstr.%s", key);
	return NULL;
}
#define ImBinary(key) UQM_GetBinary(key)

static inline void
UQM_BitRegister (const char *gamestate, int size)
{
	int i;
	char buf[40];
	static bool selected[32];
	int bitmask = D_GET_CGAME_STATE (gamestate);
	ImVec2 align = { 0.7f, 0.1f };

	Spacer ();

	ImGui_TextUnformatted (gamestate);

	ImGui_PushStyleVar (ImGuiStyleVar_SelectablesRounding, 0.0f);
	ImGui_PushStyleColorImVec4 (ImGuiCol_BorderShadow, MAKE_IV4 (0, 0, 0, 0));
	if (ImGui_BeginTable ("##RainbowTable", 8, ImGuiTableFlags_Borders |
		ImGuiTableFlags_NoHostExtendX))
	{
		for (i = 0; i < size; i++)
		{
			ImGui_TableNextColumn ();

			selected[i] = bitmask & (1 << i);
			snprintf (buf, sizeof buf, "%d##%s%d", selected[i], gamestate, i);

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

	Spacer ();
}
#define GS_Binary(SName,size) UQM_BitRegister(#SName, size)

static inline ImFontConfig *
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

static inline void
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

static inline void
DrawBorderAroundLastItem (void)
{
	ImDrawList *draw_list = ImGui_GetWindowDrawList ();
	ImVec2 min = ImGui_GetItemRectMin ();
	ImVec2 max = ImGui_GetItemRectMax ();
	ImDrawList_AddRectEx (draw_list, min, max, U32_FRAMEBG_ACT_GS,
			style->ChildRounding, 0, style->ChildBorderSize);
}

typedef struct im_rect
{
	ImVec2 corner, extent;
	ImU32 color;
	float rounding;
	ImDrawFlags flags;
} IM_RECT;

static inline void
ImGui_DrawFilledRect (IM_RECT *rect)
{
	ImDrawList *draw_list = ImGui_GetWindowDrawList ();

	ImDrawList_AddRectFilledEx (draw_list, rect->corner, rect->extent,
		rect->color, rect->rounding, rect->flags);
}

inline static void
UQM_AutoChild (const char* str_id)
{
	ImGui_BeginChild (str_id, ZERO_F,
			ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX |
			ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoBackground);
}

static void
UQM_GameStateCheckBox (const char *gs_name)
{
	bool game_state = D_GET_CGAME_STATE (gs_name);

	if (ImGui_Checkbox (gs_name, &game_state))
		D_SET_CGAME_STATE (gs_name, game_state);
}
#define GS_CHECKBOX(gs_name) UQM_GameStateCheckBox(#gs_name)

static inline void
UQM_CGameStateCheckBox (const char *gamestate, const char *label)
{
	bool on_ship = D_GET_CGAME_STATE (gamestate);

	if (ImGui_Checkbox (label, &on_ship))
		D_SET_CGAME_STATE (gamestate, on_ship);
}
#define CGAME_STATE_CHECKBOX(SGS, SLabel) UQM_CGameStateCheckBox(#SGS, SLabel)

static inline void
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
#define DBL_COL_CHECKBOX(gs_retrieved, gs_on_ship) \
	UQM_DblColTableCheckBox (#gs_retrieved, #gs_on_ship)

static inline void
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

static inline void
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

#ifdef __cplusplus
}
#endif

#endif // UQM_IMGUI_H
