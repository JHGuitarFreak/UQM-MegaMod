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

#include "planets.h"

#include "scan.h"
#include "lander.h"
#include "../colors.h"
#include "../element.h"
#include "../settings.h"
#include "../controls.h"
#include "../sounds.h"
#include "../gameopt.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../uqmdebug.h"
#include "../resinst.h"
#include "../nameref.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"


// PlanetOrbitMenu() items
enum PlanetMenuItems
{
	// XXX: Must match the enum in menustat.h
	SCAN = 0,
	STARMAP,
	EQUIP_DEVICE,
	CARGO,
	ROSTER,
	GAME_MENU,
	NAVIGATION,
};

CONTEXT PlanetContext;
		// Context for rotating planet view and lander surface view

static void
CreatePlanetContext (void)
{
	CONTEXT oldContext;
	RECT r;

	assert (PlanetContext == NULL);

	// PlanetContext rect is relative to SpaceContext
	oldContext = SetContext (SpaceContext);
	GetContextClipRect (&r);

	PlanetContext = CreateContext ("PlanetContext");
	SetContext (PlanetContext);
	SetContextFGFrame (Screen);
	r.extent.height -= MAP_HEIGHT + MAP_BORDER_HEIGHT;
	SetContextClipRect (&r);

	SetContext (oldContext);
}

static void
DestroyPlanetContext (void)
{
	if (PlanetContext)
	{
		DestroyContext (PlanetContext);
		PlanetContext = NULL;
	}
}

void
DrawScannedObjects (BOOLEAN Reversed)
{
	HELEMENT hElement, hNextElement;

	for (hElement = Reversed ? GetTailElement () : GetHeadElement ();
			hElement; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = Reversed ?
				GetPredElement (ElementPtr) :
				GetSuccElement (ElementPtr);

		if (ElementPtr->state_flags & APPEARING)
		{
			STAMP s;

			s.origin = ElementPtr->current.location;
			s.frame = ElementPtr->next.image.frame;
			DrawStamp (&s);
		}

		UnlockElement (hElement);
	}
}

void
DrawPlanetSurfaceBorder (void)
{
	CONTEXT oldContext;
	RECT oldClipRect;
	RECT clipRect;
	RECT r;

	oldContext = SetContext (SpaceContext);
	GetContextClipRect (&oldClipRect);

	// Expand the context clip-rect so that we can tweak the existing border
	clipRect = oldClipRect;
	clipRect.corner.x -= 1;
	clipRect.extent.width += 2;
	clipRect.extent.height += 1;
	SetContextClipRect (&clipRect);

	BatchGraphics ();

	// Border bulk
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	r.corner.x = 0;
	r.corner.y = clipRect.extent.height - MAP_HEIGHT - MAP_BORDER_HEIGHT;
	r.extent.width = clipRect.extent.width;
	r.extent.height = MAP_BORDER_HEIGHT - 2;
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (SIS_BOTTOM_RIGHT_BORDER_COLOR);
	
	// Border top shadow line
	r.extent.width -= 1;
	r.extent.height = 1;
	r.corner.x = 1;
	r.corner.y -= 1;
	DrawFilledRectangle (&r);
	
	// XXX: We will need bulk left and right rects here if MAP_WIDTH changes

	// Right shadow line
	r.extent.width = 1;
	r.extent.height = MAP_HEIGHT + 2;
	r.corner.y += MAP_BORDER_HEIGHT - 1;
	r.corner.x = clipRect.extent.width - 1;
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (SIS_LEFT_BORDER_COLOR);
	
	// Left shadow line
	r.corner.x -= MAP_WIDTH + 1;
	DrawFilledRectangle (&r);

	// Border bottom shadow line
	r.extent.width = MAP_WIDTH + 2;
	r.extent.height = 1;
	DrawFilledRectangle (&r);

	DrawBorder(10, FALSE);
	
	UnbatchGraphics ();

	SetContextClipRect (&oldClipRect);
	SetContext (oldContext);
}

typedef enum
{
	DRAW_ORBITAL_FULL,
	DRAW_ORBITAL_WAIT,
	DRAW_ORBITAL_UPDATE,

} DRAW_ORBITAL_MODE;

