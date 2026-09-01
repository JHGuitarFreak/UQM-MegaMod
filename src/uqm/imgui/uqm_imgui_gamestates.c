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

#define TABLE_FLAGS ImGuiTableFlags_SizingStretchSame | \
					ImGuiTableFlags_PadOuterX


static void GamestatesTab00 (void);
static void GamestatesTab01 (void);

void
draw_gamestates_menu (void)
{
	if (ImGui_BeginTabBar ("GameStateTabs", 0))
	{
		GamestatesTab00 ();
		GamestatesTab01 ();

		ImGui_EndTabBar ();
	}

}

static void
GamestatesTab00 (void)
{
	ImVec2 column_size;

	static const char **usm_states = NULL;
	static const char **zfpd_states = NULL;
	static const char **tm_states = NULL;
	static const char **tc_states = NULL;
	static const char **pkmis_states = NULL;
	static const char **cb_states = NULL;
	static const char **om_states = NULL;
	static const char **nan_states = NULL;
	static const char **lac_states = NULL;
	static const char **sm_states = NULL;
	static const char **fps_states = NULL;
	static char tab_page[32];

	if (!usm_states)
	{
		fps_states   = ImStrArr (DBG_GMS_STR_BASE);      // FOUND_PLUTO_SPATHI
		sm_states    = ImStrArr (DBG_GMS_STR_BASE +  1); // SPATHI_MANNER
		lac_states   = ImStrArr (DBG_GMS_STR_BASE +  2); // LIED_ABOUT_CREATURES
		nan_states   = ImStrArr (DBG_GMS_STR_BASE +  3); // NEW_ALLIANCE_NAME
		om_states    = ImStrArr (DBG_GMS_STR_BASE +  4); // ORZ_MANNER
		cb_states    = ImStrArr (DBG_GMS_STR_BASE +  5); // CHMMR_BOMB_STATE
		pkmis_states = ImStrArr (DBG_GMS_STR_BASE +  6); // PKUNK_MISSION
		tc_states    = ImStrArr (DBG_GMS_STR_BASE +  7); // THRADD_CULTURE
		tm_states    = ImStrArr (DBG_GMS_STR_BASE +  8); // THRADD_MISSION
		zfpd_states  = ImStrArr (DBG_GMS_STR_BASE +  9); // ZOQFOT_DISTRESS
		usm_states   = ImStrArr (DBG_GMS_STR_BASE + 10); // UTWIG_SUPOX_MISSION

		snprintf (tab_page, sizeof tab_page, ImStr (DBG_GMS_STR_BASE + 14), 1);
											// Page 1
	}

	if (!ImGui_BeginTabItem (tab_page, NULL, 0))
		return;

	GetColumnSize (&column_size, NUM_COLUMNS);

	Spacer ();

	ImGui_BeginStyledChild ("##Column1", column_size, CHILD_FLAGS, 0, NULL);
	{
		GS_CHECKBOX (SHOFIXTI_KIA);
		GS_CHECKBOX (SHOFIXTI_BRO_KIA);
		GS_CHECKBOX (SHOFIXTI_RECRUITED);

		Spacer ();

		{
			int found_pluto_spathi = GET_CGAME_STATE (FOUND_PLUTO_SPATHI);

			ImGui_Text ("FOUND_PLUTO_SPATHI");
			if (ImGui_ComboChar ("##FOUND_PLUTO_SPATHI", &found_pluto_spathi,
					fps_states, 4))
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

			ImGui_Text ("SPATHI_MANNER");
			if (ImGui_ComboChar ("##SPATHI_MANNER", &spathi_manner,
					sm_states, 4))
			{
				SET_CGAME_STATE (SPATHI_MANNER, spathi_manner);
			}
		}

		Spacer ();

		{
			int lied_about_creatures = GET_CGAME_STATE (LIED_ABOUT_CREATURES);

			ImGui_Text ("LIED_ABOUT_CREATURES");
			if (ImGui_ComboChar ("##LIED_ABOUT_CREATURES",
					&lied_about_creatures, lac_states, 3))
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

		Spacer ();
	}

	if (NUM_COLUMNS > 2)
	{
		ImGui_EndChild (); // ##Column1
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2", column_size,
				CHILD_FLAGS, 0, NULL);
	}

	{

		GS_CHECKBOX (KOHR_AH_KILLED_ALL);
		GS_CHECKBOX (MET_ARILOU);
		GS_CHECKBOX (ATTACKED_DRUUGE);

		Spacer ();

		{
			int new_alliance_name = GET_CGAME_STATE (NEW_ALLIANCE_NAME);
			static char empire_of[PATH_MAX];
			const char *nan_copy[4] =
			{
					nan_states[0],
					nan_states[1],
					nan_states[2],
					empire_of
			};

			snprintf (empire_of, sizeof (empire_of), nan_states[3],
					GLOBAL_SIS (CommanderName));

			ImGui_Text ("NEW_ALLIANCE_NAME");
			if (ImGui_ComboChar ("##NEW_ALLIANCE_NAME", &new_alliance_name,
					nan_copy, 4))
			{
				SET_CGAME_STATE (NEW_ALLIANCE_NAME, new_alliance_name);
			}
		}

		Spacer ();

		{
			bool game_state =
					GET_CGAME_STATE (PORTAL_COUNTER) == 1 ? true : false;

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

			ImGui_Text ("ORZ_MANNER");
			if (ImGui_ComboChar ("##ORZ_MANNER", &orz_manner,
					om_states, 4))
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
	}

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild (); // ##Column2
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column3", column_size, CHILD_FLAGS, 0, NULL);
	}

	{
		{
			int chmmr_bomb_state = GET_CGAME_STATE (CHMMR_BOMB_STATE);

			ImGui_Text ("CHMMR_BOMB_STATE");
			if (ImGui_ComboChar ("##CHMMR_BOMB_STATE", &chmmr_bomb_state,
					cb_states, 4))
			{
				SET_CGAME_STATE (CHMMR_BOMB_STATE, chmmr_bomb_state);
			}
		}

		Spacer ();

		GS_CHECKBOX (DRUUGE_DISCLAIMER);
		GS_CHECKBOX (YEHAT_CIVIL_WAR);
		GS_CHECKBOX (YEHAT_ABSORBED_PKUNK);

		Spacer ();

		{
			int pkunk_mission = GET_CGAME_STATE (PKUNK_MISSION);

			ImGui_Text ("PKUNK_MISSION");
			if (ImGui_ComboChar ("##PKUNK_MISSION", &pkunk_mission,
					pkmis_states, 6))
			{
				SET_CGAME_STATE (PKUNK_MISSION, pkunk_mission);
			}
			if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
			{
				ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 12));
								// PKUNK_MISSION Tooltip
			}
		}

		Spacer ();

		{
			int thradd_culture = GET_CGAME_STATE (THRADD_CULTURE);
			static char empire_of[PATH_MAX];
			const char *tc_copy[] =
			{
					tc_states[0],
					tc_states[1],
					tc_states[2],
					empire_of
			};

			snprintf (empire_of, sizeof (empire_of), tc_states[3],
					GLOBAL_SIS (CommanderName));

			ImGui_Text ("THRADD_CULTURE");
			if (ImGui_ComboChar ("##THRADD_CULTURE", &thradd_culture,
					tc_copy, 4))
			{
				SET_CGAME_STATE (THRADD_CULTURE, thradd_culture);
			}
		}

		Spacer ();

		{
			int thradd_mission = GET_CGAME_STATE (THRADD_MISSION);

			ImGui_Text ("THRADD_MISSION");
			if (ImGui_ComboChar ("##THRADD_MISSION", &thradd_mission,
					tm_states, 5))
			{
				SET_CGAME_STATE (THRADD_MISSION, thradd_mission);
			}
			if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
			{
				ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 13));
								// THRADD_MISSION Tooltip
			}
		}

		Spacer ();

		GS_CHECKBOX (ZOQFOT_HOSTILE);
		GS_CHECKBOX (MET_ZOQFOT);

		Spacer ();

		{
			int zfp_distress = GET_CGAME_STATE (ZOQFOT_DISTRESS);

			ImGui_Text ("ZOQFOT_DISTRESS");
			if (ImGui_ComboChar ("##ZOQFOT_DISTRESS", &zfp_distress,
					zfpd_states, 3))
			{
				SET_CGAME_STATE (ZOQFOT_DISTRESS, zfp_distress);
			}
			if (ImGui_IsItemHovered (ImGuiHoveredFlags_DelayNone))
			{
				ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 14));
								// ZOQFOT_DISTRESS Tooltip
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

			ImGui_Text ("UTWIG_SUPOX_MISSION");
			if (ImGui_ComboChar ("##UTWIG_SUPOX_MISSION", &utwig_supox_mission,
					usm_states, 6))
			{
				SET_CGAME_STATE (UTWIG_SUPOX_MISSION, utwig_supox_mission);
			}
		}
	}
	
	ImGui_EndChild (); // ##Column3

	ImGui_EndTabItem ();
}

