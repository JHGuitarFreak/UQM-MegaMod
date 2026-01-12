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

#include "libs/inplib.h"
#include "libs/input/sdl/vcontrol.h"
#include "libs/input/sdl/keynames.h"
#include "libs/input/input_common.h"

#include <ctype.h>

#define BB_WIDTH MAKE_IV2 (120, 0)

typedef struct
{
	BOOLEAN menu;
	BOOLEAN flight;
} DIRTY_BINDINGS;

static DIRTY_BINDINGS bindings_dirty = { FALSE, FALSE };

REBIND_STATE rebind_state = { 0 };
static MENU_BINDINGS saved_menu_bindings[NUM_MENU_KEYS];
static FLIGHT_BINDINGS saved_flight_bindings[6][NUM_KEYS];
static char saved_template_names[6][30];
static BOOLEAN binds_backed_up = FALSE;

static int template_id = 0;

// ImGui Menu Functions
static void Control_Tabs (void);
static void FlightControlsTab (void);
static void FlightControls (void);
static void MenuControlsTab (void);
static void MenuControls (void);
static void ShowMenuRebindPopup (void);
static void ShowFlightRebindPopup (void);

// Backend Functions
static void StartRebinding (int action, int binding, int *template_id);
static void SaveMenuBindings (void);
static void LoadDefaultMenuKeys (void);
static void RestoreMenuBindings (void);
static void LoadDefaultFlightKeys (void);
static void RestoreFlightBindings (void);
static const char *GetBindingDisplayText (VCONTROL_GESTURE *gesture);
static void BackupCurrentBindings (void);

static const char *pretty_menu_actions[] =
{
	"Pause", "Exit", "Abort", "Debug 1", "Fullscreen", "Up", "Down",
	"Left", "Right", "Select", "Cancel", "Special", "Page-Up",
	"Page-Down", "Home", "End", "Zoom-In", "Zoom-Out", "Delete",
	"Backspace", "Cancel Edit", "Star Search", "Tab / Next",
	"Toggle StarMaps", "Screenshot", "ImGui Toggle", "Debug 2",
	"Debug 3", "Debug 4", NULL
};

static const char *pretty_flight_actions[] =
{
	"Up", "Down", "Left", "Right", "Weapon", "Special", "Escape",
	"Thrust", NULL
};

// ImGui Menu

void
draw_controls_menu (void)
{
	int i;
	const char *control_display[] = { "Keyboard", "Xbox", "PlayStation" };
	const char *player_controls[6];

	if (!binds_backed_up)
		BackupCurrentBindings ();

	ImGui_ColumnsEx (DISPLAY_BOOL, "ControlsColumns", false);

	// Control Options
	{
		ImGui_SeparatorText ("Control Options");

		ImGui_Text ("Control Display:");
		if (ImGui_ComboChar ("##ControlDisplay",
				(int *)&optControllerType, control_display, 3))
		{
			res_PutInteger ("mm.controllerType", optControllerType);
			mmcfg_changed = true;
		}

		Spacer ();

		for (i = 0; i < 6; i++)
			player_controls[i] = input_templates[i].name;

		ImGui_Text ("Player 1:");
		if (ImGui_ComboChar ("##BottomPlayer",
				(int *)&PlayerControls[0], player_controls, 6))
		{
			res_PutInteger ("config.player1control", PlayerControls[0]);
			config_changed = true;
		}

		ImGui_Text ("Player 2:");
		if (ImGui_ComboChar ("##TopPlayer",
				(int *)&PlayerControls[1], player_controls, 6))
		{
			res_PutInteger ("config.player2control", PlayerControls[1]);
			config_changed = true;
		}

		ImGui_NewLine ();
	}

	ImGui_Columns ();

	Control_Tabs ();
}

static void
Control_Tabs (void)
{
	ImGui_SeparatorText ("Edit Controls");

	if (ImGui_BeginTabBar ("ControlTabs", 0))
	{
		FlightControlsTab ();
		MenuControlsTab ();

		ImGui_EndTabBar ();
	}
}

