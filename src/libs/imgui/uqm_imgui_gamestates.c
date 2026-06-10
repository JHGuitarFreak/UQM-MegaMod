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

#include "uqm_imgui.h"

#include "uqm/races.h"
#include "uqm/gamestr.h"
#include "uqm/gameev.h"

#define CHILD_FLAGS ImGuiChildFlags_AutoResizeY \
		| ImGuiChildFlags_AlwaysUseWindowPadding

#define TABLE_FLAGS ImGuiTableFlags_SizingStretchSame | \
					ImGuiTableFlags_PadOuterX


static void GamestatesTab00 (void);
static void GamestatesTab01 (void);
static void UQM_BitRegister (const char *gamestate, int size);

void
draw_gamestates_menu (void)
{
	//if (IN_MAIN_MENU)
	//{
	//	ImGui_Text ("Game States are not available in the Main Menu...");
	//	return;
	//}

	if (ImGui_BeginTabBar ("GameStateTabs", 0))
	{
		GamestatesTab00 ();
		GamestatesTab01 ();

		ImGui_EndTabBar ();
	}

}

static void
UQM_GameStateCheckBox (const char *gs_name)
{
	bool game_state = D_GET_CGAME_STATE (gs_name);

	if (ImGui_Checkbox (gs_name, &game_state))
		D_SET_CGAME_STATE (gs_name, game_state);
}
#define GS_CHECKBOX(gs_name) UQM_GameStateCheckBox(#gs_name)

