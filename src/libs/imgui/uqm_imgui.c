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

static bool menu_visible = 0;
static bool imgui_initialized = 0;
static SDL_Renderer *imgui_renderer = NULL;
TabState tab_state = { 0 };

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

	printf ("UQM_ImGui_Init: Initializing Dear ImGui\n");

	imgui_renderer = renderer;

	ImGui_CreateContext (NULL);

	io = ImGui_GetIO ();
	io->IniFilename = NULL;

	if (!cImGui_ImplSDL2_InitForSDLRenderer (window, renderer))
	{
		printf ("ERROR: cImGui_ImplSDL2_InitForSDLRenderer failed\n");
		return 0;
	}

	if (!cImGui_ImplSDLRenderer2_Init (renderer))
	{
		printf ("ERROR: cImGui_ImplSDLRenderer2_Init failed\n");
		cImGui_ImplSDL2_Shutdown ();
		return 0;
	}

	ImGui_StyleColorsMyTheme (NULL);

	imgui_initialized = 1;
	printf ("UQM_ImGui_Init: Success!\n\n");
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
}

// Cleans up ImGui
void UQM_ImGui_Shutdown (void)
{
	if (!imgui_initialized) return;

	printf ("ImGui_Shutdown\n");

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
	imgui_renderer = NULL;
}

// Does what it says on the tin
void UQM_ImGui_ToggleMenu (void)
{
	menu_visible = !menu_visible;
	printf ("Menu toggled: %s\n", menu_visible ? "visible" : "hidden");

	if (menu_visible)
		revalidate_game_state_cache ();
}

// This redirects input to ImGui when the menu is visible
int UQM_ImGui_WantCaptureInput (void)
{
	if (!imgui_initialized)
		return 0;

	ImGuiIO *io = ImGui_GetIO ();
	return (io->WantCaptureKeyboard || io->WantCaptureMouse) ? 1 : 0;
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