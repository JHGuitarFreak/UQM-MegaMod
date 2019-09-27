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

#ifndef UQM_COMMGLUE_H_
#define UQM_COMMGLUE_H_

#include "globdata.h"
#include "resinst.h"
#include "libs/sound/trackplayer.h"
#include "libs/callback.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern LOCDATA CommData;
extern UNICODE shared_phrase_buf[2048];

#define PLAYER_SAID(r,i) ((r)==(i))
#define PHRASE_ENABLED(p) \
		(*(UNICODE *)GetStringAddress ( \
				SetAbsStringTableIndex (CommData.ConversationPhrases, (p)-1) \
				) != '\0')
#define DISABLE_PHRASE(p) \
		(*(UNICODE *)GetStringAddress ( \
				SetAbsStringTableIndex (CommData.ConversationPhrases, (p)-1) \
				) = '\0')

#define Response(i,a) \
		DoResponsePhrase(i,(RESPONSE_FUNC)a,0)

typedef COUNT RESPONSE_REF;

typedef void (*RESPONSE_FUNC) (RESPONSE_REF R);

extern void DoResponsePhrase (RESPONSE_REF R, RESPONSE_FUNC
		response_func, UNICODE *ContstructStr);

// The CallbackFunction is queued and executes synchronously
// on the Starcon2Main thread
extern void NPCPhrase_cb (int index, CallbackFunction cb);
#define NPCPhrase(index) NPCPhrase_cb ((index), NULL)
extern void NPCPhrase_splice (int index);
extern void NPCNumber (int number, const char *fmt);

extern void construct_response (UNICODE *buf, int R /* promoted from
		RESPONSE_REF */, ...);

typedef enum {
	Segue_peace,
			// When initiating a conversation, open comms directly.
			// When terminating a conversation, depart in peace.
	Segue_hostile,
			// When initiating a conversation, offer the choice to attack.
			// When terminating a conversation, go into battle.
	Segue_victory,
			// (when terminating a conversation) instant victory
	Segue_defeat,
			// (when terminating a conversation) game over
} Segue;

void setSegue (Segue segue);
Segue getSegue (void);

extern RESPONSE_REF phraseIdStrToNum(const char *phraseId);
extern const char *phraseIdNumToStr (RESPONSE_REF response);

extern LOCDATA* init_race (CONVERSATION comm_id);

extern LOCDATA* init_arilou_comm (void);

extern LOCDATA* init_blackurq_comm (void);

extern LOCDATA* init_chmmr_comm (void);

extern LOCDATA* init_commander_comm (void);

extern LOCDATA* init_druuge_comm (void);

extern LOCDATA* init_ilwrath_comm (void);

extern LOCDATA* init_melnorme_comm (void);

extern LOCDATA* init_mycon_comm (void);

extern LOCDATA* init_orz_comm (void);

extern LOCDATA* init_pkunk_comm (void);

extern LOCDATA* init_rebel_yehat_comm (void);

extern LOCDATA* init_shofixti_comm (void);

extern LOCDATA* init_slyland_comm (void);

extern LOCDATA* init_slylandro_comm (void);

extern LOCDATA* init_spahome_comm (void);

extern LOCDATA* init_spathi_comm (void);

extern LOCDATA* init_starbase_comm (void);

extern LOCDATA* init_supox_comm (void);

extern LOCDATA* init_syreen_comm (void);

extern LOCDATA* init_talkpet_comm (void);

extern LOCDATA* init_thradd_comm (void);

extern LOCDATA* init_umgah_comm (void);

extern LOCDATA* init_urquan_comm (void);

extern LOCDATA* init_utwig_comm (void);

extern LOCDATA* init_vux_comm (void);

extern LOCDATA* init_yehat_comm (void);

extern LOCDATA* init_zoqfot_comm (void);

extern LOCDATA* init_umgah_comm (void);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_COMMGLUE_H_ */

