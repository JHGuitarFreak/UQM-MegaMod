// Copyright Michael Martin, 2004.

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

#include "setupmenu.h"

#include "controls.h"
#include "options.h"
#include "setup.h"
#include "sounds.h"
#include "colors.h"
#include "fmv.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/widgets.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/strlib.h"
#include "libs/reslib.h"
#include "libs/inplib.h"
#include "libs/vidlib.h"
#include "libs/sound/sound.h"
#include "libs/resource/stringbank.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "resinst.h"
#include "nameref.h"
#include <math.h>
#include "gamestr.h"
#include "libs/graphics/bbox.h"
#include "libs/math/random.h"
#include "libs/input/input_common.h"

#include SDL_INCLUDE(SDL_version.h)

static STRING SetupTab;

typedef struct setup_menu_state {
	BOOLEAN (*InputFunc) (struct setup_menu_state *pInputState);

	BOOLEAN initialized;
	int anim_frame_count;
	DWORD NextTime;
} SETUP_MENU_STATE;

struct option_list_value
{
	const char *str;
	int value;
};

const struct option_list_value scalerList[6] =
{
	{"no",       0},
	{"bilinear", TFB_GFXFLAGS_SCALE_BILINEAR},
	{"biadapt",  TFB_GFXFLAGS_SCALE_BIADAPT},
	{"biadv",    TFB_GFXFLAGS_SCALE_BIADAPTADV},
	{"triscan",  TFB_GFXFLAGS_SCALE_TRISCAN},
	{"hq",       TFB_GFXFLAGS_SCALE_HQXX}
};

static int SfxVol;
static int MusVol;
static int SpcVol;
static int optMScale;
static SOUND testSounds;

static int
whichPlatformRef (OPT_CONSOLETYPE opt)
{// Returns 1 if OPTVAL_3DO and 2 of OPTVAL_PC (1 and 0 respectively)
	return (opt ? OPT_3DO : OPT_PC);
}

static BOOLEAN
PutBoolOpt (OPT_ENABLABLE *glob, OPT_ENABLABLE *set, const char *key,
		BOOLEAN reload)
{
	if (*glob != *set)
	{
		*glob = *set;
		res_PutBoolean (key, (BOOLEAN)*set);
		if (reload)
			optRequiresReload = TRUE;
		return TRUE;
	}

	return FALSE;
}

static BOOLEAN
PutIntOpt (int *glob, int *set, const char *key, BOOLEAN reload)
{
	if (*glob != *set)
	{
		*glob = *set;
		res_PutInteger (key, *set);
		if (reload)
			optRequiresReload = TRUE;
		return TRUE;
	}

	return FALSE;
}

static BOOLEAN
PutConsOpt (int *glob, OPT_CONSOLETYPE *set, const char *key,
		BOOLEAN reload)
{
	if (*glob != whichPlatformRef (*set))
	{
		*glob = whichPlatformRef (*set);
		res_PutBoolean (key, (BOOLEAN)*set);
		if (reload)
			optRequiresReload = TRUE;
		return TRUE;
	}

	return FALSE;
}

static BOOLEAN DoSetupMenu (SETUP_MENU_STATE *pInputState);
static BOOLEAN done;
static WIDGET *current, *next;

static int quit_main_menu (WIDGET *self, int event);
static int quit_sub_menu (WIDGET *self, int event);
static int do_graphics (WIDGET *self, int event);
static int do_audio (WIDGET *self, int event);
static int do_engine (WIDGET *self, int event);
static int do_cheats (WIDGET *self, int event);
static int do_keyconfig (WIDGET *self, int event);
static int do_advanced (WIDGET *self, int event);
static int do_editkeys (WIDGET *self, int event);
static int do_music (WIDGET *self, int event);
static int do_visual (WIDGET *self, int event);
static int do_qol (WIDGET *self, int event);
static int do_qol (WIDGET *self, int event);
static int do_devices (WIDGET *self, int event);
static int do_upgrades (WIDGET *self, int event);
static void change_template (WIDGET_CHOICE *self, int oldval);
static void rename_template (WIDGET_TEXTENTRY *self);
static void rebind_control (WIDGET_CONTROLENTRY *widget);
static void clear_control (WIDGET_CONTROLENTRY *widget);

#define MENU_COUNT         13
#define CHOICE_COUNT      125
#define SLIDER_COUNT        5
#define BUTTON_COUNT       16
#define LABEL_COUNT         9
#define TEXTENTRY_COUNT     3
#define CONTROLENTRY_COUNT  8

/* The space for our widgets */
static WIDGET_MENU_SCREEN menus[MENU_COUNT];
static WIDGET_CHOICE choices[CHOICE_COUNT];
static WIDGET_SLIDER sliders[SLIDER_COUNT];
static WIDGET_BUTTON buttons[BUTTON_COUNT];
static WIDGET_LABEL labels[LABEL_COUNT];
static WIDGET_TEXTENTRY textentries[TEXTENTRY_COUNT];
static WIDGET_CONTROLENTRY controlentries[CONTROLENTRY_COUNT];

/* The hardcoded data that isn't strings */

typedef int (*HANDLER)(WIDGET *, int);

static HANDLER button_handlers[BUTTON_COUNT] = {
	quit_main_menu, quit_sub_menu, do_graphics, do_engine,
	do_audio, do_cheats, do_keyconfig, do_advanced, do_editkeys,
	do_keyconfig, do_music, do_visual, do_qol, do_devices, do_upgrades,
	do_cheats };

/* These refer to uninitialized widgets, but that's OK; we'll fill
 * them in before we touch them */
static WIDGET *main_widgets[] = {
	(WIDGET *)(&buttons[BTN_GFXMENU  ]),		// Graphics
	(WIDGET *)(&buttons[BTN_ENGMENU  ]),		// PC/3DO
	(WIDGET *)(&buttons[BTN_VISMENU  ]),		// Visuals
	(WIDGET *)(&buttons[BTN_SNDMENU  ]),		// Sound
	(WIDGET *)(&buttons[BTN_MUSMENU  ]),		// Music
	(WIDGET *)(&buttons[BTN_KEYMENU  ]),		// Controls
	(WIDGET *)(&buttons[BTN_QOLMENU  ]),		// Quality of Life
	(WIDGET *)(&buttons[BTN_ADVMENU  ]),		// Advanced
	(WIDGET *)(&buttons[BTN_CHTMENU  ]),		// Cheats

	(WIDGET *)(&labels[LABEL_SPACER	 ]),		// Spacer
	(WIDGET *)(&buttons[BTN_QUITSETUP]),		// Quit Setup Menu
	NULL };

static WIDGET *graphics_widgets[] = {
	(WIDGET *)(&choices[CHOICE_GRAPHICS  ]),	// Graphics
	(WIDGET *)(&choices[CHOICE_RESOLUTION]),	// Resolution
	(WIDGET *)(&textentries[TEXT_CUSTMRES]),	// Custom resolution entry
#if	SDL_MAJOR_VERSION == 1
#if defined (HAVE_OPENGL)
	(WIDGET *)(&choices[CHOICE_FRBUFFER  ]),	// Use Framebuffer
#endif
#endif
	(WIDGET *)(&choices[CHOICE_ASPRATIO  ]),	// Aspect Ratio
	(WIDGET *)(&choices[CHOICE_DISPLAY   ]),	// Display Mode
	(WIDGET *)(&sliders[SLIDER_GAMMA     ]),	// Gamma Correction
	(WIDGET *)(&choices[CHOICE_SCALER    ]),	// Scaler
	(WIDGET *)(&choices[CHOICE_SCANLINE  ]),	// Scanlines
#if	SDL_MAJOR_VERSION == 2
	(WIDGET *)(&choices[CHOICE_SHOWFPS   ]),	// Show FPS
#endif

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&buttons[BTN_QUITSUBMENU  ]),	// Exit to Menu
	NULL };

static WIDGET *engine_widgets[] = {
	(WIDGET *)(&labels[LABEL_UI			   ]),  // UI Label
	(WIDGET *)(&choices[CHOICE_WINDOWTYPE  ]),  // Window Type

	(WIDGET *)(&labels[LABEL_SPACER		   ]),  // Spacer
	(WIDGET *)(&choices[CHOICE_MENUSTYLE   ]),  // Menu Style
	(WIDGET *)(&choices[CHOICE_FONTSTYLE   ]),  // Font Style
	(WIDGET *)(&choices[CHOICE_CUTSCENE    ]),  // Cutscenes
#if defined(ANDROID) || defined(__ANDROID__)
	(WIDGET *)(&choices[CHOICE_ANDRZOOM    ]),  // Android: Melee Zoom
#else
	(WIDGET *)(&choices[CHOICE_MELEEZOOM   ]),  // Melee Zoom
#endif
	(WIDGET *)(&choices[CHOICE_FLAGSHIP    ]),  // Flagship Style
	(WIDGET *)(&choices[CHOICE_SCRMELT     ]),  // Screen Transitions

	(WIDGET *)(&labels[LABEL_SPACER		   ]),  // Spacer
	(WIDGET *)(&labels[LABEL_COMM		   ]),  // Comm Label
	(WIDGET *)(&choices[CHOICE_SCROLLSTYLE ]),  // Scroll Style
	(WIDGET *)(&choices[CHOICE_SPEECH      ]),  // Speech
	(WIDGET *)(&choices[CHOICE_SUBTITLES   ]),  // Subtitles
	(WIDGET *)(&choices[CHOICE_OSCILLSTYLE ]),  // Oscilloscope Style

	(WIDGET *)(&labels[LABEL_SPACER        ]),  // Spacer
	(WIDGET *)(&labels[LABEL_IP			   ]),  // IP Label
	(WIDGET *)(&choices[CHOICE_IPSTYLE     ]),  // Interplanetary Style
	(WIDGET *)(&choices[CHOICE_IPBACKGROUND]),  // Star Background

	(WIDGET *)(&labels[LABEL_SPACER		   ]),  // Spacer
	(WIDGET *)(&labels[LABEL_SCAN		   ]),  // Scan Label
	(WIDGET *)(&choices[CHOICE_SCANMENU    ]),  // Scan Menu Display
	(WIDGET *)(&choices[CHOICE_SLVSHIELD   ]),  // Slave Shields
	(WIDGET *)(&choices[CHOICE_SCANSTYLE   ]),  // Scan Style
	(WIDGET *)(&choices[CHOICE_SCANSPHERE  ]),  // Scan Sphere Type
	(WIDGET *)(&choices[CHOICE_SCANTINT    ]),  // Scanned Sphere Tint
	(WIDGET *)(&choices[CHOICE_LANDERSTYLE ]),  // Lander Style
	(WIDGET *)(&buttons[BTN_QUITSUBMENU    ]),	// Exit to Menu
	NULL };

static WIDGET *audio_widgets[] = {
	(WIDGET *)(&sliders[SLIDER_MUSVOLUME ]),	// Music Volume
	(WIDGET *)(&sliders[SLIDER_SFXVOLUME ]),	// SFX Volume
	(WIDGET *)(&sliders[SLIDER_SPCHVOLUME]),	// Speech Volume

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&choices[CHOICE_POSAUDIO  ]),	// Positional Audio
	(WIDGET *)(&choices[CHOICE_SNDDRIVER ]),	// Sound Driver
	(WIDGET *)(&choices[CHOICE_SNDQUALITY]),	// Sound Quality

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&buttons[BTN_QUITSUBMENU  ]),	// Exit to Menu
	NULL };

static WIDGET *music_widgets[] = {
	(WIDGET *)(&choices[CHOICE_REMIXES1  ]),	// 3DO Remixes
	(WIDGET *)(&choices[CHOICE_REMIXES2  ]),	// Precursor's Remixes
	(WIDGET *)(&choices[CHOICE_REMIXES3  ]),	// Volasaurus' Remix Pack

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&choices[CHOICE_IPMUSIC   ]),	// Volasaurus' Space Music
	(WIDGET *)(&choices[CHOICE_MMENUMUSIC]),	// Main Menu Music
	(WIDGET *)(&choices[CHOICE_MUSRESUME ]),	// Music Resume

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&buttons[BTN_QUITSUBMENU  ]),	// Exit to Menu
	NULL };

static WIDGET *cheat_widgets[] = {
	(WIDGET *)(&buttons[BTN_DEVMENU        ]),  // Devices Menu
	(WIDGET *)(&buttons[BTN_UPGMENU        ]),  // Upgrades Menu
	(WIDGET *)(&choices[CHOICE_CHEATING    ]),  // JMS: cheatMode on/off
	(WIDGET *)(&choices[CHOICE_CHDECLEAN   ]),  // Kohr-Ah DeCleansing mode
	(WIDGET *)(&choices[CHOICE_CHGODMODE   ]),  // Precursor Mode
	(WIDGET *)(&choices[CHOICE_CHTIME      ]),  // Time Dilation
	(WIDGET *)(&choices[CHOICE_CHWARP      ]),  // Bubble Warp
	(WIDGET *)(&choices[CHOICE_CHHEADSTART ]),  // Head Start
	(WIDGET *)(&choices[CHOICE_CHSHIPS     ]),  // Unlock Ships
	//(WIDGET *)(&choices[CHOICE_CHUPGRADES]),  // Unlock Upgrades
	(WIDGET *)(&choices[CHOICE_CHINFRU     ]),  // Infinite RU
	(WIDGET *)(&choices[CHOICE_CHINFFUEL   ]),  // Infinite Fuel
	(WIDGET *)(&choices[CHOICE_CHINFCRD    ]),  // Infinite Credits
	(WIDGET *)(&choices[CHOICE_CHCLEANHYPER]),  // No HyperSpace Encounters
	(WIDGET *)(&choices[CHOICE_CHNOPLANET  ]),  // No Planets in melee
	(WIDGET *)(&buttons[BTN_QUITSUBMENU    ]),  // Exit to Menu
	NULL };
	