static void
GamestatesTab01 (void)
{
	ImVec2 column_size;

	static const char **am_states    = NULL;
	static const char **pkman_states = NULL;
	static const char **kas_states   = NULL;
	static char tab_page[32];

	if (!am_states)
	{
		kas_states   = ImStrArr (DBG_GMS_STR_BASE + 11); // KNOW_ABOUT_SHATTERED
		pkman_states = ImStrArr (DBG_GMS_STR_BASE + 12); // PKUNK_MANNER
		am_states    = ImStrArr (DBG_GMS_STR_BASE + 13); // ARILOU_MANNER

		snprintf (tab_page, sizeof tab_page, ImStr (DBG_GMS_STR_BASE + 14), 2);
											// Page 2
	}

	if (!ImGui_BeginTabItem (tab_page, NULL, 0))
		return;

	GetColumnSize (&column_size, NUM_COLUMNS);

	Spacer ();

	ImGui_BeginStyledChild ("##Column1", column_size, CHILD_FLAGS, 0, NULL);
	{

		GS_CHECKBOX (ILWRATH_FIGHT_THRADDASH);
		GS_CHECKBOX (READY_TO_CONFUSE_URQUAN);
		GS_CHECKBOX (URQUAN_HYPNO_VISITS);
		GS_CHECKBOX (MENTIONED_PET_COMPULSION);

		Spacer ();

		{
			int know_about_shattered = GET_CGAME_STATE (KNOW_ABOUT_SHATTERED);

			ImGui_Text ("KNOW_ABOUT_SHATTERED");
			if (ImGui_ComboChar ("##KNOW_ABOUT_SHATTERED",
					&know_about_shattered, kas_states, 4))
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

			ImGui_Text ("PKUNK_MANNER");
			if (ImGui_ComboChar ("##PKUNK_MANNER", &pkunk_manner,
						pkman_states, 4))
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
				ImGui_SetTooltip (ImStr (TIP_WARN_STR_BASE + 15));
								// ZOQFOT_DISTRESS Tooltip
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

			ImGui_Text ("ARILOU_MANNER");
			if (ImGui_ComboChar ("##ARILOU_MANNER", &arilou_manner,
					am_states, 4))
			{
				SET_CGAME_STATE (ARILOU_MANNER, arilou_manner);
			}
		}

		Spacer ();

		GS_CHECKBOX (DRUUGE_MANNER);
	}

	if (NUM_COLUMNS > 2)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column2", column_size, CHILD_FLAGS, 0, NULL);
	}

	{
		GS_CHECKBOX (SUPOX_HOSTILE);
		GS_CHECKBOX (UTWIG_HOSTILE);
		GS_CHECKBOX (SLYLANDRO_KNOW_BROKEN);
		GS_CHECKBOX (PLAYER_KNOWS_PROBE);
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
			ImGui_InputIntEx ("##SLYLANDRO_MULTIPLIER",
					&sly_multiplier, 0, 0, 0);
			if (ImGui_IsItemDeactivatedAfterEdit ()
					&& sly_multiplier < (COUNT)~0 && sly_multiplier >= 0)
			{
				SET_CGAME_STATE (SLYLANDRO_MULTIPLIER, sly_multiplier);
			}
		}

		Spacer ();

		GS_CHECKBOX (KNOW_SPATHI_QUEST);
		GS_CHECKBOX (KNOW_SPATHI_EVIL);
		GS_CHECKBOX (PKUNK_DONE_WAR);
		GS_CHECKBOX (REFUSED_ULTRON_AT_BOMB);

		Spacer ();

		GS_Binary (KNOW_QS_PORTAL, 16);

		Spacer ();

		GS_Binary (KNOW_HOMEWORLD, 18);

		Spacer ();
	}

	if (NUM_COLUMNS != 1)
	{
		ImGui_EndChild ();
		ImGui_SameLine ();
		ImGui_BeginStyledChild ("##Column3", column_size,
				CHILD_FLAGS, 0, NULL);
	}

	{
		GS_CHECKBOX (NO_TRICK_AT_SUN);

		Spacer ();

		GS_Binary (HM_ENCOUNTERS, 9);

		Spacer ();

		{
			int gamestate = GET_CGAME_STATE (MELNORME_RAINBOW_COUNT);

			ImGui_Text ("MELNORME_RAINBOW_COUNT");
			ImGui_InputIntEx ("##MELNORME_RAINBOW_COUNT", &gamestate, 0, 0, 0);
			if (ImGui_IsItemDeactivatedAfterEdit () && gamestate < 11)
			{
				SET_CGAME_STATE (MELNORME_RAINBOW_COUNT, gamestate);
			}
		}

		Spacer ();

#define MAX_RBW 10
		{
			int i, j;
			char buf[40];
			static bool selected[MAX_RBW];
			int cur_selected = 0;
			ImVec2 align = { 0.7f, 0.1f };
			int rbw_count = GET_CGAME_STATE (MELNORME_RAINBOW_COUNT);
			int bitmask = MAKE_WORD (
				GET_CGAME_STATE (RAINBOW_WORLD0),
				GET_CGAME_STATE (RAINBOW_WORLD1));

			ImGui_BeginChild ("##RAINBOW_WORLD", ZERO_F, CARD_FLAGS,
					ImGuiWindowFlags_MenuBar);
			{
				if (ImGui_BeginMenuBar ())
				{
					ImGui_MenuItemEx ("RAINBOW_WORLD", NULL, false, false);
					ImGui_EndMenuBar ();
				}

				ImGui_PushStyleVar (ImGuiStyleVar_SelectablesRounding, 0.0f);
				ImGui_PushStyleColorImVec4 (ImGuiCol_BorderShadow,
						MAKE_IV4 (0, 0, 0, 0));
				if (ImGui_BeginTable ("##RainbowTable", 8,
						ImGuiTableFlags_Borders |
						ImGuiTableFlags_NoHostExtendX))
				{
					for (i = 0; i < MAX_RBW; i++)
					{
						ImGui_TableNextColumn ();

						selected[i] = bitmask & (1 << i);
						snprintf (buf, sizeof buf, "%d##%s%d", selected[i],
								"RBW", i);

						if (ImGui_SelectableEx (buf,
							selected[i], ImGuiSelectableFlags_None,
							MAKE_IV2 (SCALE_IT (15.0f), SCALE_IT (19.0f))))
						{
							for (j = 0; j < MAX_RBW; j++)
							{
								if (bitmask & (1 << j))
									cur_selected++;
							}

							bitmask ^= (1 << i);

							SET_CGAME_STATE (RAINBOW_WORLD0, LOBYTE (bitmask));
							SET_CGAME_STATE (RAINBOW_WORLD1, HIBYTE (bitmask));

							if (selected[i] && cur_selected == rbw_count &&
								rbw_count >= 0)
							{
								rbw_count--;
								SET_CGAME_STATE (MELNORME_RAINBOW_COUNT,
										rbw_count);
							}
						}
					}

					ImGui_EndTable ();
				}
				ImGui_PopStyleColor ();
				ImGui_PopStyleVar ();

				ImGui_PushID ("RAINBOW_WORLD");

				if (ImGui_Button (ImStr (DBG_GMS_STR_BASE + 15)))
				{
					bitmask = ~0;
					SET_CGAME_STATE (RAINBOW_WORLD0, LOBYTE (bitmask));
					SET_CGAME_STATE (RAINBOW_WORLD1, HIBYTE (bitmask));
				}
				ImGui_SameLine ();

				if (ImGui_Button (ImStr (DBG_GMS_STR_BASE + 16)))
				{
					bitmask = 0;
					SET_CGAME_STATE (RAINBOW_WORLD0, LOBYTE (bitmask));
					SET_CGAME_STATE (RAINBOW_WORLD1, HIBYTE (bitmask));
				}

				ImGui_PopID ();
			}ImGui_EndChild ();

			DrawBorderAroundLastItem ();
		}
	}

	ImGui_EndChild ();

	ImGui_EndTabItem ();
}