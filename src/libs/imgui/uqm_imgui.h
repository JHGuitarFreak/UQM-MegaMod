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

#define CARD_FLAGS CH_FLAT_NAV | ImGuiChildFlags_AutoResizeX

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

void draw_journal_menu (float height);

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

// ImGui Journal
extern char *jrnl_buf;
extern char jrnl_name[15];
extern BOOLEAN jrnl_dirty;
#define FLOPPY_SIZE 1474560
extern void SaveJournal (COUNT which_game);
extern void LoadJournal (COUNT which_game);
extern void ResetJournal (void);
extern void FreeJournal (void);

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

extern ImVec2 content_col_size;
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
#define IN_MAIN_MENU (!inFullGame ())

int NumberOfColumns (void);
#define NUM_COLUMNS (NumberOfColumns ())

#define SCALE_20F SCALE_IT(20.0f)

#define MAKE_IV2(x,y) ((ImVec2){ (x), (y) })
#define MAKE_IV4(w,x,y,z) ((ImVec4){ (w), (x), (y), (z) })

void GetColumnSize (ImVec2 *var, int num_columns);
void Spacer (void);

#define IMGUI_SPACER ImGui_Dummy (MAKE_IV2 (0, SCALE_IT (4.0f))
#define CENTER_IT MAKE_IV2 (0.5f, 0.5f)
#define ZERO_F    MAKE_IV2 (0.0f, 0.0f)

ImVec2 Float2Mult (ImVec2 iv2, float mul);

void UQM_ImGui_CheckBox (const char *label, OPT_ENABLABLE *v, const char *key,
		bool needs_reboot);
bool ImGui_SizedComboChar (const char *label, int *curr_item,
		const char *const items[], int items_count);
void ImGui_TextWrappedColored (ImVec4 col, const char *fmt, ...);
void ImGui_HoriVertSeparator (const char *str_id, bool v);
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

ImVec4 DangerGradient (void);

void UQM_GetInt (const char *key, int *var);
#define ImGetInt(SKey) UQM_GetInt(#SKey, &SKey)

void easy_PutInteger (const char *key, int var);
#define ImPutInt(SKey) easy_PutInteger(#SKey, SKey)

void UQM_GetFloat (const char *key, float *var);
#define ImGetFlt(SKey) UQM_GetFloat(#SKey, &SKey)

void easy_PutFloat (const char *key, float var);
#define ImPutFlt(SKey) easy_PutFloat(#SKey, SKey)

void UQM_GetBool (const char *key, bool *var);
#define ImGetBool(SKey) UQM_GetBool(#SKey, &SKey)

void easy_PutBool (const char *key, bool var);
#define ImPutBool(SKey) easy_PutBool(#SKey, SKey)

const char *UQM_GetStr (const char *key);
#define ImStr(key) UQM_GetStr(key)

const char **UQM_GetStrArray (const char *key);
#define ImStrArr(key) UQM_GetStrArray(key)

BINARY_RES *UQM_GetBinary (const char *key);
#define ImBinary(key) UQM_GetBinary(key)

void UQM_BitRegister (const char *gamestate, int size);
#define GS_Binary(SName,size) UQM_BitRegister(#SName, size)

ImFontConfig *InitializeFontConfig (ImFontConfig *font_cfg, const char *name,
		float size);

void AddFontFromResource (const char *res, float size);
void DrawBorderAroundLastItem (void);

typedef struct im_rect
{
	ImVec2 corner, extent;
	ImU32 color;
	float rounding;
	ImDrawFlags flags;
} IM_RECT;
void ImGui_DrawFilledRect (IM_RECT *rect);

void UQM_AutoChild (const char *str_id);

void UQM_GameStateCheckBox (const char *gs_name);
#define GS_CHECKBOX(gs_name) UQM_GameStateCheckBox(#gs_name)

void UQM_CGameStateCheckBox (const char *gamestate, const char *label);
#define CGAME_STATE_CHECKBOX(SGS, SLabel) UQM_CGameStateCheckBox(#SGS, SLabel)

void UQM_DblColTableCheckBox (const char *gs_retrieved, const char *gs_on_ship);
#define DBL_COL_CHECKBOX(gs_retrieved, gs_on_ship) \
	UQM_DblColTableCheckBox (#gs_retrieved, #gs_on_ship)

void UQM_ImGui_Style (void);
void UQM_ScaleAllSizes (void);

#ifdef __cplusplus
}
#endif

#endif // UQM_IMGUI_H
