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

#include "port.h"
#include "controls.h"
#include "libs/inplib.h"
#include "libs/memlib.h"
#include "libs/log.h"
#include "globdata.h"
#include "sounds.h"
#include "settings.h"
#include "resinst.h"
#include "nameref.h"

#if defined(ANDROID) || defined(__ANDROID__)
#include <SDL/SDL_screenkeyboard.h>
#endif

// TODO: This may be better done with UniChar at the cost of a tiny bit
//   of overhead to convert UniChar back to UTF8 string. This overhead
//   will probably be offset by removal of looped string-compare overhead ;)
struct joy_char
{
	unsigned char len;
	char enc[7];
		// 1+7 is a nice round number
};

static int
ReadOneChar (joy_char_t *ch, const UNICODE *str)
{
	UNICODE *next = skipUTF8Chars (str, 1);
	int len = next - str;
	ch->len = len;
	memcpy (ch->enc, str, len);
	ch->enc[len] = '\0'; // string term

	return len;
}

static joy_char_t *
LoadJoystickAlpha (STRING String, int *count)
{
	UNICODE *str;
	int c;
	int i;
	joy_char_t *chars;
	UNICODE *cur;

	*count = 0;
	str = GetStringAddress (String);
	if (!str)
		return 0;

	c = utf8StringCount (str);
	chars = HMalloc (c * sizeof (*chars));
	if (!chars)
		return 0;

	for (i = 0, cur = str; i < c; ++i)
	{
		int len = ReadOneChar (chars + i, cur);
		cur += len;
	}

	*count = c;
	return chars;
}

static int
JoyCharFindIn (const joy_char_t *ch, const joy_char_t *set,
		int setsize)
{
	int i;

	for (i = 0; i < setsize && strcmp (set[i].enc, ch->enc) != 0; ++i)
		;
	
	return (i < setsize) ? i : -1;
}

static int
JoyCharIsLower (const joy_char_t *ch, TEXTENTRY_STATE *pTES)
{
	return 0 <= JoyCharFindIn (ch, pTES->JoyLower, pTES->JoyRegLength);
}

static void
JoyCharSwitchReg (joy_char_t *ch, const joy_char_t *from,
		const joy_char_t *to, int regsize)
{
	int i = JoyCharFindIn (ch, from, regsize);
	if (i >= 0)
		*ch = to[i];
}

static void
JoyCharToUpper (joy_char_t *outch, const joy_char_t *ch,
		TEXTENTRY_STATE *pTES)
{
	*outch = *ch;
	JoyCharSwitchReg (outch, pTES->JoyLower, pTES->JoyUpper,
			pTES->JoyRegLength);
}

static void
JoyCharToLower (joy_char_t *outch, const joy_char_t *ch,
		TEXTENTRY_STATE *pTES)
{
	*outch = *ch;
	JoyCharSwitchReg (outch, pTES->JoyUpper, pTES->JoyLower,
			pTES->JoyRegLength);
}

