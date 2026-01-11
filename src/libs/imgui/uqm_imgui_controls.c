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

REBIND_STATE rebind_state = { 0 };
static BOOLEAN menu_bindings_dirty = FALSE;
static BOOLEAN flight_bindings_dirty = FALSE;
static MENU_BINDINGS saved_menu_bindings[NUM_MENU_KEYS];
static FLIGHT_BINDINGS saved_flight_bindings[6][NUM_KEYS];
static char saved_template_names[6][30];
static BOOLEAN bindings_backed_up = FALSE;

static int template_id = 0;

static void Control_Tabs (void);

static void StartRebinding (int action, int binding, int *template_idx);
static void UpdateRebinding (void);
static void ShowMenuRebindPopup (void);
static void ShowFlightRebindPopup (void);
static void SaveMenuBindings (void);
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

void
draw_controls_menu (void)
{
	int i;
	const char *control_display[] = { "Keyboard", "Xbox", "PlayStation" };
	const char *player_controls[6];

	if (!bindings_backed_up)
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
MenuControls (void)
{
	int i, j;

	ImGui_BeginChild ("MenuBindings", ZERO_F, 0, 0);

	ImGui_ColumnsEx (2, "MenuControlBindings", false);
	ImGui_SetColumnWidth (0, 150.0f);

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		char button_id[32];

		if (menu_res_names[i] == NULL)
			continue;

		ImGui_Text ("%s:", pretty_menu_actions[i]);

		ImGui_NextColumn ();

		for (j = 0; j < 6; j++)
		{
			VCONTROL_GESTURE *g = &curr_bindings[i].binding[j];

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
LoadDefaultMenuKeys ()
{
	int i;

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (!menu_res_names[i])
			break;

		for (int j = 0; j < 6; j++)
		{
			VControl_RemoveGestureBinding (&curr_bindings[i].binding[j],
					(int *)&menu_vec[i]);
		}

		memcpy (&curr_bindings[i], &def_bindings[i], sizeof (MENU_BINDINGS));

		for (int j = 0; j < 6; j++)
		{
			VControl_AddGestureBinding (&def_bindings[i].binding[j],
					(int *)&menu_vec[i]);
		}
	}
}

static void
LoadDefaultFlightKeys (void)
{
	int j, k;
	char keybuf[40], valbuf[40];
	char key[40];

	for (j = 0; j < NUM_KEYS; j++)
	{
		if (flight_res_names[j] == NULL)
		{
			break;
		}

		for (k = 0; k < MAX_FLIGHT_ALTERNATES; k++)
		{
			VControl_RemoveGestureBinding (
					&curr_fl_bindings[template_id][j].binding[k],
					(int *)(flight_vec + template_id * num_flight + j));
		}

		memcpy (&curr_fl_bindings[template_id][j],
				&def_fl_bindings[template_id][j],
				sizeof (FLIGHT_BINDINGS));

		for (k = 0; k < MAX_FLIGHT_ALTERNATES; k++)
		{
			VCONTROL_GESTURE *g = &def_fl_bindings[template_id][j].binding[k];

			if (def_fl_bindings[template_id][j].binding[k].type != VCONTROL_NONE)
			{
				VControl_AddGestureBinding (g,
						(int *)(flight_vec + template_id * num_flight + j));
			}

			snprintf (keybuf, sizeof (keybuf), "keys.%d.%s.%d",
					template_id + 1, flight_res_names[j], k + 1);
			VControl_DumpGesture (valbuf, sizeof valbuf, g);
			res_PutString (keybuf, valbuf);
		}
	}

	snprintf (input_templates[template_id].name,
			sizeof (input_templates[template_id].name), "%s",
			def_template_names[template_id]);

	flight_bindings_dirty = TRUE;

	snprintf (key, sizeof (key), "keys.%d.name", template_id + 1);
	res_PutString (key, def_template_names[template_id]);
}

static void
RestoreMenuBindings (void)
{
	int i, j;

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		// Remove current bindings
		for (j = 0; j < 6; j++)
		{
			VControl_RemoveGestureBinding (&curr_bindings[i].binding[j],
					(int *)&menu_vec[i]);
		}

		// Restore from backup
		memcpy (&curr_bindings[i], &saved_menu_bindings[i],
				sizeof (MENU_BINDINGS));

		// Add restored bindings back
		for (j = 0; j < 6; j++)
		{
			if (curr_bindings[i].binding[j].type != VCONTROL_NONE)
			{
				VControl_AddGestureBinding (&curr_bindings[i].binding[j],
						(int *)&menu_vec[i]);
			}
		}
	}
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
		menu_bindings_dirty = TRUE;
	}

	if (menu_bindings_dirty)
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
			menu_bindings_dirty = FALSE;
		}
	}

	ShowMenuRebindPopup ();

	Spacer ();

	MenuControls ();

	ImGui_EndTabItem ();
}