static void
FlightControlsTab (void)
{
	if (!ImGui_BeginTabItem ("Flight", NULL, 0))
		return;

	Spacer ();

	if (ImGui_Button ("Load Defaults"))
	{
		LoadDefaultFlightKeys ();
		bindings_dirty.flight = TRUE;
	}

	if (bindings_dirty.flight)
	{
		ImGui_SameLine ();
		if (ImGui_Button ("Save Changes"))
		{
			SaveKeyConfiguration (configDir, "flight.cfg");
			bindings_dirty.flight = FALSE;
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Cancel"))
		{
			RestoreFlightBindings ();
			bindings_dirty.flight = FALSE;
		}
	}

	ShowFlightRebindPopup ();

	Spacer ();

	FlightControls ();

	ImGui_EndTabItem ();
}

static void
FlightControls (void)
{
	int i, j;
	static char *control_template[6];
	char template_name[30];
	char button_id[32];
	VCONTROL_GESTURE *g;

	ImGui_BeginChild ("FlightBindings", ZERO_F, 0, 0);
	ImGui_ColumnsEx (2, "FlightTemplates", false);
	ImGui_SetColumnWidth (0, 300.0f);

	for (i = 0; i < 6; i++)
		control_template[i] = input_templates[i].name;

	ImGui_BeginDisabled (bindings_dirty.flight);
	ImGui_Text ("Template:");
	ImGui_ComboChar ("##InputTemplate", &template_id, control_template, 6);
	ImGui_EndDisabled ();

	snprintf (template_name, sizeof (template_name), "%s",
			input_templates[template_id].name);

	ImGui_Text ("Template Name:");
	ImGui_InputText ("##TemplateName", template_name,
			sizeof (template_name), 0);
	if (ImGui_IsItemDeactivatedAfterEdit ())
	{
		char key[40];

		snprintf (input_templates[template_id].name,
				sizeof (input_templates[template_id].name), "%s",
				template_name);

		bindings_dirty.flight = TRUE;
		snprintf (key, sizeof (key), "keys.%d.name", template_id + 1);
		res_PutString (key, template_name);
	}

	ImGui_Columns ();

	ImGui_NewLine ();

	ImGui_ColumnsEx (2, "FlightControlBindings", false);
	ImGui_SetColumnWidth (0, 150.0f);

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (flight_res_names[i] == NULL)
			continue;

		Spacer ();

		ImGui_Text ("%s:", pretty_flight_actions[i]);

		ImGui_NextColumn ();

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			g = &curr_fl_bindings[template_id][i].binding[j];

			snprintf (button_id, sizeof (button_id),
					"##bind_fl_%d_%d", i, j);

			ImGui_PushID (button_id);

			if (ImGui_ButtonEx (GetBindingDisplayText (g), BB_WIDTH))
			{
				StartRebinding (i, j, &template_id);
			}

			ImGui_PopID ();

			if (j < 1)
				ImGui_SameLine ();
		}

		ImGui_NextColumn ();
	}

	ImGui_EndChild ();
}

