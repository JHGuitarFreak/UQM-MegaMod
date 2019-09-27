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

#include "controls.h"
#include "options.h"
#include "settings.h"
#include "globdata.h"
#include "sis.h"
#include "setup.h"
#include "sounds.h"
#include "colors.h"
#include "fmv.h"
#include "resinst.h"
#include "nameref.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/sound/sound.h"
#include "libs/vidlib.h"
#include "libs/log.h"

#include <ctype.h>

static BOOLEAN ShowSlidePresentation (STRING PresStr);

typedef struct
{
	/* standard state required by DoInput */
	BOOLEAN (*InputFunc) (void *pInputState);

	/* Presentation state */
	TimeCount StartTime;
	TimeCount LastSyncTime;
	TimeCount TimeOut;
	int TimeOutOnSkip;
	STRING SlideShow;
#define MAX_FONTS 5
	FONT Fonts[MAX_FONTS];
	FRAME Frame;
	MUSIC_REF MusicRef;
	BOOLEAN Batched;
	FRAME SisFrame;
	FRAME RotatedFrame;
	int LastDrawKind;
	int LastAngle;
	COUNT OperIndex;
	Color TextFadeColor;
	Color TextColor;
	Color TextBackColor;
	int TextVPos;
	int TextEffect;
	RECT clip_r;
	RECT tfade_r;
#define MAX_TEXT_LINES 15
	TEXT TextLines[MAX_TEXT_LINES];
	COUNT LinesCount;
	char Buffer[512];
	int MovieFrame;
	int MovieEndFrame;
	int InterframeDelay;

} PRESENTATION_INPUT_STATE;

typedef struct {
	/* standard state required by DoInput */
	BOOLEAN (*InputFunc) (void *pInputState);

	/* Spinanim state */
	STAMP anim;
	TimeCount last_time;
	int debounce;
} SPINANIM_INPUT_STATE;

typedef struct
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (void *pInputState);

	LEGACY_VIDEO_REF CurVideo;

} VIDEO_INPUT_STATE;

static BOOLEAN DoPresentation (void *pIS);

static BOOLEAN
ParseColorString (const char *Src, Color* pColor)
{
	unsigned clr;
	if (1 != sscanf (Src, "%x", &clr))
		return FALSE;

	*pColor = BUILD_COLOR_RGBA (
			(clr >> 16) & 0xff, (clr >> 8) & 0xff, clr & 0xff, 0);
	return TRUE;
}

static BOOLEAN
DoFadeScreen (PRESENTATION_INPUT_STATE* pPIS, const char *Src, BYTE FadeType)
{
	int msecs;
	if (1 == sscanf (Src, "%d", &msecs))
	{
		pPIS->TimeOut = FadeScreen (FadeType, msecs * ONE_SECOND / 1000)
				+ ONE_SECOND / 10;
		pPIS->TimeOutOnSkip = FALSE;
	}
	return TRUE;
}

static void
DrawTextEffect (TEXT *pText, Color Fore, Color Back, int Effect)
{
	if (Effect == 'T')
	{
		font_DrawTracedText (pText, Fore, Back);
	}
	else
	{
		SetContextForeGroundColor (Fore);
		font_DrawText (pText);
	}
}

static COUNT
ParseTextLines (TEXT *Lines, COUNT MaxLines, char* Buffer)
{
	COUNT i;
	const char* pEnd = Buffer + strlen (Buffer);

	for (i = 0; i < MaxLines && Buffer < pEnd; ++i, ++Lines)
	{
		char* pTerm = strchr (Buffer, '\n');
		if (!pTerm)
			pTerm = Buffer + strlen (Buffer);
		*pTerm = '\0'; /* terminate string */
		Lines->pStr = Buffer;
		Lines->CharCount = ~0;
		Buffer = pTerm + 1;
	}
	return i;
}

static void
Present_BatchGraphics (PRESENTATION_INPUT_STATE* pPIS)
{
	if (!pPIS->Batched)
	{
		pPIS->Batched = TRUE;
		BatchGraphics ();
	}
}

static void
Present_UnbatchGraphics (PRESENTATION_INPUT_STATE* pPIS, BOOLEAN bYield)
{
	if (pPIS->Batched)
	{
		UnbatchGraphics ();
		pPIS->Batched = FALSE;
		if (bYield)
			TaskSwitch ();
	}
}