void
FlightControls (void)
{
	int i, j;
	char *control_template[6];
	char template_name[30];

	ImGui_BeginChild ("FlightBindings", ZERO_F, 0, 0);
	ImGui_ColumnsEx (2, "FlightTemplates", false);
	ImGui_SetColumnWidth (0, 300.0f);

	for (i = 0; i < 6; i++)
		control_template[i] = input_templates[i].name;

	ImGui_BeginDisabled (flight_bindings_dirty);
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

		flight_bindings_dirty = TRUE;
		snprintf (key, sizeof (key), "keys.%d.name", template_id + 1);
		res_PutString (key, template_name);
	}

	ImGui_Columns ();

	ImGui_NewLine ();

	ImGui_ColumnsEx (2, "FlightControlBindings", false);
	ImGui_SetColumnWidth (0, 150.0f);

	for (i = 0; i < NUM_KEYS; i++)
	{
		char button_id[32];
		VCONTROL_GESTURE *gesture;

		if (flight_res_names[i] == NULL)
			continue;

		Spacer ();

		ImGui_Text ("%s:", pretty_flight_actions[i]);

		ImGui_NextColumn ();

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			gesture = &curr_fl_bindings[template_id][i].binding[j];

			snprintf (button_id, sizeof (button_id), "##bind_fl_%d_%d", i, j);

			ImGui_PushID (button_id);

			if (ImGui_ButtonEx (GetBindingDisplayText (gesture), BB_WIDTH))
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
RestoreFlightBindings (void)
{
	int i, j;

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (flight_res_names[i] == NULL)
			continue;

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			VControl_RemoveGestureBinding (
					&curr_fl_bindings[template_id][i].binding[j],
					(int *)(flight_vec + template_id * num_flight + i));
		}

		memcpy (&curr_fl_bindings[template_id][i],
				&saved_flight_bindings[template_id][i],
				sizeof (FLIGHT_BINDINGS));

		for (j = 0; j < MAX_FLIGHT_ALTERNATES; j++)
		{
			if (curr_fl_bindings[template_id][i].binding[j].type != VCONTROL_NONE)
			{
				VControl_AddGestureBinding (
						&curr_fl_bindings[template_id][i].binding[j],
						(int *)(flight_vec + template_id * num_flight + i));
			}
		}
	}

	snprintf (input_templates[template_id].name,
			sizeof (input_templates[template_id].name), "%s",
			saved_template_names[template_id]);
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
		flight_bindings_dirty = TRUE;
	}

	if (flight_bindings_dirty)
	{
		ImGui_SameLine ();
		if (ImGui_Button ("Save Changes"))
		{
			SaveKeyConfiguration (configDir, "flight.cfg");
			flight_bindings_dirty = FALSE;
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Cancel"))
		{
			RestoreFlightBindings ();
			flight_bindings_dirty = FALSE;
		}
	}

	ShowFlightRebindPopup ();

	Spacer ();

	FlightControls ();

	ImGui_EndTabItem ();
}

