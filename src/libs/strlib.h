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

#ifndef LIBS_STRLIB_H_
#define LIBS_STRLIB_H_

#include "libs/compiler.h"
#include "port.h"
#include "libs/uio.h"
#include "libs/unicode.h"

#include <stddef.h>

typedef struct string_table_entry STRING_TABLE_ENTRY_DESC;
typedef struct string_table STRING_TABLE_DESC;

typedef STRING_TABLE_DESC *STRING_TABLE;
typedef STRING_TABLE_ENTRY_DESC *STRING;
typedef char *STRINGPTR;

/* This has to go here because reslib requires the above typedefs. */
#include "libs/reslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern BOOLEAN InstallStringTableResType (void);
extern STRING_TABLE LoadStringTableInstance (RESOURCE res);
extern STRING_TABLE LoadStringTableFile (uio_DirHandle *dir,
		const char *fileName);
extern BOOLEAN DestroyStringTable (STRING_TABLE StringTable);
extern STRING CaptureStringTable (STRING_TABLE StringTable);
extern STRING_TABLE ReleaseStringTable (STRING String);
extern STRING_TABLE GetStringTable (STRING String);
extern COUNT GetStringTableCount (STRING String);
extern COUNT GetStringTableIndex (STRING String);
extern STRING SetAbsStringTableIndex (STRING String, COUNT
		StringTableIndex);
extern STRING SetRelStringTableIndex (STRING String, SIZE
		StringTableOffs);
extern COUNT GetStringLength (STRING String);
extern COUNT GetStringLengthBin (STRING String);
extern STRINGPTR GetStringAddress (STRING String);
extern STRINGPTR GetStringName (STRING String);
extern STRINGPTR GetStringSoundClip (STRING String);
extern STRINGPTR GetStringTimeStamp (STRING String);
extern STRING GetStringByName (STRING_TABLE StringTable, const char *index);

#define UNICHAR_DEGREE_SIGN   0x00b0
#define STR_DEGREE_SIGN     "\xC2\xB0"
#define UNICHAR_INFINITY_SIGN 0x221e
#define STR_INFINITY_SIGN   "\xE2\x88\x9E"
#define UNICHAR_EARTH_SIGN    0x2641
#define STR_EARTH_SIGN      "\xE2\x99\x81"
#define UNICHAR_MIDDLE_DOT    0x00b7
#define STR_MIDDLE_DOT      "\xC2\xB7"
#define UNICHAR_BULLET        0x2022
#define STR_BULLET          "\xE2\x80\xA2"

#if defined(__cplusplus)
}
#endif

#endif /* LIBS_STRLIB_H_ */