static WIDGET *keyconfig_widgets[] = {
#if SDL_MAJOR_VERSION == 2 // Refined joypad controls not supported in SDL1
	(WIDGET *)(&choices[CHOICE_INPDEVICE]),		// Control Display
#endif
	(WIDGET *)(&choices[CHOICE_BTMPLAYER]),		// Bottom Player
	(WIDGET *)(&choices[CHOICE_TOPPLAYER]),		// Top Player
#if defined(ANDROID) || defined(__ANDROID__)
	(WIDGET *)(&choices[CHOICE_JOYSTICK ]),		// Directional Joystick toggle
#endif

	(WIDGET *)(&labels[LABEL_SPACER		]),     // Spacer
	(WIDGET *)(&labels[LABEL_KEYSTOOLTIP]),		// "To view or edit..."
	(WIDGET *)(&buttons[BTN_EDITKEYS    ]),		// Edit Controls

	(WIDGET *)(&labels[LABEL_SPACER		]),     // Spacer
	(WIDGET *)(&buttons[BTN_QUITSUBMENU ]),		// Exit to Menu
	NULL };

static WIDGET *advanced_widgets[] = {
	(WIDGET *)(&choices[CHOICE_SKILLLVL  ]),	// Difficulty
	(WIDGET *)(&choices[CHOICE_EXTENDED  ]),	// Extended features
	(WIDGET *)(&choices[CHOICE_NOMAD     ]),	// Nomad Mode
	(WIDGET *)(&choices[CHOICE_SLAUGHTER ]),	// Slaughter Mode
	(WIDGET *)(&choices[CHOICE_FLEETPOINT]),	// Fleet Point System

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&choices[CHOICE_GAMESEED  ]),	// Seed usage selection
	(WIDGET *)(&textentries[TEXT_GAMESEED]),	// Custom Seed entry
	(WIDGET *)(&choices[CHOICE_SOICOLOR  ]),	// SOI Color Selection

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&buttons[BTN_QUITSUBMENU  ]),	// Exit to Menu
	NULL };

static WIDGET *visual_widgets[] = {
	(WIDGET *)(&labels[LABEL_UI				]), // UI Label
	(WIDGET *)(&choices[CHOICE_DATESTRING   ]),	// Switch date formats
	(WIDGET *)(&choices[CHOICE_CUSTBORDER   ]),	// Custom Border switch
	(WIDGET *)(&choices[CHOICE_FUELDECIM    ]),	// Whole Fuel Value switch
	(WIDGET *)(&choices[CHOICE_FUELCIRCLE   ]),	// Fuel Range
	(WIDGET *)(&choices[CHOICE_ANIMHYPER    ]),	// Animated HyperStars
	(WIDGET *)(&choices[CHOICE_GAMEOVER     ]),	// Game Over switch

	(WIDGET *)(&labels[LABEL_SPACER			]), // Spacer
	(WIDGET *)(&labels[LABEL_COMM			]), // Comm Label
	(WIDGET *)(&choices[CHOICE_ORZFONT      ]),	// Alternate Orz font
	(WIDGET *)(&choices[CHOICE_NOSTOSCILL   ]),	// Non-Stop Scope

	(WIDGET *)(&labels[LABEL_SPACER			]), // Spacer
	(WIDGET *)(&labels[LABEL_IP				]), // IP Label
	(WIDGET *)(&choices[CHOICE_NEBULAE      ]),	// IP nebulae on/off
	(WIDGET *)(&sliders[SLIDER_NEBULA       ]),	// Nebulae Volume
	(WIDGET *)(&choices[CHOICE_ORBPLANETS   ]),	// orbitingPlanets on/off
	(WIDGET *)(&choices[CHOICE_TEXPLANETS   ]),	// texturedPlanets on/off
	(WIDGET *)(&choices[CHOICE_6014IP       ]),	// T6014's Classic Star System View
	(WIDGET *)(&choices[CHOICE_IPSHIPDIR    ]),	// NPC Ship Direction in IP

	(WIDGET *)(&labels[LABEL_SPACER			]), // Spacer
	(WIDGET *)(&labels[LABEL_SCAN			]), // Scan Label
	(WIDGET *)(&choices[CHOICE_HAZARDCLR    ]), // Hazard Colors
	(WIDGET *)(&choices[CHOICE_PLNTEXTURE   ]), // Planet Texture
	(WIDGET *)(&choices[CHOICE_LANDERUPGMASK]), // Show Lander Upgrades
	(WIDGET *)(&buttons[BTN_QUITSUBMENU     ]),	// Exit to Menu
	NULL };

static WIDGET *qol_widgets[] = {
	(WIDGET *)(&choices[CHOICE_SKIPINTRO   ]),  // Skip Intro
	(WIDGET *)(&choices[CHOICE_PARTPICKUP  ]),  // Partial Pickup switch
	(WIDGET *)(&choices[CHOICE_SCATTERCARGO]),  // Scatter Elements
	(WIDGET *)(&choices[CHOICE_SUBMENU     ]),  // Submenu switch
	(WIDGET *)(&choices[CHOICE_SMARTAUTO   ]),  // Smart Auto-Pilot
	(WIDGET *)(&choices[CHOICE_ADVAUTO     ]),  // Advanced Auto-Pilot
	(WIDGET *)(&choices[CHOICE_VISITED     ]),  // Show Visited Stars
	(WIDGET *)(&choices[CHOICE_MLTOOLTIP   ]),  // Melee Tool Tips

	(WIDGET *)(&labels[LABEL_SPACER		   ]),  // Spacer
	(WIDGET *)(&buttons[BTN_QUITSUBMENU    ]),  // Exit to Menu
	NULL };

static WIDGET *editkeys_widgets[] = {
	(WIDGET *)(&choices[CHOICE_KBLAYOUT    ]),  // Current layout
	(WIDGET *)(&textentries[TEXT_LOUTNAME  ]),  // Layout name
	(WIDGET *)(&labels[LABEL_TAPTOOLTIP	   ]),  // "Press return to..."
	(WIDGET *)(&controlentries[CONTROL_UP  ]),	// Up
	(WIDGET *)(&controlentries[CONTROL_DOWN]),	// Down
	(WIDGET *)(&controlentries[CONTROL_LEFT]),	// Left
	(WIDGET *)(&controlentries[CONTROL_RGHT]),	// Right
	(WIDGET *)(&controlentries[CONTROL_WEAP]),	// Weapon
	(WIDGET *)(&controlentries[CONTROL_SPEC]),	// Special
	(WIDGET *)(&controlentries[CONTROL_ESC ]),	// Escape
	(WIDGET *)(&controlentries[CONTROL_THRU]),	// Thrust
	(WIDGET *)(&buttons[BTN_PREVMENU	   ]),	// Previous menu
	NULL };

static WIDGET *devices_widgets[] = {
	(WIDGET *)(&choices[CHOICE_DEVSPAWNER]),	// Portal Spawner
	(WIDGET *)(&choices[CHOICE_DEVPET    ]),	// Talking Pet
	(WIDGET *)(&choices[CHOICE_DEVBOMB   ]),	// Utwig Bomb
	(WIDGET *)(&choices[CHOICE_DEVSUN    ]),	// Sun Device
	(WIDGET *)(&choices[CHOICE_DEVSPHERE ]),	// Rosy Sphere
	(WIDGET *)(&choices[CHOICE_DEVHELIX  ]),	// Aqua Helix
	(WIDGET *)(&choices[CHOICE_DEVSPINDLE]),	// Clear Spindle
	(WIDGET *)(&choices[CHOICE_DEVULTRON0]),	// Ultron (Broken)
	(WIDGET *)(&choices[CHOICE_DEVULTRON1]),	// Ultron (Semi-Broken)
	(WIDGET *)(&choices[CHOICE_DEVULTRON2]),	// Ultron (Semi-Fixed)
	(WIDGET *)(&choices[CHOICE_DEVULTRON3]),	// Ultron (Fixed)
	(WIDGET *)(&choices[CHOICE_DEVMAIDENS]),	// Shofixti Maidens
	(WIDGET *)(&choices[CHOICE_DEVCASTER0]),	// Umgah Caster
	(WIDGET *)(&choices[CHOICE_DEVCASTER1]),	// Burvixese Caster
	(WIDGET *)(&choices[CHOICE_DEVSHIELD ]),	// Taalo Shield
	(WIDGET *)(&choices[CHOICE_DEVEGGCS0 ]),	// Egg Case 01
	(WIDGET *)(&choices[CHOICE_DEVEGGCS1 ]),	// Egg Case 02
	(WIDGET *)(&choices[CHOICE_DEVEGGCS2 ]),	// Egg Case 03
	(WIDGET *)(&choices[CHOICE_DEVSHUTTLE]),	// Syreen Shuttle
	(WIDGET *)(&choices[CHOICE_DEVBEAST  ]),	// VUX Beast
	(WIDGET *)(&choices[CHOICE_DEVSLYCODE]),	// Slylandro Destruct
	(WIDGET *)(&choices[CHOICE_DEVWARPPOD]),	// Ur-Quan Warp Pod
	(WIDGET *)(&choices[CHOICE_DEVTRIDENT]),	// Wimbli's Trident
	(WIDGET *)(&choices[CHOICE_DEVROD    ]),	// Glowing Rod
	(WIDGET *)(&choices[CHOICE_DEVLUNBASE]),	// Lunar Base

	(WIDGET *)(&labels[LABEL_SPACER		 ]),    // Spacer
	(WIDGET *)(&buttons[BTN_CHTPREV		 ]),	// Back to Cheats
	NULL };

static WIDGET *upgrades_widgets[] = {
	(WIDGET *)(&choices[CHOICE_UPGLANDERSPD]),  // Lander Speed
	(WIDGET *)(&choices[CHOICE_UPGLANDERCRG]),  // Lander Cargo
	(WIDGET *)(&choices[CHOICE_UPGLANDERFRE]),  // Lander Rapid Fire
	(WIDGET *)(&choices[CHOICE_UPGLANDERBSH]),  // Lander Bio Shield
	(WIDGET *)(&choices[CHOICE_UPGLANDERQSH]),  // Lander Quake Shield
	(WIDGET *)(&choices[CHOICE_UPGLANDERLSH]),  // Lander Lightning Shield
	(WIDGET *)(&choices[CHOICE_UPGLANDERHSH]),  // Lander Heat Shield
	(WIDGET *)(&choices[CHOICE_MODPOINTDEF ]),  // Point Defense Module
	(WIDGET *)(&choices[CHOICE_MODFUSBLAST ]),  // Fusion Blaster Module
	(WIDGET *)(&choices[CHOICE_MODFUELTANK ]),  // Hi-Eff Fuel Module
	(WIDGET *)(&choices[CHOICE_MODTRACKING ]),  // Tracking Module
	(WIDGET *)(&choices[CHOICE_MODHELLBORE ]),  // Hellbore Cannon Module
	(WIDGET *)(&choices[CHOICE_MODFURNACE  ]),  // Shiva Furnace Module

	(WIDGET *)(&labels[LABEL_SPACER		   ]),  // Spacer
	(WIDGET *)(&buttons[BTN_CHTPREV		   ]),  // Back to Cheats
	NULL };

static const struct
{
	WIDGET **widgets;
	int bgIndex;
}
menu_defs[] =
{
	{main_widgets, 0},
	{graphics_widgets, 1},
	{audio_widgets, 2},
	{engine_widgets, 3},
	{cheat_widgets, 4},
	{keyconfig_widgets, 5},
	{advanced_widgets, 6},
	{editkeys_widgets, 7},
	{music_widgets, 8},
	{visual_widgets, 9},
	{qol_widgets, 10},
	{devices_widgets, 11},
	{upgrades_widgets, 12},
	{NULL, 0}
};

// Start with reasonable gamma bounds. These will get updated
// as we find out the actual bounds.
static float minGamma = 0.4f;
static float maxGamma = 2.5f;
// The gamma slider uses an exponential curve
// We use y = e^(2.1972*(x-1)) curve to give us a nice spread of
// gamma values 0.11 < g < 9.0 centered at g=1.0
#define GAMMA_CURVE_B  2.1972f
static float minGammaX;
static float maxGammaX;

//No longer used
//static int
//number_res_options (void)
//{
//	if (TFB_SupportsHardwareScaling ())
//	{
//		return 4;
//	}
//	else
//	{
//		return 2;
//	}
//}