static void
Control_Tabs (void)
{
	ImGui_SeparatorText ("Edit Controls");

	UpdateRebinding ();

	if (ImGui_BeginTabBar ("ControlTabs", 0))
	{
		FlightControlsTab ();
		MenuControlsTab ();

		ImGui_EndTabBar ();
	}
}

static void
StartRebinding (int action, int binding, int *template_id)
{
	rebind_state.active = TRUE;
	rebind_state.action = action;
	rebind_state.binding = binding;

	if (template_id != NULL)
	{
		rebind_state.old_g =
				curr_fl_bindings[*template_id][action].binding[binding];
	}
	else
		rebind_state.old_g = curr_bindings[action].binding[binding];

	rebind_state.new_g.type = VCONTROL_NONE;

	VControl_ClearGesture ();

	if (template_id != NULL)
	{
		log_add (log_Debug,
				"Started rebinding for template %d, %s (binding %d)",
				*template_id + 1, menu_res_names[action], binding + 1);
	}
	else
	{
		log_add (log_Debug, "Started rebinding for %s (binding %d)",
				menu_res_names[action], binding + 1);
	}
}

static void
UpdateRebinding (void)
{
	VCONTROL_GESTURE new_g;

	if (!rebind_state.active)
		return;

	if (VControl_GetLastGesture (&new_g))
	{
		memcpy (&rebind_state.new_g, &new_g, sizeof (VCONTROL_GESTURE));
		rebind_state.show_popup = TRUE;
		VControl_ClearGesture ();
	}
}

