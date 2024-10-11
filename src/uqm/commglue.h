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

#define MAX_INTERPOLATE 8192 // the max length we can robo-interpolate
#define MAX_CLIPNAME 128 // e.g. "pkunk-004c.ogg"
#define NUM_ROBO_TRACKS 11 // Need 11 for getPoint (8 digits, 2 points, 1 by)

enum
{
	ROBOT_NULL_PHRASE,
	ROBOT_DIGIT_0,
	ROBOT_DIGIT_1,
	ROBOT_DIGIT_2,
	ROBOT_DIGIT_3,
	ROBOT_DIGIT_4,
	ROBOT_DIGIT_5,
	ROBOT_DIGIT_6,
	ROBOT_DIGIT_7,
	ROBOT_DIGIT_8,
	ROBOT_DIGIT_9,
	ROBOT_POINT,
	ROBOT_BY,
	ROBOT_COLOR_RED,
	ROBOT_COLOR_ORANGE,
	ROBOT_COLOR_YELLOW,
	ROBOT_COLOR_GREEN,
	ROBOT_COLOR_BLUE,
	ROBOT_COLOR_WHITE,
	ILWRATH_COLOR_RED,
	ILWRATH_COLOR_ORANGE,
	ILWRATH_COLOR_YELLOW,
	ILWRATH_COLOR_GREEN,
	ILWRATH_COLOR_BLUE,
	ILWRATH_COLOR_WHITE,
	ROBOT_PREFIX_0, // Unused but easier
	ROBOT_ALPHA,
	ROBOT_BETA,
	ROBOT_GAMMA,
	ROBOT_DELTA,
	ROBOT_EPSILON,
	ROBOT_ZETA,
	ROBOT_ETA,
	ROBOT_THETA,
	ROBOT_IOTA,
	ROBOT_KAPPA,
	ROBOT_LAMBDA,
	ROBOT_MU,
	ROBOT_NU,
	ROBOT_XI,
	ROBOT_POSTFIX_0,
	ROBOT_POSTFIX_1,
	ROBOT_POSTFIX_2,
	ROBOT_POSTFIX_3,
	ROBOT_POSTFIX_4,
	ROBOT_POSTFIX_5,
	ROBOT_POSTFIX_6,
	ROBOT_POSTFIX_7,
	ROBOT_POSTFIX_8,
	ROBOT_POSTFIX_9,
	ROBOT_POSTFIX_10,
	ROBOT_POSTFIX_11,
	ROBOT_POSTFIX_12,
	ROBOT_POSTFIX_13,
	ROBOT_POSTFIX_14,
	ROBOT_POSTFIX_15,
	ROBOT_POSTFIX_16,
	ROBOT_POSTFIX_17,
	ROBOT_POSTFIX_18,
	ROBOT_POSTFIX_19,
	ROBOT_POSTFIX_20,
	ROBOT_POSTFIX_21,
	ROBOT_POSTFIX_22,
	ROBOT_POSTFIX_23,
	ROBOT_POSTFIX_24,
	ROBOT_POSTFIX_25,
	ROBOT_POSTFIX_26,
	ROBOT_POSTFIX_27,
	ROBOT_POSTFIX_28,
	ROBOT_POSTFIX_29,
	ROBOT_POSTFIX_30,
	ROBOT_POSTFIX_31,
	ROBOT_POSTFIX_32,
	ROBOT_POSTFIX_33,
	ROBOT_POSTFIX_34,
	ROBOT_POSTFIX_35,
	ROBOT_POSTFIX_36,
	ROBOT_POSTFIX_37,
	ROBOT_POSTFIX_38,
	ROBOT_POSTFIX_39,
	ROBOT_POSTFIX_40,
	ROBOT_POSTFIX_41,
	ROBOT_POSTFIX_42,
	ROBOT_POSTFIX_43,
	ROBOT_POSTFIX_44,
	ROBOT_POSTFIX_45,
	ROBOT_POSTFIX_46,
	ROBOT_POSTFIX_47,
	ROBOT_POSTFIX_48,
	ROBOT_POSTFIX_49,
	ROBOT_POSTFIX_50,
	ROBOT_POSTFIX_51,
	ROBOT_POSTFIX_52,
	ROBOT_POSTFIX_53,
	ROBOT_POSTFIX_54,
	ROBOT_POSTFIX_55,
	ROBOT_POSTFIX_56,
	ROBOT_POSTFIX_57,
	ROBOT_POSTFIX_58,
	ROBOT_POSTFIX_59,
	ROBOT_POSTFIX_60,
	ROBOT_POSTFIX_61,
	ROBOT_POSTFIX_62,
	ROBOT_POSTFIX_63,
	ROBOT_POSTFIX_64,
	ROBOT_POSTFIX_65,
	ROBOT_POSTFIX_66,
	ROBOT_POSTFIX_67,
	ROBOT_POSTFIX_68,
	ROBOT_POSTFIX_69,
	ROBOT_POSTFIX_70,
	ROBOT_POSTFIX_71,
	ROBOT_POSTFIX_72,
	ROBOT_POSTFIX_73,
	ROBOT_POSTFIX_74,
	ROBOT_POSTFIX_75,
	ROBOT_POSTFIX_76,
	ROBOT_POSTFIX_77,
	ROBOT_POSTFIX_78,
	ROBOT_POSTFIX_79,
	ROBOT_POSTFIX_80,
	ROBOT_POSTFIX_81,
	ROBOT_POSTFIX_82,
	ROBOT_POSTFIX_83,
	ROBOT_POSTFIX_84,
	ROBOT_POSTFIX_85,
	ROBOT_POSTFIX_86,
	ROBOT_POSTFIX_87,
	ROBOT_POSTFIX_88,
	ROBOT_POSTFIX_89,
	ROBOT_POSTFIX_90,
	ROBOT_POSTFIX_91,
	ROBOT_POSTFIX_92,
	ROBOT_POSTFIX_93,
	ROBOT_POSTFIX_94,
	ROBOT_POSTFIX_95,
	ROBOT_POSTFIX_96,
	ROBOT_POSTFIX_97,
	ROBOT_POSTFIX_98,
	ROBOT_POSTFIX_99,
	ROBOT_POSTFIX_100,
	ROBOT_POSTFIX_101,
	ROBOT_POSTFIX_102,
	ROBOT_POSTFIX_103,
	ROBOT_POSTFIX_104,
	ROBOT_POSTFIX_105,
	ROBOT_POSTFIX_106,
	ROBOT_POSTFIX_107,
	ROBOT_POSTFIX_108,
	ROBOT_POSTFIX_109,
	ROBOT_POSTFIX_110,
	ROBOT_POSTFIX_111,
	ROBOT_POSTFIX_112,
	ROBOT_POSTFIX_113,
	ROBOT_POSTFIX_114,
	ROBOT_POSTFIX_115,
	ROBOT_POSTFIX_116,
	ROBOT_POSTFIX_117,
	ROBOT_POSTFIX_118,
	ROBOT_POSTFIX_119,
	ROBOT_POSTFIX_120,
	ROBOT_POSTFIX_121,
	ROBOT_POSTFIX_122,
	ROBOT_POSTFIX_123,
	ROBOT_POSTFIX_124,
	ROBOT_POSTFIX_125,
	ROBOT_POSTFIX_126,
	ROBOT_POSTFIX_127,
	ROBOT_POSTFIX_128,
	ROBOT_POSTFIX_129,
	ROBOT_POSTFIX_130,
	ROBOT_POSTFIX_131
};

#if defined(__cplusplus)
}
#endif

#endif /* UQM_COMMGLUE_H_ */

