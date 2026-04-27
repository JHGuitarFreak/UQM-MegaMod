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

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

// ImGui Menus
void draw_graphics_menu (void);
void draw_engine_menu (void);
void draw_audio_menu (void);
void draw_controls_menu (void);
void draw_status_menu (void);

void draw_visual_menu (void);
void draw_cheats_menu (void);
void draw_qol_menu (void);
void draw_adv_menu (void);

// ImGui main
extern bool menu_visible;
extern bool config_changed;
extern bool mmcfg_changed;
extern bool cheat_changed;

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

bool UQM_ImGui_ProcessEvent (SDL_Event *event);
void UQM_ImGui_Render ();
void UQM_ImGui_Shutdown (void);
void UQM_ImGui_ToggleMenu (void);
int UQM_ImGui_WantCaptureInput (void);
void ApplyResChanges (SDL_Window *window, SDL_Renderer *renderer);
void ApplyGfxChanges (SDL_Window *window, SDL_Renderer *renderer);

// ImGui Graphics
extern int imgui_GfxFlags;
extern int imgui_SavedWidth;
extern int imgui_SavedHeight;
extern bool res_change;
extern bool gfx_change;

// ImGui Tabs
typedef struct
{
	int active_tab;
	int settings_tab;
	int enhancements_tab;
	int randomizer_tab;
	int devtools_tab;
} TabState;

void UQM_ImGui_Tabs (TabState *state, ImVec2 content_size, ImVec2 sidebar_size);

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

typedef struct menu_bindings
{
	char action[40];
	VCONTROL_GESTURE binding[6];
} MENU_BINDINGS;

typedef struct flight_bindings
{
	char action[40];
	VCONTROL_GESTURE binding[2];
} FLIGHT_BINDINGS;

extern char def_template_names[6][40];
extern MENU_BINDINGS curr_bindings[NUM_MENU_KEYS];
extern MENU_BINDINGS def_bindings[NUM_MENU_KEYS];
extern FLIGHT_BINDINGS curr_fl_bindings[6][NUM_KEYS];
extern FLIGHT_BINDINGS def_fl_bindings[6][NUM_KEYS];

bool ProcessControlEvents (SDL_Event *event);

// Helpers
#define DISPLAY_SIZE ImGui_GetIO ()->DisplaySize
#define DISPLAY_BOOL (DISPLAY_SIZE.x > 640.0f ? 3 : 1)
#define IN_MAIN_MENU (GLOBAL (CurrentActivity) == 0)

#define MAKE_IV2(x,y) ((ImVec2){ (x), (y) })
#define MAKE_IV4(w,x,y,z) ((ImVec4){ (w), (x), (y), (z) })

static inline void Spacer (void) { ImGui_Dummy ((ImVec2) { 0.0f, 4.0f }); }

#define IMGUI_SPACER ImGui_Dummy (MAKE_IV2 (0, 4))
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

void UQM_ImGui_CheckBox (const char *label, OPT_ENABLABLE *v, const char *key);
bool ImGui_SizedComboChar (const char *label, int *curr_item,
		const char *const items[], int items_count);
void ImGui_TextWrappedColored (ImVec4 col, const char *fmt, ...);
void ImGui_HorizontalSeparator (const char *str_id);
void ImGui_BeginStyledChild (const char *str_id, ImVec2 size,
		ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags,
		ImVec4 *col);

// Colors
#define STYLE_COLOR(a) ImGui_GetColorU32 (a)

#define IV4_RED_COLOR ((ImVec4){ 1.0f, 0.0f, 0.0f, 1.0f })
#define IV4_YELLOW_COLOR ((ImVec4){ 1.0f, 1.0f, 0.0f, 1.0f })

#define U32_RED_COLOR IM_COL32 (0xFF, 0x00, 0x00, 0xAF)
#define U32_GREEN_COLOR IM_COL32 (0x00, 0xFF, 0x00, 0xAF)
#define U32_GUN_SLOT_COLOR IM_COL32 (0x4F, 0x4F, 0x4F, 0x8A)

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

static inline ImVec4
ImLerp_Vec4 (ImVec4 a, ImVec4 b, float t)
{
	ImVec4 result;
	result.x = a.x + (b.x - a.x) * t;
	result.y = a.y + (b.y - a.y) * t;
	result.z = a.z + (b.z - a.z) * t;
	result.w = a.w + (b.w - a.w) * t;
	return result;
}

static inline ImVec4
AlfAdjust (ImVec4 vec, float t)
{
	ImVec4 result;
	result = vec;
	result.w = t;
	return result;
}
#define ALPHA(vec, a) AlfAdjust(vec, a)