static void
DrawOrbitalDisplay (DRAW_ORBITAL_MODE Mode)
{
	RECT r;

	SetContext (SpaceContext);
	GetContextClipRect (&r);

	BatchGraphics ();
	
	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		SetTransitionSource (NULL);

		DrawSISFrame ();
		DrawSISMessage (NULL);
		DrawSISTitle (GLOBAL_SIS (PlanetName));
		DrawStarBackGround ();
		DrawPlanetSurfaceBorder ();
	}

	if (Mode == DRAW_ORBITAL_WAIT)
	{
		STAMP s;

		SetContext (GetScanContext (NULL));
		s.frame = CaptureDrawable (LoadGraphic (ORBENTER_PMAP_ANIM));
		s.origin.x = 0;
		s.origin.y = 0;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
	}
	else if (Mode == DRAW_ORBITAL_FULL)
	{
		DrawDefaultPlanetSphere ();
	}

	if (Mode != DRAW_ORBITAL_WAIT)
	{
		SetContext (GetScanContext (NULL));
		DrawPlanet (0, BLACK_COLOR);
	}

	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		ScreenTransition (3, &r);
	}

	UnbatchGraphics ();

	// for later RepairBackRect()
	// JMS_GFX
	LoadIntoExtraScreen (&r, IS_HD);
}

// Initialise the surface graphics, and start the planet music.
// Called from the GenerateFunctions.generateOribital() function
// (when orbit is entered; either from IP, or from loading a saved game)
// and when "starmap" is selected from orbit and then cancelled;
// also after in-orbit comm and after defeating planet guards in combat.
// SurfDefFrame contains surface definition images when a planet comes
// with its own bitmap (currently only for Earth)
void
LoadPlanet (FRAME SurfDefFrame)
{
	bool WaitMode = !(LastActivity & CHECK_LOAD);
	PLANET_DESC *pPlanetDesc;

#ifdef DEBUG
	if (disableInteractivity)
		return;
#endif

	assert (pSolarSysState->InOrbit && !pSolarSysState->TopoFrame);

	CreatePlanetContext ();

	if (WaitMode)
	{
		DrawOrbitalDisplay (DRAW_ORBITAL_WAIT);
	}

	StopMusic ();

	pPlanetDesc = pSolarSysState->pOrbitalDesc;
	GeneratePlanetSurface (pPlanetDesc, SurfDefFrame, MAP_WIDTH, MAP_HEIGHT, TRUE);
	SetPlanetMusic (pPlanetDesc->data_index & ~PLANET_SHIELDED);
	GeneratePlanetSide ();

	if (!PLRPlaying ((MUSIC_REF)~0))
		PlayMusic (LanderMusic, TRUE, 1);

	if (WaitMode)
	{
		ZoomInPlanetSphere ();
		DrawOrbitalDisplay (DRAW_ORBITAL_UPDATE);
	}
	else
	{
		DrawOrbitalDisplay (DRAW_ORBITAL_FULL);
	}
}

void
FreePlanet (void)
{
	COUNT i, j;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	UninitSphereRotation ();

	StopMusic ();

	for (i = 0; i < sizeof (pSolarSysState->PlanetSideFrame)
			/ sizeof (pSolarSysState->PlanetSideFrame[0]); ++i)
	{
		DestroyDrawable (ReleaseDrawable (pSolarSysState->PlanetSideFrame[i]));
		pSolarSysState->PlanetSideFrame[i] = 0;
	}

//    FreeLanderData ();

	DestroyStringTable (ReleaseStringTable (pSolarSysState->XlatRef));
	pSolarSysState->XlatRef = 0;
	DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
	pSolarSysState->TopoFrame = 0;
	DestroyColorMap (ReleaseColorMap (pSolarSysState->OrbitalCMap));
	pSolarSysState->OrbitalCMap = 0;

	HFree (Orbit->lpTopoData);
	Orbit->lpTopoData = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->TopoZoomFrame));
	Orbit->TopoZoomFrame = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->SphereFrame));
	Orbit->SphereFrame = NULL;

	DestroyDrawable (ReleaseDrawable (Orbit->TintFrame));
	Orbit->TintFrame = 0;
	Orbit->TintColor = BLACK_COLOR;

	DestroyDrawable (ReleaseDrawable (Orbit->ObjectFrame));
	Orbit->ObjectFrame = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->WorkFrame));
	Orbit->WorkFrame = 0;

	HFree (Orbit->TopoColors);
	Orbit->TopoColors = NULL;
	HFree (Orbit->ScratchArray);
	Orbit->ScratchArray = NULL;
	if (Orbit->map_rotate && Orbit->light_diff)
	{
		for (j=0 ; j < MAP_HEIGHT+1 ; j++)
		{
			HFree (Orbit->map_rotate[j]);
			HFree (Orbit->light_diff[j]);
		}
	}

	HFree (Orbit->map_rotate);
	Orbit->map_rotate = NULL;
	HFree (Orbit->light_diff);
	Orbit->light_diff = NULL;

	DestroyStringTable (ReleaseStringTable (
			pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
			));
	pSolarSysState->SysInfo.PlanetInfo.DiscoveryString = 0;
	FreeLanderFont (&pSolarSysState->SysInfo.PlanetInfo);

	// Need to make sure our own CONTEXTs are not active because
	// we will destroy them now
	SetContext (SpaceContext);
	DestroyPlanetContext ();
	DestroyScanContext ();

}

