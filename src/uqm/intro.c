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
#include "libs/inplib.h"
#include "util.h"
#include "build.h"

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

	// For DOS Spins
	RECT StatBox;
	COUNT NumSpinStat;
	RECT GetRect;
	COUNT CurrentFrameIndex;
	BOOLEAN HaveFrame;
	BOOLEAN Skip;

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
			(clr >> 16) & 0xff, (clr >> 8) & 0xff, clr & 0xff, 0xff);
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
#define MODULE_YOFS_P  -(RES_SCALE (79) + IF_HD (33))
#define DRIVE_TOP_Y_P  (DRIVE_TOP_Y + MODULE_YOFS_P)
#define JET_TOP_Y_P    (JET_TOP_Y + MODULE_YOFS_P)
#define MODULE_TOP_Y_P (MODULE_TOP_Y + MODULE_YOFS_P)
#define MODULE_TOP_X_P MODULE_TOP_X
	CONTEXT OldContext;
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
			WANT_PIXMAP | WANT_ALPHA, r.extent.width, r.extent.height, 1
			));
	SetContextFGFrame (SisFrame);
	SetContextClipRect (NULL);
	SisBack = BUILD_COLOR_RGBA (0, 0, 0, 0);
	SetContextBackGroundColor (SisBack);
	ClearDrawable ();

	s.frame = SetAbsFrameIndex (SkelFrame, is3DO (optFlagshipColor));
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);

	for (slot = 0; slot < NUM_DRIVE_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS (DriveSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = DRIVE_TOP_X;
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
			s.origin.x = JET_TOP_X;
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
			if (piece >= BOMB_MODULE_0)
			{// Blue glow around the bomb - mimic PC-DOS pre-rendered cutscene
				Color oldColor = SetContextForeGroundColor (
						BUILD_COLOR_RGB (0x00, 0x00, 0x5F));

				s.origin.y += RES_SCALE (2);
				DrawFilledStamp (&s);

				s.origin.y -= RES_SCALE (4);
				DrawFilledStamp (&s);

				SetContextForeGroundColor (
						BUILD_COLOR_RGB (0x00, 0x07, 0xFF));

				s.origin.y += RES_SCALE (1);
				DrawFilledStamp (&s);

				s.origin.y += RES_SCALE (2);
				DrawFilledStamp (&s);

				s.origin.y -= RES_SCALE (1);
				SetContextForeGroundColor (oldColor);
			}
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
DoSpinText (UNICODE *buf, COORD x, COORD y, FRAME repair, BOOLEAN *skip)
{
	TEXT Text;

	Text.pStr = buf;
	Text.CharCount = (COUNT)utf8StringCount (buf);
	Text.align = ALIGN_LEFT;
	Text.baseline.y = y;
	Text.baseline.x = x;

	font_DrawText_Fade (&Text, repair, skip);
}

static void
DoSpinLine (LINE *l, Color front, Color back, BOOLEAN *skip)
{
	if (!*skip)
	{
		SetContextForeGroundColor (back);
		DrawLine (l, RES_SCALE (1));
		PlayMenuSound (MENU_SOUND_TEXT);
		SleepThread (ONE_SECOND / 16);
	}
	SetContextForeGroundColor (front);
	DrawLine (l, RES_SCALE (1));
}

static void
DoSpinStatBox (RECT *r, Color front, Color back, BOOLEAN *skip)
{
	if (!*skip)
	{
		DrawStarConBox (r, RES_SCALE (1), back, back, FALSE, BLACK_COLOR, FALSE, BLACK_COLOR);
		PlayMenuSound (MENU_SOUND_TEXT);
		SleepThread (ONE_SECOND / 16);
	}
	DrawStarConBox (r, RES_SCALE (1), front, front, FALSE, BLACK_COLOR, FALSE, BLACK_COLOR);
}

static void
DoSpinStat (UNICODE *buf, COORD x, COORD y, COUNT filled, COUNT empty, Color front, Color back,
		BOOLEAN *skip)
{
	TEXT Text;
	COUNT i;
	RECT sq;
	POINT c;
	RECT chd;

	sq.corner.x = x + RES_SCALE (63);
	sq.corner.y = y - RES_SCALE (5);
	sq.extent.width = sq.extent.height = RES_SCALE (5);

	chd.extent.width = chd.extent.height = 4;

	Text.pStr = buf;
	Text.CharCount = (COUNT)utf8StringCount (buf);
	Text.align = ALIGN_LEFT;
	Text.baseline.y = y;
	Text.baseline.x = x;

	if (!*skip)
	{
		SetContextForeGroundColor (back);
		font_DrawText (&Text);
		PlayMenuSound (MENU_SOUND_TEXT);
		SleepThread (ONE_SECOND / 16);
	}
	SetContextForeGroundColor (front);
	font_DrawText (&Text);

	for (i = 0; i < filled; i++)
	{
		if (!*skip)
		{
			SetContextForeGroundColor (back);
			DrawFilledRectangle (&sq);
			PlayMenuSound (MENU_SOUND_TEXT);
			SleepThread (ONE_SECOND / 16);
		}
		SetContextForeGroundColor (front);
		DrawFilledRectangle (&sq);
		sq.corner.x += RES_SCALE (6);

		UpdateInputState ();
		if (CurrentInputState.menu[KEY_MENU_CANCEL] || 
					(GLOBAL (CurrentActivity) & CHECK_ABORT))
			*skip = TRUE;
	}
	for (i = 0; i < empty; i++)
	{
		c.x = sq.corner.x + RES_SCALE (2);
		c.y = sq.corner.y + RES_SCALE (2);
		if (!*skip)
		{
			SetContextForeGroundColor (back);
			DrawStarConBox (&sq, RES_SCALE (1), back, back, FALSE, BLACK_COLOR, FALSE, BLACK_COLOR);
			if (IS_HD)
			{
				chd.corner = c;
				DrawFilledRectangle (&chd);
			}
			else
				DrawPoint (&c);
			PlayMenuSound (MENU_SOUND_TEXT);
			SleepThread (ONE_SECOND / 16);
		}
		SetContextForeGroundColor (front);
		DrawStarConBox (&sq, RES_SCALE (1), front, front, FALSE, BLACK_COLOR, FALSE, BLACK_COLOR);
		if (IS_HD)
		{
			chd.corner = c;
			DrawFilledRectangle (&chd);
		}
		else
			DrawPoint (&c);
		sq.corner.x += RES_SCALE (6);

		UpdateInputState ();
		if (CurrentInputState.menu[KEY_MENU_CANCEL] || 
					(GLOBAL (CurrentActivity) & CHECK_ABORT))
			*skip = TRUE;
	}
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

typedef struct
{
	SPECIES_ID sID;		// The ship SPECIES_ID, a second index
	char ditty[256];	// The ditty string (file name)
	char race[256];		// The race's name string e.g. EARTHLING
	char ship[256];		// The ship's name string e.g. CRUISER
	COUNT spinline;		// The line on which race/ship strings occur
	COUNT width;		// The SD pixel width of the race name
} SHIPMAP;

static const SHIPMAP ship_map[] = {
		{ ARILOU_ID, "arilou", "ARILOULALEELAY", " SKIFF", 2, 123 },
		{ CHMMR_ID, "chmmr", "CHMMR", " AVATAR", 2, 50 },
		{ EARTHLING_ID, "earthling", "EARTHLING", " CRUISER", 130, 79 },
		{ ORZ_ID, "orz", "ORZ", " NEMESIS", 2, 25 },
		{ PKUNK_ID, "pkunk", "PKUNK", " FURY", 2, 43 },
		{ SHOFIXTI_ID, "shofixti", "SHOFIXTI", " SCOUT", 4, 61 },
		{ SPATHI_ID, "spathi", "SPATHI", " ELUDER", 2, 47 },
		{ SUPOX_ID, "supox", "SUPOX", " BLADE", 2, 43 },
		{ THRADDASH_ID, "thraddash", "THRADDASH", " TORCH", 2, 87 },
		{ UTWIG_ID, "utwig", "UTWIG", " JUGGER", 2, 43 },
		{ VUX_ID, "vux", "VUX", " INTRUDER", 130, 28 },
		{ YEHAT_ID, "yehat", "YEHAT", " TERMINATOR", 130, 46 },
		{ MELNORME_ID, "melnorme", "MELNORME", " TRADER", 2, 77 },
		{ DRUUGE_ID, "druuge", "DRUUGE", " MAULER", 2, 53 },
		{ ILWRATH_ID, "ilwrath", "ILWRATH", " AVENGER", 2, 62 },
		{ MYCON_ID, "mycon", "MYCON", " PODSHIP", 127, 47 },
		{ SLYLANDRO_ID, "slylandro", "SLYLANDRO", " PROBE", 1, 80 },
		{ UMGAH_ID, "umgah", "UMGAH", " DRONE", 127, 50 },
		{ UR_QUAN_ID, "urquan", "UR-QUAN", "\nDREADNAUGHT", 119, 66 },
		{ ZOQFOTPIK_ID, "zoqfotpik", "ZOQ-FOT-PIK", " STINGER", 130, 92 },
		{ SYREEN_ID, "syreen", "SYREEN", " PENETRATOR", 2, 53 },
		{ KOHR_AH_ID, "kohrah", "KOHR-AH", "", 119, 64 },
		{ ANDROSYNTH_ID, "androsynth", "ANDROSYNTH", " GUARDIAN", 2, 94 },
		{ CHENJESU_ID, "chenjesu", "CHENJESU", "    BROODHOME", 1, 71 },
		{ MMRNMHRM_ID, "mmrnmhrm", "MMRNMHRM", " TRANSFORMER", 128, 84 },
};

#define NUM_SHIPS (sizeof (ship_map) / sizeof (SHIPMAP))

static COUNT shipID = NUM_SHIPS;
static COUNT raceID = NUM_SHIPS;
static BOOLEAN linespun = false;

static void
SeedDitty (char *buf, size_t size, char *str)
{
	if (!optShipSeed)
		goto SeedDittyPassThru;

	for (shipID = 0; shipID < NUM_SHIPS; shipID++)
		if (!strcasecmp (str, ship_map[shipID].ditty))
			break;
	if (shipID >= NUM_SHIPS)
		goto SeedDittyPassThru;

	for (raceID = 0; raceID < NUM_SHIPS; raceID++)
		if (SeedShip (ship_map[raceID].sID, false) == ship_map[shipID].sID)
			break;
	if (raceID >= NUM_SHIPS)
		goto SeedDittyPassThru;

	linespun = false;
	snprintf (buf, size, "ship.%s.ditty", ship_map[raceID].ditty);
	return;

SeedDittyPassThru:
	shipID = raceID = NUM_SHIPS;
	snprintf (buf, size, "ship.%s.ditty", str);
	return;
}

static void
SeedLineSpin (int *x1, int *y1, int *x2, int *y2)
{
	if (!optShipSeed || !x1 || !x2 || !y1 || !y2 || shipID == raceID ||
			shipID >= NUM_SHIPS || raceID >= NUM_SHIPS)
		goto SeedLineSpinPassThru;

	if (ship_map[shipID].sID == KOHR_AH_ID &&
			ship_map[raceID].sID != ARILOU_ID &&
			ship_map[raceID].width > 80 && *y2 == 122)
		*x2 -= ship_map[raceID].width - 80; // Shorten line for Spinning Blade
	if (ship_map[shipID].sID == DRUUGE_ID && !IS_HD &&
			ship_map[raceID].width > 80 && *y1 == 21)
	{ // Line for High-recoil Cannon in SD, needs to move down for wide ships
		*y1 += 10;
		*y2 += 10;
	}
SeedLineSpinPassThru:
	return;
}

static void
SeedTextSpin (char *buf, size_t size, char *str, int *x, int *y)
{
	if (!optShipSeed || !x || !y || shipID == raceID ||
			shipID >= NUM_SHIPS || raceID >= NUM_SHIPS)
		goto SeedTextSpinPassThru;

	if (ship_map[shipID].sID == SPATHI_ID && IS_HD &&
			ship_map[raceID].sID != ARILOU_ID &&
			ship_map[raceID].width > 80 && (*x == 25 || *x == 40))
		*x -= 15; // Rear-firing Missile Launch Tube, move left for wide ships
	if (ship_map[shipID].sID == DRUUGE_ID && !IS_HD &&
			ship_map[raceID].width > 80 && *x == 108)
		*y += 10; // This is High-recoil Cannon, move down for wide ships
	if (ship_map[shipID].sID == SLYLANDRO_ID && *x == 18)
		goto SeedTextSpinSkipLine; // This is 2418-B skip unless O.G.
	if (ship_map[shipID].spinline != *y)
		goto SeedTextSpinPassThru; // This is anything not on the spinline
	if (ship_map[shipID].sID == MELNORME_ID && *x == 215)
		goto SeedTextSpinPassThru; // This is Confusion Ray, on the spinline
	if (ship_map[shipID].sID == UR_QUAN_ID && *x == 222)
		goto SeedTextSpinPassThru; // This is Fusion Blast, on the spinline
	if (linespun)
		goto SeedTextSpinSkipLine; // Anything on the spinline we've done

	linespun = true;
	utf8StringCopy (buf, size, ship_map[raceID].race);
	buf += strlen(ship_map[raceID].race) * sizeof (char);

	switch (ship_map[shipID].sID)
	{
		case CHENJESU_ID:
		case EARTHLING_ID:
		case KOHR_AH_ID:
		case MYCON_ID:
		case THRADDASH_ID: // Right hand huggers
			if ((ship_map[shipID].sID == EARTHLING_ID ||
					(ship_map[shipID].sID == KOHR_AH_ID && IS_HD)) &&
					ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof (char);
				*x += ship_map[shipID].width - 50;
				break;
			}
			if (ship_map[shipID].sID == CHENJESU_ID && IS_HD)
				*x -= 10; // Better centering the gap over the broodhome
			*x += ship_map[shipID].width - ship_map[raceID].width;
			break;
		case SHOFIXTI_ID: // Right hand hugger but squeeze it a little
			*x += (ship_map[shipID].width - ship_map[raceID].width) * 4 / 5;
			break;
		case VUX_ID: // Middle but squeeze the right a little
			if (ship_map[shipID].sID == VUX_ID &&
					ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof (char);
				*x += (ship_map[shipID].width - 50) * 3 / 5;
				break;
			}
			*x += (ship_map[shipID].width - ship_map[raceID].width) * 3 / 5;
			break;
		case ANDROSYNTH_ID:
		case ARILOU_ID:
		case PKUNK_ID:
		case SPATHI_ID:
		case SYREEN_ID:
		case UTWIG_ID:
		case MMRNMHRM_ID: // Middle-ships
			if (ship_map[shipID].sID == SPATHI_ID &&
					ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof (char);
				*x += (ship_map[shipID].width - 50) / 2;
				break;
			}
			if (ship_map[shipID].sID == SPATHI_ID)
				*x += 2; // Better centering over (In Attack Position)
			if (ship_map[shipID].sID == ARILOU_ID)
				*x += 4; // Better centering over (Last Reported Position)
			*x += (ship_map[shipID].width - ship_map[raceID].width) / 2;
			break;
		case CHMMR_ID:
		case ILWRATH_ID:
		case MELNORME_ID:
		case ORZ_ID:
		case SLYLANDRO_ID:
		case UMGAH_ID:
		case UR_QUAN_ID:
		case YEHAT_ID:
		case ZOQFOTPIK_ID: // Lefties need no adjustment
			if ((ship_map[shipID].sID == CHMMR_ID ||
					ship_map[shipID].sID == ORZ_ID ||
					ship_map[shipID].sID == ZOQFOTPIK_ID) &&
					ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof (char);
				break;
			}
			if (ship_map[shipID].sID == UMGAH_ID && !IS_HD)
			{ // Right hand hugger but squeeze it a little
				*x += (ship_map[shipID].width - ship_map[raceID].width)
						* 4 / 5;
				break;
			}
			break;
		case DRUUGE_ID:
		case SUPOX_ID: // Lefties but line break when long
			if (ship_map[raceID].width > 80)
			{
				char *pad = (IS_HD ? "\n   " : "\n       ");
				utf8StringCopy (buf, size, pad);
				buf += strlen (pad) * sizeof (char);
			}
			break;
		default:
			break;
	}

	utf8StringCopy (buf, size, ship_map[shipID].ship);
	return;

SeedTextSpinSkipLine:
	utf8StringCopy (buf, size, "");
	return;

SeedTextSpinPassThru:
	utf8StringCopy (buf, size, str);
	return;
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
				w <<= RESOLUTION_FACTOR;
				h <<= RESOLUTION_FACTOR;

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
		else if (strcmp (Opcode, "DITTY") == 0)
		{	/* set ditty */
			if (optShipSeed)
				SeedDitty (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			else
				snprintf (pPIS->Buffer, sizeof (pPIS->Buffer),
						"ship.%s.ditty", pStr);

			if (pPIS->MusicRef)
			{
				StopMusic ();
				DestroyMusic (pPIS->MusicRef);
			}

			pPIS->MusicRef = LoadMusic (pPIS->Buffer);
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
		else if (strcmp (Opcode, "WAITDITTY") == 0)
		{	/* wait for ditty to end */
			while (PlayingStream (MUSIC_SOURCE))
			{
				if (CurrentInputState.menu[KEY_MENU_CANCEL]
						|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
				{
					StopMusic ();
					pPIS->Skip = TRUE;
					return TRUE;
				}
				SleepThread (ONE_SECOND / 10);
				UpdateInputState ();
			}
		}
		else if (strcmp (Opcode, "SPINWAIT") == 0)
		{	/* special wait during spin */
			int msecs;
			TimeCount TimeOut;
			if (1 == sscanf (pStr, "%d", &msecs) && !pPIS->Skip)
			{
				TimeOut = GetTimeCounter ()
						+ msecs * ONE_SECOND / 1000;
				while (GetTimeCounter () < TimeOut)
				{
					if (CurrentInputState.menu[KEY_MENU_CANCEL]
						|| (GLOBAL(CurrentActivity) & CHECK_ABORT))
					{
						Present_BatchGraphics (pPIS);
						pPIS->Skip = TRUE;
						return TRUE;
					}
					SleepThread (ONE_SECOND / 84);
					UpdateInputState ();
				}	
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
		else if (strcmp (Opcode, "BGC") == 0)
		{	/* text fore color */
			Color temp;
			ParseColorString (pStr, &temp);

			SetContextBackGroundColor (temp);
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
		{	/* text effect */
			pPIS->TextEffect = toupper (*pStr);
		}
		else if (strcmp (Opcode, "TEXT") == 0)
		{	/* simple text draw */
			int x, y;

			assert (sizeof (pPIS->Buffer) >= 256);

			if (3 == sscanf (pStr, "%d %d %255[^\n]", &x, &y, pPIS->Buffer))
			{
				TEXT t;

				t.align = ALIGN_CENTER;
				t.pStr = pPIS->Buffer;
				t.CharCount = (COUNT)~0;
				t.baseline.x = RES_SCALE (x);
				t.baseline.y = RES_SCALE (y);
				DrawTextEffect (&t, pPIS->TextColor, pPIS->TextBackColor,
						pPIS->TextEffect);
			}
		}
		else if (strcmp (Opcode, "TEXTSPIN") == 0)
		{	/* spin text draw */
			int x, y;
			int n = 0;

			assert (sizeof (pPIS->Buffer) >= 256);

			if (2 == sscanf (pStr, "%d %d %n", &x, &y, &n))
			{
				if (optShipSeed)
					SeedTextSpin (pPIS->Buffer, sizeof (pPIS->Buffer),
							pStr + n, &x, &y);
				else
					utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer),
							pStr + n);
				x <<= RESOLUTION_FACTOR;
				y <<= RESOLUTION_FACTOR;

				if (pPIS->HaveFrame
							&&(pPIS->GetRect.extent.width > 0
							&& pPIS->GetRect.extent.height > 0))
				{
					x += pPIS->GetRect.corner.x;
					y += pPIS->GetRect.corner.y;
				}

				SetContextForeGroundColor (pPIS->TextColor);
				SetContextBackGroundColor (pPIS->TextBackColor);
				if (optShipSeed && !strncmp (pPIS->Buffer, "SPATHI", 6))
				{ // Manually space the SPATHI text
					DoSpinText ("SP", x,
							y + RES_SCALE (7),
							SetAbsFrameIndex (pPIS->Frame, 0), &pPIS->Skip);
					DoSpinText ("A", x + RES_SCALE (16),
							y + RES_SCALE (7),
							SetAbsFrameIndex (pPIS->Frame, 0), &pPIS->Skip);
					DoSpinText ("THI", x + RES_SCALE (25),
							y + RES_SCALE (7),
							SetAbsFrameIndex (pPIS->Frame, 0), &pPIS->Skip);
					if (ship_map[shipID].sID == UR_QUAN_ID)
					{ // All for a ship that won't occur anyway
						x -= RES_SCALE (52);
						y += RES_SCALE (11);
					}
					DoSpinText (&(pPIS->Buffer[6]), x + RES_SCALE (52),
							y + RES_SCALE (7),
							SetAbsFrameIndex (pPIS->Frame, 0), &pPIS->Skip);
				}
				else
				{
					DoSpinText (pPIS->Buffer, x, y + RES_SCALE (7),
							SetAbsFrameIndex (pPIS->Frame, 0), &pPIS->Skip);
				}

				if (pPIS->Skip)
					Present_BatchGraphics (pPIS);
			}
		}
		else if (strcmp (Opcode, "SPINSTAT") == 0)
		{	/* spin stat draw */
			int x, y, f, e;
			SIZE leading;

			assert (sizeof (pPIS->Buffer) >= 256);

			if (3 == sscanf (pStr, "%d %d %255[^\n]", &f, &e, pPIS->Buffer))
			{
				GetContextFontLeading (&leading);

				pPIS->NumSpinStat++;

				x = pPIS->StatBox.corner.x + RES_SCALE (3);
				y = pPIS->StatBox.corner.y + RES_SCALE (1)
						+ (leading * pPIS->NumSpinStat);

				if (pPIS->NumSpinStat > 8)
				{
					log_add (log_Warning, "SPINSTAT: Number of SPINSTAT "
						"entries exceeds max amount '%s'", pStr);
					return FALSE;
				}

				if (f > 9 || (f + e) > 9)
				{
					char buf[40];
					TEXT t;

					log_add (log_Warning, "SPINSTAT: Stats exceed max "
							"values '%s'", pStr);
					snprintf (buf, sizeof (buf), "%s %s", pPIS->Buffer,
							"Exceed max!");

					t.align = ALIGN_LEFT;
					t.pStr = buf;
					t.CharCount = (COUNT)~0;
					t.baseline = MAKE_POINT (x, y);
					DrawTextEffect (&t,
							BUILD_COLOR_RGBA (0xFF, 0x55, 0x55, 0xFF),
							pPIS->TextBackColor, pPIS->TextEffect);
					
				}
				else
				{
					DoSpinStat (pPIS->Buffer,
							x, y, f, e,
							pPIS->TextColor, pPIS->TextBackColor, &pPIS->Skip);

					if (pPIS->Skip)
						Present_BatchGraphics (pPIS);
				}
			}
			else
			{
				log_add (log_Warning, "Bad SPINSTAT command '%s'", pStr);
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
				y = leading / pPIS->LinesCount + leading;
				break;
			case 'M': /* middle */
				y = (pPIS->clip_r.extent.height
						- pPIS->LinesCount * leading) / 2;
				break;
			default: /* bottom */
				y = pPIS->clip_r.extent.height - pPIS->LinesCount * leading;
			}
			pPIS->tfade_r = pPIS->clip_r;
			pPIS->tfade_r.corner.y = 0;
			pPIS->tfade_r.extent.height = SCREEN_HEIGHT;
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
					TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA);
		}
		else if (strcmp (Opcode, "RESTBG") == 0)
		{	/* restore background */
			TFB_DrawScreen_Copy (&pPIS->clip_r,
					TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN);
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
				pPIS->HaveFrame = FALSE;
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

			x <<= RESOLUTION_FACTOR;
			y <<= RESOLUTION_FACTOR;

			s.frame = NULL;
			if (draw_what == PRES_DRAW_INDEX)
			{	/* draw stamp by index */
				s.frame = SetAbsFrameIndex (pPIS->Frame, (COUNT)index);
				pPIS->CurrentFrameIndex = (COUNT)index;
				pPIS->HaveFrame = TRUE;
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

			ClearScreen ();
		}
		else if (strcmp (Opcode, "CALL") == 0)
		{	/* call another script */
			Present_UnbatchGraphics (pPIS, TRUE);

			utf8StringCopy (pPIS->Buffer, sizeof (pPIS->Buffer), pStr);
			ShowPresentationFile (pPIS->Buffer);
		}
		else if (strcmp (Opcode, "LINE") == 0)
		{	/* draw simple line */
			int x1, x2, y1, y2;
			if (4 == sscanf (pStr, "%d %d %d %d", &x1, &y1, &x2, &y2))
			{
				LINE l;

				l.first.x = RES_SCALE (x1);
				l.first.y = RES_SCALE (y1);
				l.second.x = RES_SCALE (x2);
				l.second.y = RES_SCALE (y2);
				
				SetContextForeGroundColor (pPIS->TextColor);
				DrawLine (&l, 1);
			}
			else
			{
				log_add (log_Warning, "Bad LINE command '%s'", pStr);
			}
		}
		else if (strcmp (Opcode, "LINESPIN") == 0)
		{	/* draw line for spin */
			int x1, x2, y1, y2;
			if (4 == sscanf (pStr, "%d %d %d %d", &x1, &y1, &x2, &y2))
			{
				if (optShipSeed)
					SeedLineSpin (&x1, &y1, &x2, &y2);
				LINE l;

				x1 <<= RESOLUTION_FACTOR;
				x2 <<= RESOLUTION_FACTOR;
				y1 <<= RESOLUTION_FACTOR;
				y2 <<= RESOLUTION_FACTOR;

				if (pPIS->HaveFrame
						&& (pPIS->GetRect.extent.width > 0
						&& pPIS->GetRect.extent.height > 0))
				{
					x1 += pPIS->GetRect.corner.x;
					x2 += pPIS->GetRect.corner.x;
					y1 += pPIS->GetRect.corner.y;
					y2 += pPIS->GetRect.corner.y;
				}

				l.first.x = x1;
				l.first.y = y1;
				l.second.x = x2;
				l.second.y = y2;
				
				DoSpinLine (&l, pPIS->TextColor, pPIS->TextBackColor, &pPIS->Skip);

				if (pPIS->Skip)
					Present_BatchGraphics (pPIS);
			}
			else
			{
				log_add (log_Warning, "Bad LINESPIN command '%s'", pStr);
			}
		}
		else if (strcmp (Opcode, "GETRECT") == 0)
		{	/* Get currently drawn FRAME rect */
			if (pPIS->HaveFrame)
			{
				GetFrameRect (SetAbsFrameIndex (
						pPIS->Frame, pPIS->CurrentFrameIndex),
						&pPIS->GetRect);
			}
			else
			{
				log_add (log_Warning, "Bad GETRECT command, can not use "
						"GETRECT without drawing a frame first '%s'",
						pStr);
			}
		}
		else if (strcmp (Opcode, "STATBOX") == 0)
		{	/* draw stat box for spin */
#define STATBOX_WIDTH  RES_SCALE (122)
#define STATBOX_HEIGHT RES_SCALE (60)
			int x, y;
			if (2 == sscanf (pStr, "%d %d", &x, &y))
			{
				pPIS->NumSpinStat = 0;

				x <<= RESOLUTION_FACTOR;
				y <<= RESOLUTION_FACTOR;

				if (pPIS->HaveFrame
							&&(pPIS->GetRect.extent.width > 0
							&& pPIS->GetRect.extent.height > 0))
				{
					x += pPIS->GetRect.corner.x;
					y += pPIS->GetRect.corner.y;
				}

				pPIS->StatBox.corner = MAKE_POINT (x, y);
				pPIS->StatBox.extent =
						MAKE_EXTENT (STATBOX_WIDTH, STATBOX_HEIGHT);
				
				DoSpinStatBox (&pPIS->StatBox, pPIS->TextColor,
						pPIS->TextBackColor, &pPIS->Skip);

				if (pPIS->Skip)
					Present_BatchGraphics (pPIS);
			}
			else
			{
				log_add (log_Warning, "Bad STATBOX command '%s'", pStr);
			}
		}
		else if (strcmp (Opcode, "MOVIE") == 0)
		{	/* play movie */
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
		else if (strcmp (Opcode, "ANIMATE") == 0)
		{	/* basic frame animation */
			int first_frame, last_frame, num_loops, milliseconds, fps;

			if (5 == sscanf (pStr, "%d %d %d %d %d", &first_frame,
					&last_frame, &num_loops, &milliseconds, &fps))
			{
				STAMP s;
				int loops = 0;
				COUNT index = 0;
				TimeCount Now, timeout, NextTime;
				int animation_rate = ONE_SECOND / fps;

				s.origin.x = 0;
				s.origin.y = 0;

				timeout = GetTimeCounter () + milliseconds;
				NextTime = GetTimeCounter () + animation_rate;

				while (num_loops || milliseconds)
				{
					Now = GetTimeCounter ();

					if (ActKeysPress ())
						break;

					if (Now >= NextTime)
					{
						s.frame = SetAbsFrameIndex (pPIS->Frame, index);
						DrawStamp (&s);
						index++;

						if (index == last_frame)
						{
							loops++;
							index = first_frame;
						}

						if (num_loops > 0 && loops == num_loops)
							break;

						if (Now >= timeout)
							break;

						NextTime = Now + animation_rate;
					}
				}
				return TRUE;
			}
			else
			{
				log_add (log_Warning, "Bad ANIMATION command '%s'", pStr);
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

	if (pis.MusicRef && PlayingStream (MUSIC_SOURCE))
	{
		SleepThreadUntil (FadeMusic (0, ONE_SECOND));
		StopMusic ();
		FadeMusic (NORMAL_VOLUME, 0);
	}

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
