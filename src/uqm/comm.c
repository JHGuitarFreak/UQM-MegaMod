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

#define COMM_INTERNAL
#include "comm.h"

#include "build.h"
#include "commanim.h"
#include "commglue.h"
#include "controls.h"
#include "colors.h"
#include "sis.h"
#include "units.h"
#include "encount.h"
#include "starmap.h"
#include "endian_uqm.h"
#include "gamestr.h"
#include "options.h"
#include "oscill.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "nameref.h"
#include "uqmdebug.h"
#include "lua/luacomm.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/sound/sound.h"
#include "libs/sound/trackplayer.h"
#include "libs/log.h"

#include <ctype.h>

#define MAX_RESPONSES 8
#define BACKGROUND_VOL (usingSpeech && !VolasPackPresent ? (NORMAL_VOLUME / 2) : NORMAL_VOLUME)
#define FOREGROUND_VOL NORMAL_VOLUME

// Oscilloscope frame rate
// Should be <= COMM_ANIM_RATE
// XXX: was 32 picked experimentally?
#define OSCILLOSCOPE_RATE   (ONE_SECOND / RES_STAT_SCALE(32))

// Maximum comm animation frame rate (actual execution rate)
// A gfx frame is not always produced during an execution frame,
// and several animations are combined into one gfx frame.
// The rate was originally 120fps which allowed for more animation
// precision which is ultimately wasted on the human eye anyway.
// The highest known stable animation rate is 40fps, so that's what we use.
//
// JMS: Changed this back to 120 fps since HD seems to like it... 
#define COMM_ANIM_RATE   (ONE_SECOND / RES_STAT_SCALE(40))

static CONTEXT AnimContext;

LOCDATA CommData;
UNICODE shared_phrase_buf[2048];
FONT ComputerFont;

static BOOLEAN TalkingFinished;
static CommIntroMode curIntroMode = CIM_DEFAULT;
static TimeCount fadeTime;

BOOLEAN IsProbe;
BOOLEAN IsAltSong;

typedef struct response_entry
{
	RESPONSE_REF response_ref;
	TEXT response_text;
	RESPONSE_FUNC response_func;
	char *allocedResponse;
} RESPONSE_ENTRY;

typedef struct encounter_state
{
	BOOLEAN (*InputFunc) (struct encounter_state *pES);

	COUNT Initialized;
	TimeCount NextTime; // framerate control
	BYTE num_responses;
	BYTE cur_response;
	BYTE top_response;
	RESPONSE_ENTRY response_list[MAX_RESPONSES];

	UNICODE phrase_buf[1024];
} ENCOUNTER_STATE;

static ENCOUNTER_STATE *pCurInputState;

static BOOLEAN clear_subtitles;
static TEXT SubtitleText;
static const UNICODE *last_subtitle;

static CONTEXT TextCacheContext;
static FRAME TextCacheFrame;


RECT CommWndRect = {
	// default values; actually inited by HailAlien()
	{0, 0},
	{0, 0} //was {SIS_ORG_X, SIS_ORG_Y}, // JMS_GFX
};

static void ClearSubtitles (void);
static void CheckSubtitles (void);
static void RedrawSubtitles (void);


/* _count_lines - sees how many lines a given input string would take to
 * display given the line wrapping information
 */
static int
_count_lines (TEXT *pText)
{
	SIZE text_width;
	const char *pStr;
	int numLines = 0;
	BOOLEAN eol;

	text_width = CommData.AlienTextWidth;
	SetContextFont (CommData.AlienFont);

	pStr = pText->pStr;
	do
	{
		++numLines;
		pText->pStr = pStr;
		eol = getLineWithinWidth (pText, &pStr, text_width, (COUNT)~0);
	} while (!eol);
	pText->pStr = pStr;

	return numLines;
}

