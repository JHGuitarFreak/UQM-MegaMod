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

bool menu_visible = false;
static bool imgui_initialized;
static TabState tab_state;

static SDL_Rect old_viewport;
static SDL_BlendMode old_blend;
static int old_logical_w, old_logical_h;

bool config_changed;
bool mmcfg_changed;
bool cheat_changed;
bool res_change = false;
bool gfx_change = false;

static void ShowFullScreenMenu (TabState *state)
{
	float sidebar_width, button_height, content_height;
	ImVec2 button_size, sidebar_size, content_size;
	ImVec2 display_size;
	ImGuiWindowFlags window_flags;
	ImGuiIO *io = ImGui_GetIO ();

	display_size = io->DisplaySize;

	sidebar_width = display_size.x * 0.15f;
	if (sidebar_width < 120.0f)
		sidebar_width = 120.0f;
	if (sidebar_width > 200.0f)
		sidebar_width = 200.0f;

	button_height = display_size.y * 0.06f;
	if (button_height < 30.0f)
		button_height = 30.0f;
	if (button_height > 50.0f)
		button_height = 50.0f;

	content_height = display_size.y - 40.0f;

	button_size = (ImVec2){ sidebar_width - 16.0f, button_height };
	sidebar_size = (ImVec2){ sidebar_width, 0.0f };
	content_size = (ImVec2){ 0.0f, content_height };

	ImGui_SetNextWindowPos (ZERO_F, 0);
	ImGui_SetNextWindowSize (display_size, 0);

	if (io->WantTextInput && !SDL_IsTextInputActive ())
		SDL_StartTextInput ();
	else if (!io->WantTextInput && SDL_IsTextInputActive ())
		SDL_StopTextInput ();

	window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize;

	if (!ImGui_Begin ("##FullScreenMenu", NULL, window_flags))
	{
		ImGui_End ();
		return;
	}

	UQM_ImGui_Tabs (state, content_size, sidebar_size, button_size);

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
}

void UQM_ImGui_NewFrame (void)
{
	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
		return;

	if (!imgui_initialized)
		return;

	cImGui_ImplSDLRenderer2_NewFrame ();
	cImGui_ImplSDL2_NewFrame ();
	ImGui_NewFrame ();

	if (menu_visible)
		ShowFullScreenMenu (&tab_state);
}

// ImGui implementation below

// Initializes ImGui with SDL2 and SDL_Renderer2
int UQM_ImGui_Init (SDL_Window *window, SDL_Renderer *renderer)
{
	ImGuiIO *io;

	if (imgui_initialized)
		return 1;

	ImGui_CreateContext (NULL);

	io = ImGui_GetIO ();
	io->IniFilename = NULL;

	if (!cImGui_ImplSDL2_InitForSDLRenderer (window, renderer))
	{
		log_add (log_Error, "ERROR: cImGui_ImplSDL2_InitForSDLRenderer "
				"failed");
		return 0;
	}

	if (!cImGui_ImplSDLRenderer2_Init (renderer))
	{
		log_add (log_Error, "ERROR: cImGui_ImplSDLRenderer2_Init failed");
		cImGui_ImplSDL2_Shutdown ();
		return 0;
	}

	ImGui_StyleColorsMyTheme (NULL);

	imgui_initialized = 1;
	return 1;
}

// Processes SDL events for ImGui
void UQM_ImGui_ProcessEvent (SDL_Event *event)
{
	if (!imgui_initialized)
		return;

	cImGui_ImplSDL2_ProcessEvent (event);
}

// Renders the ImGui draw data
void UQM_ImGui_Render (SDL_Renderer *renderer)
{
	if (!imgui_initialized)
		return;

	ImGui_Render ();
	cImGui_ImplSDLRenderer2_RenderDrawData (ImGui_GetDrawData (), renderer);

	SDL_RenderPresent (renderer);
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

	cImGui_ImplSDLRenderer2_Shutdown ();
	cImGui_ImplSDL2_Shutdown ();
	ImGui_DestroyContext (NULL);

	imgui_initialized = 0;
}

static void
UQM_ImGui_SaveOldRenderer (SDL_Renderer *renderer)
{
	SDL_RenderGetLogicalSize (renderer, &old_logical_w, &old_logical_h);
	SDL_RenderGetViewport (renderer, &old_viewport);
	SDL_GetRenderDrawBlendMode (renderer, &old_blend);
}

static void
UQM_ImGui_ResetOldRenderer (SDL_Renderer *renderer)
{
	SDL_RenderSetLogicalSize (renderer, old_logical_w, old_logical_h);
	SDL_RenderSetViewport (renderer, &old_viewport);
	SDL_SetRenderDrawBlendMode (renderer, old_blend);
}