static void
ShowFlightRebindPopup (void)
{
	char popup_title[128];
	char keybuf[40], valbuf[40];
	REBIND_STATE *RSPtr = &rebind_state;

	if (RSPtr->active && RSPtr->template_id != -1)
	{
		ImGui_OpenPopup ("##FlightRebindPopup", 0);
	}

	ImGui_SetNextWindowPosEx (Float2Mult (DISPLAY_SIZE, 0.5f),
			ImGuiCond_Always, CENTER_IT);

	if (!ImGui_BeginPopupModal ("##FlightRebindPopup", NULL,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	if (RSPtr->template_id < 0 || RSPtr->template_id > 5)
	{
		ImGui_TextColored (IV4_RED_COLOR, "Error: Invalid template ID");

		if (ImGui_Button ("Cancel"))
		{
			RSPtr->active = FALSE;
			RSPtr->new_g.type = VCONTROL_NONE;
			RSPtr->has_error = FALSE;
			ImGui_CloseCurrentPopup ();
		}

		ImGui_EndPopup ();
		return;
	}

	snprintf (popup_title, sizeof (popup_title),
			"Template: %s | Control: %s | Binding %d of 2",
			input_templates[RSPtr->template_id].name,
			pretty_flight_actions[RSPtr->action], RSPtr->binding + 1);

	ImGui_Text ("%s", popup_title);

	Spacer ();

	ImGui_HorizontalSeparator ("##FlightRebindTop");

	Spacer ();

	if (RSPtr->has_error && RSPtr->error_message[0] != '\0')
	{
		ImGui_TextColored (DangerGradient (), "Illegal bind detected!");
		ImGui_NewLine ();

		ImGui_TextColored (IV4_RED_COLOR, "%s", RSPtr->error_message);
		ImGui_NewLine ();
	}

	if (RSPtr->new_g.type == VCONTROL_NONE && !RSPtr->has_error)
	{
		ImGui_TextColored (IV4_YELLOW_COLOR, "Waiting for input...");
		ImGui_Text ("Press any key, button, or move an axis");
	}
	else if (RSPtr->has_error)
	{
		ImGui_TextColored (IV4_YELLOW_COLOR,
				"Please choose a different key, button, or axis...");
	}

	Spacer ();

	ImGui_HorizontalSeparator ("##FlightRebindBottom");

	Spacer ();

	if (ImGui_Button ("Clear"))
	{
		curr_fl_bindings[RSPtr->template_id][RSPtr->action].
				binding[RSPtr->binding].type = VCONTROL_NONE;

		if (RSPtr->old_g.type != VCONTROL_NONE)
		{
			int *target = (int *)(flight_vec + RSPtr->template_id
					* num_flight + RSPtr->action);
			VControl_RemoveGestureBinding (&RSPtr->old_g, target);
		}

		snprintf (keybuf, sizeof (keybuf), "keys.%d.%s.%d",
				RSPtr->template_id + 1, flight_res_names[RSPtr->action],
				RSPtr->binding + 1);
		VControl_DumpGesture (valbuf, sizeof valbuf, &RSPtr->new_g);
		res_PutString (keybuf, valbuf);

		bindings_dirty.flight = TRUE;

		RSPtr->active = FALSE;
		RSPtr->new_g.type = VCONTROL_NONE;
		RSPtr->has_error = FALSE;
		RSPtr->conflict_action[0] = '\0';
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (ImGui_Button ("Cancel"))
	{
		if (RSPtr->old_g.type != VCONTROL_NONE)
		{
			int *target = (int *)(flight_vec + RSPtr->template_id
					* num_flight + RSPtr->action);

			curr_fl_bindings[RSPtr->template_id][RSPtr->action].
				binding[RSPtr->binding] = RSPtr->old_g;

			VControl_AddGestureBinding (&RSPtr->old_g, target);
		}

		RSPtr->active = FALSE;
		RSPtr->new_g.type = VCONTROL_NONE;
		RSPtr->has_error = FALSE;
		RSPtr->conflict_action[0] = '\0';
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (RSPtr->new_g.type != VCONTROL_NONE
		&& !RSPtr->has_error)
	{
		int *target = (int *)(flight_vec + RSPtr->template_id
				* num_flight + RSPtr->action);

		curr_fl_bindings[RSPtr->template_id][RSPtr->action].
				binding[RSPtr->binding] = RSPtr->new_g;

		if (RSPtr->old_g.type != VCONTROL_NONE)
			VControl_RemoveGestureBinding (&RSPtr->old_g, target);

		VControl_AddGestureBinding (&RSPtr->new_g, target);

		snprintf (keybuf, sizeof (keybuf), "keys.%d.%s.%d",
				RSPtr->template_id + 1, flight_res_names[RSPtr->action],
				RSPtr->binding + 1);
		VControl_DumpGesture (valbuf, sizeof valbuf, &RSPtr->new_g);
		res_PutString (keybuf, valbuf);

		bindings_dirty.flight = TRUE;

		log_add (log_Debug,
				"Applied binding for template %d, %s (binding %d): %s",
				RSPtr->template_id + 1, flight_res_names[RSPtr->action],
				RSPtr->binding + 1, GetBindingDisplayText (&RSPtr->new_g));

		RSPtr->active = FALSE;
		RSPtr->new_g.type = VCONTROL_NONE;
		RSPtr->has_error = FALSE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_EndPopup ();
}

static void
MenuControlsTab (void)
{
	if (!ImGui_BeginTabItem ("Menu", NULL, 0))
		return;

	Spacer ();

	if (ImGui_Button ("Load Defaults"))
	{
		LoadDefaultMenuKeys ();
		bindings_dirty.menu = TRUE;
	}

	if (bindings_dirty.menu)
	{
		ImGui_SameLine ();
		if (ImGui_Button ("Save Changes"))
		{
			SaveMenuBindings ();
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Cancel"))
		{
			RestoreMenuBindings ();
			bindings_dirty.menu = FALSE;
		}
	}

	ShowMenuRebindPopup ();

	Spacer ();

	MenuControls ();

	ImGui_EndTabItem ();
}

static void
MenuControls (void)
{
	int i, j;
	char button_id[32];
	VCONTROL_GESTURE *g;

	ImGui_BeginChild ("MenuBindings", ZERO_F, 0, 0);

	ImGui_ColumnsEx (2, "MenuControlBindings", false);
	ImGui_SetColumnWidth (0, 150.0f);

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		ImGui_Text ("%s:", pretty_menu_actions[i]);

		ImGui_NextColumn ();

		for (j = 0; j < 6; j++)
		{
			g = &curr_bindings[i].binding[j];

			snprintf (button_id, sizeof (button_id), "##bind_%d_%d", i, j);

			ImGui_PushID (button_id);

			if (ImGui_ButtonEx (GetBindingDisplayText (g), BB_WIDTH))
				StartRebinding (i, j, NULL);

			ImGui_PopID ();

			if (j < 5)
				ImGui_SameLine ();
		}

		ImGui_NextColumn ();
	}

	ImGui_EndChild ();
}

static void
ShowMenuRebindPopup (void)
{
	VCONTROL_GESTURE new_g;
	char popup_title[128];
	REBIND_STATE *RSPtr = &rebind_state;

	if (!RSPtr->active)
		return;

#define WINDOW_FLAGS 

	ImGui_OpenPopup ("##RebindPopup", 0);

	ImGui_SetNextWindowPosEx (Float2Mult (DISPLAY_SIZE, 0.5f),
			ImGuiCond_Always, CENTER_IT);

	if (!ImGui_BeginPopupModal ("##RebindPopup", NULL,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	snprintf (popup_title, sizeof (popup_title),
			"Action Name: %s | Binding %d of 6",
			pretty_menu_actions[RSPtr->action], RSPtr->binding + 1);

	ImGui_Text ("%s", popup_title);

	Spacer ();

	ImGui_HorizontalSeparator ("##MenuRebindTop");

	Spacer ();

	if (RSPtr->has_error && RSPtr->error_message[0] != '\0')
	{
		ImGui_TextColored (DangerGradient (), "Illegal bind detected!");
		ImGui_NewLine ();

		ImGui_TextColored (IV4_RED_COLOR, "%s", RSPtr->error_message);
		ImGui_NewLine ();
	}

	if (VControl_GetLastGesture (&new_g))
	{
		memcpy (&RSPtr->new_g, &new_g, sizeof (VCONTROL_GESTURE));
		VControl_ClearGesture ();
	}

	if (RSPtr->new_g.type == VCONTROL_NONE && !RSPtr->has_error)
	{
		ImGui_TextColored (IV4_YELLOW_COLOR, "Waiting for input...");
		ImGui_Text ("Press any key, button, or move an axis");
	}
	else if (RSPtr->has_error)
	{
		ImGui_TextColored (IV4_YELLOW_COLOR,
				"Please choose a different key/button...");
	}

	Spacer ();

	ImGui_HorizontalSeparator ("##MenuRebindBottom");

	Spacer ();

	if (ImGui_Button ("Clear"))
	{
		curr_bindings[RSPtr->action].binding[RSPtr->binding].type =
				VCONTROL_NONE;

		if (RSPtr->old_g.type != VCONTROL_NONE)
		{
			VControl_RemoveGestureBinding (&RSPtr->old_g,
					(int *)&menu_vec[RSPtr->action]);
		}

		bindings_dirty.menu = TRUE;

		RSPtr->active = FALSE;
		RSPtr->new_g.type = VCONTROL_NONE;
		RSPtr->has_error = FALSE;
		RSPtr->conflict_action[0] = '\0';
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (ImGui_Button ("Cancel"))
	{
		if (RSPtr->old_g.type != VCONTROL_NONE)
		{
			curr_bindings[RSPtr->action].binding[RSPtr->binding] =
					RSPtr->old_g;

			VControl_AddGestureBinding (&RSPtr->old_g,
					(int *)&menu_vec[RSPtr->action]);
		}

		RSPtr->active = FALSE;
		RSPtr->new_g.type = VCONTROL_NONE;
		RSPtr->has_error = FALSE;
		RSPtr->conflict_action[0] = '\0';
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (RSPtr->new_g.type != VCONTROL_NONE && !RSPtr->has_error)
	{
		curr_bindings[RSPtr->action].binding[RSPtr->binding] = RSPtr->new_g;

		if (RSPtr->old_g.type != VCONTROL_NONE)
		{
			VControl_RemoveGestureBinding (&RSPtr->old_g,
					(int *)&menu_vec[RSPtr->action]);
		}

		VControl_AddGestureBinding (&RSPtr->new_g,
				(int *)&menu_vec[RSPtr->action]);

		bindings_dirty.menu = TRUE;

		log_add (log_Debug,
				"Applied new binding for %s (binding %d): %s",
				menu_res_names[RSPtr->action], RSPtr->binding + 1,
				GetBindingDisplayText (&RSPtr->new_g));

		RSPtr->active = FALSE;
		RSPtr->new_g.type = VCONTROL_NONE;
		RSPtr->has_error = FALSE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_EndPopup ();
}

// Backend

static void
StartRebinding (int action, int binding, int *template_id)
{
	REBIND_STATE *RSPtr = &rebind_state;

	RSPtr->active = TRUE;
	RSPtr->action = action;
	RSPtr->binding = binding;
	RSPtr->has_error = FALSE;
	RSPtr->error_message[0] = '\0';

	if (template_id != NULL)
	{
		RSPtr->template_id = *template_id;
		RSPtr->old_g =
				curr_fl_bindings[*template_id][action].binding[binding];
	}
	else
	{
		RSPtr->template_id = -1;
		RSPtr->old_g = curr_bindings[action].binding[binding];
	}

	RSPtr->new_g.type = VCONTROL_NONE;
	RSPtr->show_popup = FALSE;

	VControl_ClearGesture ();

	if (template_id != NULL)
	{
		log_add (log_Debug,
				"Started rebinding for template %d, %s ( Binding: %d )",
				*template_id + 1, menu_res_names[action], binding + 1);
	}
	else
	{
		log_add (log_Debug, "Started rebinding for %s ( Binding: %d )",
				menu_res_names[action], binding + 1);
	}
}

static void
SaveMenuBindings (void)
{
	int i, j;
	uio_Stream *f;
	VCONTROL_GESTURE *g;
	char g_str[128];

	f = uio_fopen (configDir, "override.cfg", "w");
	if (!f)
	{
		log_add (log_Error, "Failed to open override.cfg for writing");
		return;
	}

	uio_fprintf (f,
			"# Custom menu control bindings generated by ImGui\n\n");

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		for (j = 0; j < 6; j++)
		{
			g = &curr_bindings[i].binding[j];

			if (g->type == VCONTROL_NONE)
				continue;

			VControl_DumpGesture (g_str, sizeof (g_str), g);

			uio_fprintf (f, "%s.%d = STRING:%s\n", menu_res_names[i],
					j + 1, g_str);
		}
	}

	uio_fclose (f);
	bindings_dirty.menu = FALSE;
}

static void
LoadDefaultMenuKeys (void)
{
	int i, j;
	VCONTROL_GESTURE *g;

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (!menu_res_names[i])
			break;

		for (j = 0; j < 6; j++)
		{
			g = &curr_bindings[i].binding[j];
			VControl_RemoveGestureBinding (g, (int *)&menu_vec[i]);
		}

		memcpy (&curr_bindings[i], &def_bindings[i],
				sizeof (MENU_BINDINGS));

		for (j = 0; j < 6; j++)
		{
			g = &def_bindings[i].binding[j];
			VControl_AddGestureBinding (g, (int *)&menu_vec[i]);
		}
	}
}

static void
RestoreMenuBindings (void)
{
	int i, j;
	VCONTROL_GESTURE *g;

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		for (j = 0; j < 6; j++)
		{
			g = &curr_bindings[i].binding[j];
			VControl_RemoveGestureBinding (g, (int *)&menu_vec[i]);
		}

		memcpy (&curr_bindings[i], &saved_menu_bindings[i],
				sizeof (MENU_BINDINGS));

		for (j = 0; j < 6; j++)
		{
			g = &curr_bindings[i].binding[j];
			VControl_AddGestureBinding (g, (int *)&menu_vec[i]);
		}
	}
}

static void
LoadDefaultFlightKeys (void)
{
	int i, j;
	char keybuf[40], valbuf[40];
	char key[40];
	VCONTROL_GESTURE *g;
	int *target;

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (flight_res_names[i] == NULL)
			break;

		target = (int *)(flight_vec + template_id * num_flight + i);

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			g = &curr_fl_bindings[template_id][i].binding[j];
			VControl_RemoveGestureBinding (g, target);
		}

		memcpy (&curr_fl_bindings[template_id][i],
				&def_fl_bindings[template_id][i],
				sizeof (FLIGHT_BINDINGS));

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			g = &def_fl_bindings[template_id][i].binding[j];
			VControl_AddGestureBinding (g, target);

			snprintf (keybuf, sizeof (keybuf), "keys.%d.%s.%d",
					template_id + 1, flight_res_names[i], j + 1);
			VControl_DumpGesture (valbuf, sizeof valbuf, g);
			res_PutString (keybuf, valbuf);
		}
	}

	snprintf (input_templates[template_id].name,
			sizeof (input_templates[template_id].name), "%s",
			def_template_names[template_id]);

	bindings_dirty.flight = TRUE;

	snprintf (key, sizeof (key), "keys.%d.name", template_id + 1);
	res_PutString (key, def_template_names[template_id]);
}

static void
RestoreFlightBindings (void)
{
	int i, j;
	VCONTROL_GESTURE *g;
	int *target;

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (flight_res_names[i] == NULL)
			continue;

		target = (int *)(flight_vec + template_id * num_flight + i);

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			g = &curr_fl_bindings[template_id][i].binding[j];

			VControl_RemoveGestureBinding (g, target);
		}

		memcpy (&curr_fl_bindings[template_id][i],
				&saved_flight_bindings[template_id][i],
				sizeof (FLIGHT_BINDINGS));

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			g = &curr_fl_bindings[template_id][i].binding[j];

			VControl_AddGestureBinding (g, target);
		}
	}

	snprintf (input_templates[template_id].name,
			sizeof (input_templates[template_id].name), "%s",
			saved_template_names[template_id]);
}

static const char *
GetBindingDisplayText (VCONTROL_GESTURE *gesture)
{
	static char buffer[30];

	InterrogateInputState (-1, -1, -1, buffer, sizeof (buffer), gesture);

	return buffer;
}

static void
BackupCurrentBindings (void)
{
	int i, j;

	if (binds_backed_up)
		return;

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		memcpy (&saved_menu_bindings[i], &curr_bindings[i],
				sizeof (MENU_BINDINGS));
	}

	for (i = 0; i < 6; i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			if (flight_res_names[j] == NULL)
				continue;

			memcpy (&saved_flight_bindings[i][j],
					&curr_fl_bindings[i][j], sizeof (FLIGHT_BINDINGS));
		}

		snprintf (saved_template_names[i],
				sizeof (saved_template_names[i]), "%s",
				input_templates[i].name);
	}

	binds_backed_up = TRUE;
}