// status == -1: draw highlighted player dialog option
// status == -2: draw non-highlighted player dialog option
// status == -4: use current context, and baseline from pTextIn
// status ==  1:  draw alien speech; subtitle cache is used
static COORD
add_text (int status, TEXT *pTextIn)
{
	COUNT maxchars, numchars;
	TEXT locText;
	TEXT *pText;
	SIZE leading;
	const char *pStr;
	SIZE text_width;
	int num_lines = 0;
	static COORD last_baseline;
	BOOLEAN eol;
	CONTEXT OldContext = NULL;
	COUNT computerOn = 0;
	
	BatchGraphics ();

	maxchars = (COUNT)~0;
	if (status == 1)
	{
		if (last_subtitle == pTextIn->pStr)
		{
			// draws cached subtitle
			STAMP s;

			s.origin.x = 0;
			s.origin.y = 0;
			s.frame = TextCacheFrame;
			DrawStamp (&s);
			UnbatchGraphics ();
			return last_baseline;
		}
		else
		{
			// draw to subtitle cache; prepare first
			OldContext = SetContext (TextCacheContext);
			ClearDrawable ();

			last_subtitle = pTextIn->pStr;
		}

		text_width = CommData.AlienTextWidth;
		SetContextFont (CommData.AlienFont);
		GetContextFontLeading (&leading);

		pText = pTextIn;
	}
	else if (GetContextFontLeading (&leading), status <= -4)
	{
		text_width = (SIZE) (SIS_SCREEN_WIDTH - RES_SCALE(8) - (TEXT_X_OFFS << 2)); // JMS_GFX

		pText = pTextIn;
	}
	else
	{
		text_width = (SIZE) (SIS_SCREEN_WIDTH - RES_SCALE(8) - (TEXT_X_OFFS << 2)); // JMS_GFX

		switch (status)
		{
			case -3:
				// Unknown. Never reached; color matches the background color.
				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
				break;
			case -2:
				// Not highlighted dialog options.
				SetContextForeGroundColor (COMM_PLAYER_TEXT_NORMAL_COLOR);
				break;
			case -1:
				// Currently highlighted dialog option.
				SetContextForeGroundColor (COMM_PLAYER_TEXT_HIGHLIGHT_COLOR);
				break;
		}

		maxchars = pTextIn->CharCount;
		locText = *pTextIn;
		locText.baseline.x -= RES_SCALE(8) - 4 * RESOLUTION_FACTOR; // JMS_GFX
		locText.CharCount = (COUNT)~0;
		locText.pStr = STR_BULLET;
		font_DrawText (&locText);

		locText = *pTextIn;
		pText = &locText;
		pText->baseline.y -= leading;
	}

	numchars = 0;
	pStr = pText->pStr;

	if (status > 0 && (CommData.AlienTextValign &
			(VALIGN_MIDDLE | VALIGN_BOTTOM)))
	{
		num_lines = _count_lines(pText);
		if (CommData.AlienTextValign == VALIGN_BOTTOM)
			pText->baseline.y -= (leading * num_lines);
		else if (CommData.AlienTextValign == VALIGN_MIDDLE)
			pText->baseline.y -= ((leading * num_lines) / 2);
		if (pText->baseline.y < 0)
			pText->baseline.y = 0;
	}

	do
	{
		pText->pStr = pStr;
		pText->baseline.y += leading;

		eol = getLineWithinWidth (pText, &pStr, text_width, maxchars);

		maxchars -= pText->CharCount;
		if (maxchars != 0)
			--maxchars;
		numchars += pText->CharCount;
		
		if (status <= 0)
		{
			// Player dialog option or (status == -4) other non-alien
			// text.
			if (pText->baseline.y < SIS_SCREEN_HEIGHT)
				font_DrawText (pText);

			if (status < -4 && pText->baseline.y >= -status - 10)
			{
				// Never actually reached. Status is never <-4.
				++pStr;
				break;
			}
		}
		else
		{
			// Alien speech
			if (CommData.AlienConv == ORZ_CONVERSATION)
			{
				// BW : special case for the Orz conversations
				// the character $ is recycled as a marker to
				// switch from and to computer font
				
				const char *ptr;
				RECT rect;
				COORD baselinex = pText->baseline.x;
				COORD width = 0;
				COUNT remChars = pText->CharCount;
			        // Remaining chars until end of line within width
				const char *bakptr;
				COUNT bakChars = remChars;
				COUNT bakcompOn = computerOn;
				FONT bakFont = SetContextFont(ComputerFont);
				
				SetContextFont(bakFont);
				ptr = pText->pStr;
				bakptr = ptr;
				
				// We need to manually center the line because
				// the computer font is larger than the Orzfont
				
				// This loop computes the width of the line
				while (remChars > 0)
					{
						while ((*ptr != '$') && remChars > 0)
							{
								getCharFromString (&ptr);
								remChars--;
							}
						
						pText->CharCount -= remChars;
						TextRect (pText, &rect, NULL);
						
						width += rect.extent.width;
						
						if (*ptr == '$')
							{
								getCharFromString (&ptr);
								remChars--;
								computerOn = 1 - computerOn;
								if (computerOn)
									SetContextFont (ComputerFont);
								else
									SetContextFont (CommData.AlienFont);
							}
						pText->CharCount = remChars;
						pText->pStr = ptr;
					}

				// This to simulate a centered line
				pText->baseline.x = baselinex - (width >> 1);
				pText->align = ALIGN_LEFT;
				
				// Put everything back in place for the
				// actual display 
				remChars = bakChars;
				pText->CharCount = bakChars;
				ptr = bakptr;
				pText->pStr = bakptr;
				computerOn = bakcompOn;
				SetContextFont(bakFont);
				
				// This loop is used to look up for $
				while (remChars > 0)
					{
						while ((*ptr != '$') && remChars > 0)
							{
								getCharFromString (&ptr);
								remChars--;
							}
						
						pText->CharCount -= remChars;
						TextRect (pText, &rect, NULL);
						
						font_DrawTracedText (pText,
								     CommData.AlienTextFColor, CommData.AlienTextBColor);
						
						pText->baseline.x += rect.extent.width;
						
						if (*ptr == '$')
							{
								getCharFromString (&ptr);
								remChars--;
								computerOn = 1 - computerOn;
								if (computerOn)
									SetContextFont (ComputerFont);
								else
									SetContextFont (CommData.AlienFont);
							}
						pText->CharCount = remChars;
						pText->pStr = ptr;
					}
				pText->baseline.x = baselinex;
				pText->align = ALIGN_CENTER;
			}
			else
			{
				// Normal case : other races than Orz
				font_DrawTracedText (pText, CommData.AlienTextFColor, CommData.AlienTextBColor);
			}
		}
	} while (!eol && maxchars);
	pText->pStr = pStr;

	if (status == 1)
	{
		STAMP s;
		
		// We were drawing to cache -- flush to screen
		SetContext (OldContext);
		s.origin.x = s.origin.y = 0;
		s.frame = TextCacheFrame;
		DrawStamp (&s);
		
		last_baseline = pText->baseline.y;
	}

	UnbatchGraphics ();
	return (pText->baseline.y);
}

// This function calculates how much of a string can be fitted within
// a specific width, up to a newline or terminating \0.
// pText is the text to be fitted. pText->CharCount will be set to the
// number of characters that fitted.
// startNext will be filled with the start of the first word that
// doesn't fit in one line, or if an entire line fits, to the character
// past the newline, or if the entire string fits, to the end of the
// string.
// maxWidth is the maximum number of pixels that a line may be wide
//   ASSUMPTION: there are no words in the text wider than maxWidth
// maxChars is the maximum number of characters (not bytes) that are to
// be fitted.
// TRUE is returned if a complete line fitted
// FALSE otherwise
BOOLEAN
getLineWithinWidth(TEXT *pText, const char **startNext,
		SIZE maxWidth, COUNT maxChars)
{
	BOOLEAN eol;
			// The end of the line of text has been reached.
	BOOLEAN done;
			// We cannot add any more words.
	RECT rect;
	COUNT oldCount;
	const char *ptr;
	const char *wordStart;
	UniChar ch;
	COUNT charCount;

	//GetContextClipRect (&rect);

	eol = FALSE;	
	done = FALSE;
	oldCount = 1;
	charCount = 0;
	ch = '\0';
	ptr = pText->pStr;
	for (;;)
	{
		wordStart = ptr;

		// Scan one word.
		for (;;)
		{
			if (*ptr == '\0')
			{
				eol = TRUE;
				done = TRUE;
				break;
			}
			ch = getCharFromString (&ptr);
			eol = ch == '\0' || ch == '\n' || ch == '\r';
			done = eol || charCount >= maxChars;
			if (done || ch == ' ')
				break;
			charCount++;
		}

		oldCount = pText->CharCount;
		pText->CharCount = charCount;
		TextRect (pText, &rect, NULL);
		
		if (rect.extent.width >= maxWidth)
		{
			pText->CharCount = oldCount;
			*startNext = wordStart;
			return FALSE;
		}

		if (done)
		{
			*startNext = ptr;
			return eol;
		}
		charCount++;
				// For the space in between words.
	}
}

