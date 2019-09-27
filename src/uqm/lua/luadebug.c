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

//#include <stdlib.h>
//#include <lauxlib.h>
#include <string.h>

#define LUAUQM_INTERNAL
#include "port.h"
#include "luadebug.h"

//#include "libs/scriptlib.h"
//#include "libs/misc.h"
#include "uqm/lua/luastate.h"
#include "luafuncs/commfuncs.h"
#include "luafuncs/eventfuncs.h"
#include "luafuncs/logfuncs.h"
#include "luafuncs/statefuncs.h"


#define LINEBUFLEN 2048
		// Maximum size of one input line.
		// Long enough for long oneliners.

typedef struct luaUqm_DebugContext {
	FILE *in;
	FILE *out;
	FILE *err;
	lua_State *debugState;
} luaUqm_DebugContext;


static void luaUqm_debug_interactive(FILE *in, FILE *out, FILE *err);
static void luaUqm_debug_outputCallback(void *extra,
		const char *format, ...);
static void luaUqm_debug_errorCallback(void *extra,
		const char *format, ...);


lua_State *luaUqm_debugState = NULL;
	
static const luaL_Reg debugLibs[] = {
	{ "comm",    luaUqm_comm_open },
	{ "event",   luaUqm_event_open },
	{ "log",     luaUqm_log_open },
	//{ "package", luaUqm_package_open },
	{ "state",   luaUqm_state_open },
	{ NULL, NULL }
};

// Not reentrant.
// If 'customFuncs' is NULL, no 'custom' table is added to the Lua environment.
// If 'scriptRes' is NULL_RESOURCE, then no script is loaded. Lua is only
// available for string interpolation in this case.
void
luaUqm_debug_init(void) {
	assert(luaUqm_debugState == NULL);

	luaUqm_debugState = luaUqm_globalState;

	// Prepare the global environment.
	luaUqm_prepareEnvironment(luaUqm_debugState);
	luaUqm_loadLibs(luaUqm_debugState, debugLibs);
}

void
luaUqm_debug_uninit(void) {
	assert(luaUqm_debugState != NULL);
	luaUqm_debugState = NULL;
}

void
luaUqm_debug_run(void) {
	luaUqm_debug_init();

	luaUqm_debug_interactive(stdin, stdout, stderr);

	luaUqm_debug_uninit();
}

static void
luaUqm_debug_interactive(FILE *in, FILE *out, FILE *err) {
	char lineBuf[LINEBUFLEN];
	size_t lineLen;
	luaUqm_DebugContext debugContext;
	memset(&debugContext, '\0', sizeof (luaUqm_DebugContext));
	debugContext.in = in;
	debugContext.out = out;
	debugContext.err = err;
	debugContext.debugState = luaUqm_debugState;

	for (;;) {
		fprintf(out, "> ");

		if (fgets(lineBuf, LINEBUFLEN, in) == NULL) {
			if (feof(in)) {
				// user pressed ^D
				break;
			}
			// error occured
			clearerr(in);
			continue;
		}
		lineLen = strlen(lineBuf);
		if (lineBuf[lineLen - 1] != '\n' && lineBuf[lineLen - 1] != '\r') {
			fprintf(err, "Error: Too long command line.\n");
			// TODO: read until EOL
			continue;
		}

		luaUqm_debug_runLine(lineBuf,
				luaUqm_debug_outputCallback,
				luaUqm_debug_errorCallback,
				(void *) &debugContext);
	}
}

// Run a Lua command.
// This function is currently used by luaUqm_debug_interactive, but should
// also be suitable if we have some graphical console, when different
// callback functions than luaUqm_debug_outputCallback() and
// luaUqm_debug_errorCallback are used.
void
luaUqm_debug_runLine(const char *exprBuf,
		void (*outputCallback)(void *extra, const char *format, ...),
		void (*errorCallback)(void *extra, const char *format, ...),
		void *extra) {
	int resultType;
	const char *resultStr;
	const char *resultTypeStr;

	// Compile the string to a Lua function.
	{
		if (luaL_loadstring (luaUqm_debugState, exprBuf) != LUA_OK) {
			// An error occurred during parsing.
			errorCallback(extra, "Syntax error: %s\n",
					lua_tostring (luaUqm_debugState, -1));
			lua_pop(luaUqm_debugState, 1);
					// Pop the error.
			return;
		}
	}

	// Call the Lua function.
	if (lua_pcall (luaUqm_debugState, 0, 1, 0) != 0) {
		// An error occurred during execution.
		errorCallback(extra, "Runtime error: %s\n",
				lua_tostring (luaUqm_debugState, -1));
		lua_pop(luaUqm_debugState, 1);
				// Pop the error.
		return;
	}
	// Success. Result is on the stack.

	// Convert the result to a string.
	resultType = lua_type(luaUqm_debugState, -1);
	resultTypeStr = lua_typename(luaUqm_debugState, resultType);
	resultStr = lua_tolstring (luaUqm_debugState, -1, NULL);
			// Memory for 'resultStr' lasts until the lua_pop().
	if (resultStr == NULL) {
		// Not a string and not convertable to a string.
		// The command was executed ok though, and we treat this as such.
		outputCallback(extra, "(%s)\n", resultTypeStr);
	} else {
		outputCallback(extra, "(%s) %s\n", resultTypeStr, resultStr);
	}

	// Pop the result from the stack.
	lua_pop (luaUqm_debugState, 1);
}

// Called to output regular output messages.
static void
luaUqm_debug_outputCallback(void *extra, const char *format, ...) {
	va_list args;
	luaUqm_DebugContext *debugContext = (luaUqm_DebugContext *) extra;

	va_start(args, format);
	vfprintf(debugContext->out, format, args);
	va_end(args);
}

// Called to output error messages.
static void
luaUqm_debug_errorCallback(void *extra, const char *format, ...) {
	va_list args;
	luaUqm_DebugContext *debugContext = (luaUqm_DebugContext *) extra;

	va_start(args, format);
	vfprintf(debugContext->err, format, args);
	va_end(args);
}


