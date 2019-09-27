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

/* By Michael Martin, 2002-09-21
 */

/* The task libraries are a set of facilities for controlling synchronous 
 * processes.  They are built on top of threads, but add the ability to
 * modify a "state" variable to pass messages back and forth. */

#ifndef LIBS_TASKLIB_H_
#define LIBS_TASKLIB_H_

#include "libs/threadlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* Bitmasks for setting task state. */
#define TASK_INUSE       1
#define TASK_EXIT        2

struct taskstruct {
	Mutex state_mutex;
	volatile DWORD state;   // Protected by state_mutex
	const char *name;
	volatile Thread thread;
};

typedef struct taskstruct *Task;

extern void  InitTaskSystem (void);
extern void  CleanupTaskSystem (void);

extern Task  AssignTask (ThreadFunction task_func, SDWORD Stacksize, const char *name);
extern DWORD Task_SetState (Task task, DWORD state_mask);
extern DWORD Task_ClearState (Task task, DWORD state_mask);
extern DWORD Task_ToggleState (Task task, DWORD state_mask);
extern DWORD Task_ReadState (Task task, DWORD state_mask);
extern void  FinishTask (Task task);
extern void  ConcludeTask (Task task);

#if defined(__cplusplus)
}
#endif

#endif

