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

char *jrnl_buf = NULL;
char jrnl_name[15];
BOOLEAN jrnl_dirty = FALSE;

void draw_qol_menu (void)
{
	ImGui_BeginStyledChild ("##QoLColumn", content_col_size,
			CHILD_FLAGS, 0, NULL);
	{
		ImGui_SeparatorText ("Basic QoL Options");

		UQM_ImGui_CheckBox ("Skip Intro", &optSkipIntro, "mm.skipIntro", false);
		UQM_ImGui_CheckBox ("Partial Pickup", &optPartialPickup, "mm.partialPickup", false);
		UQM_ImGui_CheckBox ("Scatter Elements", &optScatterElements, "mm.scatterElements", false);
		UQM_ImGui_CheckBox ("In-Game Help Menus", &optSubmenu, "mm.submenu", false);
		UQM_ImGui_CheckBox ("Smart Auto-Pilot", &optSmartAutoPilot, "mm.smartAutoPilot", false);
		UQM_ImGui_CheckBox ("Advanced Auto-Pilot", &optAdvancedAutoPilot, "mm.advancedAutoPilot", false);
		UQM_ImGui_CheckBox ("Show Visited Stars", &optShowVisitedStars, "mm.showVisitedStars", false);
		UQM_ImGui_CheckBox ("Super Melee Ship Descriptions", &optMeleeToolTips, "mm.meleeToolTips", false);
		UQM_ImGui_CheckBox ("Ship Storage Queue", &optShipStore, "mm.shipStore", false);
	} ImGui_EndChild ();
}

void draw_adv_menu (void)
{
	const char *difficulties[] = { "Original", "Easy", "Hard" };
	const char *nomad_modes[] = { "Disabled", "Easy", "Normal" };
	const char *seed_modes[] = { "Prime", "Planet", "MRQ", "Starseed"};

	static bool risky_options = false;

	UWORD activity = GLOBAL (CurrentActivity);

	ImGui_BeginStyledChild ("##AdvancedOptsColumn", content_col_size,
			CHILD_FLAGS, 0, NULL);
	{
		// Risky Click
		if (!IN_MAIN_MENU)
		{
			ImGui_Text ("Let me modify risky options in-game:");
			ImGui_Checkbox ("##RiskyOptions", &risky_options);
			ImGui_TextWrappedColored (IV4_RED_COLOR,
				"WARNING! Modifying risky options in-game have the "
				"possibility of breaking the game or your game save!");
		}

		// Advanced Options
		{
			ImGui_SeparatorText ("Advanced Options");

			Spacer ();

			UQM_ImGui_CheckBox ("Slaughter Mode", &optSlaughterMode,
					"mm.slaughterMode", false);
			UQM_ImGui_CheckBox ("Fleet Point System", &optFleetPointSys,
					"mm.fleetPointSys", false);

			ImGui_NewLine ();
		}

		// Risky Options
		{
			ImGui_SeparatorText ("Risky Options");

			ImGui_BeginDisabled (!IN_MAIN_MENU && !risky_options);

			{
				ImGui_Text ("Difficulty:");
				if (ImGui_ComboChar ("##Difficulty", &optDifficulty,
						difficulties, 3))
				{
					optDiffChooser = optDifficulty;
					GLOBAL_SIS (Difficulty) = optDifficulty;
					res_PutInteger ("mm.difficulty", optDifficulty);
					mmcfg_changed = true;
				}
			}

			Spacer ();

			if (ImGui_Checkbox ("Extended Lore", (bool *)&optExtended))
			{
				GLOBAL_SIS (Extended) = optExtended;
				res_PutBoolean ("mm.extended", optExtended);
				mmcfg_changed = true;
			}

			Spacer ();

			{
				ImGui_Text ("Nomad Mode:");
				if (ImGui_ComboChar ("##Nomad", &optNomad, nomad_modes, 3))
				{
					GLOBAL_SIS (Nomad) = optNomad;
					res_PutInteger ("mm.nomad", optNomad);
					mmcfg_changed = true;
				}
			}

			ImGui_NewLine ();

			{
				ImGui_Text ("Starmap Seeding:");
				if (ImGui_ComboChar ("##Seeding", &optSeedType, seed_modes, 4))
				{
					SET_GAME_STATE (SEED_TYPE, optSeedType);
					res_PutInteger ("mm.seedType", optSeedType);
					mmcfg_changed = true;
				}
				if (!IN_MAIN_MENU)
				{
					ImGui_TextWrappedColored (IV4_RED_COLOR,
						"WARNING! When changing Seed Type in-game make "
						"sure to do so while in HyperSpace!");
					ImGui_TextWrappedColored (IV4_YELLOW_COLOR,
						"ADDENDUM: Switching to and from MRQ or StarSeed "
						"only works when changing in the Main Menu and "
						"starting a new game or if you change it then "
						"save and reload the game.");
				}
			}

			Spacer ();

			if (ImGui_Checkbox ("Ship Seeding", (bool *)&optShipSeed))
			{
				GLOBAL_SIS (ShipSeed) = (optShipSeed ? 1 : 0);
				res_PutBoolean ("mm.shipSeed", optShipSeed);
				mmcfg_changed = true;
			}
			if (!IN_MAIN_MENU)
			{
				ImGui_TextWrappedColored (IV4_RED_COLOR,
					"WARNING! Changing Ship Seed only works when changing "
					"in the Main Menu and starting a new game or if you "
					"change it then save and reload the game.");
			}

			Spacer ();

			{
				int custom_seed = optCustomSeed;

				ImGui_Text ("Custom Seed:");
				ImGui_InputInt ("##CustomSeed", &custom_seed);
				if (ImGui_IsItemDeactivatedAfterEdit ()
						&& SANE_SEED (custom_seed))
				{
					GLOBAL_SIS (Seed) = custom_seed;
					optCustomSeed = custom_seed;
					res_PutInteger ("mm.customSeed", custom_seed);
					mmcfg_changed = true;
				}
				if (!IN_MAIN_MENU)
				{
					ImGui_TextWrappedColored (IV4_RED_COLOR,
						"WARNING! When changing seed in-game make sure to do "
						"it while in HyperSpace!");
					ImGui_TextWrappedColored (IV4_YELLOW_COLOR,
						"ADDENDUM: Changing seeds with MRQ or StarSeed "
						"enabled only works when changing in the Main Menu "
						"and starting a new game or if you change it then "
						"save and reload the game.");
				}
			}
			ImGui_EndDisabled ();

			ImGui_NewLine ();
		}
	} ImGui_EndChild ();
}

