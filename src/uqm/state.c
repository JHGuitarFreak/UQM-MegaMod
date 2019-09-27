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

#include "state.h"

#include "starmap.h"
#include "libs/memlib.h"
#include "libs/log.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <memory.h>

// in-memory file i/o
struct GAME_STATE_FILE
{
	const char *symname;
	DWORD size_hint;
	int   open_count;
	BYTE *data;
	DWORD used;
	DWORD size;
	DWORD ptr;
};
#define STATE_FILE_ITRAILER  0, 0, 0, 0, 0

#define NUM_STATE_FILES 3

static GAME_STATE_FILE state_files[NUM_STATE_FILES] =
{
	{"STARINFO",    STAR_BUFSIZE, STATE_FILE_ITRAILER},
	{"RANDGRPINFO", RAND_BUFSIZE, STATE_FILE_ITRAILER},
	{"DEFGRPINFO",  DEF_BUFSIZE,  STATE_FILE_ITRAILER}
};


GAME_STATE_FILE *
OpenStateFile (int stateFile, const char *mode)
{
	GAME_STATE_FILE *fp;

	if (stateFile < 0 || stateFile >= NUM_STATE_FILES)
		return NULL;
	
	fp = &state_files[stateFile];
	fp->open_count++;
	if (fp->open_count > 1)
		log_add (log_Warning, "WARNING: "
				"State file %s open count is %d after open()",
				fp->symname, fp->open_count);
	
	if (!fp->data)
	{
		fp->data = HMalloc (fp->size_hint);
		if (!fp->data)
			return NULL;
		fp->size = fp->size_hint;
	}

	// we allow reading and writing for any open mode
	// but the mode determines what happens to the file contents
	if (mode[0] == 'w')
	{	// blow the file away
		fp->used = 0;
#ifdef DEBUG
		// paint buffer for tracking writes
		memset (fp->data, 0xCC, fp->size);
#endif
	}
	else if (mode[0] == 'r')
	{	// nothing
	}
	else
	{
		log_add (log_Warning, "WARNING: "
				"State file %s opened with unsupported mode '%s'",
				fp->symname, mode);
	}
	fp->ptr = 0;
	
	return fp;
}

void
CloseStateFile (GAME_STATE_FILE *fp)
{
	fp->ptr = 0;
	fp->open_count--;
	if (fp->open_count < 0)
		log_add (log_Warning, "WARNING: "
				"State file %s open count is %d after close()",
				fp->symname, fp->open_count);
	// Erm, Ok, it's closed! Honest!
}

void
DeleteStateFile (int stateFile)
{
	GAME_STATE_FILE *fp;

	if (stateFile < 0 || stateFile >= NUM_STATE_FILES)
		return;

	fp = &state_files[stateFile];
	if (fp->open_count != 0)
		log_add (log_Warning, "WARNING: "
				"State file %s open count is %d during delete()",
				fp->symname, fp->open_count);

	fp->used = 0;
	fp->ptr = 0;
	HFree (fp->data);
	fp->data = 0;
}

DWORD
LengthStateFile (GAME_STATE_FILE *fp)
{
	return fp->used;
}

int
ReadStateFile (void *lpBuf, COUNT size, COUNT count, GAME_STATE_FILE *fp)
{
	DWORD bytes = size * count;

	if (fp->ptr >= fp->size)
	{	// EOF
		return 0;
	}
	else if (fp->ptr + bytes > fp->size)
	{	// dont have that much data
		bytes = fp->size - fp->ptr;
		bytes -= bytes % size;
	}
	
	if (bytes > 0)
	{
		memcpy (lpBuf, fp->data + fp->ptr, bytes);
		fp->ptr += bytes;
	}
	return (bytes / size);
}

int
WriteStateFile (const void *lpBuf, COUNT size, COUNT count, GAME_STATE_FILE *fp)
{
	DWORD bytes = size * count;
	
	if (fp->ptr + bytes > fp->size)
	{	// dont have that much space available
		DWORD newsize = fp->ptr + bytes;
		// grab more space in advance
		if (newsize < fp->size * 3 / 2)
			newsize = fp->size * 3 / 2;

		fp->data = HRealloc (fp->data, newsize);
		if (!fp->data)
			return 0;
		
		fp->size = newsize;
		if (newsize > fp->size_hint)
			fp->size_hint = newsize;
	}
	
	if (bytes > 0)
	{
		memcpy (fp->data + fp->ptr, lpBuf, bytes);
		fp->ptr += bytes;
		if (fp->ptr > fp->used)
			fp->used = fp->ptr;
	}
	return (bytes / size);
}

int
SeekStateFile (GAME_STATE_FILE *fp, long offset, int whence)
{
	if (whence == SEEK_CUR)
		offset += fp->ptr;
	else if (whence == SEEK_END)
		offset += fp->used;

	if (offset < 0)
	{
		fp->ptr = 0;
		return 0;
	}
	fp->ptr = offset;
	return 1;
}


void
InitPlanetInfo (void)
{
	GAME_STATE_FILE *fp;

	fp = OpenStateFile (STARINFO_FILE, "wb");
	if (fp)
	{
		STAR_DESC *pSD;

		// Set record offsets for all stars to 0 (not present)
		pSD = &star_array[0];
		do
		{
			swrite_32 (fp, 0);
			++pSD;
		} while (pSD->star_pt.x <= MAX_X_UNIVERSE
				&& pSD->star_pt.y <= MAX_Y_UNIVERSE);

		CloseStateFile (fp);
	}
}

void
UninitPlanetInfo (void)
{
	DeleteStateFile (STARINFO_FILE);
}

