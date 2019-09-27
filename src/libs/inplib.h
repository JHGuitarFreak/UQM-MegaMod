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

#ifndef LIBS_INPLIB_H_
#define LIBS_INPLIB_H_

#include <stddef.h>
#include "libs/compiler.h"
#include "libs/uio.h"
#include "libs/unicode.h"

#if defined(__cplusplus)
extern "C" {
#endif


extern BOOLEAN AnyButtonPress (BOOLEAN DetectSpecial);

extern void TFB_ResetControls (void);

/*
 * Not used right now
extern BOOLEAN FindMouse (void);
extern void MoveMouse (SWORD x, SWORD y);
extern BYTE LocateMouse (SWORD *px, SWORD *py);
*/

extern volatile int MouseButtonDown;
extern volatile int QuitPosted;
extern volatile int GameActive;

/* Functions for dealing with Character Mode */

void EnterCharacterMode (void);
void ExitCharacterMode (void);
UniChar GetNextCharacter (void);
UniChar GetLastCharacter (void);

/* Interrogating the current key configuration */

void InterrogateInputState (int templat, int control, int index, char *buffer, int maxlen);
void RemoveInputState (int templat, int control, int index);
void RebindInputState (int templat, int control, int index);

void SaveKeyConfiguration (uio_DirHandle *path, const char *fname);

/* Separate inputs into frames for dealing with very fast inputs */

void BeginInputFrame (void);

#if defined(__cplusplus)
}
#endif

#endif /* LIBS_INPLIB_H_ */

