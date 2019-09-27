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


#include "libs/gfxlib.h"
#include "libs/threadlib.h"
#include "colors.h"
#include "setup.h"
#include "sis.h"
#include "units.h"
#include "util.h"
#include "menustat.h"

void
InitSISContexts (void)
{
	RECT r;

	SetContext (StatusContext);

	SetContext (SpaceContext);
	SetContextFGFrame (Screen);

	r.corner.x = SIS_ORG_X;
	r.corner.y = SIS_ORG_Y;
	r.extent.width = SIS_SCREEN_WIDTH;
	r.extent.height = SIS_SCREEN_HEIGHT;
	SetContextClipRect (&r);
}

void
DrawSISFrame (void)
{
	RECT r;

	SetContext (ScreenContext);

	BatchGraphics ();
	{
		// Middle grey rectangles around space window.
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
			//
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SIS_ORG_X + SIS_SCREEN_WIDTH + 1;
		r.extent.height = SIS_ORG_Y - 1;
		DrawFilledRectangle (&r);
			// Inside Left Border
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SIS_ORG_X - 1;
		r.extent.height = SIS_ORG_Y + SIS_SCREEN_HEIGHT + 1;
		DrawFilledRectangle (&r);
			// Bottom left of the border
		r.corner.x = 0;
		r.corner.y = r.extent.height;
		r.extent.width = SIS_ORG_X + SIS_SCREEN_WIDTH + 1;
		r.extent.height = SCREEN_HEIGHT - SIS_ORG_Y + SIS_SCREEN_HEIGHT;
		DrawFilledRectangle (&r);
			// Top right inside border
		r.corner.x = SIS_ORG_X + SIS_SCREEN_WIDTH + 1;
		r.corner.y = 0;
		r.extent.width = SCREEN_WIDTH - r.corner.x;
		r.extent.height = SCREEN_HEIGHT;
		DrawFilledRectangle (&r);
		
		// Light and dark grey edges of the inner space window.
		r.corner.x = SIS_ORG_X - 1;
		r.corner.y = SIS_ORG_Y - 1;
		r.extent.width = SIS_SCREEN_WIDTH + 2;
		r.extent.height = SIS_SCREEN_HEIGHT + 2;
		DrawStarConBox (&r, 1,
				BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19),
				BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F),
				TRUE, BLACK_COLOR);

		// The big Blue box in the upper edge of screen containing the star system name.
		r.corner.y = 0;
		r.extent.height = SIS_ORG_Y;
		r.corner.x = SIS_ORG_X;
		r.extent.width = SIS_MESSAGE_BOX_WIDTH;
		DrawStarConBox (&r, 1,
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x0E), 0x54),
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x1C), 0x4E),
				TRUE, BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));

		// The smaller blue box.
		r.extent.width = SIS_TITLE_BOX_WIDTH;
		r.corner.x = SIS_ORG_X + SIS_SCREEN_WIDTH - SIS_TITLE_BOX_WIDTH;
		DrawStarConBox (&r, 1,
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x0E), 0x54),
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x1C), 0x4E),
				TRUE, BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));

		// Black border between menu area and space window area
		SetContextForeGroundColor (BLACK_COLOR);
		r.corner.x = SPACE_WIDTH - 1;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = SCREEN_HEIGHT;
		DrawFilledRectangle (&r);
		
		// Bottom corners of the SIS gauges
		r.corner.x = SPACE_WIDTH;
		r.corner.y = RES_BOOL(139, 434); // JMS_GFX
		DrawPoint (&r.corner);
		
		r.corner.x = SCREEN_WIDTH - 1; // JMS_GFX
		DrawPoint (&r.corner);

		// Light grey border on the left side of big blue box.
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19));
		r.corner.y = 1;
		r.extent.width = 1;
		r.extent.height = SIS_TITLE_HEIGHT;
		r.corner.x = SIS_ORG_X - 1;
		DrawFilledRectangle (&r);
		
		// The same for small blue box
		r.corner.x = SIS_ORG_X + SIS_SCREEN_WIDTH - SIS_TITLE_BOX_WIDTH - 1;
		DrawFilledRectangle (&r);

		// Light grey horizontal line at the bottom of the screen, space window side
		r.corner.x = 0;
		r.corner.y = SCREEN_HEIGHT - 1;
		r.extent.width = SPACE_WIDTH - 1;
		r.extent.height = 1;
		DrawFilledRectangle (&r);
		
		// Light grey vertical line at the right side of space window
		r.corner.x = SPACE_WIDTH - 2;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = SCREEN_HEIGHT - 1;
		DrawFilledRectangle (&r);
		
		// Vertical line at the right side of the menu window, upper part
		r.corner.x = SCREEN_WIDTH - 1;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = RES_BOOL(139, 434); // JMS_GFX
		DrawFilledRectangle (&r);
		
		// Horizontal line at the bottom of the screen, menu window side
		r.corner.x = SPACE_WIDTH;
		r.corner.y = SCREEN_HEIGHT - 1;
		r.extent.width = SCREEN_WIDTH - r.corner.x;
		r.extent.height = 1;
		DrawFilledRectangle (&r);
		
		// Vertical line at the right side of the menu window, lower part
		r.corner.x = SCREEN_WIDTH - 1;
		r.corner.y = RES_BOOL(140, 435);
		r.extent.width = 1;
		r.extent.height = (SCREEN_HEIGHT - 1) - r.corner.y;
		DrawFilledRectangle (&r);

		// Dark grey border around blue boxes.
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F));
		// Vertical line on the right side of the big blue box
		r.corner.y = 1; // There was a reason this was supposed to be "1": Serosis
		r.extent.width = 1;
		r.extent.height = SIS_MESSAGE_HEIGHT;
		r.corner.x = SIS_ORG_X + SIS_MESSAGE_BOX_WIDTH;
		DrawFilledRectangle (&r);

		// Vertical line on the right side of the small blue box
		r.corner.x = SIS_ORG_X + SIS_SCREEN_WIDTH;
		++r.extent.height;
		DrawFilledRectangle (&r);
		//
		r.corner.y = 0;
		r.extent.width = (SPACE_WIDTH - 2) - r.corner.x;
		r.extent.height = 1;
		DrawFilledRectangle (&r);
		//
		r.corner.x = 0;
		r.extent.width = SIS_ORG_X - r.corner.x;
		DrawFilledRectangle (&r);

		// Horizontal line between boxes
		r.corner.x = SIS_ORG_X + SIS_MESSAGE_BOX_WIDTH;
		r.extent.width = SIS_SPACER_BOX_WIDTH;
		DrawFilledRectangle (&r);
		//
		r.corner.x = 0;
		r.corner.y = 1;
		r.extent.width = 1;
		r.extent.height = (SCREEN_HEIGHT - 1) - r.corner.y;
		DrawFilledRectangle (&r);

		// Dark verticle line accent for the top left of the right panel
		r.corner.x = SPACE_WIDTH;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = RES_BOOL(139, 434); // JMS_GFX 
		DrawFilledRectangle (&r);

		// Horizontal line of the separator below the SIS gauges 
		r.corner.x = SPACE_WIDTH + 1;
		r.corner.y = RES_BOOL(139, 434); // JMS_GFX
		r.extent.width = STATUS_WIDTH - 2;
		r.extent.height = 1;
		DrawFilledRectangle (&r);

		// Dark verticle line accent for the bottom left of the right panel
		r.corner.x = SPACE_WIDTH;
		r.corner.y = RES_BOOL(140, 435); // JMS_GFX
		r.extent.width = 1;
		r.extent.height = SCREEN_HEIGHT - r.corner.y;
		DrawFilledRectangle (&r);

		DrawBorder(0, FALSE);
	}

	InitSISContexts ();
	ClearSISRect (DRAW_SIS_DISPLAY);

	UnbatchGraphics ();
}