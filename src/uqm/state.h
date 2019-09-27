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

#ifndef UQM_STATE_H_
#define UQM_STATE_H_

#include "port.h"
#include "libs/compiler.h"
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void InitPlanetInfo (void);
extern void UninitPlanetInfo (void);
extern void GetPlanetInfo (void);
extern void PutPlanetInfo (void);

extern void InitGroupInfo (BOOLEAN FirstTime);
extern void UninitGroupInfo (void);
extern BOOLEAN GetGroupInfo (DWORD offset, BYTE which_group);
extern DWORD PutGroupInfo (DWORD offset, BYTE which_group);
#define GROUPS_RANDOM  ((DWORD)(0L))
#define GROUPS_ADD_NEW ((DWORD)(~0L))
#define GROUP_LIST     ((BYTE)0)
#define GROUP_INIT_IP  ((BYTE)~0)
		// Initialize IP group list (ip_group_q) from the actual groups
		// (not GROUP_LIST) in one of the state files
#define GROUP_LOAD_IP  GROUP_LIST
		// Read IP group list into ip_group_q from the list entry
		// (GROUP_LIST) in one of the state files
#define GROUP_SAVE_IP  ((BYTE)~0)
		// Write IP group list from ip_group_q to the list entry
		// (GROUP_LIST) in one of the state files
extern void BuildGroups (void);
extern void findRaceSOI(void);

typedef struct GAME_STATE_FILE GAME_STATE_FILE;

#define STARINFO_FILE 0
	//"starinfo.dat"
#define STAR_BUFSIZE (NUM_SOLAR_SYSTEMS * sizeof (DWORD) \
		+ 3800 * (3 * sizeof (DWORD)))
#define RANDGRPINFO_FILE 1
	//"randgrp.dat"
#define RAND_BUFSIZE (4 * 1024)
#define DEFGRPINFO_FILE 2
	//"defgrp.dat"
#define DEF_BUFSIZE (10 * 1024)

typedef enum
{
	STARINFO,
	RANDGRPINFO,
	DEFGRPINFO
} INFO_TYPE;

GAME_STATE_FILE* OpenStateFile (int stateFile, const char *mode);
void CloseStateFile (GAME_STATE_FILE *fp);
void DeleteStateFile (int stateFile);
DWORD LengthStateFile (GAME_STATE_FILE *fp);
int ReadStateFile (void *lpBuf, COUNT size, COUNT count, GAME_STATE_FILE *fp);
int WriteStateFile (const void *lpBuf, COUNT size, COUNT count, GAME_STATE_FILE *fp);
int SeekStateFile (GAME_STATE_FILE *fp, long offset, int whence);

static inline COUNT
sread_8 (GAME_STATE_FILE *fp, BYTE *v)
{
	BYTE t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadStateFile (v, 1, 1, fp);
}

static inline COUNT
sread_16 (GAME_STATE_FILE *fp, UWORD *v)
{
	UWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadStateFile (v, 2, 1, fp);
}

static inline COUNT
sread_16s (GAME_STATE_FILE *fp, SWORD *v)
{
	UWORD t;
	COUNT ret;
	ret = sread_16 (fp, &t);
	// unsigned to signed conversion
	if (v)
		*v = t;
	return ret;
}

static inline COUNT
sread_32 (GAME_STATE_FILE *fp, DWORD *v)
{
	DWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadStateFile (v, 4, 1, fp);
}

static inline COUNT
sread_a32 (GAME_STATE_FILE *fp, DWORD *ar, COUNT count)
{
	assert (ar != NULL);

	for ( ; count > 0; --count, ++ar)
	{
		if (sread_32 (fp, ar) != 1)
			return 0;
	}
	return 1;
}

static inline COUNT
swrite_8 (GAME_STATE_FILE *fp, BYTE v)
{
	return WriteStateFile (&v, 1, 1, fp);
}

static inline COUNT
swrite_16 (GAME_STATE_FILE *fp, UWORD v)
{
	return WriteStateFile (&v, 2, 1, fp);
}

static inline COUNT
swrite_32 (GAME_STATE_FILE *fp, DWORD v)
{
	return WriteStateFile (&v, 4, 1, fp);
}

static inline COUNT
swrite_a32 (GAME_STATE_FILE *fp, const DWORD *ar, COUNT count)
{
	for ( ; count > 0; --count, ++ar)
	{
		if (swrite_32 (fp, *ar) != 1)
			return 0;
	}
	return 1;
}

#if defined(__cplusplus)
}
#endif

#endif /* UQM_STATE_H_ */