static int
quit_main_menu (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = NULL;
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
quit_sub_menu (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		ResetOffset ();
		next = (WIDGET *)(&menus[MENU_QUITSUB]);
		(*next->receiveFocus) (next, WIDGET_EVENT_SELECT);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static void
populate_res (void)
{
	sprintf (textentries[TEXT_CUSTMRES].value, "%dx%d",
			WindowWidth, WindowHeight);
}

static int
do_graphics (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_GRAPHICS]);
		populate_res ();
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_audio (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_AUDIO]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_engine (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_ENGINE]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_cheats (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_CHEATS]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_keyconfig (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENY_KEYCONF]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static void
populate_seed (void)
{
	snprintf (textentries[TEXT_GAMESEED].value, 
		sizeof (textentries[TEXT_GAMESEED].value), "%d", optCustomSeed);
	if (!SANE_SEED (optCustomSeed))
		optCustomSeed = PrimeA;
}

static int
do_advanced (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_ADVANCED]);
		populate_seed();
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_music (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_MUSIC]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_visual (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_VISUAL]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_qol (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_QOL]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_devices (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_DEVICES]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_upgrades (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_UPGRADES]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static void
populate_editkeys (int templat)
{
	int i, j;
	
	strncpy (textentries[TEXT_LOUTNAME].value, input_templates[templat].name,
			textentries[TEXT_LOUTNAME].maxlen);
	textentries[TEXT_LOUTNAME].value[textentries[TEXT_LOUTNAME].maxlen-1] = 0;
	
	for (i = 0; i < NUM_KEYS; i++)
	{
		for (j = 0; j < 2; j++)
		{
			InterrogateInputState (templat, i, j,
					controlentries[i].controlname[j],
					WIDGET_CONTROLENTRY_WIDTH);
		}
	}
}

static int
do_editkeys (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[MENU_EDITKEYS]);
		/* Prepare the components */
		choices[CHOICE_KBLAYOUT].selected = 0;
		
		populate_editkeys (0);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static void
change_template (WIDGET_CHOICE *self, int oldval)
{
	(void) oldval;
	populate_editkeys (self->selected);
}

static void
addon_unavailable (WIDGET_CHOICE *self, int oldval)
{
	self->selected = oldval;
	DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 36));
	Widget_SetFont (PlyrFont);
}

static void
check_for_hd (WIDGET_CHOICE *self, int oldval)
{
	if (self->selected != OPTVAL_REAL_1280_960)
		return;

	if (!isAddonAvailable (HD_MODE))
	{
		oldval = OPTVAL_320_240;
		addon_unavailable (self, OPTVAL_320_240);
	}
}

static BOOLEAN
check_dos_3do_modes (WIDGET_CHOICE *self, int oldval)
{
	switch (self->selected)
	{
		case OPTVAL_PC_WINDOW:
			if (!isAddonAvailable (
					DOS_MODE (choices[CHOICE_GRAPHICS].selected && !IS_HD ? HD : IS_HD)))
			{
				oldval = OPTVAL_UQM_WINDOW;
				addon_unavailable (self, oldval);
				return FALSE;
			}
			break;
		case OPTVAL_3DO_WINDOW:
			if (!isAddonAvailable (
					THREEDO_MODE (
						choices[CHOICE_GRAPHICS].selected && !IS_HD ? HD : IS_HD)))
			{
				oldval = OPTVAL_UQM_WINDOW;
				addon_unavailable (self, OPTVAL_UQM_WINDOW);
				return FALSE;
			}
			break;
		default:
			break;
	}

	return TRUE;
}

static BOOLEAN
check_remixes (WIDGET_CHOICE *self, int oldval)
{
	BOOLEAN addon_available = FALSE;
	switch (self->choice_num)
	{
		case 9:
			addon_available = isAddonAvailable (THREEDO_MUSIC);
			break;
		case 21:
			addon_available = isAddonAvailable (REMIX_MUSIC);
			break;
		case 47:
			addon_available = isAddonAvailable (VOL_RMX_MUSIC);
			break;
		case 46:
			addon_available = isAddonAvailable (VOL_RMX_MUSIC) ||
					isAddonAvailable (REGION_MUSIC);
			break;
		default:
			log_add (log_Error, "invalid choice_num in check_remixes()");
			break;
	}

	if (!addon_available)
	{
		oldval = OPTVAL_DISABLED;
		choices[self->choice_num].selected = oldval;
		addon_unavailable (self, oldval);
		return FALSE;
	}

	return TRUE;
}

static void
check_availability (WIDGET_CHOICE *self, int oldval)
{
	if (self->choice_num == 0)
		check_for_hd (self, oldval);

	if (self->choice_num == 9 || self->choice_num == 21
			|| self->choice_num == 46 || self->choice_num == 47)
	{
		check_remixes (self, oldval);
	}

	if (self->choice_num == 81)
	{
		check_dos_3do_modes (self, oldval);
	}
}

static void
rename_template (WIDGET_TEXTENTRY *self)
{
	/* TODO: This will have to change if the size of the
	   input_templates name is changed.  It would probably be nice
	   to track this symbolically or ensure that self->value's
	   buffer is always at least this big; this will require some
	   reworking of widgets */
	strncpy (input_templates[choices[CHOICE_KBLAYOUT].selected].name, self->value, 30);
	input_templates[choices[CHOICE_KBLAYOUT].selected].name[29] = 0;
}

static void
change_seed (WIDGET_TEXTENTRY *self)
{
	if (!SANE_SEED (atoi (self->value)))
		snprintf (self->value, sizeof (self->value), "%d", PrimeA);
}

static void
adjustMusic (WIDGET_SLIDER *self)
{
	musicVolumeScale = self->value / 100.0f;
	SetMusicVolume (musicVolume);
}

static void
adjustSFX (WIDGET_SLIDER *self)
{
	sfxVolumeScale = self->value / 100.0f;
}

static void
adjustSpeech (WIDGET_SLIDER *self)
{
	speechVolumeScale = self->value / 100.0f;
	SetSpeechVolume (speechVolumeScale);

	TestSpeechSound (SetAbsSoundIndex (testSounds, (self->value == 100)));
}

static void
toggle_scanlines (WIDGET_CHOICE *self, int *NewGfxFlags)
{
	if (self->selected == 1)
		*NewGfxFlags |= TFB_GFXFLAGS_SCANLINES;
	else
		*NewGfxFlags &= ~TFB_GFXFLAGS_SCANLINES;
	res_PutBoolean ("config.scanlines", self->selected);
}

static void
change_scaling (WIDGET_CHOICE *self, int *NewWidth, int *NewHeight)
{
	if (self->selected < 6)
	{
#if !(defined(ANDROID) || defined(__ANDROID__))
		if (!self->selected)
		{	// No blowup
			*NewWidth = RES_SCALE (320);
			*NewHeight = RES_SCALE (DOS_BOOL (240, 200));
		}
		else
		{
			*NewWidth = 320 * (1 + self->selected);
			*NewHeight = DOS_BOOL (240, 200) * (1 + self->selected);
		}
#else
		if (!self->selected)
		{	// No blowup
			*NewWidth = RES_SCALE (320);
			*NewHeight = RES_SCALE (240);
		}
		else
		{
			*NewWidth = 320 * (1 + self->selected);
			*NewHeight = 240 * (1 + self->selected);
		}
#endif
	}

	PutIntOpt ((int *)(&loresBlowupScale), (int *)(&self->selected),
			"config.loresBlowupScale", FALSE);
	res_PutInteger ("config.reswidth", *NewWidth);
	res_PutInteger ("config.resheight", *NewHeight);
}

static void
toggle_fullscreen (WIDGET_CHOICE *self, int *NewGfxFlags)
{
	if (self->selected == 1)
	{
		*NewGfxFlags &= ~TFB_GFXFLAGS_FULLSCREEN;
		*NewGfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
	}
	else if (self->selected == 2)
	{
		*NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;
		*NewGfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
	}
	else
	{
		*NewGfxFlags &= ~TFB_GFXFLAGS_FULLSCREEN;
		*NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;
	}
	res_PutInteger ("config.fullscreen", self->selected);
}

static void
change_scaler (WIDGET_CHOICE *self, int OldVal, int *NewGfxFlags)
{
	*NewGfxFlags &= ~scalerList[OldVal].value;
	*NewGfxFlags |= scalerList[self->selected].value;
	res_PutString ("config.scaler", scalerList[self->selected].str);
}

static void
toggle_showfps (WIDGET_CHOICE *self, int *NewGfxFlags)
{
	if (self->selected == 1)
		*NewGfxFlags |= TFB_GFXFLAGS_SHOWFPS;
	else
		*NewGfxFlags &= ~TFB_GFXFLAGS_SHOWFPS;
	res_PutBoolean ("config.showfps", self->selected);
}

static void
change_gfxdriver (WIDGET_CHOICE *self, int *NewGfxDriver)
{
#ifdef HAVE_OPENGL
	*NewGfxDriver = (self->selected == OPTVAL_ALWAYS_GL ?
			TFB_GFXDRIVER_SDL_OPENGL : TFB_GFXDRIVER_SDL_PURE);
#else
	*NewGfxDriver = TFB_GFXDRIVER_SDL_PURE;
#endif
	if (GraphicsDriver != *NewGfxDriver)
	{
		res_PutBoolean ("config.alwaysgl", self->selected);
		res_PutBoolean ("config.usegl",
				*NewGfxDriver == TFB_GFXDRIVER_SDL_OPENGL);
	}
}

void
process_graphics_options (WIDGET_CHOICE *self, int OldVal)
{
	int NewGfxFlags = GfxFlags;
	int NewGfxDriver = GraphicsDriver;
	int NewWidth = WindowWidth;
	int NewHeight = WindowHeight;
	BOOLEAN isExclusive = FALSE;

	if (OldVal == self->selected)
		return;

	switch (self->choice_num)
	{
		case 1:
			change_gfxdriver (self, &NewGfxDriver);
			break;
		case 2:
			change_scaler (self, OldVal, &NewGfxFlags);
			break;
		case 3:
			toggle_scanlines (self, &NewGfxFlags);
			break;
		case 10:
			toggle_fullscreen (self, &NewGfxFlags);
			break;
		case 12:
			toggle_showfps (self, &NewGfxFlags);
			break;
		case 23:
			optKeepAspectRatio = self->selected;
			res_PutBoolean ("config.keepaspectratio", self->selected);
#if SDL_MAJOR_VERSION == 1
			return;
#else
			break;
#endif
		case 42:
			change_scaling (self, &NewWidth, &NewHeight);
			isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;
			break;
		default:
			return;
	}

	if (NewWidth != WindowWidth || NewHeight != WindowHeight)
	{
		WindowWidth = NewWidth;
		WindowHeight = NewHeight;

		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;
	}

	if (NewGfxFlags != GfxFlags)
		GfxFlags = NewGfxFlags;

	if (NewGfxDriver != GraphicsDriver)
		GraphicsDriver = NewGfxDriver;

	FlushInput ();
	TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
			WindowWidth, WindowHeight);

	if (isExclusive)
	{	// needed twice to reinitialize Exclusive Full Screen after a 
		// resolution change 
		GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				WindowWidth, WindowHeight);
	}

	populate_res ();
}

static BOOLEAN
res_check (int width, int height)
{
	if (width % 320)
		return FALSE;

	if (height % DOS_BOOL (240, 200))
		return FALSE;

	if (width > 1920 || height > 1440)
		return FALSE;

	return TRUE;
}

static void
change_res (WIDGET_TEXTENTRY *self)
{
	int NewWidth = WindowWidth;
	int NewHeight = WindowHeight;
	int NewGfxFlags = GfxFlags;
	BOOLEAN isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;

	if (sscanf (self->value, "%dx%d", &NewWidth, &NewHeight) != 2)
	{
		populate_res ();
		return;
	}
	
	if (NewWidth != WindowWidth || NewHeight != WindowHeight)
	{
		WindowWidth = NewWidth;
		WindowHeight = NewHeight;

		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;
	}
	else
		return;

	if (NewGfxFlags != GfxFlags)
		GfxFlags = NewGfxFlags;

	FlushInput ();

	TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
		WindowWidth, WindowHeight);

	if (isExclusive)
	{	// needed twice to reinitialize Exclusive Full Screen after a 
		// resolution change
		GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				WindowWidth, WindowHeight);
	}

	if (res_check (NewWidth, NewHeight))
	{
		choices[CHOICE_RESOLUTION].selected = (NewWidth / 320) - 1;
	}
	else
		choices[CHOICE_RESOLUTION].selected = 6;

	PutIntOpt ((int *)(&loresBlowupScale), (int *)(&choices[CHOICE_RESOLUTION].selected),
			"config.loresBlowupScale", FALSE);
	res_PutInteger ("config.reswidth", NewWidth);
	res_PutInteger ("config.resheight", NewHeight);
}

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (ONE_SECOND / 20)

#define DEVICE_START 87
#define UPGRADE_START 112