static BOOLEAN
CheckEventMatchesAction (const SDL_Event *e, int action)
{
	int i;
	VCONTROL_GESTURE *g;

	for (i = 0; i < 6; i++)
	{
		g = &curr_bindings[action].binding[i];

		if (g->type == VCONTROL_NONE)
			continue;

		// Check keyboard event
		if ((e->type == SDL_KEYDOWN || e->type == SDL_KEYUP) &&
				g->type == VCONTROL_KEY &&
				g->gesture.key == e->key.keysym.sym)
		{
			return TRUE;
		}

		// Check controller button event
		if ((e->type == SDL_CONTROLLERBUTTONDOWN ||
				e->type == SDL_CONTROLLERBUTTONUP) &&
				g->type == VCONTROL_JOYBUTTON &&
				g->gesture.button.port == e->cbutton.which &&
				g->gesture.button.index == e->cbutton.button)
		{
			return TRUE;
		}

		// Check controller axis event
		if (e->type == SDL_CONTROLLERAXISMOTION &&
				g->type == VCONTROL_JOYAXIS &&
				g->gesture.axis.port == e->caxis.which &&
				g->gesture.axis.index == e->caxis.axis)
		{
			int value = e->caxis.value;
			int polarity = (value < 0) ? -1 : 1;

			if ((polarity > 0 && g->gesture.axis.polarity > 0) ||
				(polarity < 0 && g->gesture.axis.polarity < 0))
			{
				if (abs (value) > 15000)
					return TRUE;
			}
		}
	}

	return FALSE;
}

