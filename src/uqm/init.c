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

#include "build.h"
#include "colors.h"
#include "cons_res.h"
#include "races.h"
#include "element.h"
#include "tactrans.h"
#include "pickship.h"
#include "process.h"
#include "globdata.h"
#include "encount.h"
#include "hyper.h"
#include "init.h"
#include "port.h"
#include "resinst.h"
#include "libs/reslib.h"
#include "nameref.h"
#include "setup.h"
#include "units.h"


FRAME stars_in_space;
FRAME StarPoints;
FRAME stars_in_quasispace; // JMS_GFX
FRAME crew_dots[NUM_VIEWS]; // JMS_GFX
FRAME ion_trails[NUM_VIEWS]; // JMS_GFX
FRAME asteroid[NUM_VIEWS];
FRAME blast[NUM_VIEWS];
FRAME explosion[NUM_VIEWS];


BOOLEAN
load_animation (FRAME *pixarray, RESOURCE big_res, RESOURCE med_res, RESOURCE
		sml_res)
{
	DRAWABLE d;

	d = LoadGraphic (big_res);
	if (!d)
		return FALSE;
	pixarray[0] = CaptureDrawable (d);

	if (med_res != NULL_RESOURCE)
	{
		d = LoadGraphic (med_res);
		if (!d)
			return FALSE;
	}
	pixarray[1] = CaptureDrawable (d);

	if (sml_res != NULL_RESOURCE)
	{
		d = LoadGraphic (sml_res);
		if (!d)
			return FALSE;
	}
	pixarray[2] = CaptureDrawable (d);

	return TRUE;
}

/* Warning: Some ships (such as the Umgah) will alias their pixarrays,
   so we need to track to make sure that we do not double-free. */
BOOLEAN
free_image (FRAME *pixarray)
{
	BOOLEAN retval;
	COUNT i, j;
	void *already_freed[NUM_VIEWS];

	retval = TRUE;
	for (i = 0; i < NUM_VIEWS; ++i)
	{
		if (pixarray[i] != NULL)
		{
			BOOLEAN ok = TRUE;
			for (j = 0; j < i; j++)
			{
				if (already_freed[j] == pixarray[i])
				{
					ok = FALSE;
					break;
				}
			}
			if (ok)
			{
				if (!DestroyDrawable (ReleaseDrawable (pixarray[i])))
					retval = FALSE;
			}
			already_freed[i] = pixarray[i];
			pixarray[i] = NULL;
		}
	}

	return (retval);
}

static BYTE space_ini_cnt;

BOOLEAN
InitSpace (void)
{
	if (space_ini_cnt++ == 0
			&& LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER)
	{
		stars_in_space = CaptureDrawable (
				LoadGraphic (STAR_MASK_PMAP_ANIM));
		if (stars_in_space == NULL)
			return FALSE;

		if(IS_HD){
			StarPoints = CaptureDrawable (LoadGraphic (STARPOINT_MASK_PMAP_ANIM));
			if (StarPoints == NULL)
				return FALSE;

			// JMS_GFX
			if (!load_animation (crew_dots,
					CREW_BIG_MASK_PMAP_ANIM,
					CREW_MED_MASK_PMAP_ANIM,
					CREW_SML_MASK_PMAP_ANIM))
				return FALSE;
        
			// JMS_GFX
			if (!load_animation (ion_trails,
					IONS_BIG_MASK_PMAP_ANIM,
					IONS_MED_MASK_PMAP_ANIM,
					IONS_SML_MASK_PMAP_ANIM))
				return FALSE;
		}

		if (!load_animation (explosion,
				BOOM_BIG_MASK_PMAP_ANIM,
				BOOM_MED_MASK_PMAP_ANIM,
				BOOM_SML_MASK_PMAP_ANIM))
			return FALSE;

		if (!load_animation (blast,
				BLAST_BIG_MASK_PMAP_ANIM,
				BLAST_MED_MASK_PMAP_ANIM,
				BLAST_SML_MASK_PMAP_ANIM))
			return FALSE;

		if (!load_animation (asteroid,
				ASTEROID_BIG_MASK_PMAP_ANIM,
				ASTEROID_MED_MASK_PMAP_ANIM,
				ASTEROID_SML_MASK_PMAP_ANIM))
			return FALSE;
	}

	return TRUE;
}

void
UninitSpace (void)
{
	if (space_ini_cnt && --space_ini_cnt == 0)
	{
		free_image (blast);
		free_image (explosion);
		free_image (asteroid);

		// JMS_GFX
		free_image (crew_dots);
		free_image (ion_trails);

		DestroyDrawable (ReleaseDrawable (stars_in_space));
		DestroyDrawable (ReleaseDrawable (StarPoints));
		stars_in_space = 0;
		StarPoints = 0;
	}
}