static void
DrawSISComWindow (void)
{
	CONTEXT OldContext;

	if (LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
	{
		RECT r;

		OldContext = SetContext (SpaceContext);

		r.corner.x = 0;
		r.corner.y = SLIDER_Y + SLIDER_HEIGHT;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;
		SetContextForeGroundColor (COMM_PLAYER_BACKGROUND_COLOR);
		DrawFilledRectangle (&r);

		SetContext (OldContext);
	}
}

void
init_communication (void)
{
	// now a no-op
}

void
uninit_communication (void)
{
	// now a no-op
}

static void
RefreshResponses (ENCOUNTER_STATE *pES)
{
	COORD y;
	BYTE response, extra_y; // JMS_GFX
	SIZE leading;
	STAMP s;


	SetContext (SpaceContext);
	GetContextFontLeading (&leading);
	BatchGraphics ();

	DrawSISComWindow ();
	y = SLIDER_Y + SLIDER_HEIGHT + RES_SCALE(1); // JMS_GFX
	for (response = pES->top_response; response < pES->num_responses;
			++response)
	{
		extra_y = (response == pES->top_response ? 0 : IF_HD(22)); // JMS_GFX
		
		pES->response_list[response].response_text.baseline.x = TEXT_X_OFFS + RES_SCALE(8); // JMS_GFX
		pES->response_list[response].response_text.baseline.y = y + leading + extra_y; // JMS_GFX
		pES->response_list[response].response_text.align = ALIGN_LEFT;
		if (response == pES->cur_response)
			y = add_text (-1, &pES->response_list[response].response_text);
		else
			y = add_text (-2, &pES->response_list[response].response_text);
	}

	if (pES->top_response)
	{
		s.origin.y = SLIDER_Y + SLIDER_HEIGHT + 1;
		s.frame = SetAbsFrameIndex (ActivityFrame, 6);
	}
	else if (y > SIS_SCREEN_HEIGHT)
	{
		s.origin.y = SIS_SCREEN_HEIGHT - 2;
		s.frame = SetAbsFrameIndex (ActivityFrame, 7);
	}
	else
		s.frame = 0;
	if (s.frame)
	{
		RECT r;

		GetFrameRect (s.frame, &r);
		s.origin.x = SIS_SCREEN_WIDTH - r.extent.width - 1;
		DrawStamp (&s);
	}

	UnbatchGraphics ();
}

static void
FeedbackPlayerPhrase (UNICODE *pStr)
{
	SetContext (SpaceContext);
	
	BatchGraphics ();
	DrawSISComWindow ();
	if (pStr[0])
	{
		TEXT ct;

		ct.baseline.x = SIS_SCREEN_WIDTH >> 1;
		ct.baseline.y = SLIDER_Y + SLIDER_HEIGHT + RES_SCALE(13); // JMS_GFX
		ct.align = ALIGN_CENTER;
		ct.CharCount = (COUNT)~0;

		ct.pStr = GAME_STRING (FEEDBACK_STRING_BASE);
				// "(In response to your statement)"
		SetContextForeGroundColor (COMM_RESPONSE_INTRO_TEXT_COLOR);
		font_DrawText (&ct);

		ct.baseline.y += RES_SCALE(16); // JMS_GFX
		SetContextForeGroundColor (COMM_FEEDBACK_TEXT_COLOR);
		ct.pStr = pStr;
		add_text (-4, &ct);
	}
	UnbatchGraphics ();
}

static void
InitSpeechGraphics (void)
{
	InitOscilloscope (SetAbsFrameIndex (ActivityFrame, 9));

	InitSlider (0, SLIDER_Y, SIS_SCREEN_WIDTH,
			SetAbsFrameIndex (ActivityFrame, 5),
			SetAbsFrameIndex (ActivityFrame, 2));
}

static void
UpdateSpeechGraphics (void)
{
	static TimeCount NextTime;
	CONTEXT OldContext;

	if (GetTimeCounter () < NextTime)
		return; // too early

	NextTime = GetTimeCounter () + OSCILLOSCOPE_RATE;

	OldContext = SetContext (RadarContext);
	DrawOscilloscope ();
	SetContext (SpaceContext);
	DrawSlider ();
	SetContext (OldContext);
}

static void
UpdateAnimations (bool paused)
{
	static TimeCount NextTime;
	CONTEXT OldContext;
	BOOLEAN change;

	if (GetTimeCounter () < NextTime)
		return; // too early

	NextTime = GetTimeCounter () + COMM_ANIM_RATE;

	OldContext = SetContext (AnimContext);
	BatchGraphics ();
	// Advance and draw ambient, transit and talk animations
	change = ProcessCommAnimations (clear_subtitles, paused);
	if (change)
		RedrawSubtitles ();
	UnbatchGraphics ();
	clear_subtitles = FALSE;
	SetContext (OldContext);
}

static void
UpdateCommGraphics (void)
{
	UpdateAnimations (false);
	UpdateSpeechGraphics ();
}

// Derived from INPUT_STATE_DESC
typedef struct talking_state
{
	// Fields required by DoInput()
	BOOLEAN (*InputFunc) (struct talking_state *);

	TimeCount NextTime;  // framerate control
	COUNT waitTrack;
	bool rewind;
	bool seeking;
	bool ended;

} TALKING_STATE;

static BOOLEAN
DoTalkSegue (TALKING_STATE *pTS)
{
	bool left = false;
	bool right = false;
	COUNT curTrack;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pTS->ended = true;
		return FALSE;
	}
	
	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		JumpTrack ();
		pTS->ended = true;
		return FALSE;
	}

	if (optSmoothScroll == OPT_PC)
	{
		left = PulsedInputState.menu[KEY_MENU_LEFT] != 0;
		right = PulsedInputState.menu[KEY_MENU_RIGHT] != 0;
	}
	else if (optSmoothScroll == OPT_3DO)
	{
		left = CurrentInputState.menu[KEY_MENU_LEFT] != 0;
		right = CurrentInputState.menu[KEY_MENU_RIGHT] != 0;
	}

#if DEMO_MODE || CREATE_JOURNAL
	left = false;
	right = false;