static int
GetActionForEvent (const SDL_Event *e)
{
	int action;

	for (action = 0; action < NUM_MENU_KEYS; action++)
	{
		if (menu_res_names[action] == NULL)
			continue;

		if (CheckEventMatchesAction (e, action))
			return action;
	}

	return -1;
}

static BOOLEAN
VControl_GestureEqual (VCONTROL_GESTURE *a, VCONTROL_GESTURE *b)
{
	if (a->type != b->type)
		return FALSE;

	switch (a->type)
	{
	case VCONTROL_KEY:
		return a->gesture.key == b->gesture.key;
	case VCONTROL_JOYBUTTON:
		return (a->gesture.button.port == b->gesture.button.port &&
				a->gesture.button.index == b->gesture.button.index);
	case VCONTROL_JOYAXIS:
		return (a->gesture.axis.port == b->gesture.axis.port &&
				a->gesture.axis.index == b->gesture.axis.index &&
				a->gesture.axis.polarity == b->gesture.axis.polarity);
	default:
		return FALSE;
	}
}

// These actions are not allowed to be rebound to game actions
// and vice versa.
bool
BlacklistedBindings (int action_index)
{
	switch (action_index)
	{
	case KEY_SCREENSHOT:
	case KEY_IMGUI:
		return true;
	default:
		return false;
	}
}

