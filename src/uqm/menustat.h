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

#ifndef UQM_MENUSTAT_H_
#define UQM_MENUSTAT_H_

#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "flash.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct menu_state
{
	// Standard field required by DoInput()
	BOOLEAN (*InputFunc) (struct menu_state *pMS);

	SIZE Initialized;

	BYTE CurState;
	FRAME CurFrame;
	STRING CurString;
	POINT first_item;
	SIZE delta_item;

	FRAME ModuleFrame;
	RECT flash_rect0, flash_rect1;
	FRAME flash_frame0, flash_frame1;
	FlashContext *flashContext;

	MUSIC_REF hMusic;

	// For private use by various menus
	// Usually, a menu associates its internal data struct using this
	void *privData;

} MENU_STATE;

// XXX: Should probably go to menu.h (does not yet exist)
enum
{
	PM_SCAN = 0,
	PM_STARMAP,
	PM_DEVICES,
	PM_CARGO,
	PM_ROSTER,
	PM_GAME_MENU,
	PM_NAVIGATE,
	
	PM_MIN_SCAN,
	PM_ENE_SCAN,
	PM_BIO_SCAN,
	PM_EXIT_SCAN,
	PM_AUTO_SCAN,
	PM_LAUNCH_LANDER,

	PM_SAVE_GAME,
	PM_LOAD_GAME,
	PM_QUIT_GAME,
	PM_CHANGE_SETTINGS,
	PM_EXIT_GAME_MENU,

	PM_CONVERSE,
	PM_ATTACK,
	PM_ENCOUNTER_GAME_MENU,
	
	PM_FUEL,
	PM_MODULE,
	PM_OUTFIT_GAME_MENU,
	PM_EXIT_OUTFIT,
	
	PM_CREW,
	PM_SHIPYARD_GAME_MENU,
	PM_EXIT_SHIPYARD,
	
	PM_SOUND_ON,
	PM_SOUND_OFF,
	PM_MUSIC_ON,
	PM_MUSIC_OFF,
	PM_CYBORG_OFF,
	PM_CYBORG_NORMAL,
	PM_CYBORG_DOUBLE,
	PM_CYBORG_SUPER,
	PM_CHANGE_CAPTAIN,
	PM_CHANGE_SHIP,
	PM_EXIT_SETTINGS,

	PM_ALT_SCAN,
	PM_ALT_STARMAP,
	PM_ALT_MANIFEST,
	PM_ALT_GAME_MENU,
	PM_ALT_NAVIGATE,

	PM_ALT_CARGO,
	PM_ALT_DEVICES,
	PM_ALT_ROSTER,
	PM_ALT_EXIT_MANIFEST,

	PM_ALT_MSCAN,
	PM_ALT_ESCAN,
	PM_ALT_BSCAN,
	PM_ALT_ASCAN,
	PM_ALT_DISPATCH,
	PM_ALT_EXIT_SCAN,
};

extern BOOLEAN DoMenuChooser (MENU_STATE *pMS, BYTE BaseState);
extern void DrawMenuStateStrings (BYTE beg_index, SWORD NewState);
extern void DrawSubmenu (BYTE Visible);
extern void DrawBorder(BYTE Visible, BOOLEAN InBattle);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_MENUSTAT_H_ */