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
#include "../starmap.h"
#include "../util.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "../gendef.h"
#include "uqm/gamestr.h"


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

BOOLEAN useDosSpheres = FALSE;
BOOLEAN use3DOSpheres = FALSE;
BYTE OrbitNum = 0;

void
DestroyOrbitStruct (PLANET_ORBIT* Orbit, SIZE height)
{
	DestroyDrawable (ReleaseDrawable (Orbit->TopoZoomFrame));
	Orbit->TopoZoomFrame = 0;

	if (Orbit->lpTopoData)
	{
		HFree(Orbit->lpTopoData);
		Orbit->lpTopoData = NULL;
	}

	DestroyDrawable (ReleaseDrawable (Orbit->SphereFrame));
	Orbit->SphereFrame = 0;

	DestroyDrawable (ReleaseDrawable (Orbit->ObjectFrame));
	Orbit->ObjectFrame = 0;

	DestroyDrawable (ReleaseDrawable (Orbit->TintFrame));
	Orbit->TintFrame = 0;

	Orbit->TintColor = BLACK_COLOR;

	if (Orbit->TopoColors)
	{
		HFree (Orbit->TopoColors);
		Orbit->TopoColors = NULL;
	}

	if (Orbit->ScanColors)
	{
		COUNT i;
		for (i = 0; i < NUM_SCAN_TYPES; i++)
		{
			HFree (Orbit->ScanColors[i]);
			Orbit->ScanColors[i] = NULL;
		}
		HFree (Orbit->ScanColors);
		Orbit->ScanColors = NULL;
	}
	
	if (Orbit->ScratchArray)
	{
		HFree (Orbit->ScratchArray);
		Orbit->ScratchArray = NULL;
	}

	DestroyDrawable (ReleaseDrawable (Orbit->WorkFrame));
	Orbit->WorkFrame = 0;

	DestroyDrawable (ReleaseDrawable (Orbit->BackFrame));
	Orbit->BackFrame = 0;

	if (Orbit->light_diff)
	{
		COUNT j;
		for (j = 0; j < height; j++)
		{
			HFree (Orbit->light_diff[j]);
			Orbit->light_diff[j] = NULL;
		}
		HFree (Orbit->light_diff);
		Orbit->light_diff = NULL;
	}

	if (Orbit->map_rotate)
	{
		COUNT k;
		for (k = 0; k < height; k++)
		{
			HFree (Orbit->map_rotate[k]);
			Orbit->map_rotate[k] = NULL;
		}
		HFree (Orbit->map_rotate);
		Orbit->map_rotate = NULL;
	}

	DestroyDrawable (ReleaseDrawable (Orbit->TopoMask));
	Orbit->TopoMask = 0;

	if (Orbit->sphereBytes)
	{
		HFree (Orbit->sphereBytes);
		Orbit->sphereBytes = NULL;
	}

	DestroyColorMap (ReleaseColorMap (Orbit->sphereMap));
	Orbit->sphereMap = 0;

	Orbit->scanType = 0;

	DestroyDrawable (ReleaseDrawable (Orbit->Shade));
	Orbit->Shade = 0;

	if (Orbit->ShadeColors)
	{
		HFree (Orbit->ShadeColors);
		Orbit->ShadeColors = NULL;
	}	
}



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
	clipRect.corner.x -= RES_SCALE (1);
	clipRect.extent.width += RES_SCALE (2);
	clipRect.extent.height += RES_SCALE (1);
	SetContextClipRect (&clipRect);

	BatchGraphics ();

	// Border bulk
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	r.corner.x = 0;
	r.corner.y = clipRect.extent.height - MAP_HEIGHT - MAP_BORDER_HEIGHT;
	r.extent.width = clipRect.extent.width;
	r.extent.height = MAP_BORDER_HEIGHT - RES_SCALE (2);
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (SIS_BOTTOM_RIGHT_BORDER_COLOR);
	
	// Border top shadow line
	r.extent.width -= RES_SCALE (1);
	r.extent.height = RES_SCALE (1);
	r.corner.x = RES_SCALE (1);
	r.corner.y -= RES_SCALE (1);
	DrawFilledRectangle (&r);
	
	// XXX: We will need bulk left and right rects here if MAP_WIDTH changes

	// Right shadow line
	r.extent.width = RES_SCALE (1);
	r.extent.height = MAP_HEIGHT + RES_SCALE (2);
	r.corner.y += MAP_BORDER_HEIGHT - RES_SCALE (1);
	r.corner.x = clipRect.extent.width - RES_SCALE (1);
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (SIS_LEFT_BORDER_COLOR);
	
	// Left shadow line
	r.corner.x -= MAP_WIDTH + RES_SCALE (1);
	DrawFilledRectangle (&r);

	// Border bottom shadow line
	r.extent.width = MAP_WIDTH + RES_SCALE (2);
	r.extent.height = RES_SCALE (1);
	DrawFilledRectangle (&r);

	if (isPC (optSuperPC))
	{
		r.corner.x = RES_SCALE (UQM_MAP_WIDTH - SC2_MAP_WIDTH)
				- SIS_ORG_X + RES_SCALE (1) + SAFE_POS (1);
		r.corner.y = clipRect.extent.height - MAP_HEIGHT - RES_SCALE (1);
		r.extent.width = RES_SCALE (1);
		r.extent.height = MAP_HEIGHT;
		SetContextForeGroundColor (SIS_BOTTOM_RIGHT_BORDER_COLOR);
		DrawFilledRectangle (&r);
		r.corner.x += RES_SCALE (1);
		r.extent.width = RES_SCALE (4);
		r.corner.y -= RES_SCALE (1);
		r.extent.height += RES_SCALE (2);
		SetContextForeGroundColor (
				BUILD_COLOR_RGBA (0x52, 0x52, 0x52, 0xFF));
		DrawFilledRectangle (&r);
		r.corner.x += RES_SCALE (4);
		r.extent.width = RES_SCALE (1);
		r.corner.y += RES_SCALE (1);
		r.extent.height -= RES_SCALE (2);
		SetContextForeGroundColor (SIS_LEFT_BORDER_COLOR);
		DrawFilledRectangle (&r);

		DrawBorder (LANDER_DOS_FRAME);
	}
	else
		DrawBorder (LANDER_3DO_FRAME);
	
	UnbatchGraphics ();

	SetContextClipRect (&oldClipRect);
	SetContext (oldContext);
}