// These actions are allowed to pass through to the game while the
// ImGui menu is open.
bool
WhitelistedPassThru (int action_index)
{
	switch (action_index)
	{
	case KEY_SCREENSHOT:
	case KEY_IMGUI:
		return true;
	default:
		return false;
	}
}

static BOOLEAN
CheckMenuBindingConflict (int action, VCONTROL_GESTURE *g_compare)
{
	int j;
	VCONTROL_GESTURE *g;

	for (j = 0; j < 6; j++)
	{
		g = &curr_bindings[action].binding[j];

		if (g->type == VCONTROL_NONE)
			continue;

		if (VControl_GestureEqual (g, g_compare))
		{
			snprintf (rebind_state.conflict_action,
					sizeof (rebind_state.conflict_action),
					"%s", pretty_menu_actions[action]);
			return TRUE;
		}
	}
	return FALSE;
}

static BOOLEAN
CheckFlightBindingConflict (int template_id, int action,
		VCONTROL_GESTURE *g_compare, int temp_compare)
{
	int j;
	VCONTROL_GESTURE *g;

	for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
	{
		g = &curr_fl_bindings[template_id][action].binding[j];

		if (g->type == VCONTROL_NONE)
			continue;

		if (VControl_GestureEqual (g, g_compare))
		{
			if (template_id == temp_compare)
			{
				snprintf (rebind_state.conflict_action,
						sizeof (rebind_state.conflict_action),
						"%s", pretty_flight_actions[action]);
			}
			else
			{
				snprintf (rebind_state.conflict_action,
						sizeof (rebind_state.conflict_action),
						"%s ( Template: %s )",
						pretty_flight_actions[action],
						input_templates[template_id].name);
			}
			return TRUE;
		}
	}
	return FALSE;
}

