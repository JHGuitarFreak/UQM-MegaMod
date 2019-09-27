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

#include "libs/graphics/cmap.h"
#include "libs/threadlib.h"
#include "libs/timelib.h"
#include "libs/inplib.h"
#include "libs/strlib.h"
		// for GetStringAddress()
#include "libs/log.h"
#include <string.h>
#include <stdlib.h>


typedef struct xform_control
{
	int CMapIndex; // -1 means unused
	COLORMAPPTR CMapPtr;
	SIZE Ticks;
	DWORD StartTime;
	DWORD EndTime;
	Color OldCMap[NUMBER_OF_PLUTVALS];
} XFORM_CONTROL;

#define MAX_XFORMS 16
static struct
{
	XFORM_CONTROL TaskControl[MAX_XFORMS];
	volatile int Highest;
			// 'pending' is Highest >= 0
	Mutex Lock;
} XFormControl;

static int fadeAmount = FADE_NORMAL_INTENSITY;
static int fadeDelta;
static TimeCount fadeStartTime;
static sint32 fadeInterval;
static Mutex fadeLock;

#define SPARE_COLORMAPS  20

// Colormaps are rapidly replaced in some parts of the game, so
// it pays to have some spares on hand
static TFB_ColorMap *poolhead;
static int poolcount;

static TFB_ColorMap * colormaps[MAX_COLORMAPS];
static int mapcount;
static Mutex maplock;


static void release_colormap (TFB_ColorMap *map);
static void delete_colormap (TFB_ColorMap *map);


