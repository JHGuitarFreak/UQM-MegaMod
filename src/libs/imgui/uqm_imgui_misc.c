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

void draw_qol_menu (void)
{
	ImGui_ColumnsEx (DISPLAY_BOOL, "QoLColumns", false);

	ImGui_Checkbox ("Partial Pickup", (bool *)&optPartialPickup);
	ImGui_Checkbox ("Scatter Elements", (bool *)&optScatterElements);
	ImGui_Checkbox ("In-Game Help Menus", (bool *)&optSubmenu);
	ImGui_Checkbox ("Smart Auto-Pilot", (bool *)&optSmartAutoPilot);
	ImGui_Checkbox ("Advanced Auto-Pilot", (bool *)&optAdvancedAutoPilot);
	ImGui_Checkbox ("Show Visited Stars", (bool *)&optShowVisitedStars);
	ImGui_Checkbox ("Super Melee Ship Descriptions",
		(bool *)&optMeleeToolTips);
	ImGui_Checkbox ("Ship Storage Queue", (bool *)&optShipStore);
}

void draw_adv_menu (void)
{
	const char *difficulties[4] =
			{ "Original", "Easy", "Hard", "Choose At Start" };
	const char *nomad_modes[3] = { "Disabled", "Easy", "Normal" };
	const char *seed_modes[4] = { "Prime", "Planet", "MRQ", "Starseed"};

	UWORD activity = GLOBAL (CurrentActivity);

	ImGui_ColumnsEx (DISPLAY_BOOL, "AdvancedColumns", false);

	{
		ImGui_Text ("Difficulty:");
		if (ImGui_ComboChar ("##Difficulty", &optDiffChooser,
				difficulties, 4))
		{
			// Add switching code here
		}
	}

	Spacer ();

	{
		//bool is_extended = GLOBAL_SIS (Extended);
		if (ImGui_Checkbox ("Extended Lore", (bool *)&optExtended))
		{
			// Add switching code here
		}
	}

	Spacer ();

	{
		ImGui_Text ("Nomad Mode:");
		if (ImGui_ComboChar ("##Nomad", &optNomad, nomad_modes, 3))
		{
			// Add switching code here
		}
	}

	Spacer ();

	ImGui_Checkbox ("Slaughter Mode", (bool *)&optSlaughterMode);
	ImGui_Checkbox ("Fleet Point System", (bool *)&optFleetPointSys);

	ImGui_NewLine ();

	{
		ImGui_Text ("Starmap Seeding:");
		if (ImGui_ComboChar ("##Seeding", &optSeedType, seed_modes, 4))
		{
			// Add switching code here
		}
	}

	Spacer ();

	if (ImGui_Checkbox ("Ship Seeding", (bool *)&optShipSeed))
	{
		// Add switching code here
	}

	Spacer ();

	{
		int custom_seed = optCustomSeed;

		ImGui_Text ("Custom Seed:");
		ImGui_InputInt ("##CustomSeed", &custom_seed);
		if (ImGui_IsItemDeactivatedAfterEdit ()
				&& SANE_SEED (custom_seed))
		{
			// Add switching code here
		}
	}

	ImGui_NewLine ();
}