#endif

	if (right)
	{
		SetSliderImage (SetAbsFrameIndex (ActivityFrame, 3));
		if (optSmoothScroll == OPT_PC)
			FastForward_Page ();
		else if (optSmoothScroll == OPT_3DO)
			FastForward_Smooth ();
		pTS->seeking = true;
	}
	else if (left || pTS->rewind)
	{
		pTS->rewind = false;
		SetSliderImage (SetAbsFrameIndex (ActivityFrame, 4));
		if (optSmoothScroll == OPT_PC)
			FastReverse_Page ();
		else if (optSmoothScroll == OPT_3DO)
			FastReverse_Smooth ();
		pTS->seeking = true;
	}
	else if (pTS->seeking)
	{
		// This is only done once the seeking is over (in the smooth
		// scroll case, once the user releases the seek button)
		pTS->seeking = false;
		SetSliderImage (SetAbsFrameIndex (ActivityFrame, 2));
	}
	else
	{
		// This used to have a buggy guard condition, which
		// would cause the animations to remain paused in a couple cases
		// after seeking back to the beginning.
		// Broken cases were: Syreen "several hours later" and Starbase
		// VUX Beast analysis by the scientist.
		CheckSubtitles ();
	}

	// XXX: When seeking, all animations (talking and ambient) stop
	// progressing. This is an original 3DO behavior, and I see no
	// reason why the animations cannot continue while seeking.
	UpdateAnimations (pTS->seeking);
	UpdateSpeechGraphics ();

	curTrack = PlayingTrack ();
	pTS->ended = !pTS->seeking && !curTrack;

	SleepThreadUntil (pTS->NextTime);
	// Need a high enough framerate for 3DO smooth seeking
	pTS->NextTime = GetTimeCounter () + ONE_SECOND / 60;

	return pTS->seeking || (curTrack && curTrack <= pTS->waitTrack);
}

static void
runCommAnimFrame (void)
{
	UpdateCommGraphics ();
	SleepThread (COMM_ANIM_RATE);
}

static BOOLEAN
TalkSegue (COUNT wait_track)
{
	TALKING_STATE talkingState;

	// Transition animation to talking state, if necessary
	if (wantTalkingAnim () && haveTalkingAnim ())
	{	
		if (haveTransitionAnim ())
			setRunIntroAnim ();
					
		setRunTalkingAnim ();

		// wait until the transition finishes
		while (runningIntroAnim ())
			runCommAnimFrame ();
	}

	memset (&talkingState, 0, sizeof talkingState);

	if (wait_track == 0)
	{	// Restarting with a rewind
		wait_track = WAIT_TRACK_ALL;
		talkingState.rewind = true;
	}
	else if (!PlayingTrack ())
	{	// initial start of player
		PlayTrack ();
	}

	// Run the talking controls
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	talkingState.InputFunc = DoTalkSegue;
	talkingState.waitTrack = wait_track;
	DoInput (&talkingState, FALSE);

	ClearSubtitles ();

	if (talkingState.ended)
	{	// reached the end; set STOP icon
		SetSliderImage (SetAbsFrameIndex (ActivityFrame, 8));
	}

	// transition back to silent, if necessary
	if (runningTalkingAnim ())
		setStopTalkingAnim ();

	// Wait until the animation task stops "talking"
	while (runningTalkingAnim ())
		runCommAnimFrame ();

	return talkingState.ended;
}

static void
CommIntroTransition (void)
{
	if (curIntroMode == CIM_CROSSFADE_SCREEN)
	{
		ScreenTransition (3, NULL);
		UnbatchGraphics ();
	}
	else if (curIntroMode == CIM_CROSSFADE_SPACE)
	{
		RECT r;
		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		ScreenTransition (3, &r);
		UnbatchGraphics ();
	}
	else if (curIntroMode == CIM_CROSSFADE_WINDOW)
	{
		ScreenTransition (3, &CommWndRect);
		UnbatchGraphics ();
	}
	else if (curIntroMode == CIM_FADE_IN_SCREEN)
	{
		UnbatchGraphics ();
		FadeScreen (FadeAllToColor, fadeTime);
	}
	else
	{	// Uknown transition
		// Have to unbatch anyway or no more graphics, ever
		UnbatchGraphics ();
		assert (0 && "Unknown comm intro transition");
	}

	// Reset the mode for next time. Everything that needs a
	// different one will let us know.
	curIntroMode = CIM_DEFAULT;
}

void
AlienTalkSegue (COUNT wait_track)
{
	// this skips any talk segues that follow an aborted one
	if ((GLOBAL (CurrentActivity) & CHECK_ABORT) || TalkingFinished)
		return;

	if (!pCurInputState->Initialized)
	{
		InitSpeechGraphics ();
		SetColorMap (GetColorMapAddress (CommData.AlienColorMap));
		SetContext (AnimContext);
		DrawAlienFrame (NULL, 0, TRUE);
		UpdateSpeechGraphics ();
		CommIntroTransition ();
		
		pCurInputState->Initialized = TRUE;

		PlayMusic (CommData.AlienSong, TRUE, 1);
		SetMusicVolume (BACKGROUND_VOL);

		InitCommAnimations ();

		LastActivity &= ~CHECK_LOAD;
	}
	
	TalkingFinished = TalkSegue (wait_track);
	if (TalkingFinished && !VolasPackPresent)
		FadeMusic (FOREGROUND_VOL, ONE_SECOND);
}


typedef struct summary_state
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (struct summary_state *pSS);

	// extended state
	BOOLEAN Initialized;
	BOOLEAN PrintNext;
	SUBTITLE_REF NextSub;
	const UNICODE *LeftOver;

} SUMMARY_STATE;