void
InitColorMaps (void)
{
	int i;

	// init colormaps
	maplock = CreateMutex ("Colormaps Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);

	// init xform control
	XFormControl.Highest = -1;
	XFormControl.Lock = CreateMutex ("Transform Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
	for (i = 0; i < MAX_XFORMS; ++i)
		XFormControl.TaskControl[i].CMapIndex = -1;

	fadeLock = CreateMutex ("Fade Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
}

void
UninitColorMaps (void)
{
	int i;
	TFB_ColorMap *next;

	for (i = 0; i < MAX_COLORMAPS; ++i)
	{
		TFB_ColorMap *map = colormaps[i];
		if (!map)
			continue;
		release_colormap (map);
		colormaps[i] = 0;
	}

	// free spares
	for ( ; poolhead; poolhead = next, --poolcount)
	{
		next = poolhead->next;
		delete_colormap (poolhead);
	}

	DestroyMutex (fadeLock);
	// uninit xform control
	DestroyMutex (XFormControl.Lock);
	
	// uninit colormaps
	DestroyMutex (maplock);
}

static inline TFB_ColorMap *
alloc_colormap (void)
		// returns an addrefed object
{
	TFB_ColorMap *map;

	if (poolhead)
	{	// have some spares
		map = poolhead;
		poolhead = map->next;
		--poolcount;
	}
	else
	{	// no spares, need a new one
		map = HMalloc (sizeof (*map));
		map->palette = AllocNativePalette ();
		if (!map->palette)
		{
			HFree (map);
			return NULL;
		}
	}
	map->next = NULL;
	map->index = -1;
	map->refcount = 1;
	map->version = 0;

	return map;
}

static TFB_ColorMap *
clone_colormap (TFB_ColorMap *from, int index)
		// returns an addrefed object
{
	TFB_ColorMap *map;

	map = alloc_colormap ();
	if (!map)
	{
		log_add (log_Warning, "FATAL: clone_colormap(): "
					"could not allocate a map");
		exit (EXIT_FAILURE);
	}
	else
	{	// fresh new map
		map->index = index;
		if (from)
			map->version = from->version;
	}
	map->version++;
	
	return map;
}

static void
delete_colormap (TFB_ColorMap *map)
{
	FreeNativePalette (map->palette);
	HFree (map);
}

static inline void
free_colormap (TFB_ColorMap *map)
{
	if (!map)
	{
		log_add (log_Warning, "free_colormap(): tried to free a NULL map");
		return;
	}

	if (poolcount < SPARE_COLORMAPS)
	{	// return to the spare pool
		map->next = poolhead;
		poolhead = map;
		++poolcount;
	}
	else
	{	// don't need any more spares
		delete_colormap (map);
	}
}

static inline TFB_ColorMap *
get_colormap (int index)
{
	TFB_ColorMap *map;

	map = colormaps[index];
	if (!map)
	{
		log_add (log_Fatal, "BUG: get_colormap(): map not present");
		exit (EXIT_FAILURE);
	}

	map->refcount++;
	return map;
}

static void
release_colormap (TFB_ColorMap *map)
{
	if (!map)
		return;

	if (map->refcount <= 0)
	{
		log_add (log_Warning, "BUG: release_colormap(): refcount not >0");
		return;
	}

	map->refcount--;
	if (map->refcount == 0)
		free_colormap (map);
}

void
TFB_ReturnColorMap (TFB_ColorMap *map)
{
	LockMutex (maplock);
	release_colormap (map);
	UnlockMutex (maplock);
}

TFB_ColorMap *
TFB_GetColorMap (int index)
{
	TFB_ColorMap *map;

	LockMutex (maplock);
	map = get_colormap (index);
	UnlockMutex (maplock);

	return map;
}

void
GetColorMapColors (Color *colors, TFB_ColorMap *map)
{
	int i;

	if (!map)
		return;

	for (i = 0; i < NUMBER_OF_PLUTVALS; ++i)
		colors[i] = GetNativePaletteColor (map->palette, i);
}

BOOLEAN
SetColorMap (COLORMAPPTR map)
{
	int start, end;
	int total_size;
	UBYTE *colors = (UBYTE*)map;
	TFB_ColorMap **mpp;
	
	if (!map)
		return TRUE;

	start = *colors++;
	end = *colors++;
	if (start > end)
	{
		log_add (log_Warning, "ERROR: SetColorMap(): "
				"starting map (%d) not less or eq ending (%d)",
				start, end);
		return FALSE;
	}
	if (start >= MAX_COLORMAPS)
	{
		log_add (log_Warning, "ERROR: SetColorMap(): "
				"starting map (%d) beyond range (0-%d)",
				start, (int)MAX_COLORMAPS - 1);
		return FALSE;
	}
	if (end >= MAX_COLORMAPS)
	{
		log_add (log_Warning, "SetColorMap(): "
				"ending map (%d) beyond range (0-%d)\n",
				end, (int)MAX_COLORMAPS - 1);
		end = MAX_COLORMAPS - 1;
	}

	total_size = end + 1;

	LockMutex (maplock);

	if (total_size > mapcount)
		mapcount = total_size;
	
	// parse the supplied PLUTs into our colormaps
	for (mpp = colormaps + start; start <= end; ++start, ++mpp)
	{
		int i;
		TFB_ColorMap *newmap;
		TFB_ColorMap *oldmap;

		oldmap = *mpp;
		newmap = clone_colormap (oldmap, start);
		
		for (i = 0; i < NUMBER_OF_PLUTVALS; ++i, colors += PLUTVAL_BYTE_SIZE)
		{
			Color color;

			color.a = 0xff;
			color.r = colors[PLUTVAL_RED];
			color.g = colors[PLUTVAL_GREEN];
			color.b = colors[PLUTVAL_BLUE];
			SetNativePaletteColor (newmap->palette, i, color);
		}

		*mpp = newmap;
		release_colormap (oldmap);
	}

	UnlockMutex (maplock);

	return TRUE;
}

/* Fade Transforms */

int
GetFadeAmount (void)
{
	int newAmount;

	LockMutex (fadeLock);

	if (fadeInterval)
	{	// have a pending fade
		TimeCount Now = GetTimeCounter ();
		sint32 elapsed;
		
		elapsed = Now - fadeStartTime;
		if (elapsed > fadeInterval)
			elapsed = fadeInterval;

		newAmount = fadeAmount + (long)fadeDelta * elapsed / fadeInterval;

		if (elapsed >= fadeInterval)
		{	// fade is over
			fadeAmount = newAmount;
			fadeInterval = 0;
		}
	}
	else
	{	// no fade pending, return the current
		newAmount = fadeAmount;
	}

	UnlockMutex (fadeLock);

	return newAmount;
}

static void
finishPendingFade (void)
{
	if (fadeInterval)
	{	// end the fade immediately
		fadeAmount += fadeDelta;
		fadeInterval = 0;
	}
}

static void
FlushFadeXForms (void)
{
	LockMutex (fadeLock);
	finishPendingFade ();
	UnlockMutex (fadeLock);
}

DWORD
FadeScreen (ScreenFadeType fadeType, SIZE TimeInterval)
{
	TimeCount TimeOut;
	int FadeEnd;

	switch (fadeType)
	{
	case FadeAllToBlack:
	case FadeSomeToBlack:
		FadeEnd = FADE_NO_INTENSITY;
		break;
	case FadeAllToColor:
	case FadeSomeToColor:
		FadeEnd = FADE_NORMAL_INTENSITY;
		break;
	case FadeAllToWhite:
	case FadeSomeToWhite:
		FadeEnd = FADE_FULL_INTENSITY;
		break;
	default:
		return (GetTimeCounter ());
	}

	// Don't make users wait for fades
	if (QuitPosted)
		TimeInterval = 0;

	LockMutex (fadeLock);

	finishPendingFade ();

	if (TimeInterval <= 0)
	{	// end the fade immediately
		fadeAmount = FadeEnd;
		// cancel any pending fades
		fadeInterval = 0;
		TimeOut = GetTimeCounter ();
	}
	else
	{
		fadeInterval = TimeInterval;
		fadeDelta = FadeEnd - fadeAmount;
		fadeStartTime = GetTimeCounter ();
		TimeOut = fadeStartTime + TimeInterval + 1;
	}

	UnlockMutex (fadeLock);

	return TimeOut;
}

/* Colormap Transforms */

static void
finish_colormap_xform (int which)
{
	SetColorMap (XFormControl.TaskControl[which].CMapPtr);
	XFormControl.TaskControl[which].CMapIndex = -1;
	// check Highest ptr
	if (which == XFormControl.Highest)
	{
		do
			--which;
		while (which >= 0 && XFormControl.TaskControl[which].CMapIndex == -1);
		
		XFormControl.Highest = which;
	}
}

static inline BYTE
blendChan (BYTE c1, BYTE c2, int weight, int scale)
{
	return c1 + ((int)c2 - c1) * weight / scale;
}

/* This gives the XFormColorMap task a timeslice to do its thing
 * Only one thread should ever be allowed to be calling this at any time
 */
BOOLEAN
XFormColorMap_step (void)
{
	BOOLEAN Changed = FALSE;
	int x;
	DWORD Now = GetTimeCounter ();

	LockMutex (XFormControl.Lock);

	for (x = 0; x <= XFormControl.Highest; ++x)
	{
		XFORM_CONTROL *control = &XFormControl.TaskControl[x];
		int index = control->CMapIndex;
		int TicksLeft = control->EndTime - Now;
		TFB_ColorMap *curmap;

		if (index < 0)
			continue; // unused slot

		LockMutex (maplock);

		curmap = colormaps[index];
		if (!curmap)
		{
			UnlockMutex (maplock);
			log_add (log_Error, "BUG: XFormColorMap_step(): no current map");
			finish_colormap_xform (x);
			continue;
		}

		if (TicksLeft > 0)
		{
#define XFORM_SCALE 0x10000
			TFB_ColorMap *newmap = NULL;
			UBYTE *newClr;
			Color *oldClr;
			int frac;
			int i;

			newmap = clone_colormap (curmap, index);

			oldClr = control->OldCMap;
			newClr = (UBYTE*)control->CMapPtr + 2;

			frac = (int)(control->Ticks - TicksLeft) * XFORM_SCALE
					/ control->Ticks;

			for (i = 0; i < NUMBER_OF_PLUTVALS; ++i, ++oldClr,
					newClr += PLUTVAL_BYTE_SIZE)
			{
				Color color;

				color.a = 0xff;
				color.r = blendChan (oldClr->r, newClr[PLUTVAL_RED],
						frac, XFORM_SCALE);
				color.g = blendChan (oldClr->g, newClr[PLUTVAL_GREEN],
						frac, XFORM_SCALE);
				color.b = blendChan (oldClr->b, newClr[PLUTVAL_BLUE],
						frac, XFORM_SCALE);
				SetNativePaletteColor (newmap->palette, i, color);
			}

			colormaps[index] = newmap;
			release_colormap (curmap);
		}

		UnlockMutex (maplock);

		if (TicksLeft <= 0)
		{	// asked for immediate xform or already done
			finish_colormap_xform (x);
		}
		
		Changed = TRUE;
	}

	UnlockMutex (XFormControl.Lock);

	return Changed;
}

static void
FlushPLUTXForms (void)
{
	int i;

	LockMutex (XFormControl.Lock);

	for (i = 0; i <= XFormControl.Highest; ++i)
	{
		if (XFormControl.TaskControl[i].CMapIndex >= 0)
			finish_colormap_xform (i);
	}
	XFormControl.Highest = -1; // all gone

	UnlockMutex (XFormControl.Lock);
}

static DWORD
XFormPLUT (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	TFB_ColorMap *map;
	XFORM_CONTROL *control;
	int index;
	int x;
	int first_avail = -1;
	DWORD EndTime;
	DWORD Now;

	Now = GetTimeCounter ();
	index = *(UBYTE*)ColorMapPtr;

	LockMutex (XFormControl.Lock);
	// Find an available slot, or reuse if required
	for (x = 0; x <= XFormControl.Highest
			&& index != XFormControl.TaskControl[x].CMapIndex;
			++x)
	{
		if (first_avail == -1 && XFormControl.TaskControl[x].CMapIndex == -1)
			first_avail = x;
	}

	if (index == XFormControl.TaskControl[x].CMapIndex)
	{	// already xforming this colormap -- cancel and reuse slot
		finish_colormap_xform (x);
	}
	else if (first_avail >= 0)
	{	// picked up a slot along the way
		x = first_avail;
	}
	else if (x >= MAX_XFORMS)
	{	// flush some xforms if the queue is full
		log_add (log_Debug, "WARNING: XFormPLUT(): no slots available");
		x = XFormControl.Highest;
		finish_colormap_xform (x);
	}
	// take next unused one
	control = &XFormControl.TaskControl[x];
	if (x > XFormControl.Highest)
		XFormControl.Highest = x;

	// make a copy of the current map
	LockMutex (maplock);
	map = colormaps[index];
	if (!map)
	{
		UnlockMutex (maplock);
		UnlockMutex (XFormControl.Lock);
		log_add (log_Warning, "BUG: XFormPLUT(): no current map");
		return (0);
	}
	GetColorMapColors (control->OldCMap, map);
	UnlockMutex (maplock);

	control->CMapIndex = index;
	control->CMapPtr = ColorMapPtr;
	control->Ticks = TimeInterval;
	if (control->Ticks < 0)
		control->Ticks = 0; /* prevent negative fade */
	control->StartTime = Now;
	control->EndTime = EndTime = Now + control->Ticks;

	UnlockMutex (XFormControl.Lock);

	return (EndTime);
}

DWORD
XFormColorMap (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	if (!ColorMapPtr)
		return (0);

	// Don't make users wait for transforms
	if (QuitPosted)
		TimeInterval = 0;

	return XFormPLUT (ColorMapPtr, TimeInterval);
}

void
FlushColorXForms (void)
{
	FlushFadeXForms ();
	FlushPLUTXForms ();
}

// The type conversions are implicit and will generate errors
// or warnings if types change imcompatibly
COLORMAPPTR
GetColorMapAddress (COLORMAP colormap)
{
	return GetStringAddress (colormap);
}