BOOLEAN
DoTextEntry (TEXTENTRY_STATE *pTES)
{
	UniChar ch;
	UNICODE *pStr;
	UNICODE *CacheInsPt;
	int CacheCursorPos;
	int len;
	BOOLEAN changed = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (!pTES->Initialized)
	{	// init basic vars
		int lwlen;

#if defined(ANDROID) || defined(__ANDROID__)
		SDL_ANDROID_ToggleScreenKeyboardTextInput(pTES->BaseStr);
		pTES->BaseStr[0] = 0;
#endif

		pTES->InputFunc = DoTextEntry;
		pTES->Success = FALSE;
		pTES->Initialized = TRUE;
		pTES->JoystickMode = FALSE;
		pTES->UpperRegister = TRUE;
	
		// init insertion point
		if ((size_t)pTES->CursorPos > utf8StringCount (pTES->BaseStr))
			pTES->CursorPos = utf8StringCount (pTES->BaseStr);
		pTES->InsPt = skipUTF8Chars (pTES->BaseStr, pTES->CursorPos);

		// load joystick alphabet
		pTES->JoyAlphaString = CaptureStringTable (
				LoadStringTable (JOYSTICK_ALPHA_STRTAB));
		pTES->JoyAlpha = LoadJoystickAlpha (
				SetAbsStringTableIndex (pTES->JoyAlphaString, 0),
				&pTES->JoyAlphaLength);
		pTES->JoyUpper = LoadJoystickAlpha (
				SetAbsStringTableIndex (pTES->JoyAlphaString, 1),
				&pTES->JoyRegLength);
		pTES->JoyLower = LoadJoystickAlpha (
				SetAbsStringTableIndex (pTES->JoyAlphaString, 2),
				&lwlen);
		if (lwlen != pTES->JoyRegLength)
		{
			if (lwlen < pTES->JoyRegLength)
				pTES->JoyRegLength = lwlen;
			log_add (log_Warning, "Warning: Joystick upper-lower registers"
					" size mismatch; using the smallest subset (%d)",
					pTES->JoyRegLength);
		}

		pTES->CacheStr = HMalloc (pTES->MaxSize * sizeof (*pTES->CacheStr));

		EnterCharacterMode ();
		DoInput (pTES, TRUE);
		ExitCharacterMode ();

		if (pTES->CacheStr)
			HFree (pTES->CacheStr);
		if (pTES->JoyLower)
			HFree (pTES->JoyLower);
		if (pTES->JoyUpper)
			HFree (pTES->JoyUpper);
		if (pTES->JoyAlpha)
			HFree (pTES->JoyAlpha);
		DestroyStringTable ( ReleaseStringTable (pTES->JoyAlphaString));

		return pTES->Success;
	}

	pStr = pTES->InsPt;
	len = strlen (pStr);
	// save a copy of string
	CacheInsPt = pTES->InsPt;
	CacheCursorPos = pTES->CursorPos;
	memcpy (pTES->CacheStr, pTES->BaseStr, pTES->MaxSize);

	// process the pending character buffer
	ch = GetNextCharacter ();
	if (!ch && PulsedInputState.menu[KEY_MENU_ANY])
	{	// keyboard repeat, but only when buffer empty
		ch = GetLastCharacter ();
	}
	while (ch)
	{
		UNICODE chbuf[8];
		int chsize;

		pTES->JoystickMode = FALSE;

		chsize = getStringFromChar (chbuf, sizeof (chbuf), ch);
		if (UniChar_isPrint (ch) && chsize > 0)
		{
			if (pStr + len - pTES->BaseStr + chsize < pTES->MaxSize)
			{	// insert character, when fits
				memmove (pStr + chsize, pStr, len + 1);
				memcpy (pStr, chbuf, chsize);
				pStr += chsize;
				++pTES->CursorPos;
				changed = TRUE;
			}
			else
			{	// does not fit
				PlayMenuSound (MENU_SOUND_FAILURE);
			}
		}
		ch = GetNextCharacter ();
	}

	if (PulsedInputState.menu[KEY_MENU_DELETE])
	{
		if (len)
		{
			joy_char_t ch;
			
			ReadOneChar (&ch, pStr);
			memmove (pStr, pStr + ch.len, len - ch.len + 1);
			len -= ch.len;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_BACKSPACE])
	{
		if (pStr > pTES->BaseStr)
		{
			UNICODE *prev = skipUTF8Chars (pTES->BaseStr,
					pTES->CursorPos - 1);
			
			memmove (prev, pStr, len + 1);
			pStr = prev;
			--pTES->CursorPos;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		if (pStr > pTES->BaseStr)
		{
			UNICODE *prev = skipUTF8Chars (pTES->BaseStr,
					pTES->CursorPos - 1);

			pStr = prev;
			len += (prev - pStr);
			--pTES->CursorPos;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		if (len > 0)
		{
			joy_char_t ch;
			
			ReadOneChar (&ch, pStr);
			pStr += ch.len;
			len -= ch.len;
			++pTES->CursorPos;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_HOME])
	{
		if (pStr > pTES->BaseStr)
		{
			pStr = pTES->BaseStr;
			len = strlen (pStr);
			pTES->CursorPos = 0;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_END])
	{
		if (len > 0)
		{
			pTES->CursorPos += utf8StringCount (pStr);
			pStr += len;
			len = 0;
			changed = TRUE;
		}
	}
	
	if (pTES->JoyAlpha && (
			PulsedInputState.menu[KEY_MENU_UP] ||
			PulsedInputState.menu[KEY_MENU_DOWN] ||
			PulsedInputState.menu[KEY_MENU_PAGE_UP] ||
			PulsedInputState.menu[KEY_MENU_PAGE_DOWN]) )
	{	// do joystick text
		joy_char_t ch;
		joy_char_t newch;
		joy_char_t cmpch;
		int i;
		BOOLEAN curCharUpper;

		pTES->JoystickMode = TRUE;

		if (len)
		{	// changing an existing character
			ReadOneChar (&ch, pStr);
			curCharUpper = !JoyCharIsLower (&ch, pTES);
		}
		else
		{	// adding a new character
			ch = pTES->JoyAlpha[0];
			// new characters will have case determined by the
			// currently selected register
			curCharUpper = pTES->UpperRegister;
		}
		
		newch = ch;
		JoyCharToUpper (&cmpch, &ch, pTES);

		// find current char in the alphabet
		i = JoyCharFindIn (&cmpch, pTES->JoyAlpha, pTES->JoyAlphaLength);

		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			--i;
			if (i < 0)
				i = pTES->JoyAlphaLength - 1;
			newch = pTES->JoyAlpha[i];
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			++i;
			if (i >= pTES->JoyAlphaLength)
				i = 0;
			newch = pTES->JoyAlpha[i];
		}

		if (PulsedInputState.menu[KEY_MENU_PAGE_UP] ||
				PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
		{
			if (len)
			{	// single char change
				if (!curCharUpper)
					JoyCharToUpper (&newch, &newch, pTES);
				else
					JoyCharToLower (&newch, &newch, pTES);
			}
			else
			{	// register change
				pTES->UpperRegister = !pTES->UpperRegister;
			}
		}
		else
		{	// check register
			if (curCharUpper)
				JoyCharToUpper (&newch, &newch, pTES);
			else
				JoyCharToLower (&newch, &newch, pTES);
		}

		if (strcmp (newch.enc, ch.enc) != 0)
		{	// new char is different, put it in
			if (len)
			{	// change current -- this is messy with utf8
				int l = len - ch.len;
				if (pStr + l - pTES->BaseStr + newch.len < pTES->MaxSize)
				{
					// adjust other chars if necessary
					if (newch.len != ch.len)
						memmove (pStr + newch.len, pStr + ch.len, l + 1);

					memcpy (pStr, newch.enc, newch.len);
					len = l + newch.len;
					changed = TRUE;
				}
			}
			else
			{	// append
				if (pStr + len - pTES->BaseStr + newch.len < pTES->MaxSize)
				{
					memcpy (pStr, newch.enc, newch.len);
					pStr[newch.len] = '\0';
					len += newch.len;
					changed = TRUE;
				}
				else
				{	// does not fit
					PlayMenuSound (MENU_SOUND_FAILURE);
				}
			}
		}
	}
	
	if (PulsedInputState.menu[KEY_MENU_SELECT])
	{	// done entering
		pTES->Success = TRUE;
		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_EDIT_CANCEL])
	{	// canceled entering
		pTES->Success = FALSE;
		return FALSE;
	}

	pTES->InsPt = pStr;

	if (changed && pTES->ChangeCallback)
	{
		if (!pTES->ChangeCallback (pTES))
		{	// changes not accepted - revert
			memcpy (pTES->BaseStr, pTES->CacheStr, pTES->MaxSize);
			pTES->InsPt = CacheInsPt;
			pTES->CursorPos = CacheCursorPos;

			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
		
	if (pTES->FrameCallback)
		return pTES->FrameCallback (pTES);
	else
		SleepThread (ONE_SECOND / 30);

	return TRUE;
}