void
FreeJournal (void)
{
	if (!jrnl_buf)
		return;

	HFree (jrnl_buf);
	jrnl_buf = NULL;
}

void
ResetJournal (void)
{
	FreeJournal ();

	jrnl_buf = HMalloc (FLOPPY_SIZE);
	if (jrnl_buf)
		jrnl_buf[0] = '\0';
	else
		log_add (log_Error, "Failed to allocate journal buffer");

	jrnl_name[0] = '\0';
}

void
draw_journal_menu (float height)
{
	char buf[96];
	ImVec2 iv2;

	if (!jrnl_buf)
		ResetJournal ();

	ImGui_BeginStyledChild ("##JournalCol", ZERO_F, CHILD_FLAGS, 0, NULL);
	{
		if (strlen (jrnl_name) < 14)
		{
			snprintf (buf, sizeof buf, "%s%s", "Not Saved/Loaded",
					jrnl_dirty ? " *" : "");
		}
		else
			snprintf (buf, sizeof buf, "%s%s", jrnl_name,
					jrnl_dirty ? " *" : "");

		ImGui_SeparatorText (buf);

		iv2 = ImGui_GetContentRegionAvail ();
		iv2.y = (float)WindowHeight - ImGui_GetCursorPosY () - height
				- (style->WindowPadding.y * 8); // I hate math this so much

		if (ImGui_InputTextMultilineEx ("##JournalText", jrnl_buf, FLOPPY_SIZE,
				iv2, ImGuiInputTextFlags_WordWrap |
				ImGuiInputTextFlags_AllowTabInput, NULL, NULL))
		{
			jrnl_dirty = true;
		}
	} ImGui_EndChild ();
}

void
SaveJournal (COUNT which_game)
{
	size_t len;

	if (!jrnl_buf)
		return;

	len = strlen (jrnl_buf);

	snprintf (jrnl_name, sizeof jrnl_name, "uqmsave.%02u.txt", which_game);

	if (len > 0)
	{
		uio_Stream *fp = uio_fopen (saveDir, jrnl_name, "w");
		if (fp)
		{
			uio_fwrite (jrnl_buf, 1, len, fp);
			uio_fclose (fp);
			jrnl_dirty = FALSE;
		}
	}
	else
		log_add (log_Debug, "No journal data to save to %s", jrnl_name);
}

void
LoadJournal (COUNT which_game)
{
	uio_Stream *fp;

	ResetJournal ();

	jrnl_dirty = FALSE;

	snprintf (jrnl_name, sizeof jrnl_name, "uqmsave.%02u.txt", which_game);
	fp = uio_fopen (saveDir, jrnl_name, "r");
	if (fp)
	{
		long file_size;
		uio_fseek (fp, 0, SEEK_END);
		file_size = uio_ftell (fp);
		uio_fseek (fp, 0, SEEK_SET);

		if (file_size > 0 && file_size < FLOPPY_SIZE)
		{
			size_t bytes_read = uio_fread (jrnl_buf, 1, file_size, fp);
			jrnl_buf[bytes_read] = '\0';
		}
		else
		{
			log_add (log_Error, "%s filesize error: %d",
				jrnl_name, file_size);
		}
		uio_fclose (fp);
	}
	else
		log_add (log_Error, "Couldn't open %s for reading", jrnl_name);
}