static void
Present_GenerateSIS (PRESENTATION_INPUT_STATE* pPIS)
{
#define MODULE_YOFS_P  ((RES_SCALE(-79)) + IF_HD(-94)) // JMS_GFX
#define DRIVE_TOP_Y_P  (DRIVE_TOP_Y + MODULE_YOFS_P)
#define JET_TOP_Y_P    (JET_TOP_Y + MODULE_YOFS_P)
#define MODULE_TOP_Y_P (MODULE_TOP_Y + MODULE_YOFS_P)
#define MODULE_TOP_X_P MODULE_TOP_X
#define JET_DRIVE_EXTRA_X IF_HD(-3)
	CONTEXT	OldContext;
	FRAME SisFrame;
	FRAME ModuleFrame;
	FRAME SkelFrame;
	STAMP s;
	RECT r;
	HOT_SPOT hs;
	int slot;
	COUNT piece;
	Color SisBack;

	OldContext = SetContext (OffScreenContext);

	SkelFrame = CaptureDrawable (LoadGraphic (SISSKEL_MASK_PMAP_ANIM));
	ModuleFrame = CaptureDrawable (LoadGraphic (SISMODS_MASK_PMAP_ANIM));

	GetFrameRect (SkelFrame, &r);
	SisFrame = CaptureDrawable (CreateDrawable (
			WANT_PIXMAP, r.extent.width, r.extent.height, 1
			));
	SetContextFGFrame (SisFrame);
	SetContextClipRect (NULL);
	SisBack = BUILD_COLOR (MAKE_RGB15 (0x01, 0x01, 0x01), 0x07);
	SetContextBackGroundColor (SisBack);
	ClearDrawable ();
	SetFrameTransparentColor (SisFrame, SisBack);

	s.frame = SetAbsFrameIndex (SkelFrame, 0);
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);

	for (slot = 0; slot < NUM_DRIVE_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS (DriveSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = DRIVE_TOP_X + JET_DRIVE_EXTRA_X;
			s.origin.y = DRIVE_TOP_Y_P;
			s.origin.x += slot * SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex (ModuleFrame, piece);
			DrawStamp (&s);
		}
	}
	for (slot = 0; slot < NUM_JET_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS (JetSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = JET_TOP_X + JET_DRIVE_EXTRA_X;
			s.origin.y = JET_TOP_Y_P;
			s.origin.x += slot * SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex (ModuleFrame, piece);
			DrawStamp (&s);
		}
	}
	for (slot = 0; slot < NUM_MODULE_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS (ModuleSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = MODULE_TOP_X_P;
			s.origin.y = MODULE_TOP_Y_P;
			s.origin.x += slot * SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex (ModuleFrame, piece);
			DrawStamp (&s);
		}
	}

	DestroyDrawable (ReleaseDrawable (SkelFrame));
	DestroyDrawable (ReleaseDrawable (ModuleFrame));

	hs.x = r.extent.width / 2;
	hs.y = r.extent.height / 2;
	SetFrameHot (SisFrame, hs);

	SetContext (OldContext);
	FlushGraphics ();

	pPIS->SisFrame = SisFrame;
}

static void
Present_DrawMovieFrame (PRESENTATION_INPUT_STATE* pPIS)
{
	STAMP s;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex (pPIS->Frame, pPIS->MovieFrame);
	DrawStamp (&s);
}

static BOOLEAN
ShowPresentationFile (const char *name)
{
	STRING pres = CaptureStringTable (LoadStringTableFile (contentDir, name));
	BOOLEAN result = ShowSlidePresentation (pres);
	DestroyStringTable (ReleaseStringTable (pres));
	return result;
}