static void
SetDefaults (void)
{
	GLOBALOPTS opts;
	BYTE i;
	
	GetGlobalOptions (&opts);
	choices[CHOICE_GRAPHICS        ].selected = opts.screenResolution;
	choices[CHOICE_FRBUFFER        ].selected = opts.driver;
	choices[CHOICE_SCALER          ].selected = opts.scaler;
	choices[CHOICE_SCANLINE        ].selected = opts.scanlines;
	choices[CHOICE_MENUSTYLE       ].selected = opts.menu;
	choices[CHOICE_FONTSTYLE       ].selected = opts.text;
	choices[CHOICE_SCANMENU        ].selected = opts.cscan;
	choices[CHOICE_SCROLLSTYLE     ].selected = opts.scroll;
	choices[CHOICE_SUBTITLES       ].selected = opts.subtitles;
	choices[CHOICE_REMIXES1        ].selected = opts.music3do;
	choices[CHOICE_DISPLAY         ].selected = opts.fullscreen;
	choices[CHOICE_CUTSCENE        ].selected = opts.intro;
	choices[CHOICE_SHOWFPS         ].selected = opts.fps;
#if !(defined(ANDROID) || defined(__ANDROID__))
	choices[CHOICE_MELEEZOOM       ].selected = opts.meleezoom;
#endif
	choices[CHOICE_POSAUDIO        ].selected = opts.stereo;
	choices[CHOICE_SNDDRIVER       ].selected = opts.adriver;
	choices[CHOICE_SNDQUALITY      ].selected = opts.aquality;
	choices[CHOICE_SLVSHIELD       ].selected = opts.shield;
	choices[CHOICE_BTMPLAYER       ].selected = opts.player1;
	choices[CHOICE_TOPPLAYER       ].selected = opts.player2;
	choices[CHOICE_KBLAYOUT        ].selected = 0;
	choices[CHOICE_REMIXES2        ].selected = opts.musicremix;
	choices[CHOICE_SPEECH          ].selected = opts.speech;
	choices[CHOICE_ASPRATIO        ].selected = opts.keepaspect;
 	choices[CHOICE_CHEATING        ].selected = opts.cheatMode;
	choices[CHOICE_CHGODMODE       ].selected = opts.godModes;
	choices[CHOICE_CHTIME          ].selected = opts.tdType;
	choices[CHOICE_CHWARP          ].selected = opts.bubbleWarp;
	choices[CHOICE_CHSHIPS         ].selected = opts.unlockShips;
	choices[CHOICE_CHHEADSTART     ].selected = opts.headStart;
	//choices[CHOICE_CHUPGRADES    ].selected = opts.unlockUpgrades;
	choices[CHOICE_CHINFRU         ].selected = opts.infiniteRU;
	choices[CHOICE_SKIPINTRO       ].selected = opts.skipIntro;
	choices[CHOICE_FUELCIRCLE      ].selected = opts.fuelRange;
	choices[CHOICE_MMENUMUSIC      ].selected = opts.mainMenuMusic;
	choices[CHOICE_NEBULAE         ].selected = opts.nebulae;
	choices[CHOICE_ORBPLANETS      ].selected = opts.orbitingPlanets;
	choices[CHOICE_TEXPLANETS      ].selected = opts.texturedPlanets;
	choices[CHOICE_DATESTRING      ].selected = opts.dateType;
	choices[CHOICE_CHINFFUEL       ].selected = opts.infiniteFuel;
	choices[CHOICE_PARTPICKUP      ].selected = opts.partialPickup;
	choices[CHOICE_SUBMENU         ].selected = opts.submenu;
	choices[CHOICE_RESOLUTION      ].selected = opts.loresBlowup;
	choices[CHOICE_CHINFCRD        ].selected = opts.infiniteCredits;
	choices[CHOICE_HAZARDCLR       ].selected = opts.hazardColors;
	choices[CHOICE_CUSTBORDER      ].selected = opts.customBorder;
	choices[CHOICE_IPMUSIC         ].selected = opts.spaceMusic;
	choices[CHOICE_REMIXES3        ].selected = opts.volasMusic;
	choices[CHOICE_FUELDECIM       ].selected = opts.wholeFuel;
#if defined(ANDROID) || defined(__ANDROID__)
	choices[CHOICE_JOYSTICK        ].selected = opts.directionalJoystick;
	choices[CHOICE_ANDRZOOM        ].selected = opts.meleezoom;
#endif
	choices[CHOICE_LANDERHOLD      ].selected = opts.landerHold;
	choices[CHOICE_SCRMELT         ].selected = opts.scrTrans;
	choices[CHOICE_SKILLLVL        ].selected = opts.difficulty;
	choices[CHOICE_EXTENDED        ].selected = opts.extended;
	choices[CHOICE_NOMAD           ].selected = opts.nomad;
	choices[CHOICE_GAMEOVER        ].selected = opts.gameOver;
	choices[CHOICE_IPSHIPDIR       ].selected = opts.shipDirectionIP;
	choices[CHOICE_ORZFONT         ].selected = opts.orzCompFont;
	choices[CHOICE_INPDEVICE       ].selected = opts.controllerType;
	choices[CHOICE_SMARTAUTO       ].selected = opts.smartAutoPilot;
	choices[CHOICE_SCANTINT        ].selected = opts.tintPlanSphere;
	choices[CHOICE_IPSTYLE         ].selected = opts.planetStyle;
	choices[CHOICE_IPBACKGROUND    ].selected = opts.starBackground;
	choices[CHOICE_SCANSTYLE       ].selected = opts.scanStyle;
	choices[CHOICE_NOSTOSCILL      ].selected = opts.nonStopOscill;
	choices[CHOICE_OSCILLSTYLE     ].selected = opts.scopeStyle;
	choices[CHOICE_ANIMHYPER       ].selected = opts.hyperStars;
	choices[CHOICE_LANDERSTYLE     ].selected = opts.landerStyle;
	choices[CHOICE_PLNTEXTURE      ].selected = opts.planetTexture;
	choices[CHOICE_FLAGSHIP        ].selected = opts.flagshipColor;
	choices[CHOICE_CHCLEANHYPER    ].selected = opts.noHQEncounters;
	choices[CHOICE_CHDECLEAN       ].selected = opts.deCleansing;
	choices[CHOICE_CHNOPLANET      ].selected = opts.meleeObstacles;
	choices[CHOICE_VISITED         ].selected = opts.showVisitedStars;
	choices[CHOICE_6014IP          ].selected = opts.unscaledStarSystem;
	choices[CHOICE_SCANSPHERE      ].selected = opts.sphereType;
	choices[CHOICE_SLAUGHTER       ].selected = opts.slaughterMode;
	choices[CHOICE_ADVAUTO         ].selected = opts.advancedAutoPilot;
	choices[CHOICE_MLTOOLTIP       ].selected = opts.meleeToolTips;
	choices[CHOICE_MUSRESUME       ].selected = opts.musicResume;
	choices[CHOICE_WINDOWTYPE      ].selected = opts.windowType;
	choices[CHOICE_GAMESEED        ].selected = opts.seedType;
	choices[CHOICE_SOICOLOR        ].selected = opts.sphereColors;
	choices[CHOICE_SCATTERCARGO    ].selected = opts.scatterElements;
	choices[CHOICE_LANDERUPGMASK   ].selected = opts.showUpgrades;
	choices[CHOICE_FLEETPOINT      ].selected = opts.fleetPointSys;

	// Devices
	for (i = DEVICE_START; i < DEVICE_START + NUM_DEVICES; i++)
	{
		choices[i].selected = opts.deviceArray[i - DEVICE_START];
	}

	for (i = UPGRADE_START; i < UPGRADE_START + NUM_UPGRADES; i++)
	{
		choices[i].selected = opts.upgradeArray[i - UPGRADE_START];
	}

	// Next choice should be choices[125]

	sliders[SLIDER_MUSVOLUME ].value = opts.musicvol;
	sliders[SLIDER_SFXVOLUME ].value = opts.sfxvol;
	sliders[SLIDER_SPCHVOLUME].value = opts.speechvol;
	sliders[SLIDER_GAMMA     ].value = opts.gamma;
	sliders[SLIDER_NEBULA    ].value = opts.nebulaevol;
}

static void
PropagateResults (void)
{
	GLOBALOPTS opts;
	BYTE i;

	opts.screenResolution = choices[CHOICE_GRAPHICS     ].selected;
	opts.driver =			choices[CHOICE_FRBUFFER     ].selected;
	opts.scaler =			choices[CHOICE_SCALER       ].selected;
	opts.scanlines =		choices[CHOICE_SCANLINE     ].selected;
	opts.menu =				choices[CHOICE_MENUSTYLE    ].selected;
	opts.text =				choices[CHOICE_FONTSTYLE    ].selected;
	opts.cscan =			choices[CHOICE_SCANMENU     ].selected;
	opts.scroll =			choices[CHOICE_SCROLLSTYLE  ].selected;
	opts.subtitles =		choices[CHOICE_SUBTITLES    ].selected;
	opts.music3do =			choices[CHOICE_REMIXES1     ].selected;
	opts.fullscreen =		choices[CHOICE_DISPLAY      ].selected;
	opts.intro =			choices[CHOICE_CUTSCENE     ].selected;
	opts.fps =				choices[CHOICE_SHOWFPS      ].selected;
#if !(defined(ANDROID) || defined(__ANDROID__))
	opts.meleezoom =		choices[CHOICE_MELEEZOOM    ].selected;
#endif
	opts.stereo =			choices[CHOICE_POSAUDIO     ].selected;
	opts.adriver =			choices[CHOICE_SNDDRIVER    ].selected;
	opts.aquality =			choices[CHOICE_SNDQUALITY   ].selected;
	opts.shield =			choices[CHOICE_SLVSHIELD    ].selected;
	opts.player1 =			choices[CHOICE_BTMPLAYER    ].selected;
	opts.player2 =			choices[CHOICE_TOPPLAYER    ].selected;
	opts.musicremix =		choices[CHOICE_REMIXES2     ].selected;
	opts.speech =			choices[CHOICE_SPEECH       ].selected;
	opts.keepaspect =		choices[CHOICE_ASPRATIO     ].selected;
 	opts.cheatMode =		choices[CHOICE_CHEATING     ].selected;
	opts.godModes =			choices[CHOICE_CHGODMODE    ].selected;
	opts.tdType =			choices[CHOICE_CHTIME       ].selected;
	opts.bubbleWarp =		choices[CHOICE_CHWARP       ].selected;
	opts.unlockShips =		choices[CHOICE_CHSHIPS      ].selected;
	opts.headStart =		choices[CHOICE_CHHEADSTART  ].selected;
	//opts.unlockUpgrades = choices[CHOICE_CHUPGRADES   ].selected;
	opts.infiniteRU =		choices[CHOICE_CHINFRU      ].selected;
	opts.skipIntro =		choices[CHOICE_SKIPINTRO    ].selected;
	opts.fuelRange =		choices[CHOICE_FUELCIRCLE   ].selected;
	opts.mainMenuMusic =	choices[CHOICE_MMENUMUSIC   ].selected;
	opts.nebulae =			choices[CHOICE_NEBULAE      ].selected;
	opts.orbitingPlanets =	choices[CHOICE_ORBPLANETS   ].selected;
	opts.texturedPlanets =	choices[CHOICE_TEXPLANETS   ].selected;
	opts.dateType =			choices[CHOICE_DATESTRING   ].selected;
	opts.infiniteFuel =		choices[CHOICE_CHINFFUEL    ].selected;
	opts.partialPickup =	choices[CHOICE_PARTPICKUP   ].selected;
	opts.submenu =			choices[CHOICE_SUBMENU      ].selected;
	opts.loresBlowup =		choices[CHOICE_RESOLUTION   ].selected;
	opts.infiniteCredits =	choices[CHOICE_CHINFCRD     ].selected;
	opts.hazardColors =		choices[CHOICE_HAZARDCLR    ].selected;
	opts.customBorder =		choices[CHOICE_CUSTBORDER   ].selected;
	opts.spaceMusic =		choices[CHOICE_IPMUSIC      ].selected;
	opts.volasMusic =		choices[CHOICE_REMIXES3     ].selected;
	opts.wholeFuel =		choices[CHOICE_FUELDECIM    ].selected;
#if defined(ANDROID) || defined(__ANDROID__)
	opts.directionalJoystick = choices[CHOICE_JOYSTICK  ].selected;
	opts.meleezoom =		choices[CHOICE_ANDRZOOM     ].selected;
#endif
	opts.landerHold =		choices[CHOICE_LANDERHOLD   ].selected;
	opts.scrTrans =			choices[CHOICE_SCRMELT      ].selected;
	opts.difficulty =		choices[CHOICE_SKILLLVL     ].selected;
	opts.extended =			choices[CHOICE_EXTENDED     ].selected;
	opts.nomad =			choices[CHOICE_NOMAD        ].selected;
	opts.gameOver =			choices[CHOICE_GAMEOVER     ].selected;
	opts.shipDirectionIP =	choices[CHOICE_IPSHIPDIR    ].selected;
	opts.orzCompFont =		choices[CHOICE_ORZFONT      ].selected;
	opts.controllerType =	choices[CHOICE_INPDEVICE    ].selected;
	opts.smartAutoPilot =	choices[CHOICE_SMARTAUTO    ].selected;
	opts.tintPlanSphere =	choices[CHOICE_SCANTINT     ].selected;
	opts.planetStyle =		choices[CHOICE_IPSTYLE      ].selected;
	opts.starBackground =	choices[CHOICE_IPBACKGROUND ].selected;
	opts.scanStyle =		choices[CHOICE_SCANSTYLE    ].selected;
	opts.nonStopOscill =	choices[CHOICE_NOSTOSCILL   ].selected;
	opts.scopeStyle =		choices[CHOICE_OSCILLSTYLE  ].selected;
	opts.hyperStars =		choices[CHOICE_ANIMHYPER    ].selected;
	opts.landerStyle =		choices[CHOICE_LANDERSTYLE  ].selected;
	opts.planetTexture =	choices[CHOICE_PLNTEXTURE   ].selected;
	opts.flagshipColor =	choices[CHOICE_FLAGSHIP     ].selected;
	opts.noHQEncounters =	choices[CHOICE_CHCLEANHYPER ].selected;
	opts.deCleansing =		choices[CHOICE_CHDECLEAN    ].selected;
	opts.meleeObstacles =	choices[CHOICE_CHNOPLANET   ].selected;
	opts.showVisitedStars = choices[CHOICE_VISITED      ].selected;
	opts.unscaledStarSystem=choices[CHOICE_6014IP       ].selected;
	opts.sphereType =		choices[CHOICE_SCANSPHERE   ].selected;
	opts.slaughterMode =	choices[CHOICE_SLAUGHTER    ].selected;
	opts.advancedAutoPilot= choices[CHOICE_ADVAUTO      ].selected;
	opts.meleeToolTips =	choices[CHOICE_MLTOOLTIP    ].selected;
	opts.musicResume =		choices[CHOICE_MUSRESUME    ].selected;
	opts.windowType =		choices[CHOICE_WINDOWTYPE   ].selected;
	opts.seedType =			choices[CHOICE_GAMESEED     ].selected;
	opts.sphereColors =		choices[CHOICE_SOICOLOR     ].selected;
	opts.scatterElements =	choices[CHOICE_SCATTERCARGO ].selected;
	opts.showUpgrades =		choices[CHOICE_LANDERUPGMASK].selected;
	opts.fleetPointSys =	choices[CHOICE_FLEETPOINT   ].selected;

	// Devices
	for (i = DEVICE_START; i < DEVICE_START + NUM_DEVICES; i++)
	{
		opts.deviceArray[i - DEVICE_START] = choices[i].selected;
	}

	for (i = UPGRADE_START; i < UPGRADE_START + NUM_UPGRADES; i++)
	{
		opts.upgradeArray[i - UPGRADE_START] = choices[i].selected;
	}

	opts.musicvol	= sliders[SLIDER_MUSVOLUME ].value;
	opts.sfxvol		= sliders[SLIDER_SFXVOLUME ].value;
	opts.speechvol	= sliders[SLIDER_SPCHVOLUME].value;
	opts.gamma		= sliders[SLIDER_GAMMA     ].value;
	opts.nebulaevol = sliders[SLIDER_NEBULA    ].value;
	SetGlobalOptions (&opts);
}

