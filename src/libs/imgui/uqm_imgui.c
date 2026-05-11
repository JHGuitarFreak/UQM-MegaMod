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

#include "libs/graphics/sdl/sdl_common.h"

bool menu_visible = false;
static bool imgui_initialized = false;
static TabState tab_state;

bool config_changed = false;
bool mmcfg_changed = false;
bool cheat_changed = false;
bool res_change = false;
bool gfx_change = false;
bool scr_refresh = false;

static SDL_Window *imgui_window = NULL;
static SDL_Renderer *imgui_renderer = NULL;

static ImFont *cached_font = NULL;

ImGuiIO *io = NULL;

static ImFont *
GetFont (void)
{
	char *font_path;
	int len;
	size_t base_len;
	const char *slash;
	ImFont *font;

	if (cached_font)
		return cached_font;

	base_len = strlen (baseContentPath);
	if (base_len > 0)
	{
		char last_char = baseContentPath[base_len - 1];
		slash = (last_char == '/' || last_char == '\\') ? "" : "/";
	}
	else
		slash = "/";

	len = snprintf (NULL, 0, "%s%splayerfont.ttf",
		baseContentPath, slash);

	font_path = HMalloc (len + 1);

	snprintf (font_path, len + 1, "%s%splayerfont.ttf",
		baseContentPath, slash);

	font = ImFontAtlas_AddFontFromFileTTF (io->Fonts,
			font_path, 18, NULL, NULL);

	HFree (font_path);

	cached_font = font;

	return font;
}

static void ShowFullScreenMenu (TabState *state)
{
	float sidebar_width;
	ImVec2 sidebar_size;
	ImGuiWindowFlags flags;

	ImGui_PushFont (GetFont ());

	ImGui_SetNextWindowPos (ZERO_F, 0);
	ImGui_SetNextWindowSize (DISPLAY_SIZE, 0);

	if (io->WantTextInput && !SDL_IsTextInputActive ())
		SDL_StartTextInput ();
	else if (!io->WantTextInput && SDL_IsTextInputActive ())
		SDL_StopTextInput ();

	flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

	if (!ImGui_Begin ("##FullScreenMenu", NULL, flags))
	{
		ImGui_End ();
		return;
	}

	sidebar_width = DISPLAY_SIZE.x * 0.12f;
	if (sidebar_width < 120.0f)
		sidebar_width = 120.0f;
	if (sidebar_width > 200.0f)
		sidebar_width = 200.0f;

	sidebar_size = (ImVec2){ sidebar_width, 0.0f };

	UQM_ImGui_Tabs (state, ZERO_F, sidebar_size);

	ImGui_End ();

	if (config_changed || mmcfg_changed || cheat_changed)
	{
		if (config_changed)
			SaveResourceIndex (configDir, "uqm.cfg", "config.", TRUE);
		if (mmcfg_changed)
			SaveResourceIndex (configDir, "megamod.cfg", "mm.", TRUE);
		if (cheat_changed)
			SaveResourceIndex (configDir, "cheats.cfg", "cheat.", TRUE);

		config_changed = mmcfg_changed = cheat_changed = false;
	}

	ImGui_PopFont ();
}

// ImGui implementation below

static void DestroyOverlay (void)
{
	if (imgui_renderer)
	{
		SDL_DestroyRenderer (imgui_renderer);
		imgui_renderer = NULL;
	}
	if (imgui_window)
	{
		SDL_DestroyWindow (imgui_window);
		imgui_window = NULL;
	}	
}