static void remove_char_from_string(UNICODE* str, const UNICODE c) {
	// MB: Hack for removing '$' characters from Orz dialogue when viewing summary conversation - Used by DoConvSummary below
    UNICODE *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

static BOOLEAN
DoConvSummary (SUMMARY_STATE *pSS)
{
#define DELTA_Y_SUMMARY RES_SCALE(8) // JMS_GFX
	//#define MAX_SUMM_ROWS ((SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT) / DELTA_Y_SUMMARY
#define MAX_SUMM_ROWS (SLIDER_Y	/ DELTA_Y_SUMMARY) - 1 // JMS_GFX

	if (!pSS->Initialized)
	{
		pSS->PrintNext = TRUE;
		pSS->NextSub = GetFirstTrackSubtitle ();
		pSS->LeftOver = NULL;
		pSS->InputFunc = DoConvSummary;
		pSS->Initialized = TRUE;
		DoInput (pSS, FALSE);
	}
	else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		return FALSE; // bail out
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT]
			|| PulsedInputState.menu[KEY_MENU_CANCEL]
			|| PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		if (pSS->NextSub)
		{	// we want the next page
			pSS->PrintNext = TRUE;
		}
		else
		{	// no more, we are done
			return FALSE;
		}
	}
	else if (pSS->PrintNext)
	{	// print the next page
		RECT r;
		TEXT t;
		int row;

		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SLIDER_Y; //SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT + RES_SCALE(2) + 16 * RESOLUTION_FACTOR; // JMS_GFX

		SetContext (AnimContext);
		SetContextForeGroundColor (COMM_HISTORY_BACKGROUND_COLOR);
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (COMM_HISTORY_TEXT_COLOR);

		r.extent.width -= 2 + 2;
		t.baseline.x = RES_SCALE(2); // JMS_GFX
		t.align = ALIGN_LEFT;
		t.baseline.y = DELTA_Y_SUMMARY;
		SetContextFont (TinyFont);

		for (row = 0; row < MAX_SUMM_ROWS && pSS->NextSub;
				++row, pSS->NextSub = GetNextTrackSubtitle (pSS->NextSub))
		{
			const char *next = NULL;

			if (pSS->LeftOver)
			{	// some text left from last subtitle
				t.pStr = pSS->LeftOver;
				pSS->LeftOver = NULL;
			}
			else
			{
				t.pStr = GetTrackSubtitleText (pSS->NextSub);
				if (!t.pStr)
					continue;
			}

			t.CharCount = (COUNT)~0;
			for ( ; row < MAX_SUMM_ROWS && !getLineWithinWidth (&t, &next, r.extent.width, (COUNT)~0); ++row) {
				if (CommData.AlienConv == ORZ_CONVERSATION) { // MB: nasty hack: remove '$'s from conversation for Orz
					UNICODE my_copy[128];
					strcpy(my_copy, t.pStr);
					remove_char_from_string(my_copy, '$');
					t.pStr = my_copy;
					font_DrawText(&t);
				} else { // Normal mode
					font_DrawText(&t);
				}
				t.baseline.y += DELTA_Y_SUMMARY;
				t.pStr = next;
				t.CharCount = (COUNT)~0;
			}

			if (row >= MAX_SUMM_ROWS)
			{	// no more space on screen, but some text left over
				// from the current subtitle
				pSS->LeftOver = next;
				break;
			}
		
			// this subtitle fit completely
			if (CommData.AlienConv == ORZ_CONVERSATION) { // MB: nasty hack: remove '$'s from conversation for Orz
				UNICODE my_copy[128];
				strcpy(my_copy, t.pStr);
				remove_char_from_string(my_copy, '$');
				t.pStr = my_copy;
				font_DrawText(&t);
			} else { // Normal mode
				font_DrawText(&t);
			}
			t.baseline.y += DELTA_Y_SUMMARY;
		}

		if (row >= MAX_SUMM_ROWS && (pSS->NextSub || pSS->LeftOver))
		{	// draw *MORE*
			TEXT mt;
			UNICODE buffer[128];

			mt.baseline.x = SIS_SCREEN_WIDTH >> 1;
			mt.baseline.y = t.baseline.y;
			mt.align = ALIGN_CENTER;
			snprintf (buffer, sizeof (buffer), "%s%s%s", // "MORE"
					STR_MIDDLE_DOT, GAME_STRING (FEEDBACK_STRING_BASE + 1),
					STR_MIDDLE_DOT);

			if (CommData.AlienConv == ORZ_CONVERSATION) { // MB: nasty hack: remove '$'s from conversation for Orz
				remove_char_from_string(buffer, '$');
			}

			mt.pStr = buffer;
			SetContextForeGroundColor (COMM_MORE_TEXT_COLOR);
			font_DrawText (&mt);
		}


		pSS->PrintNext = FALSE;
	}
	else
	{
		SleepThread (ONE_SECOND / 20);
	}

	return TRUE; // keep going
}

static void
ClearResponses (ENCOUNTER_STATE *pES)
{
	size_t responseI;

	for (responseI = 0; responseI < MAX_RESPONSES; responseI++) {
		RESPONSE_ENTRY *response = &pES->response_list[responseI];
		if (response->allocedResponse != NULL)
		{
			HFree (response->allocedResponse);
			response->allocedResponse = NULL;
		}
	}
}

// Called when the player presses the select button on a response.
static void
SelectResponse (ENCOUNTER_STATE *pES)
{
	TEXT *response_text =
			&pES->response_list[pES->cur_response].response_text;
	utf8StringCopy (pES->phrase_buf, sizeof pES->phrase_buf,
			response_text->pStr);
	FeedbackPlayerPhrase (pES->phrase_buf);
	StopTrack ();
	ClearSubtitles ();
	SetSliderImage (SetAbsFrameIndex (ActivityFrame, 2));

	FadeMusic (BACKGROUND_VOL, ONE_SECOND);

	TalkingFinished = FALSE;
	pES->num_responses = 0;
	ClearResponses (pES);
	(*pES->response_list[pES->cur_response].response_func)
			(pES->response_list[pES->cur_response].response_ref);
}

// Called when the player presses the cancel button in comm.
static void
SelectConversationSummary (ENCOUNTER_STATE *pES)
{
	SUMMARY_STATE SummaryState;
	
	if (pES)
		FeedbackPlayerPhrase (pES->phrase_buf);

	SummaryState.Initialized = FALSE;
	DoConvSummary (&SummaryState);

	if (pES)
		RefreshResponses (pES);
	clear_subtitles = TRUE;
}

static void
SelectReplay (ENCOUNTER_STATE *pES)
{
	FadeMusic (BACKGROUND_VOL, ONE_SECOND);
	if (pES)
		FeedbackPlayerPhrase (pES->phrase_buf);

	TalkSegue (0);
}

static void
PlayerResponseInput (ENCOUNTER_STATE *pES)
{
	BYTE response;

	if (pES->top_response == (BYTE)~0)
	{
		pES->top_response = 0;
		RefreshResponses (pES);
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		SelectResponse (pES);
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL] &&
			LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
	{
		SelectConversationSummary (pES);
	}
	else
	{
		response = pES->cur_response;
		if (PulsedInputState.menu[KEY_MENU_LEFT])
		{
			SelectReplay (pES);

			if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
			{
				RefreshResponses (pES);
				FadeMusic (FOREGROUND_VOL, ONE_SECOND);
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_UP])
			response = (BYTE)((response + (BYTE)(pES->num_responses - 1))
					% pES->num_responses);
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
			response = (BYTE)((BYTE)(response + 1) % pES->num_responses);

		if (response != pES->cur_response)
		{
			COORD y;

			BatchGraphics ();
			add_text(-2,
				&pES->response_list[pES->cur_response].response_text);

			pES->cur_response = response;

			y = add_text (-1,
					&pES->response_list[pES->cur_response].response_text);
			if (response < pES->top_response)
			{
				pES->top_response = 0;
				RefreshResponses(pES);
			}
			else if (y > SIS_SCREEN_HEIGHT)
			{
				pES->top_response = response;
				RefreshResponses(pES);
			}
			UnbatchGraphics ();
		}

		UpdateCommGraphics ();

		SleepThreadUntil (pES->NextTime);
		pES->NextTime = GetTimeCounter () + COMM_ANIM_RATE;
	}
}