typedef enum
{
	DRAW_ORBITAL_FULL,
	DRAW_ORBITAL_WAIT,
	DRAW_ORBITAL_UPDATE,
	DRAW_ORBITAL_FROM_STARMAP,

} DRAW_ORBITAL_MODE;

static void
DrawEnterOrbitText (RECT rect)
{
	TEXT text;
	FONT OldFont;
	FRAME OldFontEffect;
	SIZE leading;
	UNICODE buf[256];
	COORD og_baseline_x;

	OldFont = SetContextFont (MicroFont);
	OldFontEffect = SetContextFontEffect (
			SetAbsFrameIndex (FontGradFrame, 12));

	GetContextFontLeading (&leading);

	text.baseline = rect.corner;
	text.baseline.x += rect.extent.width >> 1;
	text.baseline.y += RES_SCALE (4) + leading - RES_SCALE (4);
	text.align = ALIGN_CENTER;

	og_baseline_x = text.baseline.x;

	utf8StringCopy ((char *)buf, sizeof (buf),
		GAME_STRING (NAVIGATION_STRING_BASE + 8));

	text.align = ALIGN_CENTER;
	text.pStr = strtok (buf, "\n");
	text.CharCount = (COUNT)~0;

	while (text.pStr != NULL)
	{
		text.pStr = AlignText ((const UNICODE *)text.pStr,
				&text.baseline.x);
		text.CharCount = (COUNT)~0;

		font_DrawText (&text);

		text.pStr = strtok (NULL, "\n");
		text.CharCount = (COUNT)~0;
		text.baseline.y += leading;
		text.baseline.x = og_baseline_x;
	}

	SetContextFont (OldFont);
	SetContextFontEffect (OldFontEffect);
}