static HSTARSHIP
BuildSIS (void)
{
	HSTARSHIP hStarShip;
	STARSHIP *StarShipPtr;

	hStarShip = Build (&race_q[0], SIS_SHIP_ID);
	if (!hStarShip)
		return 0;
	StarShipPtr = LockStarShip (&race_q[0], hStarShip);
	StarShipPtr->playerNr = RPG_PLAYER_NUM;
	StarShipPtr->captains_name_index = 0;
	UnlockStarShip (&race_q[0], hStarShip);

	return hStarShip;
}

SIZE
InitShips (void)
{
	SIZE num_ships;
	BYTE numAsteroids, numPlanets;

	numAsteroids = 5;
	numPlanets = 1;

	InitSpace ();

	SetContext (StatusContext);
	SetContext (SpaceContext);

	InitDisplayList ();
	InitGalaxy ();

	if (inHQSpace ())
	{
		ReinitQueue (&race_q[0]);
		ReinitQueue (&race_q[1]);

		BuildSIS ();
		LoadHyperspace ();

		num_ships = 1;
	}
	else
	{
		COUNT i;
		RECT r;

		SetContextFGFrame (Screen);
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SPACE_WIDTH;
		r.extent.height = SPACE_HEIGHT;
		SetContextClipRect (&r);

		SetContextBackGroundColor (BLACK_COLOR);
		{
			CONTEXT OldContext;

			OldContext = SetContext (ScreenContext);

			SetContextBackGroundColor (BLACK_COLOR);
			ClearDrawable ();

			SetContext (OldContext);
		}

		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
			free_gravity_well ();
		else
		{
			for (i = 0; i < numAsteroids; ++i)
				spawn_asteroid (NULL);
			for (i = 0; i < numPlanets; ++i)
				spawn_planet ();
		}
	
		num_ships = NUM_SIDES;
	}

	// FlushInput ();

	return (num_ships);
}

// Count the crew elements in the display list.
static COUNT
CountCrewElements (void)
{
	COUNT result;
	HELEMENT hElement, hNextElement;

	result = 0;
	for (hElement = GetHeadElement ();
			hElement != 0; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
		if (ElementPtr->state_flags & CREW_OBJECT)
			++result;

		UnlockElement (hElement);
	}

	return result;
}

void
UninitShips (void)
{
	COUNT crew_retrieved;
	int i;
	HELEMENT hElement, hNextElement;
	STARSHIP *SPtr[NUM_PLAYERS];

	StopSound ();

	UninitSpace ();

	for (i = 0; i < NUM_PLAYERS; ++i)
		SPtr[i] = 0;

	// Count the crew floating in space.
	crew_retrieved = CountCrewElements();

	for (hElement = GetHeadElement ();
			hElement != 0; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
		if ((ElementPtr->state_flags & PLAYER_SHIP)
				|| ElementPtr->death_func == new_ship)
		{
			STARSHIP *StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);

			// There should only be one ship left in battle.
			// He gets the crew still floating in space.
			if (StarShipPtr->RaceDescPtr->ship_info.crew_level)
			{
				if (crew_retrieved >=
						StarShipPtr->RaceDescPtr->ship_info.max_crew -
						StarShipPtr->RaceDescPtr->ship_info.crew_level)
					StarShipPtr->RaceDescPtr->ship_info.crew_level =
							StarShipPtr->RaceDescPtr->ship_info.max_crew;
				else
					StarShipPtr->RaceDescPtr->ship_info.crew_level +=
							crew_retrieved;
			}

			/* Record crew left after battle */
			StarShipPtr->crew_level =
					StarShipPtr->RaceDescPtr->ship_info.crew_level;
			SPtr[StarShipPtr->playerNr] = StarShipPtr;
			free_ship (StarShipPtr->RaceDescPtr, TRUE, TRUE);
			StarShipPtr->RaceDescPtr = 0;
		}
		UnlockElement (hElement);
	}

	GLOBAL (CurrentActivity) &= ~IN_BATTLE;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		// Encounter battle in full game.
		//   Record the crew left in the last ship standing. The crew left
		//   is first recorded into STARSHIP.crew_level just a few lines
		//   above here.
		for (i = NUM_PLAYERS - 1; i >= 0; --i)
		{
			if (SPtr[i] && !FleetIsInfinite (i))
				UpdateShipFragCrew (SPtr[i]);
		}
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_ENCOUNTER)
	{
		// Remove any ships left from the race queue.
		for (i = 0; i < NUM_PLAYERS; i++)
			ReinitQueue (&race_q[i]);

		if (inHQSpace ())
			FreeHyperspace ();
	}
}


