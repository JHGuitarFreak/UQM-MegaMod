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

#include "commglue.h"

#include "battle.h"
		// For instantVictory
#include "races.h"
#include "lua/luacomm.h"
#include "libs/log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

static int NPCNumberPhrase (int number, const char *fmt, UNICODE **ptrack);

// The CallbackFunction is queued and executes synchronously
// on the Starcon2Main thread
void
NPCPhrase_cb (int index, CallbackFunction cb)
{
	char *pStr;
	void *pClip;
	void *pTimeStamp;
	BOOLEAN isPStrAlloced = FALSE;

	if (index == 0)
		return;

	pStr = (UNICODE *)GetStringAddress (
			SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1));
	pClip = GetStringSoundClip (
			SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1));
	pTimeStamp = GetStringTimeStamp (
			SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1));
		
	if (luaUqm_comm_stringNeedsInterpolate (pStr))
	{
		pStr = luaUqm_comm_stringInterpolate (pStr);
		isPStrAlloced = TRUE;
	}

	SpliceTrack (pClip, pStr, pTimeStamp, cb);

	if (isPStrAlloced)
		HFree (pStr);
}

// Special case variant: prevents page breaks.
void
NPCPhrase_splice (int index)
{
	UNICODE *pStr;
	void *pClip;

	assert (index >= 0);
	if (index == 0)
		return;

	pStr = (UNICODE *)GetStringAddress (
			SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1));
	pClip = GetStringSoundClip (
			SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1));

	if (!pClip)
	{	// Just appending some text
		SpliceTrack (NULL, pStr, NULL, NULL);
	}
	else
	{	// Splicing in some voice
		UNICODE *tracks[] = {NULL, NULL};

		tracks[0] = pClip;
		SpliceMultiTrack (tracks, pStr);
	}
}

void
NPCNumber (int number, const char *fmt)
{
	UNICODE buf[32];

	if (!fmt)
		fmt = "%d";

	if (CommData.AlienNumberSpeech)
	{
		NPCNumberPhrase (number, fmt, NULL);
		return;
	}
	
	// just splice in the subtitle text
	snprintf (buf, sizeof buf, fmt, number);
	SpliceTrack (NULL, buf, NULL, NULL);
}

static int
NPCNumberPhrase (int number, const char *fmt, UNICODE **ptrack)
{
#define MAX_NUMBER_TRACKS 20
	NUMBER_SPEECH speech = CommData.AlienNumberSpeech;
	COUNT i;
	int queued = 0;
	int toplevel = 0;
	UNICODE *TrackNames[MAX_NUMBER_TRACKS];
	UNICODE numbuf[60];
	const SPEECH_DIGIT* dig = NULL;

	if (!speech)
		return 0;

	if (!ptrack)
	{
		toplevel = 1;
		if (!fmt)
			fmt = "%d";
		sprintf (numbuf, fmt, number);
		ptrack = TrackNames;
	}

	for (i = 0; i < speech->NumDigits; ++i)
	{
		int quot;

		dig = speech->Digits + i;
		quot = number / dig->Divider;
	
		if (quot == 0)
			continue;
		quot -= dig->Subtrahend;
		if (quot < 0)
			continue;

		if (dig->StrDigits)
		{
			COUNT index;

			assert (quot < 10);
			index = dig->StrDigits[quot];
			if (index == 0)
				continue;
			index -= 1;

			*ptrack++ = GetStringSoundClip (SetAbsStringTableIndex (
					CommData.ConversationPhrases, index
					));
			queued++;
		}
		else
		{
			int ctracks = NPCNumberPhrase (quot, NULL, ptrack);
			ptrack += ctracks;
			queued += ctracks;
		}

		if (dig->Names != 0)
		{
			SPEECH_DIGITNAME* name;

			for (name = dig->Names; name->Divider; ++name)
			{
				if (number % name->Divider <= name->MaxRemainder)
				{
					*ptrack++ = GetStringSoundClip (
							SetAbsStringTableIndex (
							CommData.ConversationPhrases, name->StrIndex - 1));
					queued++;
					break;
				}
			}
		}
		else if (dig->CommonNameIndex != 0)
		{
			*ptrack++ = GetStringSoundClip (SetAbsStringTableIndex (
					CommData.ConversationPhrases, dig->CommonNameIndex - 1));
			queued++;
		}

		number %= dig->Divider;
	}

	if (toplevel)
	{
		if (queued == 0)
		{	// nothing queued, say "zero"
			assert (number == 0);
			*ptrack++ = GetStringSoundClip (SetAbsStringTableIndex (
					CommData.ConversationPhrases, dig->StrDigits[number] - 1));
		}
		*ptrack++ = NULL; // term
		
		SpliceMultiTrack (TrackNames, numbuf);
	}
	
	return queued;
}