static BOOLEAN
DoSetupMenu (SETUP_MENU_STATE *pInputState)
{
	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (!pInputState->initialized) 
	{
		SetDefaultMenuRepeatDelay ();
		pInputState->NextTime = GetTimeCounter ();
		SetDefaults ();
		Widget_SetFont (PlyrFont);
		Widget_SetWindowColors (SHADOWBOX_BACKGROUND_COLOR,
				SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

		current = NULL;
		next = (WIDGET *)(&menus[0]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		
		pInputState->initialized = TRUE;
	}
	if (current != next)
	{
		SetTransitionSource (NULL);
	}
	
	BatchGraphics ();
	(*next->draw)(next, 0, 0);

	if (current != next)
	{
		ScreenTransition (3, NULL);
		current = next;
	}

	UnbatchGraphics ();

	if (PulsedInputState.menu[KEY_MENU_UP])
	{
		Widget_Event (WIDGET_EVENT_UP);
	}
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
	{
		Widget_Event (WIDGET_EVENT_DOWN);
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		Widget_Event (WIDGET_EVENT_LEFT);
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		Widget_Event (WIDGET_EVENT_RIGHT);
	}
	if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		Widget_Event (WIDGET_EVENT_SELECT);
	}
	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		Widget_Event (WIDGET_EVENT_CANCEL);
	}
	if (PulsedInputState.menu[KEY_MENU_DELETE])
	{
		Widget_Event (WIDGET_EVENT_DELETE);
	}

	SleepThreadUntil (pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter ();
	return !((GLOBAL (CurrentActivity) & CHECK_ABORT) || 
		 (next == NULL));
}

static void
redraw_menu (void)
{
	BatchGraphics ();
	(*next->draw)(next, 0, 0);
	UnbatchGraphics ();
}

static BOOLEAN
OnTextEntryChange (TEXTENTRY_STATE *pTES)
{
	WIDGET_TEXTENTRY *widget = (WIDGET_TEXTENTRY *) pTES->CbParam;

	widget->cursor_pos = pTES->CursorPos;
	if (pTES->JoystickMode)
		widget->state |= WTE_BLOCKCUR;
	else
		widget->state &= ~WTE_BLOCKCUR;
	
	// XXX TODO: Here, we can examine the text entered so far
	// to make sure it fits on the screen, for example,
	// and return FALSE to disallow the last change
	
	return TRUE; // allow change
}

static BOOLEAN
OnTextEntryFrame (TEXTENTRY_STATE *pTES)
{
	redraw_menu ();

	SleepThreadUntil (pTES->NextTime);
	pTES->NextTime = GetTimeCounter () + MENU_FRAME_RATE;

	return TRUE; // continue
}

static int
OnTextEntryEvent (WIDGET_TEXTENTRY *widget)
{	// Going to edit the text
	TEXTENTRY_STATE tes;
	UNICODE revert_buf[256];

	// position cursor at the end of text
	widget->cursor_pos = utf8StringCount (widget->value);
	widget->state = WTE_EDITING;
	redraw_menu ();

	// make a backup copy for revert on cancel
	utf8StringCopy (revert_buf, sizeof (revert_buf), widget->value);

	// text entry setup
	tes.Initialized = FALSE;
	tes.NextTime = GetTimeCounter () + MENU_FRAME_RATE;
	tes.BaseStr = widget->value;
	tes.MaxSize = widget->maxlen;
	tes.CursorPos = widget->cursor_pos;
	tes.CbParam = widget;
	tes.ChangeCallback = OnTextEntryChange;
	tes.FrameCallback = OnTextEntryFrame;

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_SELECT);
	if (!DoTextEntry (&tes))
	{	// editing failed (canceled) -- revert the changes
		utf8StringCopy (widget->value, widget->maxlen, revert_buf);
	}
	else
	{
		if (widget->onChange)
		{
			(*(widget->onChange))(widget);
		}
	}
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	widget->state = WTE_NORMAL;
	redraw_menu ();

	return TRUE; // event handled
}

static inline float
gammaCurve (float x)
{
	// The slider uses an exponential curve
	return exp ((x - 1) * GAMMA_CURVE_B);
}

static inline float
solveGammaCurve (float y)
{
	return log (y) / GAMMA_CURVE_B + 1;
}

static int
gammaToSlider (float gamma)
{
	const float x = solveGammaCurve (gamma);
	const float step = (maxGammaX - minGammaX) / 100;
	return (int) ((x - minGammaX) / step + 0.5);
}

static float
sliderToGamma (int value)
{
	const float step = (maxGammaX - minGammaX) / 100;
	const float x = minGammaX + step * value;
	const float g = gammaCurve (x);
	// report any value that is close enough as 1.0
	return (fabs (g - 1.0f) < 0.001f) ? 1.0f : g;
}

static void
updateGammaBounds (bool useUpper)
{
	float g, x;
	int slider;
	
	// The slider uses an exponential curve.
	// Calculate where on the curve the min and max gamma values are
	minGammaX = solveGammaCurve (minGamma);
	maxGammaX = solveGammaCurve (maxGamma);

	// We have 100 discrete steps through the range, so the slider may
	// skip over a 1.0 gamma. We need to ensure that there always is
	// a 1.0 on the slider by tweaking the range (expanding/contracting).
	slider = gammaToSlider (1.0f);
	g = sliderToGamma (slider);
	if (g == 1.0f)
		return; // no adjustment needed

	x = solveGammaCurve (g);
	if (useUpper)
	{	// Move the upper bound up or down to land on 1.0
		const float d = (x - 1.0f) * 100 / slider;
		maxGammaX -= d;
		maxGamma = gammaCurve (maxGammaX);
	}
	else
	{	// Move the lower bound up or down to land on 1.0
		const float d = (x - 1.0f) * 100 / (100 - slider);
		minGammaX -= d;
		minGamma = gammaCurve (minGammaX);
	}
}

static int
gamma_HandleEventSlider (WIDGET *_self, int event)
{
	WIDGET_SLIDER *self = (WIDGET_SLIDER *)_self;
	int prevValue = self->value;
	float gamma;
	bool set;

	switch (event)
	{
	case WIDGET_EVENT_LEFT:
		self->value -= self->step;
		break;
	case WIDGET_EVENT_RIGHT:
		self->value += self->step;
		break;
	default:
		return FALSE;
	}

	// Limit the slider to values accepted by gfx subsys
	gamma = sliderToGamma (self->value);
	set = TFB_SetGamma (gamma);
	if (!set)
	{	// revert
		self->value = prevValue;
		gamma = sliderToGamma (self->value);
	}

	// Grow or shrink the range based on accepted values
	if (gamma < minGamma || (!set && event == WIDGET_EVENT_LEFT))
	{
		gamma = minGamma;
		updateGammaBounds (true);
		// at the lowest end
		self->value = 0;
	}
	else if (gamma > maxGamma || (!set && event == WIDGET_EVENT_RIGHT))
	{
		gamma = maxGamma;
		updateGammaBounds (false);
		// at the highest end
		self->value = 100;
	}
	return TRUE;
}

static void
gamma_DrawValue (WIDGET_SLIDER *self, int x, int y)
{
	TEXT t;
	char buf[16];
	float gamma = sliderToGamma (self->value);
	snprintf (buf, sizeof buf, "%.4f", gamma);

	t.baseline.x = x + RES_SCALE(6);
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = buf;

	font_DrawText (&t);
}

static void
rebind_control (WIDGET_CONTROLENTRY *widget)
{
	int templat = choices[CHOICE_KBLAYOUT].selected;
	int control = widget->controlindex;
	int index = widget->highlighted;

	FlushInput ();
	DrawLabelAsWindow (&labels[LABEL_PRESSTOEDIT], NULL);
	RebindInputState (templat, control, index);
	populate_editkeys (templat);
	FlushInput ();
}

static void
clear_control (WIDGET_CONTROLENTRY *widget)
{
	int templat = choices[CHOICE_KBLAYOUT].selected;
	int control = widget->controlindex;
	int index = widget->highlighted;

	RemoveInputState (templat, control, index);
	populate_editkeys (templat);
}	

static int
count_widgets (WIDGET **widgets)
{
	int count;

	for (count = 0; *widgets != NULL; ++widgets, ++count)
		;
	return count;
}

static stringbank *bank = NULL;
static FRAME setup_frame = NULL;

#define MAX_BUFF (MENU_COUNT + CHOICE_COUNT + \
				SLIDER_COUNT + BUTTON_COUNT + \
				LABEL_COUNT + TEXTENTRY_COUNT + \
				CONTROLENTRY_COUNT)