void
DrawOrbitMapGraphic (void)
{
	STAMP s;
	FRAME SurfDefFrame = NULL;

	SetContext (GetScanContext (NULL));

	if (optScanSphere != 1)
	{
		BOOLEAN HaveString =
				strlen (GAME_STRING (NAVIGATION_STRING_BASE + 8)) > 0;

		s.frame = SetAbsFrameIndex (CaptureDrawable (
				LoadGraphic (ORBENTER_PMAP_ANIM)), HaveString);

		s.origin.x = -SAFE_X;
		s.origin.y = 0;

		if (isPC (optSuperPC))
		{
			s.origin.x -=
				RES_SCALE (((UQM_MAP_WIDTH - SC2_MAP_WIDTH) / 2)
					+ SAFE_NUM (5));
		}

		DrawStamp (&s);

		if (HaveString)
		{
			RECT rect;

			GetFrameRect (s.frame, &rect);
			rect.corner.x += s.origin.x;
			DrawEnterOrbitText (rect);
		}

		DestroyDrawable (ReleaseDrawable (s.frame));
	}
	else
	{
		DrawPlanet (0, BLACK_COLOR);
		DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
		pSolarSysState->TopoFrame = 0;
	}
#if 0
	if (never)
	{
		STAMP ss;
		PLANET_DESC *pPlanetDesc;
		PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
		int PlanetScale = RES_BOOL (319, 512);
		int PlanetRescale = 1275;

		pPlanetDesc = pSolarSysState->pOrbitalDesc;
		GeneratePlanetSurface (
			pPlanetDesc, NULL, PlanetScale, PlanetScale);
		ss.origin.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
		ss.origin.y = RES_SCALE (191);

		ss.frame = RES_BOOL (Orbit->SphereFrame, CaptureDrawable (
			RescaleFrame (
				Orbit->SphereFrame, PlanetRescale, PlanetRescale
			)));

		DrawStamp (&ss);
		DestroyDrawable (ReleaseDrawable (ss.frame));
	}
#endif
}

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
		DrawOrbitMapGraphic ();

		if (isPC (optSuperPC))
			InitPCLander (TRUE);
	}
	else if (Mode == DRAW_ORBITAL_FULL)
	{
		DrawDefaultPlanetSphere ();
		DrawMenuStateStrings (PM_SCAN, SCAN);
	}
	else if (Mode == DRAW_ORBITAL_FROM_STARMAP)
	{
		DrawDefaultPlanetSphere ();
		DrawMenuStateStrings (PM_SCAN, STARMAP);
	}
	else
		DrawMenuStateStrings (PM_SCAN, SCAN);

	if (Mode != DRAW_ORBITAL_WAIT)
	{
		SetContext (GetScanContext (NULL));
		DrawPlanet (0, BLACK_COLOR);
		if (isPC (optSuperPC))
			InitPCLander (FALSE);
	}

	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		ScreenTransition (optScrTrans, &r);
	}

	UnbatchGraphics ();

	// for later RepairBackRect()
	
	LoadIntoExtraScreen (&r);
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
	TimeCount sleep;

#ifdef DEBUG
	if (disableInteractivity)
		return;
