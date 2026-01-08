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


REBIND_STATE rebind_state = { 0 };

static MENU_BINDINGS edit_bindings[NUM_MENU_KEYS];
static BOOLEAN bindings_dirty = FALSE;
static BOOLEAN bindings_loaded = FALSE;

static void Control_Tabs (void);

static void LoadBindingsForEditing (void);
static void StartRebinding (int action, int binding);
static void UpdateRebinding (void);
static void ShowRebindPopup (void);
static void SaveCustomBindings (void);
static const char *GetBindingDisplayText (VCONTROL_GESTURE *gesture);

const char *pretty_actions[] = {
	"Pause",
	"Exit",
	"Abort",
	"Debug 1",
	"Fullscreen",
	"Up",
	"Down",
	"Left",
	"Right",
	"Select",
	"Cancel",
	"Special",
	"Page-Up",
	"Page-Down",
	"Home",
	"End",
	"Zoom-In",
	"Zoom-Out",
	"Delete",
	"Backspace",
	"Cancel Edit",
	"Star Search",
	"Tab / Next",
	"Toggle StarMaps",
	"Screenshot",
	"ImGui Toggle",
	"Debug 2",
	"Debug 3",
	"Debug 4",
	NULL
};

void draw_controls_menu (void)
{
	int i;
	const char *control_display[] = { "Keyboard", "Xbox", "PlayStation" };
	const char *player_controls[6];

	ImGui_ColumnsEx (DISPLAY_BOOL, "ControlsColumns", false);

	// Control Options
	{
		ImGui_SeparatorText ("Control Options");

		ImGui_Text ("Control Display:");
		if (ImGui_ComboChar ("##ControlDisplay", (int *)&optControllerType,
				control_display, 3))
		{
			res_PutInteger ("mm.controllerType", optControllerType);
			mmcfg_changed = true;
		}

		Spacer ();

		for (i = 0; i < 6; i++)
			player_controls[i] = &input_templates[i].name;

		ImGui_Text ("Player 1:");
		if (ImGui_ComboChar ("##BottomPlayer", (int *)&PlayerControls[0],
				player_controls, 6))
		{
			res_PutInteger ("config.player1control", PlayerControls[0]);
			config_changed = true;
		}

		ImGui_Text ("Player 2:");
		if (ImGui_ComboChar ("##TopPlayer", (int *)&PlayerControls[1],
				player_controls, 6))
		{
			res_PutInteger ("config.player2control", PlayerControls[1]);
			config_changed = true;
		}

		ImGui_NewLine ();
	}

	ImGui_Columns ();

	Control_Tabs ();
}

static void PlayerOneControls (void)
{
	// Bullshit goes here
	return;
}

static void MenuControls (void)
{
	int i, j;

	if (!bindings_loaded)
	{
		LoadBindingsForEditing ();
	}

	ImGui_SeparatorText ("Menu Controls");

	ImGui_BeginChild ("MenuBindings", ZERO_F, 0, 0);

	ImGui_ColumnsEx (2, "MenuControlBindings", false);
	ImGui_SetColumnWidth (0, 150.0f);

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		char button_id[32];
		VCONTROL_GESTURE *gesture;

		if (menu_res_names[i] == NULL)
			continue;

		ImGui_Text ("%s:", pretty_actions[i]);

		ImGui_NextColumn ();

		for (j = 0; j < 6; j++)
		{
			gesture = &edit_bindings[i].binding[j]; 

			snprintf (button_id, sizeof (button_id), "##bind_%d_%d", i, j);

			ImGui_PushID (button_id);

			if (ImGui_ButtonEx (GetBindingDisplayText (gesture),
					(ImVec2){ 120.0f, 0 }))
			{
				StartRebinding (i, j);
			}

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
	int i, j;

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (!menu_res_names[i])
			break;

		for (int j = 0; j < 6; j++)
		{
			VControl_RemoveGestureBinding (&edit_bindings[i].binding[j],
					(int *)&menu_vec[i]);
		}

		memcpy (&curr_bindings[i], &def_bindings[i], sizeof (MENU_BINDINGS));
		memcpy (&edit_bindings[i], &def_bindings[i], sizeof (MENU_BINDINGS));

		for (int j = 0; j < 6; j++)
		{
			VControl_AddGestureBinding (&def_bindings[i].binding[j],
					(int *)&menu_vec[i]);
		}
	}
}

static void MenuControlsTab (void)
{
	if (ImGui_BeginTabItem ("Menu", NULL, 0))
	{
		ImGui_NewLine ();

		if (ImGui_Button ("Load Defaults"))
		{
			LoadDefaultMenuKeys ();
			bindings_dirty = TRUE;
		}

		if (bindings_dirty)
		{
			ImGui_SameLine ();
			if (ImGui_Button ("Save Changes"))
			{
				SaveCustomBindings ();
			}
			ImGui_SameLine ();
			if (ImGui_Button ("Cancel"))
			{
				LoadBindingsForEditing ();
			}
		}

		ShowRebindPopup ();

		ImGui_NewLine ();

		MenuControls ();

		ImGui_EndTabItem ();
	}
}

