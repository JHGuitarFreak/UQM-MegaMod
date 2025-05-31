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
	(WIDGET *)(&buttons[2]),    // Graphics
	(WIDGET *)(&buttons[3]),    // PC/3DO
	(WIDGET *)(&buttons[11]),   // Visuals
	(WIDGET *)(&buttons[4]),    // Sound
	(WIDGET *)(&buttons[10]),   // Music
	(WIDGET *)(&buttons[6]),    // Controls
	(WIDGET *)(&buttons[12]),   // Quality of Life
	(WIDGET *)(&buttons[7]),    // Advanced
	(WIDGET *)(&buttons[5]),    // Cheats
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[0]),    // Quit Setup Menu
	NULL };

static WIDGET *graphics_widgets[] = {
	(WIDGET *)(&choices[0]),    // Graphics
	(WIDGET *)(&choices[42]),   // Resolution
	(WIDGET *)(&textentries[2]),// Custom Seed entry
#if	SDL_MAJOR_VERSION == 1
#if defined (HAVE_OPENGL)
	(WIDGET *)(&choices[1]),    // Use Framebuffer
#endif
#endif
	(WIDGET *)(&choices[23]),   // Aspect Ratio
	(WIDGET *)(&choices[10]),   // Display Mode
	(WIDGET *)(&sliders[3]),    // Gamma Correction
	(WIDGET *)(&choices[2]),    // Scaler
	(WIDGET *)(&choices[3]),    // Scanlines
#if	SDL_MAJOR_VERSION == 2
	(WIDGET *)(&choices[12]),   // Show FPS
#endif
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[1]),
	NULL };

static WIDGET *engine_widgets[] = {
	(WIDGET *)(&labels[5]),     // UI Label
	(WIDGET *)(&choices[81]),   // Window Type
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&choices[4]),    // Menu Style
	(WIDGET *)(&choices[5]),    // Font Style
	(WIDGET *)(&choices[11]),   // Cutscenes
#if defined(ANDROID) || defined(__ANDROID__)
	(WIDGET *)(&choices[50]),   // Android: Melee Zoom
#else
	(WIDGET *)(&choices[13]),   // Melee Zoom
#endif
	(WIDGET *)(&choices[70]),   // Flagship Style
	(WIDGET *)(&choices[52]),   // Screen Transitions

	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[6]),     // Comm Label
	(WIDGET *)(&choices[7]),    // Scroll Style
	(WIDGET *)(&choices[22]),   // Speech
	(WIDGET *)(&choices[8]),    // Subtitles
	(WIDGET *)(&choices[66]),   // Oscilloscope Style

	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[7]),     // IP Label
	(WIDGET *)(&choices[62]),   // Interplanetary Style
	(WIDGET *)(&choices[63]),   // Star Background

	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[8]),     // Scan Label
	(WIDGET *)(&choices[6]),    // Scan Style
	(WIDGET *)(&choices[17]),   // Slave Shields
	(WIDGET *)(&choices[64]),   // Scan Style
	(WIDGET *)(&choices[76]),   // Scan Sphere Type
	(WIDGET *)(&choices[61]),   // Scanned Sphere Tint
	(WIDGET *)(&choices[68]),   // Lander Style
	(WIDGET *)(&buttons[1]),
	NULL };

static WIDGET *audio_widgets[] = {
	(WIDGET *)(&sliders[0]),    // Music Volume
	(WIDGET *)(&sliders[1]),    // SFX Volume
	(WIDGET *)(&sliders[2]),    // Speech Volume
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&choices[14]),   // Positional Audio
	(WIDGET *)(&choices[15]),   // Sound Driver
	(WIDGET *)(&choices[16]),   // Sound Quality
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[1]),
	NULL };

static WIDGET *music_widgets[] = {
	(WIDGET *)(&choices[9]),    // 3DO Remixes
	(WIDGET *)(&choices[21]),   // Precursor's Remixes
	(WIDGET *)(&choices[47]),   // Volasaurus' Remix Pack
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&choices[46]),   // Volasaurus' Space Music
	(WIDGET *)(&choices[34]),   // Main Menu Music
	(WIDGET *)(&choices[80]),   // Music Resume
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[1]),
	NULL };