#endif

	assert (pSolarSysState->InOrbit && !pSolarSysState->TopoFrame);

	CreatePlanetContext ();

	StopMusic ();

	sleep = GetTimeCounter () + (ONE_SECOND * 6 / 5);
	pPlanetDesc = pSolarSysState->pOrbitalDesc;

	if (WaitMode)
	{
		if (optScanSphere == 1)
			GetPlanetTopography (pPlanetDesc, SurfDefFrame);
		DrawOrbitalDisplay (DRAW_ORBITAL_WAIT);
	}

	GeneratePlanetSurface (pPlanetDesc, SurfDefFrame, 0, 0);
	OrbitNum = SetPlanetMusic (pPlanetDesc->data_index & ~PLANET_SHIELDED);
	GeneratePlanetSide ();

	if (isPC (optScrTrans))
		SleepThreadUntil (sleep);

	if (!PLRPlaying ((MUSIC_REF)~0))
	{
		SetMusicVolume (MUTE_VOLUME);
		PlayMusic (LanderMusic, TRUE, 1);

		if (OkayToResume ())
		{
			SeekMusic (GetMusicPosition ());
			FadeMusic (NORMAL_VOLUME, ONE_SECOND * 2);
		}
		else
			SetMusicVolume (NORMAL_VOLUME);
	}

	if (WaitMode)
	{
		if (is3DO (optScrTrans) || optScanSphere == 1)
			ZoomInPlanetSphere ();
		DrawOrbitalDisplay (DRAW_ORBITAL_UPDATE);
	}
	else
	{	// to fix moon suffix on load
		if (worldIsMoon (pSolarSysState, pSolarSysState->pOrbitalDesc))
		{
			if (!(GetNamedPlanetaryBody ()) && isPC (optWhichFonts)
					&& (pSolarSysState->pOrbitalDesc->data_index
							< PRECURSOR_STARBASE
					&& pSolarSysState->pOrbitalDesc->data_index
							!= DESTROYED_STARBASE
					&& pSolarSysState->pOrbitalDesc->data_index
							!= PRECURSOR_STARBASE))
			{
				snprintf (
						(GLOBAL_SIS (PlanetName))
						+ strlen (GLOBAL_SIS (PlanetName)),
						3, "-%c%c", 'A'
						+ moonIndex (
							pSolarSysState, pSolarSysState->pOrbitalDesc),
							'\0');
			}
		}
	 	DrawOrbitalDisplay (DRAW_ORBITAL_FULL);
	}
}

void
FreePlanet (void)
{
	COUNT i;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	UninitSphereRotation ();

	SetMusicPosition ();

	StopMusic ();

	for (i = 0; i < ARRAY_SIZE (pSolarSysState->PlanetSideFrame); ++i)
	{
		DestroyDrawable (
				ReleaseDrawable (pSolarSysState->PlanetSideFrame[i]));
		pSolarSysState->PlanetSideFrame[i] = 0;
	}

//    FreeLanderData ();

	DestroyStringTable (ReleaseStringTable (pSolarSysState->XlatRef));
	pSolarSysState->XlatRef = 0;
	DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
	pSolarSysState->TopoFrame = 0;

	if (optScanStyle == OPT_PC)
	{
		COUNT k;

		for (k = 0; k < NUM_SCAN_TYPES; k++)
		{
			DestroyDrawable (
					ReleaseDrawable (pSolarSysState->ScanFrame[k]));
			pSolarSysState->ScanFrame[k] = 0;
		}
	}

	DestroyColorMap (ReleaseColorMap (pSolarSysState->OrbitalCMap));
	pSolarSysState->OrbitalCMap = 0;

	DestroyOrbitStruct (Orbit, MAP_HEIGHT);

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
	DestroyPCLanderContext ();
	useDosSpheres = FALSE;
	use3DOSpheres = FALSE;
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
	BOOLEAN select = (BOOLEAN)PulsedInputState.menu[KEY_MENU_SELECT];
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

	SetFlashRect (NULL, FALSE);

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
				DrawOrbitalDisplay (DRAW_ORBITAL_FROM_STARMAP);//WAS FULL
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
			if (pMS->CurState != STARMAP)
				DrawMenuStateStrings (PM_SCAN, pMS->CurState);
		}
		SetFlashRect (SFR_MENU_3DO, FALSE);
	}

	return TRUE;
}

static void
on_input_frame (void)
{
	RotatePlanetSphere (TRUE, NULL);
}

void
PlanetOrbitMenu (void)
{
	MENU_STATE MenuState;
	InputFrameCallback *oldCallback;

	memset (&MenuState, 0, sizeof MenuState);
	
	SetFlashRect (SFR_MENU_3DO, FALSE);

	MenuState.CurState = SCAN;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	oldCallback = SetInputCallback (on_input_frame);

	MenuState.InputFunc = DoPlanetOrbit;
	DoInput (&MenuState, TRUE);

	SetInputCallback (oldCallback);

	SetFlashRect (NULL, FALSE);
	if (!(GLOBAL(CurrentActivity) & CHECK_LOAD))
		DrawMenuStateStrings (PM_STARMAP, -NAVIGATION);
}