static void
ShowMenuRebindPopup (void)
{
	VCONTROL_GESTURE new_g;
	char popup_title[128];

	if (!rebind_state.active)
		return;

#define WINDOW_FLAGS ImGuiWindowFlags_NoTitleBar | \
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize

	ImGui_OpenPopup ("##RebindPopup", 0);
	if (!ImGui_BeginPopupModal ("##RebindPopup", NULL, WINDOW_FLAGS))
		return;

	snprintf (popup_title, sizeof (popup_title),
			"Action Name: %s | Binding #: %d",
			pretty_menu_actions[rebind_state.action],
			rebind_state.binding + 1);

	ImGui_Text ("%s", popup_title);

	Spacer ();

	ImGui_PushStyleColor (ImGuiCol_ChildBg, STYLE_COLOR (ImGuiCol_Border));
	ImGui_BeginChild ("HorizontalSeparator", MAKE_IV2 (0, 1), 0, 0);
	ImGui_EndChild ();
	ImGui_PopStyleColor ();

	Spacer ();

	if (VControl_GetLastGesture (&new_g))
	{
		memcpy (&rebind_state.new_g, &new_g,
				sizeof (VCONTROL_GESTURE));
		VControl_ClearGesture ();
	}

	if (rebind_state.new_g.type == VCONTROL_NONE)
	{
		ImGui_TextColored (IV4_YELLOW_COLOR, "Waiting for input...");
		ImGui_Text ("Press any key, button, or move an axis");
	}

	Spacer ();

	if (ImGui_Button ("Clear"))
	{
		curr_bindings[rebind_state.action].
				binding[rebind_state.binding].type = VCONTROL_NONE;

		if (rebind_state.old_g.type != VCONTROL_NONE)
		{
			VControl_RemoveGestureBinding (&rebind_state.old_g,
					(int *)&menu_vec[rebind_state.action]);
		}

		menu_bindings_dirty = TRUE;

		rebind_state.active = FALSE;
		rebind_state.new_g.type = VCONTROL_NONE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (ImGui_Button ("Cancel"))
	{
		if (rebind_state.old_g.type != VCONTROL_NONE)
		{
			curr_bindings[rebind_state.action].
					binding[rebind_state.binding] = rebind_state.old_g;

			VControl_AddGestureBinding (&rebind_state.old_g,
					(int *)&menu_vec[rebind_state.action]);
		}

		rebind_state.active = FALSE;
		rebind_state.new_g.type = VCONTROL_NONE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (rebind_state.new_g.type != VCONTROL_NONE)
	{
		curr_bindings[rebind_state.action].
				binding[rebind_state.binding] = rebind_state.new_g;

		if (rebind_state.old_g.type != VCONTROL_NONE)
		{
			VControl_RemoveGestureBinding (&rebind_state.old_g,
					(int *)&menu_vec[rebind_state.action]);
		}

		VControl_AddGestureBinding (&rebind_state.new_g,
				(int *)&menu_vec[rebind_state.action]);

		menu_bindings_dirty = TRUE;

		log_add (log_Debug,
				"Applied new binding for %s (binding %d): %s",
				menu_res_names[rebind_state.action],
				rebind_state.binding + 1,
				GetBindingDisplayText (&rebind_state.new_g));

		rebind_state.active = FALSE;
		rebind_state.new_g.type = VCONTROL_NONE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_EndPopup ();
}

static void
ShowFlightRebindPopup (void)
{
	char popup_title[128];
	char keybuf[40], valbuf[40];

	if (rebind_state.active)
	{
		ImGui_OpenPopup ("##FlightRebindPopup", 0);
	}

	if (!ImGui_BeginPopupModal ("##FlightRebindPopup", NULL,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	snprintf (popup_title, sizeof (popup_title),
			"Template: %s | Control: %s | Binding #: %d",
			input_templates[template_id].name,
			pretty_flight_actions[rebind_state.action],
			rebind_state.binding + 1);

	ImGui_Text ("%s", popup_title);

	Spacer ();

	ImGui_PushStyleColor (ImGuiCol_ChildBg, STYLE_COLOR (ImGuiCol_Border));
	ImGui_BeginChild ("HorizontalSeparator", MAKE_IV2 (0, 1), 0, 0);
	ImGui_EndChild ();
	ImGui_PopStyleColor ();

	Spacer ();

	if (rebind_state.new_g.type == VCONTROL_NONE)
	{
		ImGui_TextColored (IV4_YELLOW_COLOR, "Waiting for input...");
		ImGui_Text ("Press any key, button, or move an axis");
	}

	Spacer ();

	if (ImGui_Button ("Clear"))
	{
		curr_fl_bindings[template_id][rebind_state.action].
				binding[rebind_state.binding].type = VCONTROL_NONE;

		if (rebind_state.old_g.type != VCONTROL_NONE)
		{
			VControl_RemoveGestureBinding (&rebind_state.old_g,
					(int *)(flight_vec + template_id * num_flight +
					rebind_state.action));
		}

		snprintf (keybuf, sizeof (keybuf), "keys.%d.%s.%d",
				template_id + 1, flight_res_names[rebind_state.action],
				rebind_state.binding + 1);
		VControl_DumpGesture (valbuf, sizeof valbuf, &rebind_state.new_g);
		res_PutString (keybuf, valbuf);

		flight_bindings_dirty = TRUE;

		rebind_state.active = FALSE;
		rebind_state.new_g.type = VCONTROL_NONE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (ImGui_Button ("Cancel"))
	{
		if (rebind_state.old_g.type != VCONTROL_NONE)
		{
			curr_fl_bindings[template_id][rebind_state.action].
					binding[rebind_state.binding] = rebind_state.old_g;

			VControl_AddGestureBinding (&rebind_state.old_g,
					(int *)(flight_vec + template_id * num_flight +
					rebind_state.action));
		}

		rebind_state.active = FALSE;
		rebind_state.new_g.type = VCONTROL_NONE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_SameLine ();

	if (rebind_state.new_g.type != VCONTROL_NONE)
	{
		char keybuf[40], valbuf[40];

		curr_fl_bindings[template_id][rebind_state.action].
				binding[rebind_state.binding] = rebind_state.new_g;

		if (rebind_state.old_g.type != VCONTROL_NONE)
		{
			VControl_RemoveGestureBinding (&rebind_state.old_g,
					(int *)(flight_vec + template_id * num_flight +
					rebind_state.action));
		}

		VControl_AddGestureBinding (&rebind_state.new_g,
				(int *)(flight_vec + template_id * num_flight +
				rebind_state.action));

		snprintf (keybuf, sizeof (keybuf), "keys.%d.%s.%d",
				template_id + 1, flight_res_names[rebind_state.action],
				rebind_state.binding + 1);
		VControl_DumpGesture (valbuf, sizeof valbuf, &rebind_state.new_g);
		res_PutString (keybuf, valbuf);

		flight_bindings_dirty = TRUE;

		log_add (log_Debug,
				"Applied new flight binding for template %d, %s (binding %d): %s",
				template_id + 1, flight_res_names[rebind_state.action],
				rebind_state.binding + 1,
				GetBindingDisplayText (&rebind_state.new_g));

		rebind_state.active = FALSE;
		rebind_state.new_g.type = VCONTROL_NONE;
		ImGui_CloseCurrentPopup ();
	}

	ImGui_EndPopup ();
}

static void
SaveMenuBindings (void)
{
	uio_Stream *f;
	int i;
	int j;
	VCONTROL_GESTURE *gesture;
	char gesture_str[128];

	f = uio_fopen (configDir, "override.cfg", "w");
	if (!f)
	{
		log_add (log_Error, "Failed to open override.cfg for writing");
		return;
	}

	uio_fprintf (f, "# Custom menu control bindings generated by ImGui\n\n");

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		for (j = 0; j < 6; j++)
		{
			gesture = &curr_bindings[i].binding[j];

			if (gesture->type == VCONTROL_NONE)
				continue;

			VControl_DumpGesture (gesture_str, sizeof (gesture_str),
					gesture);

			uio_fprintf (f, "%s.%d = STRING:%s\n",
					menu_res_names[i], j + 1, gesture_str);
		}
	}

	uio_fclose (f);
	menu_bindings_dirty = FALSE;
}

static const char *
GetBindingDisplayText (VCONTROL_GESTURE *gesture)
{
	static char buffer[128];

	if (gesture->type == VCONTROL_NONE)
	{
		return "";
	}

	switch (gesture->type)
	{
		case VCONTROL_KEY:
			return VControl_code2name (gesture->gesture.key);

		case VCONTROL_JOYBUTTON:
			if (optControllerType == 1)
			{
				snprintf (buffer, sizeof (buffer), "J%d %s",
						gesture->gesture.button.port,
						xbx_buttons[gesture->gesture.button.index]);
			}
			else if (optControllerType == 2)
			{
				snprintf (buffer, sizeof (buffer), "J%d %s",
						gesture->gesture.button.port,
						ds4_buttons[gesture->gesture.button.index]);
			}
			else
			{
				snprintf (buffer, sizeof (buffer), "J%d B%d",
						gesture->gesture.button.port,
						gesture->gesture.button.index);
			}
			return buffer;
		case VCONTROL_JOYAXIS:
			if (optControllerType == 1)
			{
				snprintf (buffer, sizeof (buffer), "J%d %s%c",
						gesture->gesture.axis.port,
						xbx_axes[gesture->gesture.axis.index],
						gesture->gesture.axis.polarity > 0 ? '+' : '-');
			}
			else if (optControllerType == 2)
			{
				snprintf (buffer, sizeof (buffer), "J%d %s%c",
						gesture->gesture.axis.port,
						ds4_axes[gesture->gesture.axis.index],
						gesture->gesture.axis.polarity > 0 ? '+' : '-');
			}
			else
			{
				snprintf (buffer, sizeof (buffer), "J%d A%d %c",
						gesture->gesture.axis.port,
						gesture->gesture.axis.index,
						gesture->gesture.axis.polarity > 0 ? '+' : '-');
			}
			return buffer;

		default:
			return "Unknown";
	}
}

static void
BackupCurrentBindings (void)
{
	int i, j;

	if (bindings_backed_up)
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

	bindings_backed_up = TRUE;
}