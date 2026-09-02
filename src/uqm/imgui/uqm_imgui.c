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
static bool imgui_initialized = false;
static TabState tab_state;

ImGuiIO *io = NULL;
ImGuiStyle *style = NULL;

bool show_demo = false;

STRING ImGuiStrings;

static void ShowFullScreenMenu (TabState *state)
{
	ImGuiWindowFlags flags;

	ImGui_SetNextWindowPos (ZERO_F, 0);
	ImGui_SetNextWindowSize (DISPLAY_SIZE, 0);

	if (io->WantTextInput && !SDL_IsTextInputActive ())
		SDL_StartTextInput ();
	else if (!io->WantTextInput && SDL_IsTextInputActive ())
		SDL_StopTextInput ();

	flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

	UQM_SetCursor (CURSOR_POINTER);

	if (!ImGui_Begin ("##FullScreenMenu", NULL, flags))
	{
		ImGui_End ();
		return;
	}

	UQM_ImGui_Tabs (state);

	ImGui_End ();

	UQM_SaveConfigs ();
}

// Initializes ImGui with SDL2 and SDL_Renderer2
static int
UQM_ImGui_Init (void)
{
	if (imgui_initialized)
		return 1;

	CIMGUI_CHECKVERSION ();
	ImGui_CreateContext (NULL);

	if (!cImGui_ImplSDL2_InitForSDLRenderer (window, renderer))
	{
		log_add (log_Error, "cImGui_ImplSDL2_InitForSDLRenderer failed");
		cImGui_ImplSDLRenderer2_Shutdown ();
		return 0;
	}

	if (!cImGui_ImplSDLRenderer2_Init (renderer))
	{
		log_add (log_Error, "cImGui_ImplSDLRenderer2_Init failed");
		cImGui_ImplSDL2_Shutdown ();
		return 0;
	}

	style = ImGui_GetStyle ();
	io = ImGui_GetIO ();
	io->IniFilename = NULL;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Set the theme
	UQM_ImGui_Style ();

	// Load resources
	AddFontFromResource ("playerfont.ttf", 18.0f);
	AddFontFromResource ("tinyfont.ttf",   18.0f);
	AddFontFromResource ("urquanfont.ttf", 20.0f);
	ImFontAtlas_AddFontDefault (io->Fonts, NULL);
	LoadResourceIndex (configDir, "imgui.cfg", "imgui.");
	ImGuiStrings = CaptureStringTable (LoadStringTable ("text.imgui"));
	UQM_LoadImGuiSettings ();

	imgui_initialized = true;
	return 1;
}

// Renders the ImGui draw data
void UQM_ImGui_Render (void)
{
	ImDrawData *draw_data;

	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
		return;

	if (!imgui_initialized)
		return;

	cImGui_ImplSDLRenderer2_NewFrame ();
	cImGui_ImplSDL2_NewFrame ();
	ImGui_NewFrame ();

	ImGui_PushFontFloat (io->FontDefault, GetDefaultFontSize ());

	if (show_demo)
	{
		if (!style->Colors[ImGuiCol_WindowBg].w)
			style->Colors[ImGuiCol_WindowBg].w = 0.80f;
		ImGui_ShowDemoWindow (&show_demo);
	}
	else
	{
		if (style->Colors[ImGuiCol_WindowBg].w)
			style->Colors[ImGuiCol_WindowBg].w = 0.0f;
		ShowFullScreenMenu (&tab_state);
	}

	ImGui_PopFont ();

	ImGui_Render ();

	draw_data = ImGui_GetDrawData ();
	cImGui_ImplSDLRenderer2_RenderDrawData (draw_data, renderer);

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

	if (ImGuiStrings)
		DestroyStringTable (ReleaseStringTable (ImGuiStrings));

	cImGui_ImplSDLRenderer2_Shutdown ();
	cImGui_ImplSDL2_Shutdown ();
	ImGui_DestroyContext (NULL);

	imgui_initialized = false;
	slots_cached = false;
}

// Does what it says on the tin
void UQM_ImGui_ToggleMenu (void)
{
	menu_visible = !menu_visible;

	if (menu_visible)
	{
		if (!UQM_ImGui_Init ())
		{
			log_add (log_Error, "Failed to initialize ImGui overlay");

			menu_visible = false;

			UQM_ImGui_Shutdown ();
			return;
		}

		revalidate_game_state_cache ();
		return;
	}
}

// Processes SDL events for ImGui
bool UQM_ImGui_ProcessEvent (SDL_Event *event)
{
	if (!imgui_initialized)
		return false;

	cImGui_ImplSDL2_ProcessEvent (event);

	return ProcessControlEvents (event);
}

// This redirects input to ImGui when the menu is visible
int UQM_ImGui_WantCaptureInput (void)
{
	if (!imgui_initialized)
		return 0;

	return (io->WantCaptureKeyboard || io->WantCaptureMouse) ? 1 : 0;
}