// Initializes ImGui with SDL2 and SDL_Renderer2
static int
UQM_ImGui_Init (SDL_Window *window)
{
	int game_x, game_y, game_w, game_h;

	if (imgui_initialized)
		return 1;

	SDL_GetWindowPosition (window, &game_x, &game_y);
	SDL_GetWindowSize (window, &game_w, &game_h);

	imgui_window = SDL_CreateWindow (
			"ImGui Overlay",
			game_x, game_y, game_w, game_h,
			SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP |
			SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_POPUP_MENU
	);

	if (!imgui_window)
	{
		log_add (log_Error, "Failed to create ImGui window");
		return 0;
	}

	imgui_renderer = SDL_CreateRenderer (imgui_window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!imgui_renderer)
	{
		log_add (log_Error, "ERROR: Could not create ImGui renderer");
		DestroyOverlay ();
		return 0;
	}

	SDL_SetRenderDrawBlendMode (imgui_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetWindowOpacity (imgui_window, 0.85f);

	ImGui_CreateContext (NULL);

	io = ImGui_GetIO ();

	io->IniFilename = NULL;

	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	if (!cImGui_ImplSDL2_InitForSDLRenderer (imgui_window, imgui_renderer))
	{
		log_add (log_Error, "ERROR: cImGui_ImplSDL2_InitForSDLRenderer "
				"failed");
		DestroyOverlay ();
		return 0;
	}

	if (!cImGui_ImplSDLRenderer2_Init (imgui_renderer))
	{
		log_add (log_Error, "ERROR: cImGui_ImplSDLRenderer2_Init failed");
		cImGui_ImplSDL2_Shutdown ();
		DestroyOverlay ();
		return 0;
	}

	UQM_ImGui_Style ();

	imgui_initialized = true;
	return 1;
}

// Processes SDL events for ImGui
bool UQM_ImGui_ProcessEvent (SDL_Event *event)
{
	if (!imgui_initialized)
		return false;

	cImGui_ImplSDL2_ProcessEvent (event);

	return ProcessControlEvents (event);
}

// Keeps the ImGui window within the game window.
static void
UQM_ImGui_SyncWindow (void)
{
	int imgui_x, imgui_y, imgui_w, imgui_h;
	int game_x, game_y, game_w, game_h;

	if (!imgui_window || !window)
		return;

	SDL_GetWindowPosition (window, &game_x, &game_y);
	SDL_GetWindowSize (window, &game_w, &game_h);

	SDL_GetWindowPosition (imgui_window, &imgui_x, &imgui_y);
	SDL_GetWindowSize (imgui_window, &imgui_w, &imgui_h);

	if (game_w != imgui_w || game_h != imgui_h
			|| game_x != imgui_x || game_y != imgui_y)
	{
		SDL_SetWindowPosition (imgui_window, game_x, game_y);
		SDL_SetWindowSize (imgui_window, game_w, game_h);
		SDL_RaiseWindow (imgui_window);
	}
}

// Renders the ImGui draw data
void UQM_ImGui_Render (void)
{
	ImDrawData *draw_data;

	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
		return;

	if (!imgui_initialized)
		return;

	UQM_ImGui_SyncWindow ();

	cImGui_ImplSDLRenderer2_NewFrame ();
	cImGui_ImplSDL2_NewFrame ();
	ImGui_NewFrame ();

	if (menu_visible)
		ShowFullScreenMenu (&tab_state);

	ImGui_Render ();

	draw_data = ImGui_GetDrawData ();
	cImGui_ImplSDLRenderer2_RenderDrawData (draw_data, imgui_renderer);

	SDL_RenderPresent (imgui_renderer);
}

// Cleans up ImGui
void UQM_ImGui_Shutdown (void)
{
	if (!imgui_initialized)
		return;

	if (gs_cache.entries)
	{
		free (gs_cache.entries);
		gs_cache.entries = NULL;
		gs_cache.count = 0;
	}

	cached_font = NULL;

	cImGui_ImplSDLRenderer2_Shutdown ();
	cImGui_ImplSDL2_Shutdown ();
	ImGui_DestroyContext (NULL);

	if (imgui_renderer)
	{
		SDL_DestroyRenderer (imgui_renderer);
		imgui_renderer = NULL;
	}
	
	if (imgui_window)
	{
		SDL_DestroyWindow (imgui_window);
		imgui_window = NULL;
	}

	imgui_initialized = false;
	slots_cached = false;
}

// Does what it says on the tin
void UQM_ImGui_ToggleMenu (void)
{
	menu_visible = !menu_visible;

	if (menu_visible)
	{
		if (!UQM_ImGui_Init (window))
		{
			log_add (log_Error, "Failed to initialize ImGui overlay");

			menu_visible = false;

			UQM_ImGui_Shutdown ();
			return;
		}
		else
			UQM_ImGui_SyncWindow ();

		revalidate_game_state_cache ();
		return;
	}

	UQM_ImGui_Shutdown ();
}

// Applies resolution changes.
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

	NewWidth = inBounds (NewWidth, 320, 1920);
	NewHeight = inBounds (NewHeight, 200, 1440);

	SavedWidth = NewWidth;
	SavedHeight = NewHeight;

	isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;

	if (optKeepAspectRatio)
	{
		float threshold = 0.75f;
		float ratio = (float)NewHeight / (float)NewWidth;

		if (ratio > threshold) // screen is narrower than 4:3
			NewWidth = NewHeight / threshold;
		else if (ratio < threshold) // screen is wider than 4:3
			NewHeight = NewWidth * threshold;
	}

	if (NewWidth != WindowWidth || NewHeight != WindowHeight)
	{
		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;

		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				NewWidth, NewHeight);
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

	res_PutBoolean ("config.keepaspectratio", optKeepAspectRatio);
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

	printf ("FlagStats refreshed\n");
}

// This redirects input to ImGui when the menu is visible
int UQM_ImGui_WantCaptureInput (void)
{
	if (!imgui_initialized)
		return 0;

	ImGuiIO *io = ImGui_GetIO ();
	return (io->WantCaptureKeyboard || io->WantCaptureMouse) ? 1 : 0;
}

// Helper functions

void
UQM_ImGui_CheckBox (const char *label, OPT_ENABLABLE *v, const char *key)
{
	if (!ImGui_Checkbox (label, (bool *)v) || key == NULL)
		return;

	res_PutBoolean (key, *v);

	if (strncmp (key, "cheat.", 6) == 0)
		cheat_changed = true;
	else if (strncmp (key, "mm.", 3) == 0)
		mmcfg_changed = true;
	else if (strncmp (key, "config.", 7) == 0)
		config_changed = true;
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
	ImGuiStyle *style = ImGui_GetStyle ();
	float column_width = ImGui_GetColumnWidth (ImGui_GetColumnIndex ());
	float combo_width = column_width * 0.75f;
	float center_offset = (column_width - combo_width) * 0.5f
			- style->WindowPadding.x * 2;

	snprintf (buf, sizeof buf, "##%s", label);

	ImGui_AlignTextToFramePadding ();
	ImGui_Text (label);
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

void revalidate_game_state_cache (void)
{
	for (size_t i = 0; i < gs_cache.count; i++)
		gs_cache.entries[i].valid = false;
}