static BOOLEAN
CheckRebindConflict (VCONTROL_GESTURE *g, int action, int template_id)
{
	int i, j;
	BOOLEAN is_menu_rebind = (template_id == -1);
	BOOLEAN binding_special = FALSE;

	if (is_menu_rebind)
		binding_special = BlacklistedBindings (action);

	rebind_state.conflict_action[0] = '\0';

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		BOOLEAN key_special;

		if (menu_res_names[i] == NULL)
			continue;

		if (is_menu_rebind && i == action)
			continue;

		key_special = BlacklistedBindings (i);

		if (binding_special || key_special)
		{
			if (CheckMenuBindingConflict (i, g))
				return TRUE;
		}
	}

	if (is_menu_rebind && binding_special)
	{
		for (i = 0; i < 6; i++)
		{
			for (j = 0; j < NUM_KEYS; j++)
			{
				if (flight_res_names[j] == NULL)
					continue;

				if (CheckFlightBindingConflict (i, j, g, template_id))
					return TRUE;
			}
		}
	}

	return FALSE;
}

bool
ProcessControlEvents (SDL_Event *event)
{
	REBIND_STATE *RSPtr = &rebind_state;

	if (RSPtr->active)
	{
		VCONTROL_GESTURE new_g;

		switch (event->type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		case SDL_CONTROLLERAXISMOTION:
			event2gesture (event, &new_g);

			if (new_g.type != VCONTROL_NONE)
			{
				if (CheckRebindConflict (&new_g, RSPtr->action,
					RSPtr->template_id))
				{
					const char *pretty_actions;
					int action = RSPtr->action;

					RSPtr->has_error = TRUE;

					if (RSPtr->template_id == -1)
						pretty_actions = pretty_menu_actions[action];
					else
						pretty_actions = pretty_flight_actions[action];

					snprintf (RSPtr->error_message,
							sizeof (RSPtr->error_message),
							"Cannot bind to `%s' using the `%s' ( %s ) "
							"binding",
							pretty_actions, RSPtr->conflict_action,
							GetBindingDisplayText(&new_g));

					memcpy (&RSPtr->new_g, &new_g, sizeof (new_g));
					RSPtr->show_popup = TRUE;
					VControl_ClearGesture ();
					return TRUE;
				}
				else
				{
					RSPtr->has_error = FALSE;
					RSPtr->error_message[0] = '\0';
					memcpy (&RSPtr->new_g, &new_g, sizeof (new_g));
					RSPtr->show_popup = TRUE;
				}

				VControl_ClearGesture ();
				return TRUE;
			}
			return TRUE;

		case SDL_QUIT:
			return FALSE;

		default:
			return TRUE;
		}
	}

	switch (event->type)
	{
	case SDL_QUIT:
		return FALSE;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
	case SDL_CONTROLLERAXISMOTION:
		return !WhitelistedPassThru (GetActionForEvent (event));
	default:
		return TRUE;
	}
}