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

#include <stdio.h>
#include <stdlib.h>
#include "libs/tasklib.h"
#include "libs/log.h"

#define TASK_MAX 64

static struct taskstruct task_array[TASK_MAX];

Task
AssignTask (ThreadFunction task_func, SDWORD stackSize, const char *name)
{
	int i;
	for (i = 0; i < TASK_MAX; ++i)
	{
		if (!Task_SetState (task_array+i, TASK_INUSE))
		{
			// log_add (log_Debug, "Assigning Task #%i: %s", i+1, name);
			Task_ClearState (task_array+i, ~TASK_INUSE);
			task_array[i].name = name;
			task_array[i].thread = CreateThread (task_func, task_array+i,
					stackSize, name);
			return task_array+i;
		}
	}
	log_add (log_Error, "Task error!  Task array exhausted.  Check for thread leaks.");
	return NULL;
}

void
FinishTask (Task task)
{
	// log_add (log_Debug, "Releasing Task: %s", task->name);
	task->thread = 0;
	if (!Task_ClearState (task, TASK_INUSE))
	{
		log_add (log_Debug, "Task error!  Attempted to FinishTask '%s'... "
				"but it was already done!", task->name);
	}
}

/* This could probably be done better with a condition variable of some kind. */
void
ConcludeTask (Task task)
{
	Thread old = task->thread;
	// log_add (log_Debug, "Awaiting conclusion of %s", task->name);
	if (old)
	{
		Task_SetState (task, TASK_EXIT);
		while (task->thread == old)
		{
			TaskSwitch ();
		}
	}
}

DWORD
Task_SetState (Task task, DWORD state_mask)
{
	DWORD old_state;
	LockMutex (task->state_mutex);
	old_state = task->state;
	task->state |= state_mask;
	UnlockMutex (task->state_mutex);
	old_state &= state_mask;
	return old_state;
}

DWORD
Task_ClearState (Task task, DWORD state_mask)
{
	DWORD old_state;
	LockMutex (task->state_mutex);
	old_state = task->state;
	task->state &= ~state_mask;
	UnlockMutex (task->state_mutex);
	old_state &= state_mask;
	return old_state;
}

DWORD
Task_ToggleState (Task task, DWORD state_mask)
{
	DWORD old_state;
	LockMutex (task->state_mutex);
	old_state = task->state;
	task->state ^= state_mask;
	UnlockMutex (task->state_mutex);
	old_state &= state_mask;
	return old_state;
}

DWORD
Task_ReadState (Task task, DWORD state_mask)
{
	return task->state & state_mask;
}

void 
InitTaskSystem (void)
{
	int i;
	for (i = 0; i < TASK_MAX; ++i)
	{
		task_array[i].state_mutex = CreateMutex ("task manager lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_RESOURCE);
	}
}

void 
CleanupTaskSystem (void)
{
	int i;
	for (i = 0; i < TASK_MAX; ++i)
	{
		DestroyMutex (task_array[i].state_mutex);
		task_array[i].state_mutex = 0;
	}
}

