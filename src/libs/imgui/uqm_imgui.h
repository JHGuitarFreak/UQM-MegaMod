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
bool menu_visible;
bool config_changed;
bool mmcfg_changed;
bool cheat_changed;

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

GameStateCache gs_cache;

int get_cached_gamestate (const char *name);
void set_cached_gamestate (const char *name, int value);
void revalidate_game_state_cache (void);

#define SET_CGAME_STATE(SName,val) \
	set_cached_gamestate (#SName,val)
#define GET_CGAME_STATE(SName) \
	get_cached_gamestate (#SName)

int UQM_ImGui_Init (SDL_Window *window, SDL_Renderer *renderer);
bool UQM_ImGui_ProcessEvent (SDL_Event *event);
void UQM_ImGui_NewFrame (void);
void UQM_ImGui_Render (SDL_Renderer *renderer);
void UQM_ImGui_Shutdown (void);
void UQM_ImGui_ToggleMenu (void);
int UQM_ImGui_WantCaptureInput (void);
void ApplyResChanges (SDL_Window *window, SDL_Renderer *renderer);
void ApplyGfxChanges (SDL_Window *window, SDL_Renderer *renderer);

// ImGui Graphics
int imgui_GfxFlags;
int imgui_SavedWidth;
int imgui_SavedHeight;
bool res_change;
bool gfx_change;

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

REBIND_STATE rebind_state;

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
void ImGui_TextWrappedColored (ImVec4 col, const char *fmt, ...);
void ImGui_HorizontalSeparator (const char *str_id);

// Colors
#define STYLE_COLOR(a) ImGui_GetColorU32 (a)

#define IV4_RED_COLOR ((ImVec4){ 1.0f, 0.0f, 0.0f, 1.0f })
#define IV4_YELLOW_COLOR ((ImVec4){ 1.0f, 1.0f, 0.0f, 1.0f })

#define U32_RED_COLOR IM_COL32 (0xFF, 0x00, 0x00, 0xFF)
#define U32_GREEN_COLOR IM_COL32 (0x00, 0xFF, 0x00, 0xFF)
#define U32_GUN_SLOT_COLOR IM_COL32 (0x69, 0x80, 0xA1, 0x8A)

static inline ImVec4
DangerGradient (void)
{
	static DWORD c_index = 0;
	static TimeCount NextTime = 0;
	static int direction = 1;
	const size_t c_count = 10;
	static ImVec4 gradient;

	if (GetTimeCounter () >= NextTime)
	{
		c_index = (c_index + direction + c_count) % c_count;
		if (c_index == 0 || c_index == c_count - 1)
			direction = -direction;

		NextTime = GetTimeCounter () + (ONE_SECOND * 3 / 40);
	}

	return MAKE_IV4 (1, (float)c_index / (float)c_count, 0, 1);
}

#ifdef __cplusplus
}
#endif

#endif // UQM_IMGUI_H