void
construct_response (UNICODE *buf, int R /* promoted from RESPONSE_REF */, ...)
{
	UNICODE *buf_start = buf;
	UNICODE *name;
	va_list vlist;
	
	va_start (vlist, R);
	
	do
	{
		COUNT len;
		STRING S;
		
		S = SetAbsStringTableIndex (CommData.ConversationPhrases, R - 1);
		
		strcpy (buf, (UNICODE *)GetStringAddress (S));
		
		len = strlen (buf);
		
		buf += len;
		
		name = va_arg (vlist, UNICODE *);
		
		if (name)
		{
			len = strlen (name);
			strncpy (buf, name, len);
			buf += len;
			
			/*
			if ((R = va_arg (vlist, RESPONSE_REF)) == (RESPONSE_REF)-1)
				name = 0;
			*/
					
			R = va_arg(vlist, int);
			if (R == ((RESPONSE_REF) -1))
				name = 0;
		}
	} while (name);
	va_end (vlist);
	
	*buf = '\0';

	// XXX: this should someday be changed so that the function takes
	//   the buffer size as an argument
	if ((buf_start == shared_phrase_buf) &&
			(buf > shared_phrase_buf + sizeof (shared_phrase_buf)))
	{
		log_add (log_Fatal, "Error: shared_phrase_buf size exceeded,"
				" please increase!\n");
		exit (EXIT_FAILURE);
	}
}

void
setSegue (Segue segue)
{
	switch (segue)
	{
		case Segue_peace:
			SET_GAME_STATE (BATTLE_SEGUE, 0);
			break;
		case Segue_hostile:
			SET_GAME_STATE (BATTLE_SEGUE, 1);
			break;
		case Segue_victory:
			instantVictory = TRUE;
			SET_GAME_STATE (BATTLE_SEGUE, 1);
			break;
		case Segue_defeat:
			SET_GAME_STATE (BATTLE_SEGUE, 0);
			GLOBAL_SIS(CrewEnlisted) = (COUNT)~0;
			GLOBAL(CurrentActivity) |= CHECK_RESTART;
			break;
	}
}

Segue
getSegue (void)
{
	if (GET_GAME_STATE(BATTLE_SEGUE) == 0) {
		if (GLOBAL_SIS(CrewEnlisted) == (COUNT)~0 &&
				(GLOBAL(CurrentActivity) & CHECK_RESTART)) {
			return Segue_defeat;
		} else {
			return Segue_peace;
		}
	} else /* GET_GAME_STATE(BATTLE_SEGUE) == 1) */ {
		if (instantVictory) {
			return Segue_victory;
		} else {
			return Segue_hostile;
		}
	}
}

LOCDATA*
init_race (CONVERSATION comm_id)
{
	switch (comm_id)
	{
		case ARILOU_CONVERSATION:
			return init_arilou_comm ();
		case BLACKURQ_CONVERSATION:
			return init_blackurq_comm ();
		case CHMMR_CONVERSATION:
			return init_chmmr_comm ();
		case COMMANDER_CONVERSATION:
			if (!GET_GAME_STATE (STARBASE_AVAILABLE))
				return init_commander_comm ();
			else
				return init_starbase_comm ();
		case DRUUGE_CONVERSATION:
			return init_druuge_comm ();
		case ILWRATH_CONVERSATION:
			return init_ilwrath_comm ();
		case MELNORME_CONVERSATION:
			return init_melnorme_comm ();
		case MYCON_CONVERSATION:
			return init_mycon_comm ();
		case ORZ_CONVERSATION:
			return init_orz_comm ();
		case PKUNK_CONVERSATION:
			return init_pkunk_comm ();
		case SHOFIXTI_CONVERSATION:
			return init_shofixti_comm ();
		case SLYLANDRO_CONVERSATION:
			return init_slyland_comm ();
		case SLYLANDRO_HOME_CONVERSATION:
			return init_slylandro_comm ();
		case SPATHI_CONVERSATION:
			if (!(GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
				return init_spathi_comm ();
			else
				return init_spahome_comm ();
		case SUPOX_CONVERSATION:
			return init_supox_comm ();
		case SYREEN_CONVERSATION:
			return init_syreen_comm ();
		case TALKING_PET_CONVERSATION:
			return init_talkpet_comm ();
		case THRADD_CONVERSATION:
			return init_thradd_comm ();
		case UMGAH_CONVERSATION:
			return init_umgah_comm ();
		case URQUAN_CONVERSATION:
			return init_urquan_comm ();
		case UTWIG_CONVERSATION:
			return init_utwig_comm ();
		case VUX_CONVERSATION:
			return init_vux_comm ();
		case YEHAT_REBEL_CONVERSATION:
			return init_rebel_yehat_comm ();
		case YEHAT_CONVERSATION:
			return init_yehat_comm ();
		case ZOQFOTPIK_CONVERSATION:
			return init_zoqfot_comm ();
		default:
			return init_chmmr_comm ();
	}
}

RESPONSE_REF
phraseIdStrToNum(const char *phraseIdStr)
{
	STRING phrase = GetStringByName (GetStringTable(
			CommData.ConversationPhrases), phraseIdStr);
	if (phrase == NULL)
		return (RESPONSE_REF) -1;

	return GetStringTableIndex (phrase) + 1;
			// Index 0 is for NULL_PHRASE, hence the '+ 1"
}

const char *
phraseIdNumToStr (RESPONSE_REF response)
{
	STRING phrase = SetAbsStringTableIndex (
			CommData.ConversationPhrases, response - 1);
			// Index 0 is for NULL_PHRASE, hence the '- 1'.
	if (phrase == NULL)
		return NULL;
	return GetStringName (phrase);
}

