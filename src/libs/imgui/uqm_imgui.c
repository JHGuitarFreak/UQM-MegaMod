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
#include "dcimgui.h"
#include "dcimgui_impl_sdl2.h"
#include "dcimgui_impl_sdlrenderer2.h"
#include "uqm/setupmenu.h"
#include "options.h"
#include "types.h"
#include "uqm/globdata.h"
#include "uqm/planets/planets.h"
#include "libs/graphics/gfx_common.h"
#include <stdio.h>

static bool menu_visible = 0;
static bool imgui_initialized = 0;
static SDL_Renderer *imgui_renderer = NULL;

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

	ImGui_StyleColorsDark (NULL);

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
}

// This redirects input to ImGui when the menu is visible
int UQM_ImGui_WantCaptureInput (void)
{
	if (!imgui_initialized)
		return 0;

	ImGuiIO *io = ImGui_GetIO ();
	return (io->WantCaptureKeyboard || io->WantCaptureMouse) ? 1 : 0;
}

// Begin actual menu drawing

typedef struct
{
	int settings_tab;
	int enhancements_tab;
	int randomizer_tab;
	int devtools_tab;
} MenuState;

static void draw_engine_menu (void);
static void draw_status_menu (void);
static void draw_graphics_menu (void);
static void draw_visual_menu (void);
static void draw_cheats_menu (void);
static void draw_qol_menu (void);
static void ShowFullScreenMenu (MenuState *state);
static MenuState menu_state = { 0 };

#define CENTER_TEXT (ImVec2){ 0.5f, 0.5f }
#define ZERO_F      (ImVec2){ 0.0f, 0.0f }

void UQM_ImGui_NewFrame (void)
{
	ImGuiIO *io;

	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
		return;

	if (!imgui_initialized)
		return;

	cImGui_ImplSDLRenderer2_NewFrame ();
	cImGui_ImplSDL2_NewFrame ();
	ImGui_NewFrame ();

	io = ImGui_GetIO ();
	if (io->WantTextInput && !SDL_IsTextInputActive ())
		SDL_StartTextInput ();
	else if (!io->WantTextInput && SDL_IsTextInputActive ())
		SDL_StopTextInput ();

	if (menu_visible)
		ShowFullScreenMenu (&menu_state);
}