static WIDGET *cheat_widgets[] = {
	(WIDGET *)(&buttons[13]),   // Devices Menu
	(WIDGET *)(&buttons[14]),   // Upgrades Menu
	(WIDGET *)(&choices[24]),   // JMS: cheatMode on/off
	(WIDGET *)(&choices[72]),   // Kohr-Ah DeCleansing mode
	(WIDGET *)(&choices[25]),   // Precursor Mode
	(WIDGET *)(&choices[26]),   // Time Dilation
	(WIDGET *)(&choices[27]),   // Bubble Warp
	(WIDGET *)(&choices[29]),   // Head Start
	(WIDGET *)(&choices[28]),   // Unlock Ships
	//(WIDGET *)(&choices[30]),   // Unlock Upgrades
	(WIDGET *)(&choices[31]),   // Infinite RU
	(WIDGET *)(&choices[39]),   // Infinite Fuel
	//(WIDGET *)(&choices[43]),   // Add Devices
	(WIDGET *)(&choices[71]),   // No HyperSpace Encounters
	(WIDGET *)(&choices[73]),   // No Planets in melee
	(WIDGET *)(&buttons[1]),    // Exit to Menu
	NULL };
	
static WIDGET *keyconfig_widgets[] = {
#if SDL_MAJOR_VERSION == 2 // Refined joypad controls not supported in SDL1
	(WIDGET *)(&choices[59]),   // Control Display
#endif
	(WIDGET *)(&choices[18]),   // Bottom Player
	(WIDGET *)(&choices[19]),   // Top Player
#if defined(ANDROID) || defined(__ANDROID__)
	(WIDGET *)(&choices[49]),   // Directional Joystick toggle
#endif
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[1]),
	(WIDGET *)(&buttons[8]),    // Edit Controls
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[1]),
	NULL };

static WIDGET *advanced_widgets[] = {
	(WIDGET *)(&choices[53]),   // Difficulty
	(WIDGET *)(&choices[54]),   // Extended features
	(WIDGET *)(&choices[55]),   // Nomad Mode
	(WIDGET *)(&choices[77]),   // Slaughter Mode
	(WIDGET *)(&choices[86]),   // Fleet Point System
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&choices[82]),   // Seed usage selection
	(WIDGET *)(&textentries[1]),// Custom Seed entry
	(WIDGET *)(&choices[83]),   // SOI Color Selection
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[1]),
	NULL };

static WIDGET *visual_widgets[] = {
	(WIDGET *)(&labels[5]),     // UI Label
	(WIDGET *)(&choices[38]),   // Switch date formats
	(WIDGET *)(&choices[45]),   // Custom Border switch
	(WIDGET *)(&choices[48]),   // Whole Fuel Value switch
	(WIDGET *)(&choices[33]),   // Fuel Range
	(WIDGET *)(&choices[67]),   // Animated HyperStars
	(WIDGET *)(&choices[56]),   // Game Over switch

	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[6]),     // Comm Label
	(WIDGET *)(&choices[58]),   // Alternate Orz font
	(WIDGET *)(&choices[65]),   // Non-Stop Scope

	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[7]),     // IP Label
	(WIDGET *)(&choices[35]),   // IP nebulae on/off
	(WIDGET *)(&sliders[4]),    // Nebulae Volume
	(WIDGET *)(&choices[36]),   // orbitingPlanets on/off
	(WIDGET *)(&choices[37]),   // texturedPlanets on/off
	(WIDGET *)(&choices[75]),   // T6014's Classic Star System View
	(WIDGET *)(&choices[57]),   // NPC Ship Direction in IP

	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&labels[8]),     // Scan Label
	(WIDGET *)(&choices[44]),   // Hazard Colors
	(WIDGET *)(&choices[69]),   // Planet Texture
	(WIDGET *)(&choices[85]),   // Show Lander Upgrades
	(WIDGET *)(&buttons[1]),    // Exit to Menu
	NULL };

static WIDGET *qol_widgets[] = {
	(WIDGET *)(&choices[32]),   // Skip Intro
	(WIDGET *)(&choices[40]),   // Partial Pickup switch
	(WIDGET *)(&choices[84]),   // Scatter Elements
	(WIDGET *)(&choices[41]),   // Submenu switch
	(WIDGET *)(&choices[60]),   // Smart Auto-Pilot
	(WIDGET *)(&choices[78]),   // Advanced Auto-Pilot
	(WIDGET *)(&choices[74]),   // Show Visited Stars
	(WIDGET *)(&choices[79]),   // Melee Tool Tips
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[1]),    // Exit to Menu
	NULL };