static void
GamestatesTab00 (void)
{
	if (!ImGui_BeginTabItem ("Page 1", NULL, 0))
		return;

	Spacer ();

	ImGui_ColumnsEx (DISPLAY_BOOL, "GameStateColumns", false);
	if (DISPLAY_BOOL != 1)
		ImGui_BeginStyledChild ("##Column1", ZERO_F, CHILD_FLAGS, 0, NULL);

	GS_CHECKBOX (SHOFIXTI_KIA);
	GS_CHECKBOX (SHOFIXTI_BRO_KIA);
	GS_CHECKBOX (SHOFIXTI_RECRUITED);

	Spacer ();

	{
		int found_pluto_spathi = GET_CGAME_STATE (FOUND_PLUTO_SPATHI);
		const char *fps_states[] =
		{
			"Not Found", "Talking to", "Post-dialog", "Told the Safe Ones"
		};
		ImGui_Text ("FOUND_PLUTO_SPATHI");
		if (ImGui_ComboChar ("##FOUND_PLUTO_SPATHI", &found_pluto_spathi,
			fps_states, ARRAY_SIZE (fps_states)))
		{
			SET_CGAME_STATE (FOUND_PLUTO_SPATHI, found_pluto_spathi);
		}
	}

	Spacer ();

	GS_CHECKBOX (SPATHI_SHIELDED_SELVES);
	GS_CHECKBOX (SPATHI_CREATURES_EXAMINED);
	GS_CHECKBOX (SPATHI_CREATURES_ELIMINATED);

	Spacer ();

	{
		int spathi_manner = GET_CGAME_STATE (SPATHI_MANNER);
		const char *sm_states[] =
		{
			"Neutral", "Miffed", "Pissed", "Friendly"
		};
		ImGui_Text ("SPATHI_MANNER");
		if (ImGui_ComboChar ("##SPATHI_MANNER", &spathi_manner,
			sm_states, ARRAY_SIZE (sm_states)))
		{
			SET_CGAME_STATE (SPATHI_MANNER, spathi_manner);
		}
	}

	Spacer ();

	{
		int lied_about_creatures = GET_CGAME_STATE (LIED_ABOUT_CREATURES);
		const char *lac_states[] =
		{
			"Haven't Lied", "Lied Once", "Lied Twice"
		};
		ImGui_Text ("SPATHI_MANNER");
		if (ImGui_ComboChar ("##SPATHI_MANNER", &lied_about_creatures,
			lac_states, ARRAY_SIZE (lac_states)))
		{
			SET_CGAME_STATE (LIED_ABOUT_CREATURES, lied_about_creatures);
		}
	}

	Spacer ();

	GS_CHECKBOX (SPATHI_QUEST);
	GS_CHECKBOX (SPATHI_PARTY);
	GS_CHECKBOX (KNOW_SPATHI_PASSWORD);
	GS_CHECKBOX (ARILOU_SPACE);
	GS_CHECKBOX (MET_MELNORME);
	GS_CHECKBOX (MELNORME_RESCUE_REFUSED);
	GS_CHECKBOX (TRADED_WITH_MELNORME);
	GS_CHECKBOX (WHY_MELNORME_PURPLE);
	GS_CHECKBOX (PROBE_MESSAGE_DELIVERED);
	GS_CHECKBOX (PROBE_ILWRATH_ENCOUNTER);
	GS_CHECKBOX (STARBASE_AVAILABLE);
	GS_CHECKBOX (STARBASE_VISITED);
	GS_CHECKBOX (RADIOACTIVES_PROVIDED);
	GS_CHECKBOX (AWARE_OF_SAMATRA);
	GS_CHECKBOX (YEHAT_CAVALRY_ARRIVED);
	GS_CHECKBOX (URQUAN_MESSED_UP);
	GS_CHECKBOX (KOHR_AH_KILLED_ALL);

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_NextColumn ();
		ImGui_BeginStyledChild ("##Column2", ZERO_F, CHILD_FLAGS, 0, NULL);
	}

	Spacer ();

	GS_CHECKBOX (MET_ARILOU);
	GS_CHECKBOX (ATTACKED_DRUUGE);

	{
		int new_alliance_name = GET_CGAME_STATE (NEW_ALLIANCE_NAME);
		static char empire_of[PATH_MAX];
		const char *nan_states[] =
		{
				"The New Alliance of Free Stars",
				"The Concordance of Alien Nations",
				"The United Federation of Worlds",
				empire_of
		};

		snprintf (empire_of, sizeof (empire_of), "The Empire of %s",
			GLOBAL_SIS (CommanderName));

		ImGui_Text ("NEW_ALLIANCE_NAME");
		if (ImGui_ComboChar ("##NEW_ALLIANCE_NAME", &new_alliance_name,
			nan_states, ARRAY_SIZE (nan_states)))
		{
			SET_CGAME_STATE (NEW_ALLIANCE_NAME, new_alliance_name);
		}
	}

	Spacer ();

	{
		bool game_state = GET_CGAME_STATE (PORTAL_COUNTER) == 1 ? true : false;

		if (ImGui_Checkbox ("PORTAL_COUNTER", &game_state))
			SET_CGAME_STATE (PORTAL_COUNTER, game_state);
	}

	GS_CHECKBOX (KOHR_AH_FRENZY);
	GS_CHECKBOX (ILWRATH_DECEIVED);
	GS_CHECKBOX (FLAGSHIP_CLOAKED);
	GS_CHECKBOX (MYCON_AMBUSH);
	GS_CHECKBOX (MYCON_FELL_FOR_AMBUSH);

	Spacer ();

	{
		int orz_manner = GET_CGAME_STATE (ORZ_MANNER);
		const char *om_states[] =
		{
				"Neutral",
				"Kinda miffed",
				"FRUMPLE!",
				"Friendly"
		};

		ImGui_Text ("ORZ_MANNER");
		if (ImGui_ComboChar ("##ORZ_MANNER", &orz_manner,
			om_states, ARRAY_SIZE (om_states)))
		{
			SET_CGAME_STATE (ORZ_MANNER, orz_manner);
		}
	}

	Spacer ();

	GS_CHECKBOX (PROBE_EXHIBITED_BUG);
	GS_CHECKBOX (PLAYER_HYPNOTIZED);
	GS_CHECKBOX (ZEX_IS_DEAD);
	GS_CHECKBOX (KNOW_ZEX_WANTS_MONSTER);
	GS_CHECKBOX (UTWIG_HAVE_ULTRON);
	GS_CHECKBOX (BOMB_UNPROTECTED);
	GS_CHECKBOX (TAALO_UNPROTECTED);
	GS_CHECKBOX (UMGAH_ZOMBIE_BLOBBIES);
	GS_CHECKBOX (KNOW_UMGAH_ZOMBIES);
	GS_CHECKBOX (KNOW_ARILOU_WANT_WRECK);
	GS_CHECKBOX (MET_NORMAL_UMGAH);
	GS_CHECKBOX (KNOW_SYREEN_VAULT);
	GS_CHECKBOX (SUN_DEVICE_UNGUARDED);
	GS_CHECKBOX (CHMMR_EMERGING);
	GS_CHECKBOX (CHMMR_UNLEASHED);

	Spacer ();

	{
		int chmmr_bomb_state = GET_CGAME_STATE (CHMMR_BOMB_STATE);
		const char *cb_states[] =
		{
				"Ignorant about Precursor Bomb",
				"You need the Precursor Bomb",
				"Chmmr Bomb is being Installed",
				"Chmmr Bomb Installed"
		};

		ImGui_Text ("CHMMR_BOMB_STATE");
		if (ImGui_ComboChar ("##CHMMR_BOMB_STATE", &chmmr_bomb_state,
			cb_states, ARRAY_SIZE (cb_states)))
		{
			SET_CGAME_STATE (CHMMR_BOMB_STATE, chmmr_bomb_state);
		}
	}

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_NextColumn ();
		ImGui_BeginStyledChild ("##Column3", ZERO_F, CHILD_FLAGS, 0, NULL);
	}

	GS_CHECKBOX (DRUUGE_DISCLAIMER);
	GS_CHECKBOX (YEHAT_CIVIL_WAR);
	GS_CHECKBOX (YEHAT_ABSORBED_PKUNK);

	Spacer ();

	{
		int pkunk_mission = GET_CGAME_STATE (PKUNK_MISSION);
		const char *pm_states[] =
		{
				"Stationary",
				"On the move",
				"Returning",
				"On the move again",
				"Returning again",
				"On the move yet again"
		};

		ImGui_Text ("PKUNK_MISSION");
		if (ImGui_ComboChar ("##PKUNK_MISSION", &pkunk_mission,
			pm_states, ARRAY_SIZE (pm_states)))
		{
			SET_CGAME_STATE (PKUNK_MISSION, pkunk_mission);
		}
		if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
		{
			ImGui_SetTooltip ("Switch this to \"Returning\" if you've turned\n"
				"off any of the GOOD_REASONS.");
		}
	}

	Spacer ();

	{
		int thradd_culture = GET_CGAME_STATE (THRADD_CULTURE);
		static char empire_of[PATH_MAX];
		const char *tc_states[] =
		{
				"Unknown culture",
				"Culture Twenty",
				"The Fat Obstreperous Jerks",
				empire_of
		};

		snprintf (empire_of, sizeof (empire_of),
			"The Glorious Slave Empire of %s", GLOBAL_SIS (CommanderName));

		ImGui_Text ("THRADD_CULTURE");
		if (ImGui_ComboChar ("##THRADD_CULTURE", &thradd_culture,
			tc_states, ARRAY_SIZE (tc_states)))
		{
			SET_CGAME_STATE (THRADD_CULTURE, thradd_culture);
		}
	}

	Spacer ();

	{
		int thradd_mission = GET_CGAME_STATE (THRADD_MISSION);
		const char *tm_states[] =
		{
				"No Mission",
				"Heading towards the Kohr-Ah",
				"Fighting the Kohr-Ah",
				"Returning from fight",
				"Back at home"
		};

		ImGui_Text ("THRADD_MISSION");
		if (ImGui_ComboChar ("##THRADD_MISSION", &thradd_mission,
			tm_states, ARRAY_SIZE (tm_states)))
		{
			SET_CGAME_STATE (THRADD_MISSION, thradd_mission);
		}
		if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
		{
			ImGui_SetTooltip ("Editing this only changes dialog responses\n"
					"and doesn't affect the actual mission.");
		}
	}

	Spacer ();

	GS_CHECKBOX (ZOQFOT_HOSTILE);
	GS_CHECKBOX (MET_ZOQFOT);

	Spacer ();

	{
		int zfp_distress = GET_CGAME_STATE (ZOQFOT_DISTRESS);
		const char *zfpd_states[] =
		{
				"ZFP aren't in distress",
				"ZFP Are under attack!",
				"ZFP have been destroyed by attack"
		};

		ImGui_Text ("ZOQFOT_DISTRESS");
		if (ImGui_ComboChar ("##ZOQFOT_DISTRESS", &zfp_distress,
			zfpd_states, ARRAY_SIZE (zfpd_states)))
		{
			SET_CGAME_STATE (ZOQFOT_DISTRESS, zfp_distress);
		}
	}

	Spacer ();

	GS_CHECKBOX (USED_BROADCASTER);
	GS_CHECKBOX (BROADCASTER_RESPONSE);
	GS_CHECKBOX (MET_ORZ_BEFORE);
	GS_CHECKBOX (YEHAT_REBEL_TOLD_PKUNK);
	GS_CHECKBOX (PLAYER_HAD_SEX);

	Spacer ();

	{
		int CrewSold = MAKE_WORD (
			GET_CGAME_STATE (CREW_SOLD_TO_DRUUGE0),
			GET_CGAME_STATE (CREW_SOLD_TO_DRUUGE1));

		ImGui_Text ("CREW_SOLD_TO_DRUUGE");
		ImGui_InputIntEx ("##CREW_SOLD_TO_DRUUGE", &CrewSold, 0, 0, 0);
		if (ImGui_IsItemDeactivatedAfterEdit ()
			&& CrewSold < (COUNT)~0 && CrewSold > 0)
		{
			SET_CGAME_STATE (CREW_SOLD_TO_DRUUGE0, LOBYTE (CrewSold));
			SET_CGAME_STATE (CREW_SOLD_TO_DRUUGE1, HIBYTE (CrewSold));
		}
	}

	Spacer ();

	GS_CHECKBOX (URQUAN_PROTECTING_SAMATRA);

	Spacer ();

	{
		int thradd_body_count = GET_CGAME_STATE (THRADDASH_BODY_COUNT);

		ImGui_Text ("THRADDASH_BODY_COUNT");
		ImGui_InputIntEx ("##THRADDASH_BODY_COUNT", &thradd_body_count, 0, 0, 0);
		if (ImGui_IsItemDeactivatedAfterEdit ()
			&& thradd_body_count < (COUNT)~0 && thradd_body_count > 0)
		{
			SET_CGAME_STATE (THRADDASH_BODY_COUNT, thradd_body_count);
		}
	}

	Spacer ();

	{
		int utwig_supox_mission = GET_CGAME_STATE (UTWIG_SUPOX_MISSION);
		const char *usm_states[] =
		{
			"U&S fleet haven't left",
			"U&S are on their way",
			"U&S are fighting the Kohr-Ah",
			"U&S are fighting the Kohr-Ah 2",
			"U&S are returning home",
			"U&S are back home"
		};

		ImGui_Text ("UTWIG_SUPOX_MISSION");
		if (ImGui_ComboChar ("##UTWIG_SUPOX_MISSION", &utwig_supox_mission,
			usm_states, ARRAY_SIZE (usm_states)))
		{
			SET_CGAME_STATE (UTWIG_SUPOX_MISSION, utwig_supox_mission);
		}
	}

	Spacer ();

	GS_CHECKBOX (ILWRATH_FIGHT_THRADDASH);
	GS_CHECKBOX (READY_TO_CONFUSE_URQUAN);
	GS_CHECKBOX (URQUAN_HYPNO_VISITS);

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_Columns ();
	}

	ImGui_EndTabItem ();
}