static void
init_widgets (void)
{
	const char *buffer[MAX_BUFF], *str, *title;
	int count, i, index;

	if (bank == NULL)
	{
		bank = StringBank_Create ();
	}
	
	if (setup_frame == NULL || optRequiresReload)
	{
		// Load the different menus depending on the resolution factor.
		setup_frame = CaptureDrawable (LoadGraphic (MENUBKG_PMAP_ANIM));
		LoadArrows ();
	}

	count = GetStringTableCount (SetupTab);

	if (count < 3)
	{
		log_add (log_Fatal, "PANIC: Setup string table too short to even "
				"hold all indices!");
		exit (EXIT_FAILURE);
	}

	/* Menus */
	title = StringBank_AddOrFindString (bank,
			GetStringAddress (SetAbsStringTableIndex (SetupTab, 0)));
	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, 1)), '\n', MAX_BUFF, buffer, bank) != MENU_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Fatal, "PANIC: Incorrect number of Menu Subtitles");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < MENU_COUNT; i++)
	{
		menus[i].tag = WIDGET_TYPE_MENU_SCREEN;
		menus[i].parent = NULL;
		menus[i].handleEvent = Widget_HandleEventMenuScreen;
		menus[i].receiveFocus = Widget_ReceiveFocusMenuScreen;
		menus[i].draw = Widget_DrawMenuScreen;
		menus[i].height = Widget_HeightFullScreen;
		menus[i].width = Widget_WidthFullScreen;
		menus[i].title = title;
		menus[i].subtitle = buffer[i];
		menus[i].bgStamp.origin.x = 0;
		menus[i].bgStamp.origin.y = 0;
		menus[i].bgStamp.frame =
				SetAbsFrameIndex (setup_frame, menu_defs[i].bgIndex);
		menus[i].num_children = count_widgets (menu_defs[i].widgets);
		menus[i].child = menu_defs[i].widgets;
		menus[i].highlighted = 0;
	}
	if (menu_defs[i].widgets != NULL)
	{
		log_add (log_Error, "Menu definition array has more items!");
	}
		
	/* Options */
	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, 2)), '\n', MAX_BUFF, buffer, bank) != CHOICE_COUNT)
	{
		log_add (log_Fatal, "PANIC: Incorrect number of Choice Options: "
				"%d. Should be %d", CHOICE_COUNT,
				SplitString (GetStringAddress (
					SetAbsStringTableIndex (SetupTab, 2)),
					'\n', MAX_BUFF, buffer, bank));
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < CHOICE_COUNT; i++)
	{
		choices[i].tag = WIDGET_TYPE_CHOICE;
		choices[i].parent = NULL;
		choices[i].handleEvent = Widget_HandleEventChoice;
		choices[i].receiveFocus = Widget_ReceiveFocusChoice;
		choices[i].draw = Widget_DrawChoice;
		choices[i].height = Widget_HeightChoice;
		choices[i].width = Widget_WidthFullScreen;
		choices[i].category = buffer[i];
		choices[i].numopts = 0;
		choices[i].options = NULL;
		choices[i].selected = 0;
		choices[i].highlighted = 0;
		choices[i].onChange = NULL;
	}

	/* Fill in the options now */
	index = 3;  /* Index into string table */
	for (i = 0; i < CHOICE_COUNT; i++)
	{
		int j, optcount;

		if (index >= count)
		{
			log_add (log_Fatal, "PANIC: String table cut short while "
					"reading choices");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (
				SetAbsStringTableIndex (SetupTab, index++));
		optcount = SplitString (str, '\n', MAX_BUFF, buffer, bank);
		choices[i].numopts = optcount;
		choices[i].options = HMalloc (optcount * sizeof (CHOICE_OPTION));
		choices[i].choice_num = i;
		for (j = 0; j < optcount; j++)
		{
			choices[i].options[j].optname = buffer[j];
			choices[i].options[j].tooltip[0] = "";
			choices[i].options[j].tooltip[1] = "";
			choices[i].options[j].tooltip[2] = "";
		}
		for (j = 0; j < optcount; j++)
		{
			int k, tipcount;

			if (index >= count)
			{
				log_add (log_Fatal, "PANIC: String table cut short while "
						"reading choices");
				exit (EXIT_FAILURE);
			}
			str = GetStringAddress (
					SetAbsStringTableIndex (SetupTab, index++));
			tipcount = SplitString (str, '\n', MAX_BUFF, buffer, bank);
			if (tipcount > 3)
			{
				tipcount = 3;
			}
			for (k = 0; k < tipcount; k++)
			{
				choices[i].options[j].tooltip[k] = buffer[k];
			}
		}
	}

	for (i = 0; i < choices[CHOICE_RESOLUTION].numopts - 1; i++)
	{
		snprintf(choices[CHOICE_RESOLUTION].options[i].optname,
			strlen(choices[CHOICE_RESOLUTION].options[i].optname),
			"%dx%d", RES_DESCALE (CanvasWidth)*(i+1), 
			RES_DESCALE (CanvasHeight)* (i + 1));
	}

	// Choices 18-20 are also special, being the names of the key
	// configurations
	for (i = 0; i < 6; i++)
	{
		choices[CHOICE_BTMPLAYER].options[i].optname = input_templates[i].name;
		choices[CHOICE_TOPPLAYER].options[i].optname = input_templates[i].name;
		choices[CHOICE_KBLAYOUT ].options[i].optname = input_templates[i].name;
	}

	/* Choice 20 has a special onChange handler, too. */
	choices[CHOICE_KBLAYOUT].onChange = change_template;

	// Check addon availability for HD mode, DOS/3DO mode, and music remixes
	choices[CHOICE_GRAPHICS  ].onChange = check_availability;
	choices[CHOICE_REMIXES1  ].onChange = check_availability;
	choices[CHOICE_REMIXES2  ].onChange = check_availability;
	choices[CHOICE_IPMUSIC   ].onChange = check_availability;
	choices[CHOICE_REMIXES3  ].onChange = check_availability;
	choices[CHOICE_WINDOWTYPE].onChange = check_availability;

	// Handle display option
	choices[CHOICE_FRBUFFER  ].onChange = process_graphics_options;
	choices[CHOICE_SCALER    ].onChange = process_graphics_options;
	choices[CHOICE_SCANLINE  ].onChange = process_graphics_options;
	choices[CHOICE_DISPLAY   ].onChange = process_graphics_options;
	choices[CHOICE_SHOWFPS   ].onChange = process_graphics_options;
	choices[CHOICE_ASPRATIO  ].onChange = process_graphics_options;
	choices[CHOICE_RESOLUTION].onChange = process_graphics_options;

	/* Sliders */
	if (index >= count)
	{
		log_add (log_Fatal,
				"PANIC: String table cut short while reading sliders");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, index++)), '\n', MAX_BUFF, buffer, bank) != SLIDER_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Fatal, "PANIC: Incorrect number of Slider Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < SLIDER_COUNT; i++)
	{
		sliders[i].tag = WIDGET_TYPE_SLIDER;
		sliders[i].parent = NULL;
		sliders[i].handleEvent = Widget_HandleEventSlider;
		sliders[i].receiveFocus = Widget_ReceiveFocusSimple;
		sliders[i].draw = Widget_DrawSlider;
		sliders[i].height = Widget_HeightOneLine;
		sliders[i].width = Widget_WidthFullScreen;
		sliders[i].draw_value = Widget_Slider_DrawValue;
		sliders[i].min = 0;
		sliders[i].max = 100;
		sliders[i].step = 5;
		sliders[i].value = 75;
		sliders[i].category = buffer[i];
		sliders[i].tooltip[0] = "";
		sliders[i].tooltip[1] = "";
		sliders[i].tooltip[2] = "";
		sliders[i].onChange = NULL;
	}
	sliders[SLIDER_MUSVOLUME ].onChange = adjustMusic;
	sliders[SLIDER_SFXVOLUME ].onChange = adjustSFX;
	sliders[SLIDER_SPCHVOLUME].onChange = adjustSpeech;

	// gamma is a special case
	sliders[SLIDER_GAMMA].step = 1;
	sliders[SLIDER_GAMMA].handleEvent = gamma_HandleEventSlider;
	sliders[SLIDER_GAMMA].draw_value = gamma_DrawValue;

	// nebulaevol is a special case
	sliders[SLIDER_NEBULA].step = 1;
	sliders[SLIDER_NEBULA].max = 50;

	for (i = 0; i < SLIDER_COUNT; i++)
	{
		int j, tipcount;
		
		if (index >= count)
		{
			log_add (log_Fatal,
					"PANIC: String table cut short while reading sliders");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (
				SetAbsStringTableIndex (SetupTab, index++));
		tipcount = SplitString (str, '\n', MAX_BUFF, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			sliders[i].tooltip[j] = buffer[j];
		}
	}

	/* Buttons */
	if (index >= count)
	{
		log_add (log_Fatal,
				"PANIC: String table cut short while reading buttons");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, index++)), '\n', MAX_BUFF, buffer, bank) != BUTTON_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Fatal, "PANIC: Incorrect number of Button Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < BUTTON_COUNT; i++)
	{
		buttons[i].tag = WIDGET_TYPE_BUTTON;
		buttons[i].parent = NULL;
		buttons[i].handleEvent = button_handlers[i];
		buttons[i].receiveFocus = Widget_ReceiveFocusSimple;
		buttons[i].draw = Widget_DrawButton;
		buttons[i].height = Widget_HeightOneLine;
		buttons[i].width = Widget_WidthFullScreen;
		buttons[i].name = buffer[i];
		buttons[i].tooltip[0] = "";
		buttons[i].tooltip[1] = "";
		buttons[i].tooltip[2] = "";
	}

	for (i = 0; i < BUTTON_COUNT; i++)
	{
		int j, tipcount;
		
		if (index >= count)
		{
			log_add (log_Fatal,
					"PANIC: String table cut short while reading buttons");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (
				SetAbsStringTableIndex (SetupTab, index++));
		tipcount = SplitString (str, '\n', MAX_BUFF, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			buttons[i].tooltip[j] = buffer[j];
		}
	}

	/* Labels */
	if (index >= count)
	{
		log_add (log_Fatal,
				"PANIC: String table cut short while reading labels");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, index++)), '\n', MAX_BUFF, buffer, bank) != LABEL_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Fatal, "PANIC: Incorrect number of Label Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		labels[i].tag = WIDGET_TYPE_LABEL;
		labels[i].parent = NULL;
		labels[i].handleEvent = Widget_HandleEventIgnoreAll;
		labels[i].receiveFocus = Widget_ReceiveFocusRefuseFocus;
		labels[i].draw = Widget_DrawLabel;
		labels[i].height = Widget_HeightLabel;
		labels[i].width = Widget_WidthFullScreen;
		labels[i].line_count = 0;
		labels[i].lines = NULL;
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		int j, linecount;
		
		if (index >= count)
		{
			log_add (log_Fatal, "PANIC: String table cut short while "
					"reading labels");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (
				SetAbsStringTableIndex (SetupTab, index++));
		linecount = SplitString (str, '\n', MAX_BUFF, buffer, bank);
		labels[i].line_count = linecount;
		labels[i].lines =
				(const char **)HMalloc(linecount * sizeof(const char *));
		for (j = 0; j < linecount; j++)
		{
			labels[i].lines[j] = buffer[j];
		}
	}

	/* Text Entry boxes */
	if (index >= count)
	{
		log_add (log_Fatal, "PANIC: String table cut short while reading "
				"text entries");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, index++)), '\n', MAX_BUFF, buffer, bank)
			!= TEXTENTRY_COUNT)
	{
		log_add (log_Fatal, "PANIC: Incorrect number of Text Entries");
		exit (EXIT_FAILURE);
	}
	for (i = 0; i < TEXTENTRY_COUNT; i++)
	{
		textentries[i].tag = WIDGET_TYPE_TEXTENTRY;
		textentries[i].parent = NULL;
		textentries[i].handleEvent = Widget_HandleEventTextEntry;
		textentries[i].receiveFocus = Widget_ReceiveFocusSimple;
		textentries[i].draw = Widget_DrawTextEntry;
		textentries[i].height = Widget_HeightOneLine;
		textentries[i].width = Widget_WidthFullScreen;
		textentries[i].handleEventSelect = OnTextEntryEvent;
		textentries[i].category = buffer[i];
		textentries[i].value[0] = 0;
		textentries[i].maxlen = WIDGET_TEXTENTRY_WIDTH-1;
		textentries[i].state = WTE_NORMAL;
		textentries[i].cursor_pos = 0;
		textentries[i].tooltip[0] = "";
		textentries[i].tooltip[1] = "";
		textentries[i].tooltip[2] = "";
	}
	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, index++)), '\n', MAX_BUFF, buffer, bank)
			!= TEXTENTRY_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Fatal, "PANIC: Incorrect number of Text Entries");
		exit (EXIT_FAILURE);
	}
	for (i = 0; i < TEXTENTRY_COUNT; i++)
	{
		int j, tipcount;

		strncpy (textentries[i].value, buffer[i], textentries[i].maxlen);
		textentries[i].value[textentries[i].maxlen] = 0;

		if (index >= count)
		{
			log_add(log_Fatal, "PANIC: String table cut short while "
					"reading text entries");
			exit(EXIT_FAILURE);
		}
		str = GetStringAddress(SetAbsStringTableIndex(SetupTab, index++));
		tipcount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			textentries[i].tooltip[j] = buffer[j];
		}
	}

	textentries[TEXT_LOUTNAME].onChange = rename_template;
	textentries[TEXT_GAMESEED].onChange = change_seed;
	textentries[TEXT_CUSTMRES].onChange = change_res;

	/* Control Entry boxes */
	if (index >= count)
	{
		log_add (log_Fatal, "PANIC: String table cut short while reading "
				"control entries");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (
			SetupTab, index++)), '\n', MAX_BUFF, buffer, bank)
			!= CONTROLENTRY_COUNT)
	{
		log_add (log_Fatal, "PANIC: Incorrect number of Control Entries");
		exit (EXIT_FAILURE);
	}
	for (i = 0; i < CONTROLENTRY_COUNT; i++)
	{
		controlentries[i].tag = WIDGET_TYPE_CONTROLENTRY;
		controlentries[i].parent = NULL;
		controlentries[i].handleEvent = Widget_HandleEventControlEntry;
		controlentries[i].receiveFocus = Widget_ReceiveFocusControlEntry;
		controlentries[i].draw = Widget_DrawControlEntry;
		controlentries[i].height = Widget_HeightOneLine;
		controlentries[i].width = Widget_WidthFullScreen;
		controlentries[i].category = buffer[i];
		controlentries[i].highlighted = 0;
		controlentries[i].controlname[0][0] = 0;
		controlentries[i].controlname[1][0] = 0;
		controlentries[i].controlindex = i;
		controlentries[i].onChange = rebind_control;
		controlentries[i].onDelete = clear_control;
	}

	/* Check for garbage at the end */
	if (index < count)
	{
		log_add (log_Warning, "WARNING: Setup strings had %d garbage "
				"entries at the end.", count - index);
	}

	testSounds = CaptureSound (LoadSound (TEST_SOUNDS));
}

static void
clean_up_widgets (void)
{
	int i;

	for (i = 0; i < CHOICE_COUNT; i++)
	{
		if (choices[i].options)
		{
			HFree (choices[i].options);
		}
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		if (labels[i].lines)
		{
			HFree ((void *)labels[i].lines);
		}
	}

	/* Clear out the master tables */
	
	if (SetupTab)
	{
		DestroyStringTable (ReleaseStringTable (SetupTab));
		SetupTab = 0;
	}
	if (bank)
	{
		StringBank_Free (bank);
		bank = NULL;
	}
	if (setup_frame)
	{
		DestroyDrawable (ReleaseDrawable (setup_frame));
		setup_frame = NULL;
		ReleaseArrows ();
	}

	if (testSounds)
	{
		DestroySound (ReleaseSound (testSounds));
		testSounds = 0;
	}
}