// Derived from INPUT_STATE_DESC
typedef struct last_replay_state
{
	// Fields required by DoInput()
	BOOLEAN (*InputFunc) (struct last_replay_state *);

	TimeCount NextTime; // framerate control
	TimeCount TimeOut;

} LAST_REPLAY_STATE;

static BOOLEAN
DoLastReplay (LAST_REPLAY_STATE *pLRS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	if (GetTimeCounter () > pLRS->TimeOut)
		return FALSE; // timed out and done

	if (PulsedInputState.menu[KEY_MENU_CANCEL] &&
			LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
	{
		FadeMusic (BACKGROUND_VOL, ONE_SECOND);
		SelectConversationSummary (NULL);
		pLRS->TimeOut = FadeMusic (0, ONE_SECOND * 2) + ONE_SECOND / 60;
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		SelectReplay (NULL);
		pLRS->TimeOut = FadeMusic (0, ONE_SECOND * 2) + ONE_SECOND / 60;
	}

	UpdateCommGraphics ();
	
	SleepThreadUntil (pLRS->NextTime);
	pLRS->NextTime = GetTimeCounter () + COMM_ANIM_RATE;

	return TRUE;
}

static BOOLEAN
DoCommunication (ENCOUNTER_STATE *pES)
{
	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);

	// First, finish playing all queued tracks if not done yet
	if (!TalkingFinished)
	{
		AlienTalkSegue (WAIT_TRACK_ALL);
		return TRUE;
	}

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		;
	else if (pES->num_responses == 0)
	{
		// The player doesn't get a chance to say anything,
		// but can still review alien's last phrases.
		LAST_REPLAY_STATE replayState;

		memset (&replayState, 0, sizeof replayState);
		replayState.TimeOut = FadeMusic (0, ONE_SECOND * 3) + ONE_SECOND / 60;
		replayState.InputFunc = DoLastReplay;
		DoInput (&replayState, FALSE);
	}
	else
	{
		PlayerResponseInput (pES);
		return TRUE;
	}

	SetContext (SpaceContext);
	DestroyContext (AnimContext);
	AnimContext = NULL;

	FlushColorXForms ();
	ClearSubtitles ();

	StopMusic ();
	StopSound ();
	StopTrack ();
	SleepThreadUntil (FadeMusic (NORMAL_VOLUME, 0) + ONE_SECOND / 60);

	return FALSE;
}

void
DoResponsePhrase (RESPONSE_REF R, RESPONSE_FUNC response_func,
		UNICODE *ConstructStr)
{
	ENCOUNTER_STATE *pES = pCurInputState;
	RESPONSE_ENTRY *pEntry;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
			
	if (pES->num_responses == 0)
	{
		pES->cur_response = 0;
		pES->top_response = (BYTE)~0;
	}

	pEntry = &pES->response_list[pES->num_responses];
	pEntry->response_ref = R;
	pEntry->response_text.pStr = ConstructStr;
	if (pEntry->response_text.pStr)
		pEntry->response_text.CharCount = (COUNT)~0;
	else
	{
		STRING locString;
		
		locString = SetAbsStringTableIndex (CommData.ConversationPhrases,
				(COUNT) (R - 1));
		pEntry->response_text.pStr =
				(UNICODE *) GetStringAddress (locString);

		if (luaUqm_comm_stringNeedsInterpolate (pEntry->response_text.pStr))
		{
			pEntry->allocedResponse = luaUqm_comm_stringInterpolate(
					pEntry->response_text.pStr);
			pEntry->response_text.pStr = pEntry->allocedResponse;
		}

		pEntry->response_text.CharCount = (COUNT)~0;
	}
	pEntry->response_func = response_func;
	++pES->num_responses;
}