static WIDGET *editkeys_widgets[] = {
	(WIDGET *)(&choices[20]),       // Current layout
	(WIDGET *)(&textentries[0]),    // Layout name
	(WIDGET *)(&labels[2]),         // "Tap to Edit..."
	(WIDGET *)(&controlentries[0]), // Up
	(WIDGET *)(&controlentries[1]), // Down
	(WIDGET *)(&controlentries[2]), // Left
	(WIDGET *)(&controlentries[3]), // Right
	(WIDGET *)(&controlentries[4]), // Weapon
	(WIDGET *)(&controlentries[5]), // Special
	(WIDGET *)(&controlentries[6]), // Escape
	(WIDGET *)(&controlentries[7]), // Thrust
	(WIDGET *)(&buttons[9]),        // Previous menu
	NULL };

static WIDGET *devices_widgets[] = {
	(WIDGET *)(&choices[87]),   // Portal Spawner
	(WIDGET *)(&choices[88]),   // Talking Pet
	(WIDGET *)(&choices[89]),   // Utwig Bomb
	(WIDGET *)(&choices[90]),   // Sun Device
	(WIDGET *)(&choices[91]),   // Rosy Sphere
	(WIDGET *)(&choices[92]),   // Aqua Helix
	(WIDGET *)(&choices[93]),   // Clear Spindle
	(WIDGET *)(&choices[94]),   // Ultron (Broken)
	(WIDGET *)(&choices[95]),   // Ultron (Semi-Broken)
	(WIDGET *)(&choices[96]),   // Ultron (Semi-Fixed)
	(WIDGET *)(&choices[97]),   // Ultron (Fixed)
	(WIDGET *)(&choices[98]),   // Shofixti Maidens
	(WIDGET *)(&choices[99]),   // Umgah Caster
	(WIDGET *)(&choices[100]),  // Burvixese Caster
	(WIDGET *)(&choices[101]),  // Taalo Shield
	(WIDGET *)(&choices[102]),  // Egg Case 01
	(WIDGET *)(&choices[103]),  // Egg Case 02
	(WIDGET *)(&choices[104]),  // Egg Case 03
	(WIDGET *)(&choices[105]),  // Syreen Shuttle
	(WIDGET *)(&choices[106]),  // VUX Beast
	(WIDGET *)(&choices[107]),  // Slylandro Destruct
	(WIDGET *)(&choices[108]),  // Ur-Quan Warp Pod
	(WIDGET *)(&choices[109]),  // Wimbli's Trident
	(WIDGET *)(&choices[110]),  // Glowing Rod
	(WIDGET *)(&choices[111]),  // Lunar Base
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[15]),   // Back to Cheats
	NULL };