void
SetupMenu (void)
{
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = FALSE;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetupTab = CaptureStringTable (LoadStringTable (SETUP_MENU_STRTAB));
	if (SetupTab) 
	{
		init_widgets ();
	}
	else
	{
		log_add (log_Fatal,
				"PANIC: Could not find strings for the setup menu!");
		exit (EXIT_FAILURE);
	}
	done = FALSE;

	DoInput (&s, TRUE);
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	PropagateResults ();
	if (SetupTab)
	{
		clean_up_widgets ();
	}

	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN,
						MENU_SOUND_SELECT);
}

void
GetGlobalOptions (GLOBALOPTS *opts)
{
	bool whichBound;
	int flags;
	BYTE i;

/*
 *		Graphics options
 */
	opts->screenResolution = resolutionFactor >> 1;

	if (GfxFlags & TFB_GFXFLAGS_FULLSCREEN)
		opts->fullscreen = 2;
	else if (GfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN)
		opts->fullscreen = 1;
	else
		opts->fullscreen = 0;
	/*opts->fullscreen = (GfxFlags & TFB_GFXFLAGS_FULLSCREEN) ?
		OPTVAL_ENABLED : OPTVAL_DISABLED;*/
	opts->fps = (GfxFlags & TFB_GFXFLAGS_SHOWFPS) ?
		OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->scanlines = (GfxFlags & TFB_GFXFLAGS_SCANLINES) ?
		OPTVAL_ENABLED : OPTVAL_DISABLED;

	flags = GfxFlags & 248; // 11111000 - only scaler fralgs

	switch (flags)
	{// this works because there is only 1 scaler flag at a time
		case TFB_GFXFLAGS_SCALE_BILINEAR:
			opts->scaler = OPTVAL_BILINEAR_SCALE;
			break;
		case TFB_GFXFLAGS_SCALE_BIADAPT:
			opts->scaler = OPTVAL_BIADAPT_SCALE;
			break;
		case TFB_GFXFLAGS_SCALE_BIADAPTADV:
			opts->scaler = OPTVAL_BIADV_SCALE;
			break;
		case TFB_GFXFLAGS_SCALE_TRISCAN:
			opts->scaler = OPTVAL_TRISCAN_SCALE;
			break;
		case TFB_GFXFLAGS_SCALE_HQXX:
			opts->scaler = OPTVAL_HQXX_SCALE;
			break;
		default:
			opts->scaler = OPTVAL_NO_SCALE;
			break;
	}

	opts->keepaspect = optKeepAspectRatio;

	whichBound = (optGamma < maxGamma);
	// The option supplied by the user may be beyond our starting range
	// but valid nonetheless. We need to account for that.
	if (optGamma <= minGamma)
		minGamma = optGamma - 0.03f;
	else if (optGamma >= maxGamma)
		maxGamma = optGamma + 0.3f;
	updateGammaBounds (whichBound);
	opts->gamma = gammaToSlider (optGamma);



	opts->loresBlowup = loresBlowupScale;
	/* Work out resolution.  On the way, try to guess a good default
	 * for config.alwaysgl, then overwrite it if it was set previously. */
	if ((!IS_HD && (GraphicsDriver != TFB_GFXDRIVER_SDL_PURE) &&
		((WindowWidth == 320) || (WindowWidth == 640))) ||
		res_GetBoolean ("config.alwaysgl"))
		opts->driver = OPTVAL_ALWAYS_GL;
	else
		opts->driver = OPTVAL_PURE_IF_POSSIBLE;

	opts->windowType = optWindowType;
	switch (opts->windowType)
	{
		case OPTVAL_PC_WINDOW:
			if (!isAddonAvailable (DOS_MODE (IS_HD)))
			{
				opts->windowType = 2;
			}
			break;
		case OPTVAL_3DO_WINDOW:
			if (!isAddonAvailable (THREEDO_MODE (IS_HD)))
			{
				opts->windowType = 2;
			}
			break;
		default:
			break;
	}

/*
 *		Audio options
 */
	opts->stereo = optStereoSFX;
	opts->music3do = opt3doMusic;
	opts->musicremix = optRemixMusic; // Precursors Pack
	opts->volasMusic = optVolasMusic;

	opts->spaceMusic = optSpaceMusic;
	opts->mainMenuMusic = optMainMenuMusic;
	opts->musicResume = optMusicResume;
	opts->speech = optSpeech;

	switch (snddriver) 
	{
		case audio_DRIVER_OPENAL:
			opts->adriver = OPTVAL_OPENAL;
			break;
		case audio_DRIVER_MIXSDL:
			opts->adriver = OPTVAL_MIXSDL;
			break;
		default:
			opts->adriver = OPTVAL_SILENCE;
			break;
	}
	audioDriver = opts->adriver;

	if (soundflags & audio_QUALITY_HIGH)
		opts->aquality = OPTVAL_HIGH;
	else if (soundflags & audio_QUALITY_LOW)
		opts->aquality = OPTVAL_LOW;
	else
		opts->aquality = OPTVAL_MEDIUM;

	audioQuality = opts->aquality;

	MusVol = opts->musicvol =
			(((int)(musicVolumeScale * 100.0f) + 2) / 5) * 5;
	SfxVol = opts->sfxvol = (((int)(sfxVolumeScale * 100.0f) + 2) / 5) * 5;
	SpcVol = opts->speechvol =
			(((int)(speechVolumeScale * 100.0f) + 2) / 5) * 5;


/*
 *		Engine&Visuals options
 */
	// Mics
	opts->subtitles = optSubtitles;
	opts->menu = is3DO (optWhichMenu);
	opts->submenu = optSubmenu;
	opts->text = is3DO (optWhichFonts);
	opts->scrTrans = is3DO (optScrTrans);
	opts->intro = is3DO (optWhichIntro);
	opts->skipIntro = optSkipIntro;	
#if defined(ANDROID) || defined(__ANDROID__)
	optMScale = opts->meleezoom = optMeleeScale;
#else
	optMScale = opts->meleezoom =
			(OPT_MELEEZOOM)(optMeleeScale == TFB_SCALE_STEP ?
			OPTVAL_PC : OPTVAL_3DO);
#endif
	opts->controllerType = optControllerType;
	opts->directionalJoystick = optDirectionalJoystick; // For Android
	opts->dateType = optDateFormat;
	opts->customBorder = optCustomBorder;
	opts->flagshipColor = is3DO (optFlagshipColor);
	opts->gameOver = optGameOver;
	opts->hyperStars = optHyperStars;
	opts->showVisitedStars = optShowVisitedStars;
	opts->fuelRange = optFuelRange;
	opts->wholeFuel = optWholeFuel;
	opts->meleeToolTips = optMeleeToolTips;
	opts->sphereColors = optSphereColors;

	// Interplanetary
	opts->nebulae = optNebulae;
	opts->nebulaevol = optNebulaeVolume;
	opts->starBackground = optStarBackground;
	opts->unscaledStarSystem = optUnscaledStarSystem;
	opts->planetStyle = is3DO (optPlanetStyle);
	opts->orbitingPlanets = optOrbitingPlanets;
	opts->texturedPlanets = optTexturedPlanets;

	// Orbit
	opts->landerHold = is3DO (optLanderHold);
	opts->partialPickup = optPartialPickup;
	opts->cscan = optWhichCoarseScan;
	opts->hazardColors = optHazardColors;
	opts->scanStyle = is3DO (optScanStyle);
	opts->landerStyle = is3DO (optSuperPC);
	opts->planetTexture = optPlanetTexture;
	opts->sphereType = optScanSphere;
	opts->tintPlanSphere = is3DO (optTintPlanSphere);
	opts->shield = is3DO (optWhichShield);

	// Game modes
	opts->difficulty = optDiffChooser;
	opts->extended = optExtended;
	opts->nomad = optNomad;
	opts->slaughterMode = optSlaughterMode;
	opts->seedType = optSeedType;
	opts->fleetPointSys = optFleetPointSys;
	
	// Comm screen
	opts->scroll = is3DO (optSmoothScroll);
	opts->orzCompFont = optOrzCompFont;
	opts->scopeStyle = is3DO (optScopeStyle);
	opts->nonStopOscill = optNonStopOscill;

	// Auto-Pilot
	opts->smartAutoPilot = optSmartAutoPilot;
	opts->advancedAutoPilot = optAdvancedAutoPilot;
	opts->shipDirectionIP = optShipDirectionIP;

	// Controls
	opts->player1 = PlayerControls[0];
	opts->player2 = PlayerControls[1];

	// QoL
	opts->scatterElements = optScatterElements;

	opts->showUpgrades = optShowUpgrades;

/*
 *		Cheats
 */
 	opts->cheatMode = optCheatMode;
	opts->godModes = optGodModes;
	opts->tdType = timeDilationScale;
	opts->bubbleWarp = optBubbleWarp;
	opts->unlockShips = optUnlockShips;
	opts->headStart = optHeadStart;
	//opts->unlockUpgrades = optUnlockUpgrades;
	opts->infiniteCredits = optInfiniteCredits;
	opts->infiniteRU = optInfiniteRU;
	opts->infiniteFuel = optInfiniteFuel;
	opts->noHQEncounters = optNoHQEncounters;
	opts->deCleansing = optDeCleansing;
	opts->meleeObstacles = optMeleeObstacles;

	// Devices
	for (i = 0; i < NUM_DEVICES; i++)
	{
		opts->deviceArray[i] = optDeviceArray[i];
	}

	// Upgrades
	for (i = 0; i < NUM_UPGRADES; i++)
	{
		opts->upgradeArray[i] = optUpgradeArray[i];
	}
}

void
SetGlobalOptions (GLOBALOPTS *opts)
{
	int NewSndFlags = 0;
	int resFactor = resolutionFactor;
	int newFactor;
	BYTE i;

/*
 *		Graphics options
 */

	newFactor = (int)(opts->screenResolution << 1);
	PutIntOpt (&resFactor, &newFactor, "config.resolutionfactor", TRUE);

	if (resFactor != resolutionFactor)
	{
		SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
		resolutionFactor = resFactor;

#if defined(ANDROID) || defined(__ANDROID__)
		// Switch to native resolution on Res Factor change
		ScreenWidthActual = 320 << RESOLUTION_FACTOR;
		ScreenHeightActual = DOS_BOOL (240, 200) << RESOLUTION_FACTOR;
		loresBlowupScale = (ScreenWidthActual / 320) - 1;
		res_PutInteger("config.loresBlowupScale", loresBlowupScale);
		res_PutInteger("config.reswidth", ScreenWidthActual);
		res_PutInteger("config.resheight", ScreenHeightActual);
#endif

		switch (opts->windowType)
		{
			case OPTVAL_PC_WINDOW:
				if (!isAddonAvailable (DOS_MODE (resolutionFactor)))
				{
					opts->windowType = OPTVAL_UQM_WINDOW;
				}
				break;
			case OPTVAL_3DO_WINDOW:
				if (!isAddonAvailable (THREEDO_MODE (resolutionFactor)))
				{
					opts->windowType = OPTVAL_UQM_WINDOW;
				}
				break;
			default:
				break;
		}
	}

	if (optWindowType != opts->windowType)
	{
		int nh;
		PutIntOpt ((int *)&optWindowType, (int *)&opts->windowType,
				"mm.windowType", TRUE);

		nh = DOS_BOOL (240, 200) * (1 + opts->loresBlowup);

		if (nh != WindowHeight)
		{
			WindowHeight = nh;
			res_PutInteger("config.resheight", WindowHeight);
		}
	}

//#if !(defined(ANDROID) || defined(__ANDROID__))
//	if (opts->fullscreen)
//		NewGfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
//#endif
//	if (IS_HD)
//	{// Kruzen - adjust scalers for HD
//		if (opts->scaler != OPTVAL_BILINEAR_SCALE)
//			opts->scaler = OPTVAL_BILINEAR_SCALE;
//
//#if !(defined(ANDROID) || defined(__ANDROID__))
//		if (!(NewGfxFlags & TFB_GFXFLAGS_FULLSCREEN) &&
//			(opts->loresBlowup == NO_BLOWUP ||
//				opts->loresBlowup == OPTVAL_SCALE_1280_960))
//			opts->scaler = OPTVAL_NO_SCALE;
//#endif
//	}

	//PutBoolOpt (&optKeepAspectRatio, &opts->keepaspect, "config.keepaspectratio", FALSE);

	// Avoid setting gamma when it is not necessary
	if (optGamma != 1.0f || sliderToGamma (opts->gamma) != 1.0f)
	{
		optGamma = sliderToGamma (opts->gamma);
		setGammaCorrection (optGamma);
		res_PutInteger ("config.gamma", (int) (optGamma * GAMMA_SCALE + 0.5));
	}


/*
 *		Audio options
 */
	PutBoolOpt (&optStereoSFX, &opts->stereo, "config.positionalsfx", TRUE);
	PutBoolOpt (&opt3doMusic, &opts->music3do, "config.3domusic", TRUE);
	PutBoolOpt (&optRemixMusic, &opts->musicremix, "config.remixmusic", TRUE);
	PutBoolOpt (&optVolasMusic, &opts->volasMusic, "mm.volasMusic", TRUE);

	PutIntOpt (&optSpaceMusic, (int *)&opts->spaceMusic, "mm.spaceMusic", TRUE);

	if (PutBoolOpt (&optMainMenuMusic, &opts->mainMenuMusic, "mm.mainMenuMusic", FALSE))
	{
		if (optMainMenuMusic)
			InitMenuMusic ();
		else
			UninitMenuMusic ();
	}

	PutIntOpt (&optMusicResume, (int*)&opts->musicResume, "mm.musicResume", FALSE);
	PutBoolOpt (&optSpeech, &opts->speech, "config.speech", TRUE);

	if (audioDriver != opts->adriver)
	{
		audioDriver = opts->adriver;
		switch (opts->adriver)
		{
			case OPTVAL_SILENCE:
				snddriver = audio_DRIVER_NOSOUND;
				res_PutString ("config.audiodriver", "none");
				break;
			case OPTVAL_MIXSDL:
				snddriver = audio_DRIVER_MIXSDL;
				res_PutString ("config.audiodriver", "mixsdl");
				break;
			case OPTVAL_OPENAL:
				snddriver = audio_DRIVER_OPENAL;
				res_PutString ("config.audiodriver", "openal");
				break;
			default:
				/* Shouldn't happen; leave config untouched */
				break;
		}

		optRequiresRestart = TRUE;
	}

	if (audioQuality != opts->aquality)
	{
		audioQuality = opts->aquality;
		switch (opts->aquality)
		{
			case OPTVAL_LOW:
				NewSndFlags |= audio_QUALITY_LOW;
				res_PutString ("config.audioquality", "low");
				break;
			case OPTVAL_MEDIUM:
				NewSndFlags |= audio_QUALITY_MEDIUM;
				res_PutString ("config.audioquality", "medium");
				break;
			case OPTVAL_HIGH:
				NewSndFlags |= audio_QUALITY_HIGH;
				res_PutString ("config.audioquality", "high");
				break;
			default:
				/* Shouldn't happen; leave config untouched */
				break;
		}
		soundflags = NewSndFlags;

		optRequiresRestart = TRUE;
	}

	// update actual volumes
	PutIntOpt (&SfxVol, &opts->sfxvol, "config.sfxvol", FALSE);
	PutIntOpt (&MusVol, &opts->musicvol, "config.musicvol", FALSE);
	PutIntOpt (&SpcVol, &opts->speechvol, "config.speechvol", FALSE);


/*
 *		Engine&Visuals options
 */
	// Mics
	PutBoolOpt (&optSubtitles, &opts->subtitles, "config.subtitles", FALSE);
	PutConsOpt (&optWhichMenu, &opts->menu, "config.textmenu", FALSE);
	PutBoolOpt (&optSubmenu, &opts->submenu, "mm.submenu", FALSE);
	PutConsOpt (&optWhichFonts, &opts->text, "config.textgradients", FALSE);
	PutConsOpt (&optScrTrans, &opts->scrTrans, "mm.scrTransition", FALSE);
	PutConsOpt (&optWhichIntro, &opts->intro, "config.3domovies", TRUE);
	PutBoolOpt (&optSkipIntro, &opts->skipIntro, "mm.skipIntro", FALSE);
	if (optMScale != (int)opts->meleezoom)
	{
#if defined(ANDROID) || defined(__ANDROID__)
		switch (opts->meleezoom) 
		{
			case TFB_SCALE_NEAREST:
				optMeleeScale = OPTVAL_NEAREST;
				break;
			case TFB_SCALE_BILINEAR:
				optMeleeScale = OPTVAL_BILINEAR;
				break;
			case TFB_SCALE_TRILINEAR:
				optMeleeScale = OPTVAL_TRILINEAR;
				break;
			case TFB_SCALE_STEP:
			default:
				optMeleeScale = OPTVAL_STEP;
				break;
		}
		res_PutInteger ("config.smoothmelee", opts->meleezoom);
#else
		optMeleeScale = ((int)opts->meleezoom == OPTVAL_3DO)
				? TFB_SCALE_TRILINEAR : TFB_SCALE_STEP;
		res_PutBoolean ("config.smoothmelee", (int)opts->meleezoom == OPTVAL_3DO);
#endif
	}
#if SDL_MAJOR_VERSION == 1 // Refined joypad controls aren't supported on SDL1
		opts->controllerType = 0;
#endif
	if (PutIntOpt (&optControllerType, (int*)(&opts->controllerType), "mm.controllerType", FALSE))
	{
		TFB_UninitInput ();
		TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);
	}