static void
HailAlien (void)
{
	ENCOUNTER_STATE ES;
	FONT PlayerFont, OldFont;
	MUSIC_REF SongRef = 0;
	Color TextBack;

	IsAltSong = FALSE;

	pCurInputState = &ES;
	memset (pCurInputState, 0, sizeof (*pCurInputState));

	TalkingFinished = FALSE;

	ES.InputFunc = DoCommunication;
	PlayerFont = LoadFont (PLAYER_FONT);
	ComputerFont = LoadFont (COMPUTER_FONT);

	CommData.AlienFrame = CaptureDrawable (
			LoadGraphic (CommData.AlienFrameRes));
	CommData.AlienFont = LoadFont (CommData.AlienFontRes);
	CommData.AlienColorMap = CaptureColorMap (
			LoadColorMap (CommData.AlienColorMapRes));
	if ((CommData.AlienSongFlags & LDASF_USE_ALTERNATE)
			&& CommData.AlienAltSongRes)
		SongRef = LoadMusic (CommData.AlienAltSongRes);
	if (SongRef) {
		CommData.AlienSong = SongRef;
		IsAltSong = TRUE;
	} else
		CommData.AlienSong = LoadMusic(CommData.AlienSongRes);

	CommData.ConversationPhrases = CaptureStringTable (
			LoadStringTable (CommData.ConversationPhrasesRes));

	SubtitleText.baseline = CommData.AlienTextBaseline;
	SubtitleText.align = CommData.AlienTextAlign;


	// init subtitle cache context
	TextCacheContext = CreateContext ("TextCacheContext");
	//TextCacheFrame = CaptureDrawable (
	//		CreateDrawable (WANT_PIXMAP, SIS_SCREEN_WIDTH,
	//		SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT + 2, 1));
	// BW: previous lines were just a complex and wrong way of obtaining 107
 	TextCacheFrame = CaptureDrawable (
 			CreateDrawable (WANT_PIXMAP, SIS_SCREEN_WIDTH, SLIDER_Y, 1));
	SetContext (TextCacheContext);
	SetContextFGFrame (TextCacheFrame);
	TextBack = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x00);
			// Color key for the background.
	SetContextBackGroundColor (TextBack);
	ClearDrawable ();
	SetFrameTransparentColor (TextCacheFrame, TextBack);

	ES.phrase_buf[0] = '\0';

	SetContext (SpaceContext);
	OldFont = SetContextFont (PlayerFont);

	{
		RECT r;

		AnimContext = CreateContext ("AnimContext");
		SetContext (AnimContext);
		SetContextFGFrame (Screen);
		GetFrameRect (CommData.AlienFrame, &r);
		r.extent.width = SIS_SCREEN_WIDTH;
		CommWndRect.corner.x = SIS_ORG_X; // JMS_GFX: Added these lines because of the 
		CommWndRect.corner.y = SIS_ORG_Y; // changed init of CommWndRect in the beginning of comm.c
		CommWndRect.extent = r.extent;
		
		SetTransitionSource (NULL);
		BatchGraphics ();
		if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
		{
			// set the position of outtakes comm
			CommWndRect.corner.x = ((SCREEN_WIDTH - CommWndRect.extent.width) / 2); // JMS_GFX
			CommWndRect.corner.y = RES_SCALE(5); // JMS_GFX
			r.corner = CommWndRect.corner;
			SetContextClipRect (&r);
		}
		else
		{
			r.corner.x = SIS_ORG_X;
			r.corner.y = SIS_ORG_Y;
			SetContextClipRect (&r);
			CommWndRect.corner = r.corner;

			DrawSISFrame ();
			// TODO: find a better way to do this, perhaps set the titles
			// forward from callers.
			if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0
					&& GET_GAME_STATE (STARBASE_AVAILABLE))
			{	// Talking to allied Starbase
				DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 1));
						// "Starbase Commander"
				DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE + 0));
						// "Starbase"
			}
			else
			{	// Default titles: star name + planet name
				DrawSISMessage (NULL);
				DrawSISTitle (GLOBAL_SIS (PlanetName));
			}
		}

		DrawSISComWindow ();
	}


	LastActivity |= CHECK_LOAD; /* prevent spurious input */
	(*CommData.init_encounter_func) ();
	DoInput (&ES, FALSE);
	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
		(*CommData.post_encounter_func) ();
	(*CommData.uninit_encounter_func) ();

	ClearResponses (&ES);
	SetContext (SpaceContext);
	SetContextFont (OldFont);

	DestroyStringTable (ReleaseStringTable (CommData.ConversationPhrases));
	DestroyMusic (CommData.AlienSong);
	DestroyColorMap (ReleaseColorMap (CommData.AlienColorMap));
	DestroyFont (CommData.AlienFont);
	DestroyDrawable (ReleaseDrawable (CommData.AlienFrame));

	DestroyContext (TextCacheContext);
	DestroyDrawable (ReleaseDrawable (TextCacheFrame));

	DestroyFont (PlayerFont);
	DestroyFont (ComputerFont);

	// Some support code tests either of these to see if the
	// game is currently in comm or encounter
	CommData.ConversationPhrasesRes = 0;
	CommData.ConversationPhrases = 0;
	pCurInputState = 0;
}

void
SetCommIntroMode (CommIntroMode newMode, TimeCount howLong)
{
	curIntroMode = newMode;
	fadeTime = howLong;
}

COUNT
InitCommunication (CONVERSATION which_comm)
{
	COUNT status;
	LOCDATA *LocDataPtr;

	IsProbe = FALSE;

#ifdef DEBUG
	if (disableInteractivity)
		return 0;
#endif
	

	if (LastActivity & CHECK_LOAD)
	{
		LastActivity &= ~CHECK_LOAD;
		if (which_comm != COMMANDER_CONVERSATION)
		{
			if (LOBYTE (LastActivity) == 0)
			{
				DrawSISFrame ();
			}
			else
			{
				ClearSISRect (DRAW_SIS_DISPLAY);
				RepairSISBorder ();
			}
			DrawSISMessage (NULL);
			if (inHQSpace ())
				DrawHyperCoords (GLOBAL (ShipStamp.origin));
			else if (GLOBAL (ip_planet) == 0)
				DrawHyperCoords (CurStarDescPtr->star_pt);
			else
				DrawSISTitle (GLOBAL_SIS (PlanetName));
		}
	}


	if (which_comm == URQUAN_DRONE_CONVERSATION)
	{
		status = URQUAN_DRONE_SHIP;
		which_comm = URQUAN_CONVERSATION;
		IsProbe = TRUE;
	}
	else
	{
		if (which_comm == YEHAT_REBEL_CONVERSATION)
		{
			status = YEHAT_REBEL_SHIP;
			which_comm = YEHAT_CONVERSATION;
		}
		else
		{
			COUNT commToShip[] = {
				RACE_SHIP_FOR_COMM
			};
			status = commToShip[which_comm];
			if (status >= YEHAT_REBEL_SHIP) {
				/* conversation exception, set to self */
				status = HUMAN_SHIP;
			}
		}
		StartSphereTracking (status);

		if (which_comm == ORZ_CONVERSATION
				|| (which_comm == TALKING_PET_CONVERSATION
				&& (!GET_GAME_STATE (TALKING_PET_ON_SHIP)
				|| LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE))
				|| (which_comm != CHMMR_CONVERSATION
				&& which_comm != SYREEN_CONVERSATION
				))//&& CheckAlliance (status) == BAD_GUY))
			BuildBattle (NPC_PLAYER_NUM);
	}

	LocDataPtr = init_race (
			status != YEHAT_REBEL_SHIP ? which_comm :
			YEHAT_REBEL_CONVERSATION);
	if (LocDataPtr)
	{	// We make a copy here
		CommData = *LocDataPtr;
	}

	if (GET_GAME_STATE (BATTLE_SEGUE) == 0)
	{
		// Not offered the chance to attack.
		status = HAIL;
	}
	else if ((status = InitEncounter ()) == HAIL && LocDataPtr)
	{
		// The player chose to talk.
		SET_GAME_STATE (BATTLE_SEGUE, 0);
	}
	else
	{
		// The player chose to attack.
		status = ATTACK;
		SET_GAME_STATE (BATTLE_SEGUE, 1);
	}

	if (status == HAIL)
	{
		HailAlien ();
	}
	else if (LocDataPtr)
	{	// only when comm initied successfully
		if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			(*CommData.post_encounter_func) (); // process states

		(*CommData.uninit_encounter_func) (); // cleanup
	}

	status = 0;
	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		// The Sa-Matra battle is skipped when Cyborg is enabled.
		// Most likely because the Cyborg is too dumb to know what
		// to do in this battle.
		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE
				&& (GLOBAL (glob_flags) & CYBORG_ENABLED))
			ReinitQueue (&GLOBAL (npc_built_ship_q));

		// Clear the location flags
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 0);
		status = (GET_GAME_STATE (BATTLE_SEGUE)
				&& GetHeadLink (&GLOBAL (npc_built_ship_q)));
		if (status)
		{
			// Start combat
			BuildBattle (RPG_PLAYER_NUM);
			EncounterBattle ();
		}
		else
		{
			SET_GAME_STATE (BATTLE_SEGUE, 0);
		}
	}

	UninitEncounter ();

	return (status);
}