static inline void
UQM_ImGui_Style (void)
{
	ImGuiStyle *style = ImGui_GetStyle ();
	ImVec4 *colors = style->Colors;

	const ImVec4 bg_dark = MAKE_IV4 (0.06f, 0.06f, 0.06f, 1.00f);
	const ImVec4 bg_med = MAKE_IV4 (0.16f, 0.16f, 0.16f, 1.00f);
	const ImVec4 bg_light = MAKE_IV4 (0.26f, 0.26f, 0.26f, 1.00f);
	const ImVec4 text = MAKE_IV4 (1.00f, 1.00f, 1.00f, 1.00f);
	const ImVec4 text_dim = MAKE_IV4 (0.50f, 0.50f, 0.50f, 1.00f);
	const ImVec4 white = MAKE_IV4 (1.00f, 1.00f, 1.00f, 1.00f);
	const ImVec4 black = MAKE_IV4 (0.00f, 0.00f, 0.00f, 1.00f);
	const ImVec4 transparent = MAKE_IV4 (0.00f, 0.00f, 0.00f, 0.00f);

	colors[ImGuiCol_WindowBg] = transparent;
	colors[ImGuiCol_ModalWindowDimBg] = ALPHA (black, 0.64f);
	colors[ImGuiCol_ChildBg] = ALPHA (bg_dark, 0.95f);
	colors[ImGuiCol_Border] = transparent;
	colors[ImGuiCol_BorderShadow] = ALPHA (white, 0.06f);

	colors[ImGuiCol_Text] = text;
	colors[ImGuiCol_TextDisabled] = text_dim;
	colors[ImGuiCol_PopupBg] = ALPHA (bg_dark, 0.95f);
	colors[ImGuiCol_FrameBg] = ALPHA (bg_med, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ALPHA (bg_light, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ALPHA (bg_light, 0.67f);
	colors[ImGuiCol_TitleBg] = MAKE_IV4 (0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = bg_med;
	colors[ImGuiCol_TitleBgCollapsed] = ALPHA (black, 0.51f);
	colors[ImGuiCol_MenuBarBg] = MAKE_IV4 (0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = MAKE_IV4 (0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = MAKE_IV4 (0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = MAKE_IV4 (0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = MAKE_IV4 (0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = MAKE_IV4 (0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_SliderGrab] = MAKE_IV4 (0.24f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = bg_light;

	colors[ImGuiCol_Button] = ALPHA (bg_light, 0.40f);
	colors[ImGuiCol_ButtonHovered] = bg_light;
	colors[ImGuiCol_ButtonActive] = bg_dark;
	colors[ImGuiCol_Header] = ALPHA (bg_light, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ALPHA (bg_light, 0.80f);
	colors[ImGuiCol_HeaderActive] = bg_light;
	colors[ImGuiCol_Separator] = colors[ImGuiCol_FrameBgActive];
	colors[ImGuiCol_SeparatorHovered] = MAKE_IV4 (0.10f, 0.10f, 0.10f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = MAKE_IV4 (0.10f, 0.10f, 0.10f, 1.00f);

	colors[ImGuiCol_ResizeGrip] = ALPHA (bg_light, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ALPHA (bg_light, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ALPHA (bg_light, 0.95f);
	colors[ImGuiCol_InputTextCursor] = colors[ImGuiCol_Text];
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_FrameBgHovered];
	colors[ImGuiCol_Tab] = colors[ImGuiCol_FrameBg];
	colors[ImGuiCol_TabSelected] = colors[ImGuiCol_FrameBgActive];
	colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
	colors[ImGuiCol_TabDimmed] = ImLerp_Vec4 (colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabDimmedSelected] = ImLerp_Vec4 (colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = transparent;

	colors[ImGuiCol_PlotLines] = MAKE_IV4 (0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = MAKE_IV4 (1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ALPHA (bg_light, 0.40f);
	colors[ImGuiCol_PlotHistogramHovered] = bg_light;
	colors[ImGuiCol_TableHeaderBg] = MAKE_IV4 (0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = MAKE_IV4 (0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = MAKE_IV4 (0.23f, 0.23f, 0.23f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ALPHA (black, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ALPHA (white, 0.06f);
	colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];
	colors[ImGuiCol_TextSelectedBg] = ALPHA (bg_light, 0.75f);
	colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Border];
	colors[ImGuiCol_DragDropTarget] = MAKE_IV4 (1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_DragDropTargetBg] = ALPHA (black, 0.00f);
	colors[ImGuiCol_UnsavedMarker] = white;
	colors[ImGuiCol_NavCursor] = MAKE_IV4 (1.0f, 0.0f, 0.50f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ALPHA (white, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ALPHA (black, 0.64f);

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
}

#ifdef __cplusplus
}
#endif

#endif // UQM_IMGUI_H