static BOOLEAN
DoPresentation (void *pIS)
{
	PRESENTATION_INPUT_STATE* pPIS = (PRESENTATION_INPUT_STATE*) pIS;

	if (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
		return FALSE; /* abort requested - we are done */

	if (pPIS->TimeOut)
	{
		TimeCount Delay = ONE_SECOND / 84;

		if (GetTimeCounter () >= pPIS->TimeOut)
		{
			if (pPIS->MovieFrame >= 0)
			{	/* Movie mode */
				Present_DrawMovieFrame (pPIS);
				++pPIS->MovieFrame;
				if (pPIS->MovieFrame > pPIS->MovieEndFrame)
					pPIS->MovieFrame = -1; /* movie is done */
				Delay = pPIS->InterframeDelay;
			}
			else
			{	/* time elapsed - continue normal ops */
				pPIS->TimeOut = 0;
				return TRUE;
			}
		}
		
		if (pPIS->TimeOutOnSkip &&
			(PulsedInputState.menu[KEY_MENU_SELECT]
			|| PulsedInputState.menu[KEY_MENU_SPECIAL]
			|| PulsedInputState.menu[KEY_MENU_RIGHT]) )
		{	/* skip requested - continue normal ops */
			pPIS->TimeOut = 0;
			pPIS->MovieFrame = -1; /* abort any movie in progress */
			return TRUE;
		}

		SleepThread (Delay);
		return TRUE;
	}

	while (pPIS->OperIndex < GetStringTableCount (pPIS->SlideShow))
	{
		char Opcode[16];
		char *pStr = GetStringAddress (pPIS->SlideShow);

		pPIS->OperIndex++;
		pPIS->SlideShow = SetRelStringTableIndex (pPIS->SlideShow, 1);

		if (!pStr)
			continue;
		if (1 != sscanf (pStr, "%15s", Opcode))
			continue;
		pStr += strlen (Opcode);
		if (*pStr != '\0')
			++pStr;
		strupr (Opcode);

		if (strcmp (Opcode, "DIMS") == 0)
		{	/* set dimensions */
			int w, h;
			if (2 == sscanf (pStr, "%d %d", &w, &h))
			{
				w <<= RESOLUTION_FACTOR; // JMS_GFX
				h <<= RESOLUTION_FACTOR; // JMS_GFX

				pPIS->clip_r.extent.width = w;
				pPIS->clip_r.extent.height = h;
				/* center on screen */
				pPIS->clip_r.corner.x = (SCREEN_WIDTH - w) / 2;
				pPIS->clip_r.corner.y = (SCREEN_HEIGHT - h) / 2;
				SetContextClipRect (&pPIS->clip_r);
			}
		}
		else if (strcmp (Opcode, "FONT") == 0)
		{	/* set and/or load a font */
			int index;
			FONT *pFont;

			assert (sizeof (pPIS->Buffer) >= 256);

			pPIS->Buffer[0] = '\0';
			if (1 > sscanf (pStr, "%d %255[^\n]", &index, pPIS->Buffer) ||
					index < 0 || index >= MAX_FONTS)
			{
				log_add (log_Warning, "Bad FONT command '%s'", pStr);
				continue;
			}
			pFont = &pPIS->Fonts[index];

			if (pPIS->Buffer[0])
			{	/* asked to load a font */
				if (*pFont)
					DestroyFont (*pFont);
				*pFont = LoadFontFile (pPIS->Buffer);
			}

			SetContextFont (*pFont);
		}		
		else if (strcmp (Opcode, "FONT1X") == 0 && !IS_HD)
		{	/* set and/or load a font */
			int index;
			FONT *pFont;
			
			assert (sizeof (pPIS->Buffer) >= 256);
			
			pPIS->Buffer[0] = '\0';
			if (1 > sscanf (pStr, "%d %255[^\n]", &index, pPIS->Buffer) ||
				index < 0 || index >= MAX_FONTS)
			{
				log_add (log_Warning, "Bad FONT command '%s'", pStr);
				continue;
			}
			pFont = &pPIS->Fonts[index];
			
			if (pPIS->Buffer[0])
			{	/* asked to load a font */
				if (*pFont)
					DestroyFont (*pFont);
				*pFont = LoadFontFile (pPIS->Buffer);
			}
			SetContextFont (*pFont);
		}
		else if (strcmp (Opcode, "FONT4X") == 0 && IS_HD)
		{	/* set and/or load a font */
			int index;
			FONT *pFont;
			
			assert (sizeof (pPIS->Buffer) >= 256);
			
			pPIS->Buffer[0] = '\0';
			if (1 > sscanf (pStr, "%d %255[^\n]", &index, pPIS->Buffer) ||
				index < 0 || index >= MAX_FONTS)
			{
				log_add (log_Warning, "Bad FONT command '%s'", pStr);
				continue;
			}
			pFont = &pPIS->Fonts[index];
			
			if (pPIS->Buffer[0])
			{	/* asked to load a font */
				if (*pFont)
					DestroyFont (*pFont);
				*pFont = LoadFontFile (pPIS->Buffer);
			}
			SetContextFont (*pFont);
		}
		else if (strcmp (Opcode, "ANI") == 0)
		{	/* set ani */
			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			if (pPIS->Frame)
				DestroyDrawable (ReleaseDrawable (pPIS->Frame));
			pPIS->Frame = CaptureDrawable (LoadGraphicFile (pPIS->Buffer));
		}
		else if (strcmp (Opcode, "ANI1X") == 0 && !IS_HD)
		{	/* set ani */
			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			if (pPIS->Frame)
				DestroyDrawable (ReleaseDrawable (pPIS->Frame));
			pPIS->Frame = CaptureDrawable (LoadGraphicFile (pPIS->Buffer));
		}
		else if (strcmp (Opcode, "ANI4X") == 0 && IS_HD)
		{	/* set ani */
			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			if (pPIS->Frame)
				DestroyDrawable (ReleaseDrawable (pPIS->Frame));
			pPIS->Frame = CaptureDrawable (LoadGraphicFile (pPIS->Buffer));
		}
		else if (strcmp (Opcode, "MUSIC") == 0)
		{	/* set music */
			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			if (pPIS->MusicRef)
			{
				StopMusic ();
				DestroyMusic (pPIS->MusicRef);
			}
			pPIS->MusicRef = LoadMusicFile (pPIS->Buffer);
			PlayMusic (pPIS->MusicRef, FALSE, 1);
		}
		else if (strcmp (Opcode, "WAIT") == 0)
		{	/* wait */
			int msecs;
			Present_UnbatchGraphics (pPIS, TRUE);
			if (1 == sscanf (pStr, "%d", &msecs))
			{
				pPIS->TimeOut = GetTimeCounter ()
						+ msecs * ONE_SECOND / 1000;
				pPIS->TimeOutOnSkip = TRUE;
				return TRUE;
			}
		}
		else if (strcmp (Opcode, "SYNC") == 0)
		{	/* absolute time-sync */
			int msecs;
			Present_UnbatchGraphics (pPIS, TRUE);
			if (1 == sscanf (pStr, "%d", &msecs))
			{
				pPIS->LastSyncTime = pPIS->StartTime
						+ msecs * ONE_SECOND / 1000;
				pPIS->TimeOut = pPIS->LastSyncTime;
				pPIS->TimeOutOnSkip = FALSE;
				return TRUE;
			}
		}
		else if (strcmp (Opcode, "RESYNC") == 0)
		{	/* flush and update absolute sync point */
			pPIS->LastSyncTime = pPIS->StartTime = GetTimeCounter ();
		}
		else if (strcmp (Opcode, "DSYNC") == 0)
		{	/* delta time-sync; from the last absolute sync */
			int msecs;
			Present_UnbatchGraphics (pPIS, TRUE);
			if (1 == sscanf (pStr, "%d", &msecs))
			{
				pPIS->TimeOut = pPIS->LastSyncTime
						+ msecs * ONE_SECOND / 1000;
				pPIS->TimeOutOnSkip = FALSE;
				return TRUE;
			}
		}
		else if (strcmp (Opcode, "TC") == 0)
		{	/* text fore color */
			ParseColorString (pStr, &pPIS->TextColor);
		}
		else if (strcmp (Opcode, "TBC") == 0)
		{	/* text back color */
			ParseColorString (pStr, &pPIS->TextBackColor);
		}
		else if (strcmp (Opcode, "TFC") == 0)
		{	/* text fade color */
			ParseColorString (pStr, &pPIS->TextFadeColor);
		}
		else if (strcmp (Opcode, "TVA") == 0)
		{	/* text vertical align */
			pPIS->TextVPos = toupper (*pStr);
		}
		else if (strcmp (Opcode, "TE") == 0)
		{	/* text vertical align */
			pPIS->TextEffect = toupper (*pStr);
		}
		else if (strcmp (Opcode, "TEXT") == 0)
		{	/* simple text draw */
			int x, y;

			assert (sizeof (pPIS->Buffer) >= 256);

			if (3 == sscanf (pStr, "%d %d %255[^\n]", &x, &y, pPIS->Buffer))
			{
				TEXT t;

				x <<= RESOLUTION_FACTOR; // JMS_GFX
				y <<= RESOLUTION_FACTOR; // JMS_GFX

				t.align = ALIGN_CENTER;
				t.pStr = pPIS->Buffer;
				t.CharCount = (COUNT)~0;
				t.baseline.x = x;
				t.baseline.y = y;
				DrawTextEffect (&t, pPIS->TextColor, pPIS->TextBackColor,
						pPIS->TextEffect);
			}
		}
		else if (strcmp (Opcode, "TFI") == 0)
		{	/* text fade-in */
			SIZE leading;
			COUNT i;
			COORD y;
			
			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			pPIS->LinesCount = ParseTextLines (pPIS->TextLines,
					MAX_TEXT_LINES, pPIS->Buffer);
			
			Present_UnbatchGraphics (pPIS, TRUE);

			GetContextFontLeading (&leading);

			switch (pPIS->TextVPos)
			{
			case 'T': /* top */
				y = leading;
				break;
			case 'M': /* middle */
				y = (pPIS->clip_r.extent.height
						- pPIS->LinesCount * leading) / 2;
				break;
			default: /* bottom */
				y = pPIS->clip_r.extent.height - pPIS->LinesCount * leading;
			}
			pPIS->tfade_r = pPIS->clip_r;
			pPIS->tfade_r.corner.y += y - leading;
			pPIS->tfade_r.extent.height = (pPIS->LinesCount + 1) * leading;
			for (i = 0; i < pPIS->LinesCount; ++i, y += leading)
			{
				pPIS->TextLines[i].align = ALIGN_CENTER;
				pPIS->TextLines[i].baseline.x = SCREEN_WIDTH / 2;
				pPIS->TextLines[i].baseline.y = y;
			}

			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTextEffect (pPIS->TextLines + i, pPIS->TextFadeColor,
						pPIS->TextFadeColor, pPIS->TextEffect);

			/* do transition */
			SetTransitionSource (&pPIS->tfade_r);
			BatchGraphics ();
			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTextEffect (pPIS->TextLines + i, pPIS->TextColor,
						pPIS->TextBackColor, pPIS->TextEffect);
			ScreenTransition (3, &pPIS->tfade_r);
			UnbatchGraphics ();
			
		}
		else if (strcmp (Opcode, "TFO") == 0)
		{	/* text fade-out */
			COUNT i;
			
			Present_UnbatchGraphics (pPIS, TRUE);

			/* do transition */
			SetTransitionSource (&pPIS->tfade_r);
			BatchGraphics ();
			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTextEffect (pPIS->TextLines + i, pPIS->TextFadeColor,
						pPIS->TextFadeColor, pPIS->TextEffect);
			ScreenTransition (3, &pPIS->tfade_r);
			UnbatchGraphics ();
		}
		else if (strcmp (Opcode, "SAVEBG") == 0)
		{	/* save background */
			TFB_DrawScreen_Copy (&pPIS->clip_r,
					TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA, FALSE);
		}
		else if (strcmp (Opcode, "RESTBG") == 0)
		{	/* restore background */
			TFB_DrawScreen_Copy (&pPIS->clip_r,
					TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN, FALSE);
		}
		else if (strcmp (Opcode, "DRAW") == 0)
		{	/* draw a graphic */
#define PRES_DRAW_INDEX 0
#define PRES_DRAW_SIS   1
			int cargs;
			int draw_what;
			int index = 0;
			int x, y;
			int scale;
			int angle;
			int scale_mode;
			char ImgName[16];
			int old_scale, old_mode;
			STAMP s;

			if (1 == sscanf (pStr, "%15s", ImgName)
					&& strcmp (strupr (ImgName), "SIS") == 0)
			{
				draw_what = PRES_DRAW_SIS;
				scale_mode = TFB_SCALE_NEAREST;
				cargs = sscanf (pStr, "%*s %d %d %d %d",
							&x, &y, &scale, &angle) + 1;
			}
			else
			{
				draw_what = PRES_DRAW_INDEX;
				scale_mode = TFB_SCALE_BILINEAR;
				cargs = sscanf (pStr, "%d %d %d %d %d",
							&index, &x, &y, &scale, &angle);
			}

			if (cargs < 1)
			{
				log_add (log_Warning, "Bad DRAW command '%s'", pStr);
				continue;
			}
			if (cargs < 5)
				angle = 0;
			if (cargs < 4)
				scale = GSCALE_IDENTITY;
			if (cargs < 3)
			{
				x = 0;
				y = 0;
			}

			x <<= RESOLUTION_FACTOR; // JMS_GFX
			y <<= RESOLUTION_FACTOR; // JMS_GFX

			s.frame = NULL;
			if (draw_what == PRES_DRAW_INDEX)
			{	/* draw stamp by index */
				s.frame = SetAbsFrameIndex (pPIS->Frame, (COUNT)index);
			}
			else if (draw_what == PRES_DRAW_SIS)
			{	/* draw dynamic SIS image with player's modules */
				if (!pPIS->SisFrame)
					Present_GenerateSIS (pPIS);

				s.frame = SetAbsFrameIndex (pPIS->SisFrame, 0);
			}
			if (angle != 0)
			{
				if (angle != pPIS->LastAngle
						|| draw_what != pPIS->LastDrawKind)
				{
					DestroyDrawable (ReleaseDrawable (pPIS->RotatedFrame));
					pPIS->RotatedFrame = CaptureDrawable (
							RotateFrame (s.frame, -angle));
					pPIS->LastAngle = angle;
					pPIS->LastDrawKind = draw_what;
				}
				s.frame = pPIS->RotatedFrame;
			}
			s.origin.x = x;
			s.origin.y = y;
			old_mode = SetGraphicScaleMode (scale_mode);
			old_scale = SetGraphicScale (scale);
			DrawStamp (&s);
			SetGraphicScale (old_scale);
			SetGraphicScaleMode (old_mode);
		}
		else if (strcmp (Opcode, "BATCH") == 0)
		{	/* batch graphics */
			Present_BatchGraphics (pPIS);
		}
		else if (strcmp (Opcode, "UNBATCH") == 0)
		{	/* unbatch graphics */
			Present_UnbatchGraphics (pPIS, FALSE);
		}
		else if (strcmp (Opcode, "FTC") == 0)
		{	/* fade to color */
			Present_UnbatchGraphics (pPIS, TRUE);
			return DoFadeScreen (pPIS, pStr, FadeAllToColor);
		}
		else if (strcmp (Opcode, "FTB") == 0)
		{	/* fade to black */
			Present_UnbatchGraphics (pPIS, TRUE);
			return DoFadeScreen (pPIS, pStr, FadeAllToBlack);
		}
		else if (strcmp (Opcode, "FTW") == 0)
		{	/* fade to white */
			Present_UnbatchGraphics (pPIS, TRUE);
			return DoFadeScreen (pPIS, pStr, FadeAllToWhite);
		}
		else if (strcmp (Opcode, "CLS") == 0)
		{	/* clear screen */
			Present_UnbatchGraphics (pPIS, TRUE);

			ClearDrawable ();	
		}
		else if (strcmp (Opcode, "CALL") == 0)
		{	/* call another script */
			Present_UnbatchGraphics (pPIS, TRUE);

			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			ShowPresentationFile (pPIS->Buffer);
		}
		else if (strcmp (Opcode, "LINE") == 0)
		{
			int x1, x2, y1, y2;
			if (4 == sscanf (pStr, "%d %d %d %d", &x1, &y1, &x2, &y2))
			{
				LINE l;

				x1 <<= RESOLUTION_FACTOR; // JMS_GFX
				y1 <<= RESOLUTION_FACTOR; // JMS_GFX
				x2 <<= RESOLUTION_FACTOR; // JMS_GFX
				y2 <<= RESOLUTION_FACTOR; // JMS_GFX

				l.first.x = x1;
				l.first.y = y1;
				l.second.x = x2;
				l.second.y = y2;
				
				SetContextForeGroundColor (pPIS->TextColor);
				DrawLine (&l);
			}
			else
			{
				log_add (log_Warning, "Bad LINE command '%s'", pStr);
			}
		}
		else if (strcmp (Opcode, "MOVIE") == 0)
		{
			int fps, from, to;
		
			if (3 == sscanf (pStr, "%d %d %d", &fps, &from, &to) &&
					fps > 0 && from >= 0 && to >= 0 && to >= from)
			{
				Present_UnbatchGraphics (pPIS, TRUE);
				
				pPIS->MovieFrame = from;
				pPIS->MovieEndFrame = to;
				pPIS->InterframeDelay = ONE_SECOND / fps;

				pPIS->TimeOut = GetTimeCounter ();
				pPIS->TimeOutOnSkip = TRUE;
				return TRUE;
			}
			else
			{
				log_add (log_Warning, "Bad MOVIE command '%s'", pStr);
			}
		}
		else if (strcmp (Opcode, "NOOP") == 0)
		{	/* no operation - must be a comment in script */
			/* do nothing */
		}
	}
	/* we are all done */
	return FALSE;
}

static BOOLEAN
ShowSlidePresentation (STRING PresStr)
{
	CONTEXT OldContext;
	FONT OldFont;
	RECT OldRect;
	PRESENTATION_INPUT_STATE pis;
	int i;

	memset (&pis, 0, sizeof(pis));
	pis.SlideShow = PresStr;
	if (!pis.SlideShow)
		return FALSE;
	pis.SlideShow = SetAbsStringTableIndex (pis.SlideShow, 0);
	pis.OperIndex = 0;

	OldContext = SetContext (ScreenContext);
	GetContextClipRect (&OldRect);
	OldFont = SetContextFont (NULL);
	SetContextBackGroundColor (BLACK_COLOR);

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	pis.InputFunc = DoPresentation;
	pis.LastDrawKind = -1;
	pis.TextVPos = 'B';
	pis.MovieFrame = -1;
	pis.StartTime = GetTimeCounter ();
	pis.LastSyncTime = pis.StartTime;

	DoInput(&pis, TRUE);

	SleepThreadUntil (FadeMusic (0, ONE_SECOND));
	StopMusic ();
	FadeMusic (NORMAL_VOLUME, 0);

	DestroyMusic (pis.MusicRef);
	DestroyDrawable (ReleaseDrawable (pis.RotatedFrame));
	DestroyDrawable (ReleaseDrawable (pis.Frame));
	for (i = 0; i < MAX_FONTS; ++i)
		DestroyFont (pis.Fonts[i]);

	SetContextFont (OldFont);
	SetContextClipRect (&OldRect);
	SetContext (OldContext);

	return TRUE;
}

static BOOLEAN
DoVideoInput (void *pIS)
{
	VIDEO_INPUT_STATE* pVIS = (VIDEO_INPUT_STATE*) pIS;

	if (!PlayingLegacyVideo (pVIS->CurVideo))
	{	// Video probably finished
		return FALSE;
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT]
			|| PulsedInputState.menu[KEY_MENU_CANCEL]
			|| PulsedInputState.menu[KEY_MENU_SPECIAL]
			|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
	{	// abort movie
		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT]
			|| PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		SDWORD newpos = VidGetPosition ();
		if (PulsedInputState.menu[KEY_MENU_LEFT])
			newpos -= 2000;
		else if (PulsedInputState.menu[KEY_MENU_RIGHT])
			newpos += 1000;
		if (newpos < 0)
			newpos = 0;

		VidSeek (newpos);
	}
	else
	{
		if (!VidProcessFrame ())
			return FALSE;

		SleepThread (ONE_SECOND / 40);
	}

	return TRUE;
}