static void PlayerOneTab (void)
{
	if (ImGui_BeginTabItem ("Player 1", NULL, 0))
	{
		ImGui_NewLine ();

		if (ImGui_Button ("Clear All"))
		{
			// Add switching code here
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Set Defaults"))
		{
			// Add switching code here
		}

		ImGui_NewLine ();

		PlayerOneControls ();

		ImGui_EndTabItem ();
	}
}

static void PlayerTwoTab (void)
{
	if (ImGui_BeginTabItem ("Player 2", NULL, 0))
	{
		ImGui_NewLine ();

		if (ImGui_Button ("Clear All"))
		{
			// Add switching code here
		}
		ImGui_SameLine ();
		if (ImGui_Button ("Set Defaults"))
		{
			// Add switching code here
		}

		ImGui_NewLine ();

		ImGui_EndTabItem ();
	}
}

static void Control_Tabs (void)
{
	ImGui_SeparatorText ("Edit Controls");

	UpdateRebinding ();

	if (ImGui_BeginTabBar ("ControlTabs", 0))
	{
		MenuControlsTab ();
		PlayerOneTab ();
		PlayerTwoTab ();

		ImGui_EndTabBar ();
	}
}

static void
LoadBindingsForEditing (void)
{
	int i;
	
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		memcpy (&edit_bindings[i], &curr_bindings[i],
				sizeof (MENU_BINDINGS));
	}

	bindings_loaded = TRUE;
	bindings_dirty = FALSE;
}

static void
StartRebinding (int action, int binding)
{
	rebind_state.active = TRUE;
	rebind_state.action = action;
	rebind_state.binding = binding;

	rebind_state.old_g = edit_bindings[action].binding[binding];
	rebind_state.new_g.type = VCONTROL_NONE;

	VControl_ClearGesture ();

	log_add (log_Debug, "Started rebinding for %s (binding %d)",
		menu_res_names[action], binding + 1);
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
ShowRebindPopup (void)
{
	VCONTROL_GESTURE new_g;
	
	if (rebind_state.active)
	{
		ImGui_OpenPopup ("##RebindPopup", 0);
	}

#define WINDOW_FLAGS ImGuiWindowFlags_NoTitleBar | \
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize

	if (ImGui_BeginPopupModal ("##RebindPopup", NULL, WINDOW_FLAGS))
	{
		char popup_title[128];
		const char *display_text;

		snprintf (popup_title, sizeof (popup_title),
				"Action Name: %s | Binding #: %d",
				pretty_actions[rebind_state.action],
				rebind_state.binding + 1);

		ImGui_Text ("%s", popup_title);

		Spacer ();

		ImGui_Separator ();

		Spacer ();

		if (VControl_GetLastGesture (&new_g))
		{
			memcpy (&rebind_state.new_g, &new_g,
					sizeof (VCONTROL_GESTURE));
			VControl_ClearGesture ();
		}

		if (rebind_state.new_g.type != VCONTROL_NONE)
		{
			display_text = GetBindingDisplayText (&rebind_state.new_g);
			ImGui_Text ("Captured Input: %s",
					display_text[0] ? display_text : "None");
		}
		else
		{
			ImGui_TextColored (IV4_YELLOW_COLOR, "Waiting for input...");
			ImGui_Text ("Press any key, button, or move an axis");
		}

		ImGui_NewLine ();

		if (ImGui_Button ("Clear"))
		{
			edit_bindings[rebind_state.action].
					binding[rebind_state.binding].type = VCONTROL_NONE;
			curr_bindings[rebind_state.action].
					binding[rebind_state.binding].type = VCONTROL_NONE;

			if (rebind_state.old_g.type != VCONTROL_NONE)
			{
				VControl_RemoveGestureBinding (&rebind_state.old_g,
					(int *)&menu_vec[rebind_state.action]);
			}

			bindings_dirty = TRUE;

			rebind_state.active = FALSE;
			rebind_state.new_g.type = VCONTROL_NONE;
			ImGui_CloseCurrentPopup ();
		}

		ImGui_SameLine ();

		if (ImGui_Button ("Cancel"))
		{
			if (rebind_state.old_g.type != VCONTROL_NONE)
			{
				edit_bindings[rebind_state.action].
						binding[rebind_state.binding] = rebind_state.old_g;
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

		if (rebind_state.new_g.type == VCONTROL_NONE)
		{
			ImGui_PushStyleVar (ImGuiStyleVar_Alpha, 0.5f);
			ImGui_Button ("OK");
			ImGui_PopStyleVar ();
		}
		else
		{
			if (ImGui_Button ("OK"))
			{
				edit_bindings[rebind_state.action].
						binding[rebind_state.binding] = rebind_state.new_g;
				curr_bindings[rebind_state.action].
						binding[rebind_state.binding] = rebind_state.new_g;

				if (rebind_state.old_g.type != VCONTROL_NONE)
				{
					VControl_RemoveGestureBinding (&rebind_state.old_g,
						(int *)&menu_vec[rebind_state.action]);
				}

				VControl_AddGestureBinding (&rebind_state.new_g,
					(int *)&menu_vec[rebind_state.action]);

				bindings_dirty = TRUE;

				log_add (log_Debug,
						"Applied new binding for %s (binding %d): %s",
						menu_res_names[rebind_state.action],
						rebind_state.binding + 1,
						GetBindingDisplayText (&rebind_state.new_g));

				rebind_state.active = FALSE;
				rebind_state.new_g.type = VCONTROL_NONE;
				ImGui_CloseCurrentPopup ();
			}
		}

		ImGui_EndPopup ();
	}
}

static void
SaveCustomBindings (void)
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

	uio_fprintf (f,"# Custom menu control bindings generated by ImGui\n\n");

	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (menu_res_names[i] == NULL)
			continue;

		for (j = 0; j < 6; j++)
		{
			gesture = &curr_bindings[i].binding[j];

			if (gesture->type == VCONTROL_NONE)
				continue;

			VControl_DumpGesture (
					gesture_str, sizeof (gesture_str), gesture);

			uio_fprintf (f, "%s.%d = STRING:%s\n",
				menu_res_names[i], j + 1, gesture_str);
		}
	}

	uio_fclose (f);
	bindings_dirty = FALSE;
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