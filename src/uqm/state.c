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
#include "gamestr.h"

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

#define OFFSET_SIZE           (sizeof (DWORD))
#define CORE_SCAN_RECORD_SIZE (sizeof (DWORD) * NUM_SCAN_TYPES)
#define SCAN_RECORD_SIZE      (CORE_SCAN_RECORD_SIZE + (NUM_SCAN_TYPES * 32))

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
				BYTE *ar = &(pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[0][0]);
				
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
				BYTE *ar = &(PartiallyScavengedList[0][0]);

				swrite_a32 (fp, ScanRetrieveMask, NUM_SCAN_TYPES);

				// JMS: Also init with zeroes the list of partially scavenged mineral amounts.
				for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
					for (k = 0; k < 32; k++, ar++)
						swrite_8 (fp, *ar);

				// init moons
				for (j = 0; j < pSolarSysState->PlanetDesc[i].NumPlanets; ++j) {
					BYTE *ar = &(PartiallyScavengedList[0][0]);
					
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
			BYTE *ar = &(pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[0][0]);
			
			// JMS: Store which mineral deposits are partially retrieved (and how much).
			for (l = MINERAL_SCAN; l < NUM_SCAN_TYPES; l++)
				for (k = 0; k < 32; k++, ar++)
					swrite_8 (fp, *ar);
		}

		CloseStateFile (fp);
	}
}

#define STARINFO_TABLE_SIZE (NUM_SOLAR_SYSTEMS * sizeof(DWORD))
#define SCAN_RECORD_DIFF (SCAN_RECORD_SIZE - CORE_SCAN_RECORD_SIZE)

void
ProcessVanillaStarInfo (GAME_STATE_FILE *fp)
{
	BYTE *fp_data, *old_records, *new_records;
	DWORD i, num_entries, *star_offs, *old_star_offs;
	DWORD old_table_size, new_table_size;

	if (!fp || !fp->data || fp->used == 0)
		return;

	old_table_size = fp->used - STARINFO_TABLE_SIZE;

#ifdef STARINFO_DEBUG
	printf ("fp->used %d, old_table_size %d\n", fp->used, old_table_size);
#endif

	if (old_table_size == 0 || old_table_size % SCAN_RECORD_SIZE == 0)
		return; // Table either empty or already compliant

	if (old_table_size % CORE_SCAN_RECORD_SIZE != 0)
	{
		log_add (log_Error, "STARINFO table is corrupt");
		return;
	}

	num_entries = old_table_size / CORE_SCAN_RECORD_SIZE;
	new_table_size = STARINFO_TABLE_SIZE + (num_entries * SCAN_RECORD_SIZE);

#ifdef STARINFO_DEBUG
	printf ("num_entries %d, new_table_size %d\n",
			num_entries, new_table_size);
#endif

	fp_data = HMalloc (new_table_size);
	if (!fp_data)
		return;

	// Copy the original table header to fp_data
	memcpy (fp_data, fp->data, STARINFO_TABLE_SIZE);

	star_offs = (DWORD *)fp_data;
	old_star_offs = (DWORD *)fp->data;

	// Skip past the table header to work directly with the record entries
	old_records = fp->data + STARINFO_TABLE_SIZE;
	new_records = fp_data + STARINFO_TABLE_SIZE;

	for (i = 0; i < num_entries; i++)
	{
		// Zero out the entire record for this entry
		memset (new_records, 0, SCAN_RECORD_SIZE);
		// Copy the data from the old record to the new record
		memcpy (new_records, old_records, CORE_SCAN_RECORD_SIZE);

#ifdef STARINFO_DEBUG
		printf ("[%03d]: ", i);
		printf ("Min 0x%08X Ene 0x%08X Bio 0x%08X : ",
				((DWORD *)old_records)[0], ((DWORD *)old_records)[1],
				((DWORD *)old_records)[2]);
		printf ("Min 0x%08X Ene 0x%08X Bio 0x%08X\n",
				((DWORD *)new_records)[0], ((DWORD *)new_records)[1],
				((DWORD *)new_records)[2]);
#endif

		old_records += CORE_SCAN_RECORD_SIZE;
		new_records += SCAN_RECORD_SIZE;
	}

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
		DWORD entry_num;

		if (old_star_offs[i] == 0)
			continue;

		// This star's original table offset
		entry_num = (old_star_offs[i] - STARINFO_TABLE_SIZE)
				/ CORE_SCAN_RECORD_SIZE;

		// Update it to point to the new offset
		star_offs[i] = STARINFO_TABLE_SIZE + (entry_num * SCAN_RECORD_SIZE);

#ifdef STARINFO_DEBUG
		printf ("[%03d] 0x%04X : 0x%04X\n",
				i, old_star_offs[i], star_offs[i]);
#endif
	}

	HFree (fp->data);
	fp->data = fp_data;
	fp->size = new_table_size;
	fp->used = new_table_size;
	fp->ptr = 0;
}

void MinedStarSystems (GAME_STATE_FILE *fp)
{
	DWORD i, *data;

	if (!fp || !fp->data || fp->used < STARINFO_TABLE_SIZE)
		return;

	data = (DWORD *)fp->data;

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
		if (data[i] != 0)
		{
#ifdef STARINFO_DEBUG
			UNICODE buf[PATH_MAX];
			POINT star = star_array[i].star_pt;
			STAR_DESC *pSD = FindStar (NULL, &star, 0, 0);
			GetClusterName (pSD, buf);
			printf ("%s : 0x%04X\n", buf, data[i]);
#endif
			if (!isStarMarked (i, "VISITED"))
				setStarMarked (i, "VISITED");
		}
	}
}