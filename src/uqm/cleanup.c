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

#include "nameref.h"
#include "libs/reslib.h"
#include "gamestr.h"
#include "init.h"
#include "element.h"
#include "hyper.h"
#include "planets/lander.h"
#include "starcon.h"
#include "setup.h"
#include "planets/solarsys.h"
#include "sounds.h"
#include "libs/sndlib.h"
#include "libs/vidlib.h"


void
FreeKernel (void)
{
	UninitPlayerInput ();

	UninitResourceSystem ();

	DestroyDrawable (ReleaseDrawable (Screen));
	Screen = 0;
	DestroyContext (ScreenContext);
	ScreenContext = 0;

	UninitVideoPlayer ();
	UninitSound ();
}

static void
UninitContexts (void)
{
	UninitQueue (&disp_q);

	DestroyContext (OffScreenContext);
	DestroyContext (SpaceContext);
	DestroyContext (StatusContext);
}

static void
UninitKernel (void)
{
	UninitSpace ();

	DestroySound (ReleaseSound (MenuSounds));
	DestroyFont (MicroFont);
	DestroyStringTable (ReleaseStringTable (GameStrings));
	DestroyDrawable (ReleaseDrawable (BorderFrame)); // JMS
	DestroyDrawable (ReleaseDrawable (StatusFrame));
	DestroyDrawable (ReleaseDrawable (SubmenuFrame)); // JMS
	DestroyDrawable (ReleaseDrawable (ConstellationsFrame)); // JMS
	DestroyDrawable (ReleaseDrawable (NebulaeFrame));	// JMS
	DestroyDrawable (ReleaseDrawable (hyperspacesuns));	// JMS
	DestroyDrawable (ReleaseDrawable (ActivityFrame));
	DestroyFont (TinyFont);
	DestroyFont (StarConFont);
	DestroyFont (PlyrFont);
	DestroyFont (MeleeFont);

	UninitQueue (&race_q[0]);
	UninitQueue (&race_q[1]);

	ActivityFrame = 0;
}

void
FreeGameData (void)
{
	FreeSC2Data ();
	FreeLanderData ();
	FreeIPData ();
	FreeHyperData ();
}

void
UninitGameKernel (void)
{
	if (ActivityFrame)
	{
		FreeGameData ();

		UninitKernel ();
		UninitContexts ();
	}
}