#define OFFSET_SIZE       (sizeof (DWORD))
//#define SCAN_RECORD_SIZE  (sizeof (DWORD) * NUM_SCAN_TYPES)
// JMS: Increased the size of scan record to house partially scavenged minerals.
#define SCAN_RECORD_SIZE  ((sizeof (DWORD) * NUM_SCAN_TYPES) + (sizeof(BYTE) * NUM_SCAN_TYPES * 32))

void
GetPlanetInfo (void)
{
	GAME_STATE_FILE *fp;
	COUNT k,l;

	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[BIOLOGICAL_SCAN] = 0;
	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[MINERAL_SCAN] = 0;
	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN] = 0;

	// JMS: Init also the partially scavenged mineral deposit values.
	for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
		for (k = 0; k < 32; k++)
			pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[l][k] = 0;

	fp = OpenStateFile (STARINFO_FILE, "rb");
	if (fp)
	{
		COUNT star_index, planet_index, moon_index;
		DWORD offset;

		star_index = (COUNT)(CurStarDescPtr - star_array);
		planet_index = (COUNT)(pSolarSysState->pBaseDesc->pPrevDesc
				- pSolarSysState->PlanetDesc);
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == pSolarSysState->SunDesc)
			moon_index = 0;
		else
			moon_index = (COUNT)(pSolarSysState->pOrbitalDesc
					- pSolarSysState->MoonDesc + 1);

		SeekStateFile (fp, star_index * OFFSET_SIZE, SEEK_SET);
		sread_32 (fp, &offset);

		if (offset)
		{
			COUNT i;

			// Skip scan records for all preceeding planets to the one we need
			for (i = 0; i < planet_index; ++i)
				offset += (pSolarSysState->PlanetDesc[i].NumPlanets + 1) *
						SCAN_RECORD_SIZE;
				
			// Skip scan records for all preceeding moons to the one we need
			offset += moon_index * SCAN_RECORD_SIZE;

			SeekStateFile (fp, offset, SEEK_SET);
			sread_a32 (fp, pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask,
					NUM_SCAN_TYPES);

			{
				BYTE *ar = &(pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[0][0]); // JMS
				
				// JMS: Read which mineral deposits are partially retrieved (and how much).
				for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
					for (k = 0; k < 32; k++, ar++)
						sread_8 (fp, ar);
			}
		}

		CloseStateFile (fp);
	}
}

void
PutPlanetInfo (void)
{
	GAME_STATE_FILE *fp;

	fp = OpenStateFile (STARINFO_FILE, "r+b");
	if (fp)
	{
		COUNT i, k, l;
		COUNT star_index, planet_index, moon_index;
		DWORD offset;

		star_index = (COUNT)(CurStarDescPtr - star_array);
		planet_index = (COUNT)(pSolarSysState->pBaseDesc->pPrevDesc
				- pSolarSysState->PlanetDesc);
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == pSolarSysState->SunDesc)
			moon_index = 0;
		else
			moon_index = (COUNT)(pSolarSysState->pOrbitalDesc
					- pSolarSysState->MoonDesc + 1);

		SeekStateFile (fp, star_index * OFFSET_SIZE, SEEK_SET);
		sread_32 (fp, &offset);

		if (offset == 0)
		{	// Scan record not present yet -- init it
			DWORD ScanRetrieveMask[NUM_SCAN_TYPES] =
			{
				0, 0, 0,
			};

			// JMS: Init also the partially scavenged mineral deposit values.
			BYTE PartiallyScavengedList[NUM_SCAN_TYPES][32];
			for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
				for (k = 0; k < 32; k++)
					PartiallyScavengedList[l][k] = 0;

			offset = LengthStateFile (fp);

			// Write the record offset
			SeekStateFile (fp, star_index * OFFSET_SIZE, SEEK_SET);
			swrite_32 (fp, offset);

			// Init scan records for all planets and moons in the system
			SeekStateFile (fp, offset, SEEK_SET);
			for (i = 0; i < pSolarSysState->SunDesc[0].NumPlanets; ++i)
			{
				COUNT j;
				BYTE *ar = &(PartiallyScavengedList[0][0]); // JMS

				swrite_a32 (fp, ScanRetrieveMask, NUM_SCAN_TYPES);

				// JMS: Also init with zeroes the list of partially scavenged mineral amounts.
				for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
					for (k = 0; k < 32; k++, ar++)
						swrite_8 (fp, *ar);

				// init moons
				for (j = 0; j < pSolarSysState->PlanetDesc[i].NumPlanets; ++j) {
					BYTE *ar = &(PartiallyScavengedList[0][0]); // JMS
					
 					swrite_a32 (fp, ScanRetrieveMask, NUM_SCAN_TYPES);
					
					// JMS: Ditto for the moons.
					for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
						for (k = 0; k < 32; k++, ar++)
							swrite_8 (fp, *ar);
				}
			}
		}

		// Skip scan records for all preceeding planets to the one we need
		for (i = 0; i < planet_index; ++i)
			offset += (pSolarSysState->PlanetDesc[i].NumPlanets + 1) *
					SCAN_RECORD_SIZE;
				
		// Skip scan records for all preceeding moons to the one we need
		offset += moon_index * SCAN_RECORD_SIZE;

		SeekStateFile (fp, offset, SEEK_SET);

		// Store which mineral deposits we have already retrieved.
		swrite_a32 (fp, pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask,
				NUM_SCAN_TYPES);

		{
			BYTE *ar = &(pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[0][0]); // JMS
			
			// JMS: Store which mineral deposits are partially retrieved (and how much).
			for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
				for (k = 0; k < 32; k++, ar++)
					swrite_8 (fp, *ar);
		}

		CloseStateFile (fp);
	}
}