static void
FadeClearScreen (void)
{
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
	
	// clear the screen with black
	SetContext (ScreenContext);
	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();

	FadeScreen (FadeAllToColor, 0);
}

static BOOLEAN
ShowLegacyVideo (LEGACY_VIDEO vid)
{
	VIDEO_INPUT_STATE vis;
	LEGACY_VIDEO_REF ref;

	FadeClearScreen ();

	ref = PlayLegacyVideo (vid);
	if (!ref)
		return FALSE;

	vis.InputFunc = DoVideoInput;
	vis.CurVideo = ref;
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);

	DoInput(&vis, TRUE);

	StopLegacyVideo (ref);
	FadeClearScreen ();

	return TRUE;
}

BOOLEAN
ShowPresentation (RESOURCE res)
{
	const char *resType = res_GetResourceType (res);
	if (!resType)
	{
		return FALSE;
	}
	if (!strcmp (resType, "STRTAB"))
	{
		STRING pres = CaptureStringTable (LoadStringTable (res));
		BOOLEAN result = ShowSlidePresentation (pres);
		DestroyStringTable (ReleaseStringTable (pres));
		return result;
	}
	else if (!strcmp (resType, "3DOVID"))
	{
		LEGACY_VIDEO vid = LoadLegacyVideoInstance (res);
		BOOLEAN result = ShowLegacyVideo (vid);
		DestroyLegacyVideo (vid);
		return result;
	}
	
	log_add (log_Warning, "Tried to present '%s', of non-presentable type '%s'", res, resType);
	return FALSE;
}
