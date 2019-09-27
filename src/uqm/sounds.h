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

#ifndef UQM_SOUNDS_H_
#define UQM_SOUNDS_H_

#include "element.h"
#include "libs/compiler.h"
#include "libs/sndlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum
{
	GRAB_CREW = 0,
	SHIP_EXPLODES,
	TARGET_DAMAGED_FOR_1_PT,
	TARGET_DAMAGED_FOR_2_3_PT,
	TARGET_DAMAGED_FOR_4_5_PT,
	TARGET_DAMAGED_FOR_6_PLUS_PT
} SOUND_EFFECTS;

typedef enum
{
	MENU_SOUND_MOVE = 0,
	MENU_SOUND_SUCCESS,
	MENU_SOUND_FAILURE,
	MENU_SOUND_INVOKED,
	MENU_SOUND_BUBBLEWARP,
} MENU_SOUND_EFFECT;

extern SOUND MenuSounds;
extern SOUND GameSounds;

/* Constants for DoInput */
typedef UWORD MENU_SOUND_FLAGS;
#define MENU_SOUND_UP       ((MENU_SOUND_FLAGS)(1 << 0))
#define MENU_SOUND_DOWN     ((MENU_SOUND_FLAGS)(1 << 1))
#define MENU_SOUND_LEFT     ((MENU_SOUND_FLAGS)(1 << 2))
#define MENU_SOUND_RIGHT    ((MENU_SOUND_FLAGS)(1 << 3))
#define MENU_SOUND_SELECT   ((MENU_SOUND_FLAGS)(1 << 4))
#define MENU_SOUND_CANCEL   ((MENU_SOUND_FLAGS)(1 << 5))
#define MENU_SOUND_SPECIAL  ((MENU_SOUND_FLAGS)(1 << 6))
#define MENU_SOUND_PAGEUP   ((MENU_SOUND_FLAGS)(1 << 7))
#define MENU_SOUND_PAGEDOWN ((MENU_SOUND_FLAGS)(1 << 8))
#define MENU_SOUND_DELETE   ((MENU_SOUND_FLAGS)(1 << 9))
#define MENU_SOUND_ARROWS   (MENU_SOUND_UP | MENU_SOUND_DOWN | MENU_SOUND_LEFT | MENU_SOUND_RIGHT)
#define MENU_SOUND_PAGE		(MENU_SOUND_PAGEUP | MENU_SOUND_PAGEDOWN)
#define MENU_SOUND_ACTION	(MENU_SOUND_SELECT | MENU_SOUND_CANCEL)
#define MENU_SOUND_NONE     ((MENU_SOUND_FLAGS)0)

extern void SetMenuSounds (MENU_SOUND_FLAGS sound_0,
		MENU_SOUND_FLAGS sound_1);
extern void GetMenuSounds (MENU_SOUND_FLAGS *sound_0,
		MENU_SOUND_FLAGS *sound_1);

extern void PlaySound (SOUND S, SoundPosition Pos,
		ELEMENT *PositionalObject, BYTE Priority);
extern void PlayMenuSound (MENU_SOUND_EFFECT S);
extern void ProcessSound (SOUND Sound, ELEMENT *PositionalObject);
extern SoundPosition CalcSoundPosition (ELEMENT *ElementPtr);
extern SoundPosition NotPositional (void);
extern void UpdateSoundPositions (void);
extern void FlushSounds (void);
extern void RemoveSoundsForObject (ELEMENT *PosObj);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_SOUNDS_H_ */