void
RaceCommunication (void)
{
	COUNT i, status;
	HSHIPFRAG hStarShip;
	SHIP_FRAGMENT *FragPtr;
	HENCOUNTER hEncounter = 0;
	CONVERSATION RaceComm[] =
	{
		RACE_COMMUNICATION
	};

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
	{
		/* Going into talking pet conversation */
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		CloneShipFragment (SAMATRA_SHIP, &GLOBAL (npc_built_ship_q), 0);
		InitCommunication (TALKING_PET_CONVERSATION);
		if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
		{
			GLOBAL (CurrentActivity) = WON_LAST_BATTLE;
		}
		return;
	}
	else if (NextActivity & CHECK_LOAD)
	{
		BYTE ec;

		ec = GET_GAME_STATE (ESCAPE_COUNTER);

		if (GET_GAME_STATE (FOUND_PLUTO_SPATHI) == 1)
			InitCommunication (SPATHI_CONVERSATION);
		else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == 0)
			InitCommunication (TALKING_PET_CONVERSATION);
		else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) &
				((1 << 4) | (1 << 5)))
			// Communicate with the Ilwrath using a Hyperwave Broadcaster.
			InitCommunication (ILWRATH_CONVERSATION);
		else
			InitCommunication (CHMMR_CONVERSATION);
		if (GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
		{
			NextActivity = GLOBAL (CurrentActivity) & ~START_ENCOUNTER;
			if (LOBYTE (NextActivity) == IN_INTERPLANETARY)
				NextActivity |= START_INTERPLANETARY;
			GLOBAL (CurrentActivity) |= CHECK_LOAD; /* fake a load game */
		}

		SET_GAME_STATE (ESCAPE_COUNTER, ec);
		return;
	}
	else if (inHQSpace ())
	{
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) >= 2)
		{
			InitCommunication (ARILOU_CONVERSATION);
			return;
		}
		else
		{
			/* Encounter with a black globe in HS, prepare enemy ship list */
			ENCOUNTER *EncounterPtr;

			// The encounter globe that the flagship collided with is moved
			// to the head of the queue in hyper.c:cleanup_hyperspace()
			hEncounter = GetHeadEncounter ();
			LockEncounter (hEncounter, &EncounterPtr);

			for (i = 0; i < EncounterPtr->num_ships; ++i)
			{
				CloneShipFragment (EncounterPtr->race_id,
						&GLOBAL (npc_built_ship_q),
						EncounterPtr->ShipList[i].crew_level);
			}

			// XXX: Bug: CurStarDescPtr was abused to point within
			//    an ENCOUNTER struct, which is immediately unlocked
			//CurStarDescPtr = (STAR_DESC*)&EncounterPtr->SD;
			UnlockEncounter (hEncounter);
		}
	}

	// First ship in the npc queue defines which alien race
	// the player will be talking to
	hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
	FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
	i = FragPtr->race_id;
	UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);

	status = InitCommunication (RaceComm[i]);

	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
		return;

	if (i == CHMMR_SHIP)
		ReinitQueue (&GLOBAL (npc_built_ship_q));

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
	{
		/* if used destruct code in interplanetary */
		if (i == SLYLANDRO_SHIP && status == 0)
			ReinitQueue (&GLOBAL (npc_built_ship_q));
	}
	else if (hEncounter)
	{
		/* Update HSpace encounter info, ships lefts, etc. */
		BYTE i, NumShips;
		ENCOUNTER *EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);

		NumShips = CountLinks (&GLOBAL (npc_built_ship_q));
		EncounterPtr->num_ships = NumShips;
		EncounterPtr->flags |= ENCOUNTER_REFORMING;
		if (status == 0)
			EncounterPtr->flags |= ONE_SHOT_ENCOUNTER;

		for (i = 0; i < NumShips; ++i)
		{
			HSHIPFRAG hStarShip;
			SHIP_FRAGMENT *FragPtr;
			BRIEF_SHIP_INFO *BSIPtr;

			hStarShip = GetStarShipFromIndex (&GLOBAL (npc_built_ship_q), i);
			FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			BSIPtr = &EncounterPtr->ShipList[i];
			BSIPtr->race_id = FragPtr->race_id;
			BSIPtr->crew_level = FragPtr->crew_level;
			BSIPtr->max_crew = FragPtr->max_crew;
			BSIPtr->max_energy = FragPtr->max_energy;
			UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		}
		
		UnlockEncounter (hEncounter);
		ReinitQueue (&GLOBAL (npc_built_ship_q));
	}
}

static void
RedrawSubtitles (void)
{
	TEXT t;

	if (!optSubtitles)
		return;

	if (SubtitleText.pStr)
	{
		t = SubtitleText;
		add_text (1, &t);
	}
}

static void
ClearSubtitles (void)
{
	clear_subtitles = TRUE;
	last_subtitle = NULL;
	SubtitleText.pStr = NULL;
	SubtitleText.CharCount = 0;
}

static void
CheckSubtitles (void)
{
	const UNICODE *pStr;

	pStr = GetTrackSubtitle ();

	if (pStr != SubtitleText.pStr)
	{	// Subtitles changed
		clear_subtitles = TRUE;
		// Baseline may be updated by the ZFP
		SubtitleText.baseline = CommData.AlienTextBaseline;
		SubtitleText.align = CommData.AlienTextAlign;
		SubtitleText.pStr = pStr;
		// may have been cleared too
		if (pStr)
			SubtitleText.CharCount = (COUNT)~0;
		else
			SubtitleText.CharCount = 0;
	}
}

void
EnableTalkingAnim (BOOLEAN enable)
{
	if (enable)
		CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
	else
		CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
}
