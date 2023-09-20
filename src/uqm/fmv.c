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

#include "fmv.h"
#include "controls.h"
#include "hyper.h"
#include "options.h"
#include "master.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "setup.h"
#include "libs/vidlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/sound/sound.h"
#include "init.h"
#include "gameopt.h"

#define MAX_LOAD_ENTRIES 40

void
DoShipSpin (COUNT index, MUSIC_REF hMusic)
{
	char vnbuf[24]; // From 32 to 24
	RECT old_r;
	CONTEXT OldContext;

	OldContext = SetContext (ScreenContext);


	LoadIntoExtraScreen (NULL);
	
	if (hMusic)
		StopMusic ();

	FreeHyperData ();

	if (isPC (optWhichIntro))
	{
		if (IS_PAD || OutfitOrShipyard == 3)
		{
			SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 4));
			FlushColorXForms ();
		}

		ShowPresentation (ACCESSDATA_STRTAB);
	}

	// TODO: It would be nice to have better resource names for these.
	sprintf (vnbuf, "slides.spins.%02u", (unsigned)index);

	ShowPresentation (vnbuf);

	if (is3DO (optWhichIntro))
		SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 4));
	FlushColorXForms ();

	SetContext (OldContext);

	GetContextClipRect (&old_r);
	SetContextClipRect (NULL);
	DrawFromExtraScreen (NULL);
	SetContextClipRect (&old_r);

	if (hMusic)
		PlayMusic (hMusic, TRUE, 1);

	if (is3DO (optWhichIntro))
		SleepThreadUntil (FadeScreen (FadeAllToColor, ONE_SECOND / 4));
	FlushColorXForms ();
}

void
SplashScreen (void (* DoProcessing)(DWORD TimeOut))
{
	STAMP s;
	DWORD TimeOut = 0;
	TimeCount OverallWait = GetTimeCounter () + (ONE_SECOND * 3);

	if (!optSkipIntro)
	{
		SetContext (ScreenContext);
		s.origin.x = s.origin.y = 0;

		s.frame = CaptureDrawable (LoadGraphic (TITLE_ANIM));

		if (optFlagshipColor == OPT_3DO)
			s.frame = SetAbsFrameIndex (s.frame, 1);
		else
			s.frame = SetAbsFrameIndex (s.frame, 0);

		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));

		TimeOut = FadeScreen (FadeAllToColor, ONE_SECOND / 2);
	}
	
	if (DoProcessing)
		DoProcessing (TimeOut);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
	
	/* There was a forcible setting of CHECK_ABORT here.  I cannot
	 * find any purpose for this that DoRestart doesn't handle
	 * better (forcing all other threads but this one to quit out,
	 * I believe), and have thus removed it.  It was interfering
	 * with the proper operation of the quit operation.
	 * --Michael */

	if (!optSkipIntro)
		SleepThreadUntil (OverallWait);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;

	if (!optSkipIntro)
		SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
}

void
Introduction (void)
{
	ShowPresentation (INTROPRES_STRTAB);
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
}

void
Victory (void)
{
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));

	/* by default we do 3DO cinematics; or PC slides when 3DO files are
	 * not present */
	ShowPresentation (FINALPRES_STRTAB);
		
	FadeScreen (FadeAllToBlack, 0);
}

void
GameOver (BYTE DeathType)
{
	if (DeathType != SUICIDE)
		SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));

	switch (DeathType)
	{
		case DEATH_MARCH:
			ShowPresentation (DEATHMARCHPRES_STRTAB);
			break;
		case DEATH_MARCH_QUASI:
			ShowPresentation (DEATHMARCHQUASIPRES_STRTAB);
			break;
		case SUICIDE:
			SleepThreadUntil (FadeScreen (FadeAllToWhite, ONE_SECOND / 2));
			ShowPresentation (SUICIDEPRES_STRTAB);
			break;
		case SURRENDERED:
			ShowPresentation (SURRENDEREDPRES_STRTAB);
			break;
		case DIED_IN_BATTLE:
		default:
			ShowPresentation (DEFEATEDPRES_STRTAB);
			break;
	}
	FadeScreen (FadeAllToBlack, 0);
}

void
Logo (void)
{
	ShowPresentation (LOGOPRES_STRTAB);
	FadeScreen (FadeAllToBlack, 0);
}

void
Drumall (void)
{
	ShowPresentation (DRUMALLPRES_STRTAB);
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
}

void
Reload (void)
{
	ShowPresentation (RELOADPRES_STRTAB);

	ReloadGameContent ();
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
}

void
AdvanceLoadProgress (void)
{
	if (optRequiresReload)
	{
		RECT r;
		static COUNT i = 0;

		r.corner.x = RES_SCALE (16);
		r.corner.y = ScreenHeight - DOS_BOOL_SCL (50, 30);
		r.extent.height = RES_SCALE (15);
		r.extent.width = RES_SCALE ((i + 1) * 7);

		if (i == MAX_LOAD_ENTRIES)
		{
			SetContextForeGroundColor (BUILD_COLOR_RGBA (0, 192, 0, 255));
			i = 0;
		}
		else
		{
			SetContextForeGroundColor (
					BUILD_COLOR_RGBA (0, 128, 192, 255));
			i++;
		}
		DrawFilledRectangle (&r);
	}
}