static WIDGET *upgrades_widgets[] = {
	(WIDGET *)(&choices[112]),  // Lander Speed
	(WIDGET *)(&choices[113]),  // Lander Cargo
	(WIDGET *)(&choices[114]),  // Lander Rapid Fire
	(WIDGET *)(&choices[115]),  // Lander Bio Shield
	(WIDGET *)(&choices[116]),  // Lander Quake Shield
	(WIDGET *)(&choices[117]),  // Lander Lightning Shield
	(WIDGET *)(&choices[118]),  // Lander Heat Shield
	(WIDGET *)(&choices[119]),  // Point Defense Module
	(WIDGET *)(&choices[120]),  // Fusion Blaster Module
	(WIDGET *)(&choices[121]),  // Hi-Eff Fuel Module
	(WIDGET *)(&choices[122]),  // Tracking Module
	(WIDGET *)(&choices[123]),  // Hellbore Cannon Module
	(WIDGET *)(&choices[124]),  // Shiva Furnace Module
	(WIDGET *)(&labels[4]),     // Spacer
	(WIDGET *)(&buttons[15]),   // Back to Cheats
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
		next = (WIDGET *)(&menus[0]);
		(*next->receiveFocus) (next, WIDGET_EVENT_SELECT);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static void
populate_res (void)
{
	sprintf (textentries[2].value, "%dx%d",
			ScreenWidthActual, ScreenHeightActual);
}

static int
do_graphics (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[1]);
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
		next = (WIDGET *)(&menus[2]);
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
		next = (WIDGET *)(&menus[3]);
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
		next = (WIDGET *)(&menus[4]);
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
		next = (WIDGET *)(&menus[5]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static void
populate_seed (void)
{
	snprintf (textentries[1].value, sizeof (textentries[1].value), "%d",
			optCustomSeed);
	if (!SANE_SEED (optCustomSeed))
		optCustomSeed = PrimeA;
}

static int
do_advanced (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[6]);
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
		next = (WIDGET *)(&menus[8]);
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
		next = (WIDGET *)(&menus[9]);
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
		next = (WIDGET *)(&menus[10]);
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
		next = (WIDGET *)(&menus[11]);
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
		next = (WIDGET *)(&menus[12]);
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
	
	strncpy (textentries[0].value, input_templates[templat].name,
			textentries[0].maxlen);
	textentries[0].value[textentries[0].maxlen-1] = 0;
	
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
		next = (WIDGET *)(&menus[7]);
		/* Prepare the components */
		choices[20].selected = 0;
		
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
					DOS_MODE (choices[0].selected && !IS_HD ? HD : IS_HD)))
			{
				oldval = OPTVAL_UQM_WINDOW;
				addon_unavailable (self, oldval);
				return FALSE;
			}
			break;
		case OPTVAL_3DO_WINDOW:
			if (!isAddonAvailable (
					THREEDO_MODE (
						choices[0].selected && !IS_HD ? HD : IS_HD)))
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
	strncpy (input_templates[choices[20].selected].name, self->value, 30);
	input_templates[choices[20].selected].name[29] = 0;
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
	int NewWidth = ScreenWidthActual;
	int NewHeight = ScreenHeightActual;
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

	if (NewWidth != ScreenWidthActual || NewHeight != ScreenHeightActual)
	{
		ScreenWidthActual = NewWidth;
		ScreenHeightActual = NewHeight;

		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;
	}

	if (NewGfxFlags != GfxFlags)
		GfxFlags = NewGfxFlags;

	if (NewGfxDriver != GraphicsDriver)
		GraphicsDriver = NewGfxDriver;

	FlushInput ();
	TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
			ScreenWidthActual, ScreenHeightActual);

	if (isExclusive)
	{	// needed twice to reinitialize Exclusive Full Screen after a 
		// resolution change 
		GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				ScreenWidthActual, ScreenHeightActual);
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
	int NewWidth = ScreenWidthActual;
	int NewHeight = ScreenHeightActual;
	int NewGfxFlags = GfxFlags;
	BOOLEAN isExclusive = NewGfxFlags & TFB_GFXFLAGS_EX_FULLSCREEN;

	if (sscanf (self->value, "%dx%d", &NewWidth, &NewHeight) != 2)
	{
		populate_res ();
		return;
	}
	
	if (NewWidth != ScreenWidthActual || NewHeight != ScreenHeightActual)
	{
		ScreenWidthActual = NewWidth;
		ScreenHeightActual = NewHeight;

		if (isExclusive)
			NewGfxFlags &= ~TFB_GFXFLAGS_EX_FULLSCREEN;
	}
	else
		return;

	if (NewGfxFlags != GfxFlags)
		GfxFlags = NewGfxFlags;

	FlushInput ();

	TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
		ScreenWidthActual, ScreenHeightActual);

	if (isExclusive)
	{	// needed twice to reinitialize Exclusive Full Screen after a 
		// resolution change
		GfxFlags |= TFB_GFXFLAGS_EX_FULLSCREEN;
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				ScreenWidthActual, ScreenHeightActual);
	}

	if (res_check (NewWidth, NewHeight))
	{
		choices[42].selected = (NewWidth / 320) - 1;
	}
	else
		choices[42].selected = 6;

	PutIntOpt ((int *)(&loresBlowupScale), (int *)(&choices[42].selected),
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
	choices[0].selected = opts.screenResolution;
	choices[1].selected = opts.driver;
	choices[2].selected = opts.scaler;
	choices[3].selected = opts.scanlines;
	choices[4].selected = opts.menu;
	choices[5].selected = opts.text;
	choices[6].selected = opts.cscan;
	choices[7].selected = opts.scroll;
	choices[8].selected = opts.subtitles;
	choices[9].selected = opts.music3do;
	choices[10].selected = opts.fullscreen;
	choices[11].selected = opts.intro;
	choices[12].selected = opts.fps;
#if !(defined(ANDROID) || defined(__ANDROID__))
	choices[13].selected = opts.meleezoom;
#endif
	choices[14].selected = opts.stereo;
	choices[15].selected = opts.adriver;
	choices[16].selected = opts.aquality;
	choices[17].selected = opts.shield;
	choices[18].selected = opts.player1;
	choices[19].selected = opts.player2;
	choices[20].selected = 0;
	choices[21].selected = opts.musicremix;
	choices[22].selected = opts.speech;
	choices[23].selected = opts.keepaspect;
 	choices[24].selected = opts.cheatMode;
	choices[25].selected = opts.godModes;
	choices[26].selected = opts.tdType;
	choices[27].selected = opts.bubbleWarp;
	choices[28].selected = opts.unlockShips;
	choices[29].selected = opts.headStart;
	choices[30].selected = opts.unlockUpgrades;
	choices[31].selected = opts.infiniteRU;
	choices[32].selected = opts.skipIntro;
	choices[33].selected = opts.fuelRange;
	choices[34].selected = opts.mainMenuMusic;
	choices[35].selected = opts.nebulae;
	choices[36].selected = opts.orbitingPlanets;
	choices[37].selected = opts.texturedPlanets;
	choices[38].selected = opts.dateType;
	choices[39].selected = opts.infiniteFuel;
	choices[40].selected = opts.partialPickup;
	choices[41].selected = opts.submenu;
	choices[42].selected = opts.loresBlowup;
	choices[43].selected = opts.addDevices;
	choices[44].selected = opts.hazardColors;
	choices[45].selected = opts.customBorder;
	choices[46].selected = opts.spaceMusic;
	choices[47].selected = opts.volasMusic;
	choices[48].selected = opts.wholeFuel;
#if defined(ANDROID) || defined(__ANDROID__)
	choices[49].selected = opts.directionalJoystick;
	choices[50].selected = opts.meleezoom;
#endif
	choices[51].selected = opts.landerHold;
	choices[52].selected = opts.scrTrans;
	choices[53].selected = opts.difficulty;
	choices[54].selected = opts.extended;
	choices[55].selected = opts.nomad;
	choices[56].selected = opts.gameOver;
	choices[57].selected = opts.shipDirectionIP;
	choices[58].selected = opts.orzCompFont;
	choices[59].selected = opts.controllerType;
	choices[60].selected = opts.smartAutoPilot;
	choices[61].selected = opts.tintPlanSphere;
	choices[62].selected = opts.planetStyle;
	choices[63].selected = opts.starBackground;
	choices[64].selected = opts.scanStyle;
	choices[65].selected = opts.nonStopOscill;
	choices[66].selected = opts.scopeStyle;
	choices[67].selected = opts.hyperStars;
	choices[68].selected = opts.landerStyle;
	choices[69].selected = opts.planetTexture;
	choices[70].selected = opts.flagshipColor;
	choices[71].selected = opts.noHQEncounters;
	choices[72].selected = opts.deCleansing;
	choices[73].selected = opts.meleeObstacles;
	choices[74].selected = opts.showVisitedStars;
	choices[75].selected = opts.unscaledStarSystem;
	choices[76].selected = opts.sphereType;
	choices[77].selected = opts.slaughterMode;
	choices[78].selected = opts.advancedAutoPilot;
	choices[79].selected = opts.meleeToolTips;
	choices[80].selected = opts.musicResume;
	choices[81].selected = opts.windowType;
	choices[82].selected = opts.seedType;
	choices[83].selected = opts.sphereColors;
	choices[84].selected = opts.scatterElements;
	choices[85].selected = opts.showUpgrades;
	choices[86].selected = opts.fleetPointSys;

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

	sliders[0].value = opts.musicvol;
	sliders[1].value = opts.sfxvol;
	sliders[2].value = opts.speechvol;
	sliders[3].value = opts.gamma;
	sliders[4].value = opts.nebulaevol;
}