static void
GamestatesTab01 (void)
{
	if (!ImGui_BeginTabItem ("Page 2", NULL, 0))
		return;

	Spacer ();

	ImGui_ColumnsEx (DISPLAY_BOOL, "GameStateColumns", false);
	if (DISPLAY_BOOL != 1)
		ImGui_BeginStyledChild ("##Column1", ZERO_F, CHILD_FLAGS, 0, NULL);

	GS_CHECKBOX (MENTIONED_PET_COMPULSION);

	Spacer ();

	{
		int know_about_shattered = GET_CGAME_STATE (KNOW_ABOUT_SHATTERED);
		const char *kas_states[] =
		{
			"Ignorant of Shattered Worlds",
			"Encountered Shattered World",
			"Know are caused by Deep Children",
			"Told the Syreen about Deep Children"
		};

		ImGui_Text ("KNOW_ABOUT_SHATTERED");
		if (ImGui_ComboChar ("##KNOW_ABOUT_SHATTERED", &know_about_shattered,
			kas_states, ARRAY_SIZE (kas_states)))
		{
			SET_CGAME_STATE (KNOW_ABOUT_SHATTERED, know_about_shattered);
		}
	}

	Spacer ();

	GS_CHECKBOX (MYCON_KNOW_AMBUSH);
	GS_CHECKBOX (KNOW_SYREEN_WORLD_SHATTERED);
	GS_CHECKBOX (LEARNED_TALKING_PET);
	GS_CHECKBOX (DNYARRI_LIED);
	GS_CHECKBOX (SHIP_TO_COMPEL);
	GS_CHECKBOX (REFUSED_ORZ_ALLIANCE);

	Spacer ();

	{
		int pkunk_manner = GET_CGAME_STATE (PKUNK_MANNER);
		const char *pm_states[] =
		{
			"Not met Pkunk",
			"Fought Pkunk, relation salvageable",
			"Hostile relations with Pkunk",
			"Friendly relations with Pkunk"
		};

		ImGui_Text ("PKUNK_MANNER");
		if (ImGui_ComboChar ("##PKUNK_MANNER", &pkunk_manner,
			pm_states, ARRAY_SIZE (pm_states)))
		{
			SET_CGAME_STATE (PKUNK_MANNER, pkunk_manner);
		}
	}

	Spacer ();

	GS_CHECKBOX (PKUNK_ON_THE_MOVE);
	GS_CHECKBOX (PKUNK_RETURN);

	Spacer ();

	{
		BYTE ReasonMask = GET_CGAME_STATE (PKUNK_REASONS);
		bool good_reason_1 = ReasonMask & (1 << 0);
		bool good_reason_2 = ReasonMask & (1 << 1);
		bool bad_reason_1 = ReasonMask & (1 << 2);
		bool bad_reason_2 = ReasonMask & (1 << 3);

		ImGui_Text ("PKUNK_REASONS");

		ImGui_BeginGroup ();
		if (ImGui_Checkbox ("##GOOD_REASON_1", &good_reason_1))
		{
			ReasonMask &= ~(1 << 0);

			if (good_reason_1)
				ReasonMask |= (1 << 0);

			SET_CGAME_STATE (PKUNK_REASONS, ReasonMask);
		}
		ImGui_SameLine ();
		if (ImGui_Checkbox ("GOOD_REASONS", &good_reason_2))
		{
			ReasonMask &= ~(1 << 1);

			if (good_reason_2)
				ReasonMask |= (1 << 1);

			SET_CGAME_STATE (PKUNK_REASONS, ReasonMask);
		}
		ImGui_EndGroup ();
		if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
		{
			ImGui_SetTooltip ("When turning these off make sure to set\n"
					"PKUNK_MISSION to \"Returning\"");
		}
		if (ImGui_Checkbox ("##BAD_REASON_1", &bad_reason_1))
		{
			ReasonMask &= ~(1 << 2);

			if (bad_reason_1)
				ReasonMask |= (1 << 2);

			SET_CGAME_STATE (PKUNK_REASONS, ReasonMask);
		}
		ImGui_SameLine ();
		if (ImGui_Checkbox ("BAD_REASONS", &bad_reason_2))
		{
			ReasonMask &= ~(1 << 3);

			if (bad_reason_2)
				ReasonMask |= (1 << 3);

			SET_CGAME_STATE (PKUNK_REASONS, ReasonMask);
		}
	}

	Spacer ();

	GS_CHECKBOX (PKUNK_SWITCH);
	GS_CHECKBOX (UMGAH_HOSTILE);
	GS_CHECKBOX (UMGAH_EVIL_BLOBBIES);
	GS_CHECKBOX (BOMB_CARRIER);
	GS_CHECKBOX (THRADD_MANNER);

	Spacer ();

	{
		int arilou_manner = GET_CGAME_STATE (ARILOU_MANNER);
		const char *am_states[] =
		{
			"Not met Arilou",
			"Fought Arilou, relation salvageable",
			"Hostile relations with Arilou",
			"Friendly relations with Arilou"
		};

		ImGui_Text ("ARILOU_MANNER");
		if (ImGui_ComboChar ("##ARILOU_MANNER", &arilou_manner,
			am_states, ARRAY_SIZE (am_states)))
		{
			SET_CGAME_STATE (ARILOU_MANNER, arilou_manner);
		}
	}

	Spacer ();

	GS_CHECKBOX (DRUUGE_MANNER);
	GS_CHECKBOX (SUPOX_HOSTILE);
	GS_CHECKBOX (UTWIG_HOSTILE);
	GS_CHECKBOX (SLYLANDRO_KNOW_BROKEN);
	GS_CHECKBOX (PLAYER_KNOWS_PROBE);

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_NextColumn ();
		ImGui_BeginStyledChild ("##Column2", ZERO_F, CHILD_FLAGS, 0, NULL);
	}

	GS_CHECKBOX (PLAYER_KNOWS_PROGRAM);
	GS_CHECKBOX (PLAYER_KNOWS_EFFECTS);
	GS_CHECKBOX (PLAYER_KNOWS_PRIORITY);
	GS_CHECKBOX (SLYLANDRO_KNOW_EARTH);
	GS_CHECKBOX (SLYLANDRO_KNOW_EXPLORE);
	GS_CHECKBOX (SLYLANDRO_KNOW_GATHER);

	Spacer ();

	{
		int sly_multiplier = GET_CGAME_STATE (SLYLANDRO_MULTIPLIER);

		ImGui_Text ("SLYLANDRO_MULTIPLIER");
		ImGui_InputIntEx ("##SLYLANDRO_MULTIPLIER", &sly_multiplier, 0, 0, 0);
		if (ImGui_IsItemDeactivatedAfterEdit ()
			&& sly_multiplier < (COUNT)~0 && sly_multiplier > 0)
		{
			SET_CGAME_STATE (SLYLANDRO_MULTIPLIER, sly_multiplier);
		}
	}

	Spacer ();

	GS_CHECKBOX (KNOW_SPATHI_QUEST);
	GS_CHECKBOX (KNOW_SPATHI_EVIL);
	GS_CHECKBOX (PKUNK_DONE_WAR);
	GS_CHECKBOX (REFUSED_ULTRON_AT_BOMB);
	GS_CHECKBOX (NO_TRICK_AT_SUN);

	GS_Binary (KNOW_QS_PORTAL, 16);
	GS_Binary (KNOW_HOMEWORLD, 18);
	GS_Binary (HM_ENCOUNTERS, 9);

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_NextColumn ();
		ImGui_BeginStyledChild ("##Column3", ZERO_F, CHILD_FLAGS, 0, NULL);
	}

	if (DISPLAY_BOOL != 1)
	{
		ImGui_EndChild ();
		ImGui_Columns ();
	}

	ImGui_EndTabItem ();
}