static void
GeneralTab (MenuState *state, ImVec2 content_size, ImVec2 sidebar_size,
		ImVec2 button_size)
{
	BYTE i;

	const char *tab_names[] = {
		"Graphics",
		"PC / 3DO",
		"Sound",
		"Music",
		"Controls",
		"Status"
	};

	if (ImGui_BeginTabItem ("General", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("GeneralContent", content_size,
				ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("GeneralSidebar", sidebar_size,
				ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
				ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->settings_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->settings_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("GeneralContentArea", ZERO_F,
				ImGuiChildFlags_Borders, 0);

		switch (state->settings_tab)
		{
			case 0:
				draw_graphics_menu ();
				break;
			case 1:
				draw_engine_menu ();
				break;
			case 2:
				break;
			case 3:
				break;
			case 4:
				break;
			case 5:
				draw_status_menu ();
				break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

static void
EnhancementsTab (MenuState *state, ImVec2 content_size, ImVec2 sidebar_size,
		ImVec2 button_size)
{
	BYTE i;
	const char *tab_names[] = {
		"Quality of Life",
		"Skips & Speed-ups",
		"Visuals",
		"Fixes",
		"Difficulty",
		"Cheats"
//		"Timers"
	};

	if (ImGui_BeginTabItem ("Enhancements", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("EnhancementsContent", content_size,
				ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("EnhancementsSidebar", sidebar_size,
				ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
				ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->enhancements_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->enhancements_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("EnhancementsContentArea", ZERO_F,
				ImGuiChildFlags_Borders, 0);

		switch (state->enhancements_tab)
		{
			case 0:
				//ImGui_Text ("Quality of Life");
				draw_qol_menu ();
				break;
			case 1:
				ImGui_Text ("Skips & Speed-ups");
				break;
			case 2:
				//ImGui_Text ("Visuals");
				draw_visual_menu ();
				break;
			case 3:
				ImGui_Text ("Fixes");
				break;
			case 4:
				ImGui_Text ("Difficulty");
				break;
			case 5:
				draw_cheats_menu ();
				break;
			//case 6:
			//	ImGui_Text ("Timers");
			//	break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

static void
RandomizerTab (MenuState *state, ImVec2 content_size, ImVec2 sidebar_size,
		ImVec2 button_size)
{
	BYTE i;
	const char *tab_names[] = {
		"Seed Settings",
		"Enhancements"
	};

	if (ImGui_BeginTabItem ("Randomizer", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("RandomizerContent", content_size,
				ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("RandomizerSidebar", sidebar_size,
				ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
				ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->randomizer_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->randomizer_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("RandomizerContentArea", ZERO_F,
				ImGuiChildFlags_Borders, 0);

		switch (state->randomizer_tab)
		{
			case 0:
				ImGui_Text ("Seed Settings");
				break;
			case 1:
				ImGui_Text ("Enhancements");
				break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

static void
DevToolsTab (MenuState *state, ImVec2 content_size, ImVec2 sidebar_size,
		ImVec2 button_size)
{
	BYTE i;
	const char *tab_names[] = {
		"General",
		"Stats",
		"Console",
		"Save Editor"
	};

	if (ImGui_BeginTabItem ("Dev Tools", NULL, 0))
	{
		bool selected;

		ImGui_BeginChild ("DevToolsContent", content_size,
				ImGuiChildFlags_Borders, 0);
		ImGui_BeginChild ("DevToolsSidebar", sidebar_size,
				ImGuiChildFlags_Borders, 0);

		ImGui_PushStyleVarImVec2 (
				ImGuiStyleVar_SelectableTextAlign, CENTER_TEXT);

		for (i = 0; i < ARRAY_SIZE (tab_names); i++)
		{
			selected = (state->devtools_tab == i);
			if (ImGui_SelectableEx (tab_names[i], selected, 0, button_size))
				state->devtools_tab = i;

			if (i < ARRAY_SIZE (tab_names) - 1)
				ImGui_Spacing ();
		}

		ImGui_PopStyleVar ();
		ImGui_EndChild ();

		ImGui_SameLine ();
		ImGui_BeginChild ("DevToolsContentArea", ZERO_F,
				ImGuiChildFlags_Borders, 0);

		switch (state->devtools_tab)
		{
			case 0:
				ImGui_Text ("General Dev Tools");
				break;
			case 1:
				ImGui_Text ("Statistics");
				break;
			case 2:
				ImGui_Text ("Console");
				break;
			case 3:
				ImGui_Text ("Save Editor");
				break;
		}

		ImGui_EndChild ();
		ImGui_EndChild ();
		ImGui_EndTabItem ();
	}
}

ImVec2 display_size;

#define DISPLAY_BOOL (display_size.x > 640.0f ? 3 : 1)

void ShowFullScreenMenu (MenuState *state)
{
	float sidebar_width, button_height, content_height;
	ImVec2 button_size, sidebar_size, content_size;
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

	window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;

	if (!ImGui_Begin ("Full Screen Menu", NULL, window_flags))
	{
		ImGui_End ();
		return;
	}

	if (ImGui_BeginTabBar ("MainTabs", 0))
	{
		GeneralTab (state, content_size, sidebar_size, button_size);
		EnhancementsTab (state, content_size, sidebar_size, button_size);
		RandomizerTab (state, content_size, sidebar_size, button_size);
		DevToolsTab (state, content_size, sidebar_size, button_size);

		ImGui_EndTabBar ();
	}
	ImGui_End ();
}

static void draw_engine_menu (void)
{
	// Added blank spot because the combo box wouldn't quite work with
	// just two entries. Will fix later.
	const char *pc_or_3do[] = { "", "3DO", "DOS" };
	const char *star_backgrounds[] = { "DOS", "3DO", "UQM", "HD-mod" };
	const char *sphere_types[] = { "DOS", "3DO", "UQM" };
	const char *coarse_scan[] =
	{
		"DOS",
		"3DO",
		"DOS (6014)",
		"3DO (6014)"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "EngineColumns", false);

	ImGui_Checkbox ("Subtitles", (bool*)&optSubtitles);

	ImGui_ComboChar ("Text Scroll Style", &optSmoothScroll, pc_or_3do, 3);
	ImGui_ComboChar ("Oscilloscope Style", &optScopeStyle, pc_or_3do, 3);
	ImGui_ComboChar ("Screen Transitions", &optScrTrans, pc_or_3do, 3);
	ImGui_ComboChar ("Star Background", &optStarBackground,
			star_backgrounds, 4);
	ImGui_ComboChar ("Coarse Scan Display", &optWhichCoarseScan,
			coarse_scan, 4);
	ImGui_ComboChar ("Scan Style", &optScanStyle, pc_or_3do, 3);
	ImGui_ComboChar ("Scan Sphere Type", &optScanSphere, sphere_types, 3);
	ImGui_ComboChar ("Tint Scanned Sphere", &optTintPlanSphere,
			pc_or_3do, 3);
	ImGui_ComboChar ("Lander Style", &optSuperPC, pc_or_3do, 3);

	// ImGui_CollapsingHeader example
	//if (ImGui_CollapsingHeader ("Scanning", ImGuiTreeNodeFlags_DefaultOpen))
	//{
	//}
}

static void draw_status_menu (void)
{
	ImGui_ColumnsEx (DISPLAY_BOOL, "StatusColumns", false);

	{	// Edit Captain's Name
		char CaptainsName[SIS_NAME_SIZE];

		snprintf ((char *)&CaptainsName, sizeof (CaptainsName),
			"%s", GLOBAL_SIS (CommanderName));

		ImGui_Text ("Captain's Name:");
		if (ImGui_InputText ("##CaptainsName", CaptainsName,
			sizeof (CaptainsName), 0))
		{
			snprintf (GLOBAL_SIS (CommanderName),
				sizeof (GLOBAL_SIS (CommanderName)),
				"%s", CaptainsName);
		}
	}

	{	// Edit Ship Name
		char SISName[SIS_NAME_SIZE];

		snprintf ((char *)&SISName, sizeof (SISName),
			"%s", GLOBAL_SIS (ShipName));

		ImGui_Text ("Ship Name:");
		if (ImGui_InputText ("##SISName", SISName, sizeof (SISName), 0))
		{
			snprintf (GLOBAL_SIS (ShipName),
				sizeof (GLOBAL_SIS (ShipName)),
				"%s", SISName);
		}
	}

	ImGui_Text ("Current R.U.:");
	ImGui_InputInt ("##CurrentRU", (int *)&GLOBAL_SIS (ResUnits));

	{
		int CurrentFuel = GLOBAL_SIS (FuelOnBoard);
		int volume = FUEL_RESERVE +
			((DWORD)CountSISPieces (FUEL_TANK)
				* FUEL_TANK_CAPACITY
				+ (DWORD)CountSISPieces (HIGHEFF_FUELSYS)
				* HEFUEL_TANK_CAPACITY);

		ImGui_Text ("Current Fuel:");
		if (ImGui_InputInt ("##CurrentFuel", &CurrentFuel))
		{
			if (CurrentFuel > volume)
				CurrentFuel = volume;
			if (CurrentFuel < 0)
				CurrentFuel = 0;

			GLOBAL_SIS (FuelOnBoard) = CurrentFuel;
		}
	}

	{
		int Credits = MAKE_WORD (GET_GAME_STATE (MELNORME_CREDIT0),
			GET_GAME_STATE (MELNORME_CREDIT1));

		ImGui_Text ("Current Credits:");
		if (ImGui_InputInt ("##CurrentCredits", &Credits))
		{
			if (Credits > (uint16)~0)
				Credits = (uint16)~0;
			if (Credits < 0)
				Credits = 0;

			SET_GAME_STATE (MELNORME_CREDIT0, LOBYTE (Credits));
			SET_GAME_STATE (MELNORME_CREDIT1, HIBYTE (Credits));
		}
	}
}

static void draw_graphics_menu (void)
{
	bool value = false;
	const char *resolutions[] =
	{
		"Default",
		"640x480",
		"960x720",
		"1280x960",
		"1600x1200",
		"1920x1440",
		"Custom"
	};
	const char *aspect_ratios[] =
	{
		"Any",
		"Force 4:3"
	};
	const char *display_modes[] =
	{
		"Windowed",
		"Exclusive Fullscreen",
		"Borderless Fullscreen"
	};
	const char *scalers[] =
	{
		"None",
		"Bilinear",
		"Adapt. Bilinear",
		"Adv. Bilinear",
		"Triscan (Scale2x)",
		"HQ (2x)"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "GraphicsColumns", false);

	ImGui_BeginDisabled (true);
	if (ImGui_Checkbox ("HD Mode", (bool *)&resolutionFactor))
	{
		// Add HD switching code here
	}

	if (ImGui_IsItemHovered (ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui_SetTooltip ("Example of a tooltip");
	}
	ImGui_EndDisabled ();

	ImGui_Text ("Resolution:");
	if (ImGui_ComboChar ("##Resolution",
			(int *)&loresBlowupScale, resolutions, 7))
	{
		// Add resolution switch code here
	}

	{	// Edit Captain's Name
		char CustomResolution[SIS_NAME_SIZE];

		snprintf ((char *)&CustomResolution, sizeof (CustomResolution),
				"%dx%d", SavedWidth, SavedHeight);

		ImGui_Text ("Custom Resolution:");
		if (ImGui_InputText ("##CustomeResolution", CustomResolution,
			sizeof (CustomResolution), 0))
		{
			// Add custom resolution code here
		}
	}

	ImGui_Text ("Aspect Ratio:");
	if (ImGui_ComboChar ("##AspectRatio", (int *)&optKeepAspectRatio,
			aspect_ratios, 2))
	{
		// Add aspect ratio code here
	}

	{
		int display_mode = 0;

		if (GfxFlags & TFB_GFXFLAGS_FULLSCREEN)
			display_mode = 2;
		else if (GfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN)
			display_mode = 1;
		else
			display_mode = 0;

		ImGui_Text ("Display Mode:");
		if (ImGui_ComboChar ("##DisplayMode", (int *)&display_mode,
			display_modes, 3))
		{
			// Add display mode code here
		}
	}

	{
		ImGui_Text ("Gamma:");
		if (ImGui_SliderFloatEx ("##Gamma", &optGamma, 0.4f, 2.5f, "%.2f",
				ImGuiSliderFlags_Logarithmic))
		{
			setGammaCorrection (optGamma);
		}
		if (ImGui_Button ("Reset Gamma"))
		{
			optGamma = 1.0f;
			setGammaCorrection (optGamma);
		}
	}

	{
		int flags, curr_scaler;

		flags = GfxFlags & 248; // 11111000 - only scaler flags

		switch (flags)
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

		ImGui_Text ("Scaler:");
		if (ImGui_ComboChar ("##Scaler", &curr_scaler,
			scalers, 6))
		{
			// Add scaler code here
		}
	}

	{
		bool flags = GfxFlags & TFB_GFXFLAGS_SCANLINES;

		if (ImGui_Checkbox ("Scanlines", &flags))
		{
			// Add scanline code here
		}
	}

	{
		bool flags = GfxFlags & TFB_GFXFLAGS_SHOWFPS;

		if (ImGui_Checkbox ("Show FPS", &flags))
		{
			// Add show FPS code here
		}
	}
}

static void draw_cheats_menu (void)
{
	int MaxScrounged = MAX_SCROUNGED;

	const char *god_modes[] =
	{
		"None",
		"Infinite Energy",
		"Invulnerable",
		"Full God Mode"
	};
	const char *time_modes[] =
	{
		"Normal",
		"Slow (x6)",
		"Fast (x5)"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "CheatColumns", false); // For taming width

	ImGui_SeparatorText ("Basic Cheats");

	ImGui_Checkbox ("Kohr-Stahp", (bool *)&optCheatMode);
	ImGui_Checkbox ("Kohr-Ah DeCleansing", (bool *)&optDeCleansing);

	ImGui_Text ("God Modes:");
	ImGui_ComboChar ("##GodModes", &optGodModes, god_modes, 4);
	ImGui_Text ("Time Dilation:");
	ImGui_ComboChar ("##TimeDilation", &timeDilationScale, time_modes, 3);

	ImGui_Checkbox ("Bubble Warp", (bool*)&optBubbleWarp);
	ImGui_Checkbox ("Head Start", (bool *)&optHeadStart);
	ImGui_Checkbox ("Unlock All Ships", (bool *)&optUnlockShips);
	ImGui_Checkbox ("Infinite R.U.", (bool *)&optInfiniteRU);
	ImGui_Checkbox ("Infinite Fuel", (bool *)&optInfiniteFuel);
	ImGui_Checkbox ("Infinite Credits", (bool *)&optInfiniteCredits);
	ImGui_Checkbox ("No Hyperspace Encounters", (bool *)&optNoHQEncounters);
	ImGui_Checkbox ("No Melee Obstacles", (bool *)&optMeleeObstacles);

	ImGui_SeparatorText ("Expanded Cheats");

	ImGui_Text ("Lander Capacity:");
	ImGui_Checkbox ("##ChangeLanderCapacity", &changeLanderCapacity);
	ImGui_SameLine ();
	ImGui_InputInt ("##LanderCapacity",
			!changeLanderCapacity ? &MaxScrounged : &optLanderHold);

	//ImGui_NextColumn ();
}

static void draw_visual_menu (void)
{
	const char *date_formats[] =
	{
		"MMM DD.YYYY",
		"MM.DD.YYYY",
		"DD MMM.YYYY",
		"DD.MM.YYYY"
	};
	const char *fuel_ranges[] =
	{
		"None",
		"At Destination",
		"To Sol",
		"Both"
	};
	const char *planet_textures[] =
	{
		"3DO",
		"UQM"
	};

	ImGui_ColumnsEx (DISPLAY_BOOL, "VisualsColumns", false);

	ImGui_ComboChar ("Date Format", &optDateFormat, date_formats, 4);

	ImGui_Checkbox ("Show Whole Fuel Value", (bool *)&optWholeFuel);

	ImGui_ComboChar ("Fuel Range Display", &optFuelRange, fuel_ranges, 4);

	ImGui_Checkbox ("SOI Colors", (bool *)&optSphereColors);
	ImGui_Checkbox ("Animated HyperSpace Stars", (bool *)&optHyperStars);
	ImGui_Checkbox ("Captain Names in Shipyard", (bool *)&optCaptainNames);
	ImGui_Checkbox ("Alternate Orz Font", (bool *)&optOrzCompFont);
	ImGui_Checkbox ("Non-stop Oscilloscope", (bool *)&optNonStopOscill);
	ImGui_Checkbox ("Nebulae", (bool *)&optNebulae);
	ImGui_Checkbox ("Orbiting Planets", (bool *)&optOrbitingPlanets);
	ImGui_Checkbox ("Textured Planets", (bool *)&optTexturedPlanets);
	ImGui_Checkbox ("Unscaled Star System (HD Only)",
			(bool *)&optUnscaledStarSystem);
	ImGui_Checkbox ("NPC Ship Orientation", (bool *)&optShipDirectionIP);
	ImGui_Checkbox ("Hazard Colors", (bool *)&optHazardColors);

	ImGui_ComboChar ("Planet Map Textures", (int *)&optPlanetTexture,
			planet_textures, 2);

	ImGui_Checkbox ("Show Lander Upgrades", (bool *)&optShowUpgrades);

	ImGui_SliderInt ("Nebulae Volume", &optNebulaeVolume, 0, 50);
}

static void draw_qol_menu (void)
{
	ImGui_ColumnsEx (DISPLAY_BOOL, "QoLColumns", false);

	ImGui_Checkbox ("Partial Pickup", (bool *)&optPartialPickup);
	ImGui_Checkbox ("Scatter Elements", (bool *)&optScatterElements);
	ImGui_Checkbox ("In-Game Help Menus", (bool *)&optSubmenu);
	ImGui_Checkbox ("Smart Auto-Pilot", (bool *)&optSmartAutoPilot);
	ImGui_Checkbox ("Advanced Auto-Pilot", (bool *)&optAdvancedAutoPilot);
	ImGui_Checkbox ("Show Visited Stars", (bool *)&optShowVisitedStars);
	ImGui_Checkbox ("Super Melee Ship Descriptions",
			(bool *)&optMeleeToolTips);
	ImGui_Checkbox ("Ship Storage Queue", (bool *)&optShipStore);
}