static void
PropagateResults (void)
{
	GLOBALOPTS opts;
	BYTE i;

	opts.screenResolution = choices[0].selected;
	opts.driver = choices[1].selected;
	opts.scaler = choices[2].selected;
	opts.scanlines = choices[3].selected;
	opts.menu = choices[4].selected;
	opts.text = choices[5].selected;
	opts.cscan = choices[6].selected;
	opts.scroll = choices[7].selected;
	opts.subtitles = choices[8].selected;
	opts.music3do = choices[9].selected;
	opts.fullscreen = choices[10].selected;
	opts.intro = choices[11].selected;
	opts.fps = choices[12].selected;
#if !(defined(ANDROID) || defined(__ANDROID__))
	opts.meleezoom = choices[13].selected;
#endif
	opts.stereo = choices[14].selected;
	opts.adriver = choices[15].selected;
	opts.aquality = choices[16].selected;
	opts.shield = choices[17].selected;
	opts.player1 = choices[18].selected;
	opts.player2 = choices[19].selected;
	opts.musicremix = choices[21].selected;
	opts.speech = choices[22].selected;
	opts.keepaspect = choices[23].selected;
 	opts.cheatMode = choices[24].selected;
	opts.godModes = choices[25].selected;
	opts.tdType = choices[26].selected;
	opts.bubbleWarp = choices[27].selected;
	opts.unlockShips = choices[28].selected;
	opts.headStart = choices[29].selected;
	opts.unlockUpgrades = choices[30].selected;
	opts.infiniteRU = choices[31].selected;
	opts.skipIntro = choices[32].selected;
	opts.fuelRange = choices[33].selected;
	opts.mainMenuMusic = choices[34].selected;
	opts.nebulae = choices[35].selected;
	opts.orbitingPlanets = choices[36].selected;
	opts.texturedPlanets = choices[37].selected;
	opts.dateType = choices[38].selected;
	opts.infiniteFuel = choices[39].selected;
	opts.partialPickup = choices[40].selected;
	opts.submenu = choices[41].selected;
	opts.loresBlowup = choices[42].selected;
	opts.addDevices = choices[43].selected;
	opts.hazardColors = choices[44].selected;
	opts.customBorder = choices[45].selected;
	opts.spaceMusic = choices[46].selected;
	opts.volasMusic = choices[47].selected;
	opts.wholeFuel = choices[48].selected;
#if defined(ANDROID) || defined(__ANDROID__)
	opts.directionalJoystick = choices[49].selected;
	opts.meleezoom = choices[50].selected;
#endif
	opts.landerHold = choices[51].selected;
	opts.scrTrans = choices[52].selected;
	opts.difficulty = choices[53].selected;
	opts.extended = choices[54].selected;
	opts.nomad = choices[55].selected;
	opts.gameOver = choices[56].selected;
	opts.shipDirectionIP = choices[57].selected;
	opts.orzCompFont = choices[58].selected;
	opts.controllerType = choices[59].selected;
	opts.smartAutoPilot = choices[60].selected;
	opts.tintPlanSphere = choices[61].selected;
	opts.planetStyle = choices[62].selected;
	opts.starBackground = choices[63].selected;
	opts.scanStyle = choices[64].selected;
	opts.nonStopOscill = choices[65].selected;
	opts.scopeStyle = choices[66].selected;
	opts.hyperStars = choices[67].selected;
	opts.landerStyle = choices[68].selected;
	opts.planetTexture = choices[69].selected;
	opts.flagshipColor = choices[70].selected;
	opts.noHQEncounters = choices[71].selected;
	opts.deCleansing = choices[72].selected;
	opts.meleeObstacles = choices[73].selected;
	opts.showVisitedStars = choices[74].selected;
	opts.unscaledStarSystem = choices[75].selected;
	opts.sphereType = choices[76].selected;
	opts.slaughterMode = choices[77].selected;
	opts.advancedAutoPilot = choices[78].selected;
	opts.meleeToolTips = choices[79].selected;
	opts.musicResume = choices[80].selected;
	opts.windowType = choices[81].selected;
	opts.seedType = choices[82].selected;
	opts.sphereColors = choices[83].selected;
	opts.scatterElements = choices[84].selected;
	opts.showUpgrades = choices[85].selected;
	opts.fleetPointSys = choices[86].selected;

	// Devices
	for (i = DEVICE_START; i < DEVICE_START + NUM_DEVICES; i++)
	{
		opts.deviceArray[i - DEVICE_START] = choices[i].selected;
	}

	for (i = UPGRADE_START; i < UPGRADE_START + NUM_UPGRADES; i++)
	{
		opts.upgradeArray[i - UPGRADE_START] = choices[i].selected;
	}

	opts.musicvol = sliders[0].value;
	opts.sfxvol = sliders[1].value;
	opts.speechvol = sliders[2].value;
	opts.gamma = sliders[3].value;
	opts.nebulaevol = sliders[4].value;
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
	int templat = choices[20].selected;
	int control = widget->controlindex;
	int index = widget->highlighted;

	FlushInput ();
	DrawLabelAsWindow (&labels[3], NULL);
	RebindInputState (templat, control, index);
	populate_editkeys (templat);
	FlushInput ();
}