void
LoadStdLanderFont (PLANET_INFO *info)
{
	info->LanderFont = LoadFont (LANDER_FONT);
	info->LanderFontEff = CaptureDrawable (
			LoadGraphic (LANDER_FONTEFF_PMAP_ANIM));
}

void
FreeLanderFont (PLANET_INFO *info)
{
	DestroyFont (info->LanderFont);
	info->LanderFont = NULL;
	DestroyDrawable (ReleaseDrawable (info->LanderFontEff));
	info->LanderFontEff = NULL;
}

static BOOLEAN
DoPlanetOrbit (MENU_STATE *pMS)
{
	BOOLEAN select = PulsedInputState.menu[KEY_MENU_SELECT];
	BOOLEAN handled;

	if ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
			|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0)
		return FALSE;

	// XXX: pMS actually refers to pSolarSysState->MenuState
	handled = DoMenuChooser (pMS, PM_SCAN);
	if (handled)
		return TRUE;

	if (!select)
		return TRUE;

	SetFlashRect (NULL);

	switch (pMS->CurState)
	{
		case SCAN:
			ScanSystem ();
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
			{	// Found Fwiffo on Pluto
				return FALSE;
			}
			break;
		case EQUIP_DEVICE:
			select = DevicesMenu ();
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
			{	// Invoked Talking Pet, a Caster or Sun Device over Chmmr,
				// or a Caster for Ilwrath
				// Going into conversation
				return FALSE;
			}
			break;
		case CARGO:
			CargoMenu ();
			break;
		case ROSTER:
			select = RosterMenu ();
			break;
		case GAME_MENU:
			if (!GameOptions ())
				return FALSE; // abort or load
			break;
		case STARMAP:
		{
			BOOLEAN AutoPilotSet;
			InputFrameCallback *oldCallback;

			// Deactivate planet rotation
			oldCallback = SetInputCallback (NULL);

			RepairSISBorder ();

			AutoPilotSet = StarMap ();
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return FALSE;

			// Reactivate planet rotation
			SetInputCallback (oldCallback);

			if (!AutoPilotSet)
			{	// Redraw the orbital display
				DrawOrbitalDisplay (DRAW_ORBITAL_FULL);
				break;
			}
			// Fall through !!!
		}
		case NAVIGATION:
			return FALSE;
	}

	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		if (select)
		{	// 3DO menu jumps to NAVIGATE after a successful submenu run
			if (optWhichMenu != OPT_PC)
				pMS->CurState = NAVIGATION;
			DrawMenuStateStrings (PM_SCAN, pMS->CurState);
		}
		SetFlashRect (SFR_MENU_3DO);
	}

	return TRUE;
}

static void
on_input_frame (void)
{
	RotatePlanetSphere (TRUE);
}

void
PlanetOrbitMenu (void)
{
	MENU_STATE MenuState;
	InputFrameCallback *oldCallback;

	memset (&MenuState, 0, sizeof MenuState);

	DrawMenuStateStrings (PM_SCAN, SCAN);
	SetFlashRect (SFR_MENU_3DO);

	MenuState.CurState = SCAN;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	oldCallback = SetInputCallback (on_input_frame);

	MenuState.InputFunc = DoPlanetOrbit;
	DoInput (&MenuState, TRUE);

	SetInputCallback (oldCallback);

	SetFlashRect (NULL);
	DrawMenuStateStrings (PM_STARMAP, -NAVIGATION);
}