#if defined(ANDROID) || defined(__ANDROID__)
	PutBoolOpt (&optDirectionalJoystick, &opts->directionalJoystick, "mm.directionalJoystick", FALSE);
#endif
	PutIntOpt  (&optDateFormat, (int*)(&opts->dateType), "mm.dateFormat", FALSE);
	PutBoolOpt (&optCustomBorder, &opts->customBorder, "mm.customBorder", FALSE);
	PutConsOpt (&optFlagshipColor, &opts->flagshipColor, "mm.flagshipColor", FALSE);
	PutBoolOpt (&optGameOver, &opts->gameOver, "mm.gameOver", FALSE);
	PutBoolOpt (&optHyperStars, &opts->hyperStars, "mm.hyperStars", FALSE);
	PutBoolOpt (&optShowVisitedStars, &opts->showVisitedStars, "mm.showVisitedStars", FALSE);
	PutIntOpt  (&optFuelRange, (int*)(&opts->fuelRange), "mm.fuelRange", FALSE);
	PutBoolOpt (&optWholeFuel, &opts->wholeFuel, "mm.wholeFuel", FALSE);
	PutBoolOpt (&optMeleeToolTips, &opts->meleeToolTips, "mm.meleeToolTips", FALSE);
	PutIntOpt  (&optSphereColors, (int *)&opts->sphereColors, "mm.sphereColors", FALSE);
	PutBoolOpt (&optScatterElements, &opts->scatterElements, "mm.scatterElements", FALSE);
	
	// Interplanetary
	PutBoolOpt (&optNebulae, &opts->nebulae, "mm.nebulae", FALSE);
	PutIntOpt  (&optNebulaeVolume, &opts->nebulaevol, "mm.nebulaevol", FALSE);
	PutIntOpt  (&optStarBackground, &opts->starBackground, "mm.starBackground", FALSE);
	PutBoolOpt (&optUnscaledStarSystem, &opts->unscaledStarSystem, "mm.unscaledStarSystem", FALSE);
	PutConsOpt (&optPlanetStyle, &opts->planetStyle, "mm.planetStyle", FALSE);
	PutBoolOpt (&optOrbitingPlanets, &opts->orbitingPlanets, "mm.orbitingPlanets", FALSE);
	PutBoolOpt (&optTexturedPlanets, &opts->texturedPlanets, "mm.texturedPlanets", FALSE);
	
	// Orbit
	PutConsOpt (&optLanderHold, &opts->landerHold, "mm.landerHold", FALSE);
	PutBoolOpt (&optPartialPickup, &opts->partialPickup, "mm.partialPickup", FALSE);
	PutIntOpt  (&optWhichCoarseScan, &opts->cscan, "config.iconicscan", FALSE);
	PutBoolOpt (&optHazardColors, &opts->hazardColors, "mm.hazardColors", FALSE);
	PutConsOpt (&optScanStyle, &opts->scanStyle, "mm.scanStyle", FALSE);
	PutConsOpt (&optSuperPC, &opts->landerStyle, "mm.landerStyle", FALSE);
	PutBoolOpt (&optPlanetTexture, &opts->planetTexture, "mm.planetTexture", FALSE);
	PutIntOpt  (&optScanSphere, (int*)&opts->sphereType, "mm.sphereType", FALSE);
	PutConsOpt (&optTintPlanSphere, &opts->tintPlanSphere, "mm.tintPlanSphere", FALSE);
	PutConsOpt (&optWhichShield, &opts->shield, "config.pulseshield", FALSE);
	PutBoolOpt (&optShowUpgrades, &opts->showUpgrades, "mm.showUpgrades", FALSE);

	// Game modes
	{
		PutIntOpt (&optSeedType, (int*)(&opts->seedType), "mm.seedType", FALSE);
		int customSeed = atoi (textentries[TEXT_GAMESEED].value);
		if (!SANE_SEED (customSeed))
			customSeed = PrimeA;
		PutIntOpt (&optCustomSeed, &customSeed, "mm.customSeed", FALSE);
	}

	PutIntOpt (&optDiffChooser, (int*)&opts->difficulty, "mm.difficulty", FALSE);
	if ((optDifficulty = opts->difficulty) == OPTVAL_IMPO)
		optDifficulty = OPTVAL_NORM;
	PutBoolOpt (&optExtended, &opts->extended, "mm.extended", FALSE);
	PutIntOpt (&optNomad, (int *)&opts->nomad, "mm.nomad", FALSE);
	PutBoolOpt (&optSlaughterMode, &opts->slaughterMode, "mm.slaughterMode", FALSE);
	PutBoolOpt (&optFleetPointSys, &opts->fleetPointSys, "mm.fleetPointSys", FALSE);

	// Comm screen
	PutConsOpt (&optSmoothScroll, &opts->scroll, "config.smoothscroll", FALSE);
	PutBoolOpt (&optOrzCompFont, &opts->orzCompFont, "mm.orzCompFont", FALSE);
	PutConsOpt (&optScopeStyle, &opts->scopeStyle, "mm.scopeStyle", FALSE);
	PutBoolOpt (&optNonStopOscill, &opts->nonStopOscill, "mm.nonStopOscill", FALSE);

	// Auto-Pilot
	PutBoolOpt (&optSmartAutoPilot, &opts->smartAutoPilot, "mm.smartAutoPilot", FALSE);
	PutBoolOpt (&optAdvancedAutoPilot, &opts->advancedAutoPilot, "mm.advancedAutoPilot", FALSE);
	PutBoolOpt (&optShipDirectionIP, &opts->shipDirectionIP, "mm.shipDirectionIP", FALSE);

	// Controls
	PlayerControls[1] = opts->player2;

	if (optControllerType == 2)
		PlayerControls[0] = CONTROL_TEMPLATE_JOY_3;
	else if (optControllerType == 1)
		PlayerControls[0] = CONTROL_TEMPLATE_JOY_2;
	else
		PlayerControls[0] = opts->player1;

	res_PutInteger ("config.player1control", opts->player1);
	res_PutInteger ("config.player2control", opts->player2);

	res_PutString ("keys.1.name", input_templates[0].name);
	res_PutString ("keys.2.name", input_templates[1].name);
	res_PutString ("keys.3.name", input_templates[2].name);
	res_PutString ("keys.4.name", input_templates[3].name);
	res_PutString ("keys.5.name", input_templates[4].name);
	res_PutString ("keys.6.name", input_templates[5].name);


/*
 *		Cheats
 */
	PutBoolOpt (&optCheatMode, &opts->cheatMode, "cheat.kohrStahp", FALSE);
	PutIntOpt  (&optGodModes, (int*)&opts->godModes, "cheat.godModes", FALSE);
	PutIntOpt  (&timeDilationScale, (int*)&opts->tdType, "cheat.timeDilation", FALSE);
	PutBoolOpt (&optBubbleWarp, &opts->bubbleWarp, "cheat.bubbleWarp", FALSE);
	PutBoolOpt (&optUnlockShips, &opts->unlockShips, "cheat.unlockShips", FALSE);
	PutBoolOpt (&optHeadStart, &opts->headStart, "cheat.headStart", FALSE);
	//PutBoolOpt (&optUnlockUpgrades, &opts->unlockUpgrades, "cheat.unlockUpgrades", FALSE);
	PutBoolOpt (&optInfiniteCredits, &opts->infiniteCredits, "cheat.infiniteCredits", FALSE);
	PutBoolOpt (&optInfiniteRU, &opts->infiniteRU, "cheat.infiniteRU", FALSE);
	PutBoolOpt (&optInfiniteFuel, &opts->infiniteFuel, "cheat.infiniteFuel", FALSE);
	PutBoolOpt (&optNoHQEncounters, &opts->noHQEncounters, "cheat.noHQEncounters", FALSE);
	PutBoolOpt (&optDeCleansing, &opts->deCleansing, "cheat.deCleansing", FALSE);
	PutBoolOpt (&optMeleeObstacles, &opts->meleeObstacles, "cheat.meleeObstacles", FALSE);

	// Devices
	for (i = 0; i < NUM_DEVICES; i++)
	{
		optDeviceArray[i] = opts->deviceArray[i];
	}

	// Upgrades
	for (i = 0; i < NUM_UPGRADES; i++)
	{
		optUpgradeArray[i] = opts->upgradeArray[i];
	}

	SaveResourceIndex (configDir, "uqm.cfg", "config.", TRUE);
	SaveKeyConfiguration (configDir, "flight.cfg");
	
	SaveResourceIndex (configDir, "megamod.cfg", "mm.", TRUE);
	SaveResourceIndex (configDir, "cheats.cfg", "cheat.", TRUE);

	if (optRequiresReload)
	{
		SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));

		FlushGraphics ();
		UninitVideoPlayer ();

		ResetOffset ();

		RESOLUTION_FACTOR = resolutionFactor;
		CanvasWidth = 320 << resolutionFactor;
		CanvasHeight = DOS_BOOL (240, 200) << resolutionFactor;

		log_add (log_Debug, "ScreenWidth:%d, ScreenHeight:%d, "
				"Wactual:%d, Hactual:%d", CanvasWidth, CanvasHeight,
				WindowWidth, WindowHeight);

		// These solve the context problem that plagued the setupmenu
		// when changing to higher resolution.
		TFB_BBox_Reset ();
		TFB_BBox_Init (CanvasWidth, CanvasHeight);
		FlushColorXForms ();

		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				WindowWidth, WindowHeight);
		InitVideoPlayer (TRUE);

		Reload ();
	}
}
