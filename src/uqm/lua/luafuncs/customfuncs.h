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

#ifndef UQM_LUA_LUAFUNCS_CUSTOMFUNCS_H_
#define UQM_LUA_LUAFUNCS_CUSTOMFUNCS_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct luaUqm_custom_Function luaUqm_custom_Function;

#if defined(__cplusplus)
}
#endif

#include "libs/scriptlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct
luaUqm_custom_Function {
	const char *name;
	int (*fun)(int);
};

int luaUqm_custom_init(lua_State *luaState,
		const luaUqm_custom_Function *funs);

#if defined(__cplusplus)
}
#endif

#endif  /* CUSTOMFUNCS_H */

