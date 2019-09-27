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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This file contains code for running scripts when a new game is started
 * or an old game is loaded.
 */

#define LUAUQM_INTERNAL
#include "port.h"
#include "luacomm.h"

#include "luainit.h"
#include "options.h"
		// for contentDir
#include "uqm/resinst.h"
#include "uqm/lua/luastate.h"
#include "luafuncs/eventfuncs.h"
#include "luafuncs/logfuncs.h"
#include "luafuncs/statefuncs.h"
#include "libs/log.h"

static const luaL_Reg initLibs[] = {
	{ "event",  luaUqm_event_open },
	{ "log",    luaUqm_log_open },
	{ "state",  luaUqm_state_open },
	{ NULL, NULL }
};

void
luaUqm_runInitScripts(void) {
	const char *scriptDir;

	// Set up an environment and run the init scripts in this environment.
	// Note that the environment will not be used after this; when a script
	// is run for eg. communication, a new environment will be set up.
	luaUqm_prepareEnvironment(luaUqm_globalState);
	luaUqm_loadLibs(luaUqm_globalState, initLibs);

	scriptDir = res_GetString (SCRIPT_DIR_INITGAME);
	if (scriptDir == NULL) {
		log_add(log_Warning, "Location of game initialisation scripts ('%s')"
				" was not specified.", SCRIPT_DIR_INITGAME);
	} else {
		luaUqm_runLuaDir(luaUqm_globalState, contentDir, scriptDir);
	}
}