static void
UQM_ImGui_SetNewRenderer (SDL_Renderer *renderer, SDL_Window *window)
{
	SDL_Rect new_viewport;
	int window_w, window_h;

	SDL_SetRenderDrawBlendMode (renderer, SDL_BLENDMODE_BLEND);
	SDL_GetWindowSize (window, &window_w, &window_h);
	SDL_GetDisplayBounds (0, &new_viewport);
	SDL_RenderSetViewport (renderer, &new_viewport);
	SDL_RenderSetLogicalSize (renderer, window_w, window_h);
}

// Does what it says on the tin
void UQM_ImGui_ToggleMenu (void)
{
	SDL_Window *window = SDL_GetWindowFromID (1);
	SDL_Renderer *renderer = SDL_GetRenderer (window);

	menu_visible = !menu_visible;

	if (menu_visible && window && renderer)
	{
		UQM_ImGui_SaveOldRenderer (renderer);
		UQM_ImGui_SetNewRenderer (renderer, window);

		if (!UQM_ImGui_Init (window, renderer))
		{
			log_add (log_Error, "Failed to initialize ImGui menu");

			menu_visible = false;

			UQM_ImGui_ResetOldRenderer (renderer);
			SDL_RenderPresent (renderer);
			UQM_ImGui_Shutdown ();
			return;
		}

		revalidate_game_state_cache ();
		return;
	}

	UQM_ImGui_ResetOldRenderer (renderer);
	UQM_ImGui_Shutdown ();
}

static void
UQM_ImGui_ResetMenu (SDL_Window *window, SDL_Renderer *renderer)
{
	SDL_Delay (50);

	menu_visible = true;

	UQM_ImGui_SaveOldRenderer (renderer);
	UQM_ImGui_SetNewRenderer (renderer, window);

	if (!UQM_ImGui_Init (window, renderer))
	{
		log_add (log_Error, "Failed to reinitialize ImGui menu");

		menu_visible = false;

		UQM_ImGui_ResetOldRenderer (renderer);
		SDL_RenderPresent (renderer);
		UQM_ImGui_Shutdown ();
		return;
	}

	revalidate_game_state_cache ();
}

void
ApplyResChanges (SDL_Window *window, SDL_Renderer *renderer)
{
	BOOLEAN isExclusive;
	bool was_visible;
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

	was_visible = menu_visible;
	if (was_visible)
	{
		UQM_ImGui_ResetOldRenderer (renderer);
		UQM_ImGui_Shutdown ();
		menu_visible = false;
	}

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
	else if (was_visible)
	{
		UQM_ImGui_ResetMenu (window, renderer);
		return;
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

	if (was_visible)
		UQM_ImGui_ResetMenu (window, renderer);

	res_PutBoolean ("config.keepaspectratio", optKeepAspectRatio);
	res_PutInteger ("config.loresBlowupScale", loresBlowupScale);
	res_PutInteger ("config.reswidth", SavedWidth);
	res_PutInteger ("config.resheight", SavedHeight);

	config_changed = true;
}

void
ApplyGfxChanges (SDL_Window *window, SDL_Renderer *renderer)
{
	BOOLEAN isExclusive;
	bool was_visible;
	int NewGfxFlags;

	if (!window || !renderer || !gfx_change)
		return;

	gfx_change = false;

	NewGfxFlags = imgui_GfxFlags;

	isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;

	was_visible = menu_visible;
	if (was_visible)
	{
		UQM_ImGui_ResetOldRenderer (renderer);
		UQM_ImGui_Shutdown ();
		menu_visible = false;
	}

	if (NewGfxFlags != GfxFlags)
	{
		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;

		TFB_DrawScreen_ReinitVideo (GraphicsDriver, NewGfxFlags,
				WindowWidth, WindowHeight);
	}
	else if (was_visible)
	{
		UQM_ImGui_ResetMenu (window, renderer);
		return;
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

	if (was_visible)
		UQM_ImGui_ResetMenu (window, renderer);

	//res_PutInteger ("config.loresBlowupScale", loresBlowupScale);
	//res_PutInteger ("config.reswidth", SavedWidth);
	//res_PutInteger ("config.resheight", SavedHeight);

	config_changed = true;
}

// This redirects input to ImGui when the menu is visible
int UQM_ImGui_WantCaptureInput (void)
{
	if (!imgui_initialized)
		return 0;

	ImGuiIO *io = ImGui_GetIO ();
	return (io->WantCaptureKeyboard || io->WantCaptureMouse) ? 1 : 0;
}

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