static void
clear_control (WIDGET_CONTROLENTRY *widget)
{
	int templat = choices[20].selected;
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

	// Choices 18-20 are also special, being the names of the key
	// configurations
	for (i = 0; i < 6; i++)
	{
		choices[18].options[i].optname = input_templates[i].name;
		choices[19].options[i].optname = input_templates[i].name;
		choices[20].options[i].optname = input_templates[i].name;
	}

	/* Choice 20 has a special onChange handler, too. */
	choices[20].onChange = change_template;

	// Check addon availability for HD mode, DOS/3DO mode, and music remixes
	choices[ 0].onChange = check_availability;
	choices[ 9].onChange = check_availability;
	choices[21].onChange = check_availability;
	choices[46].onChange = check_availability;
	choices[47].onChange = check_availability;
	choices[81].onChange = check_availability;

	// Handle display option
	choices[ 1].onChange = process_graphics_options;
	choices[ 2].onChange = process_graphics_options;
	choices[ 3].onChange = process_graphics_options;
	choices[10].onChange = process_graphics_options;
	choices[12].onChange = process_graphics_options;
	choices[23].onChange = process_graphics_options;
	choices[42].onChange = process_graphics_options;

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
	sliders[0].onChange = adjustMusic;
	sliders[1].onChange = adjustSFX;
	sliders[2].onChange = adjustSpeech;

	// gamma is a special case
	sliders[3].step = 1;
	sliders[3].handleEvent = gamma_HandleEventSlider;
	sliders[3].draw_value = gamma_DrawValue;

	// nebulaevol is a special case
	sliders[4].step = 1;
	sliders[4].max = 50;

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

	textentries[0].onChange = rename_template;
	textentries[1].onChange = change_seed;
	textentries[2].onChange = change_res;

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
		((ScreenWidthActual == 320) || (ScreenWidthActual == 640))) ||
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
	opts->unlockUpgrades = optUnlockUpgrades;
	opts->addDevices = optAddDevices;
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

		if (nh != ScreenHeightActual)
		{
			ScreenHeightActual = nh;
			res_PutInteger("config.resheight", ScreenHeightActual);
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
		int customSeed = atoi (textentries[1].value);
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
	PutBoolOpt (&optUnlockUpgrades, &opts->unlockUpgrades, "cheat.unlockUpgrades", FALSE);
	PutBoolOpt (&optAddDevices, &opts->addDevices, "cheat.addDevices", FALSE);
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
		ScreenWidth = 320 << resolutionFactor;
		ScreenHeight = DOS_BOOL (240, 200) << resolutionFactor;

		log_add (log_Debug, "ScreenWidth:%d, ScreenHeight:%d, "
				"Wactual:%d, Hactual:%d", ScreenWidth, ScreenHeight,
				ScreenWidthActual, ScreenHeightActual);

		// These solve the context problem that plagued the setupmenu
		// when changing to higher resolution.
		TFB_BBox_Reset ();
		TFB_BBox_Init (ScreenWidth, ScreenHeight);
		FlushColorXForms ();

		TFB_DrawScreen_ReinitVideo (GraphicsDriver, GfxFlags,
				ScreenWidthActual, ScreenHeightActual);
		InitVideoPlayer (TRUE);

		Reload ();
	}
}
