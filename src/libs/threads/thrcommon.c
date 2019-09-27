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

#include <stdio.h>
#include <stdlib.h>
#include "libs/threadlib.h"
#include "libs/timelib.h"
#include "libs/log.h"
#include "libs/async.h"
#include "libs/memlib.h"
#include "thrcommon.h"

#define LIFECYCLE_SIZE 8
typedef struct {
	ThreadFunction func;
	void *data;
	SDWORD stackSize;
	Semaphore sem;
	Thread value;
#ifdef NAMED_SYNCHRO
	const char *name;
#endif
} SpawnRequest_struct;

typedef SpawnRequest_struct *SpawnRequest;

static Mutex        lifecycleMutex;
static SpawnRequest pendingBirth[LIFECYCLE_SIZE];
static Thread       pendingDeath[LIFECYCLE_SIZE];

void
InitThreadSystem (void)
{
	int i;
	NativeInitThreadSystem ();
	for (i = 0; i < LIFECYCLE_SIZE; i++)
	{
		pendingBirth[i] = NULL;
		pendingDeath[i] = NULL;
	}
	lifecycleMutex = CreateMutex ("Thread Lifecycle Mutex", SYNC_CLASS_RESOURCE);
}

void
UnInitThreadSystem (void)
{
	NativeUnInitThreadSystem ();
	DestroyMutex (lifecycleMutex);
}

static Thread
FlagStartThread (SpawnRequest s)
{
	int i;
	LockMutex (lifecycleMutex);
	for (i = 0; i < LIFECYCLE_SIZE; i++)
	{
		if (pendingBirth[i] == NULL)
		{
			pendingBirth[i] = s;
			UnlockMutex (lifecycleMutex);
			if (s->sem)
			{
				Thread result;
				SetSemaphore (s->sem);
				DestroySemaphore (s->sem);
				result = s->value;
				HFree (s);
				return result;
			}
			return NULL;
		}
	}
	log_add (log_Fatal, "Thread Lifecycle array filled.  This is a fatal error!  Make LIFECYCLE_SIZE something larger than %d.", LIFECYCLE_SIZE);
	exit (EXIT_FAILURE);
}

void
FinishThread (Thread thread)
{
	int i;
	LockMutex (lifecycleMutex);
	for (i = 0; i < LIFECYCLE_SIZE; i++)
	{
		if (pendingDeath[i] == NULL)
		{
			pendingDeath[i] = thread;
			UnlockMutex (lifecycleMutex);
			return;
		}
	}
	log_add (log_Fatal, "Thread Lifecycle array filled.  This is a fatal error!  Make LIFECYCLE_SIZE something larger than %d.", LIFECYCLE_SIZE);
	exit (EXIT_FAILURE);
}

/* Only call from main thread! */
void
ProcessThreadLifecycles (void)
{
	int i;
	LockMutex (lifecycleMutex);
	for (i = 0; i < LIFECYCLE_SIZE; i++)
	{
		SpawnRequest s = pendingBirth[i];
		if (s != NULL)
		{
#ifdef NAMED_SYNCHRO
			s->value = NativeCreateThread (s->func, s->data, s->stackSize, s->name);
#else
			s->value = NativeCreateThread (s->func, s->data, s->stackSize);
#endif
			if (s->sem)
			{
				ClearSemaphore (s->sem);
				/* The spawning thread's FlagStartThread will clean up s */
			}
			else
			{
				/* The thread value has been lost to the game logic.  We must
				   clean up s ourself. */
				HFree (s);
			}
			pendingBirth[i] = NULL;
		}
	}

	for (i = 0; i < LIFECYCLE_SIZE; i++)
	{
		Thread t = pendingDeath[i];
		if (t != NULL)
		{
			WaitThread (t, NULL);
			pendingDeath[i] = NULL;	
			DestroyThread (t);
		}
	}
	UnlockMutex (lifecycleMutex);
}


/* The Create routines look different based on whether NAMED_SYNCHRO
   is defined or not. */

#ifdef NAMED_SYNCHRO
Thread
CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize, const char *name)
{
	SpawnRequest s = HMalloc(sizeof (SpawnRequest_struct));
	s->func = func;
	s->data = data;
	s->stackSize = stackSize;
	s->name = name;
	s->sem = CreateSemaphore (0, "SpawnRequest semaphore", SYNC_CLASS_RESOURCE);
	return FlagStartThread (s);
}

void
StartThread_Core (ThreadFunction func, void *data, SDWORD stackSize, const char *name)
{
	SpawnRequest s = HMalloc(sizeof (SpawnRequest_struct));
	s->func = func;
	s->data = data;
	s->stackSize = stackSize;
	s->name = name;
	s->sem = NULL;
	FlagStartThread (s);
}

Mutex
CreateMutex_Core (const char *name, DWORD syncClass)
{
	return NativeCreateMutex (name, syncClass);
}

Semaphore
CreateSemaphore_Core (DWORD initial, const char *name, DWORD syncClass)
{
	return NativeCreateSemaphore (initial, name, syncClass);
}

RecursiveMutex
CreateRecursiveMutex_Core (const char *name, DWORD syncClass)
{
	return NativeCreateRecursiveMutex (name, syncClass);
}

CondVar
CreateCondVar_Core (const char *name, DWORD syncClass)
{
	return NativeCreateCondVar (name, syncClass);
}

#else
/* These are the versions of Create* without the names. */
Thread
CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize)
{
	SpawnRequest s = HMalloc(sizeof (SpawnRequest_struct));
	s->func = func;
	s->data = data;
	s->stackSize = stackSize;
	s->sem = CreateSemaphore (0, "SpawnRequest semaphore", SYNC_CLASS_RESOURCE);
	return FlagStartThread (s);
}

void
StartThread_Core (ThreadFunction func, void *data, SDWORD stackSize)
{
	SpawnRequest s = HMalloc(sizeof (SpawnRequest_struct));
	s->func = func;
	s->data = data;
	s->stackSize = stackSize;
	s->sem = NULL;
	FlagStartThread (s);
}

Mutex
CreateMutex_Core (void)
{
	return NativeCreateMutex ();
}

Semaphore
CreateSemaphore_Core (DWORD initial)
{
	return NativeCreateSemaphore (initial);
}

RecursiveMutex
CreateRecursiveMutex_Core (void)
{
	return NativeCreateRecursiveMutex ();
}

CondVar
CreateCondVar_Core (void)
{
	return NativeCreateCondVar ();
}
#endif

void
DestroyThread (Thread t)
{
	NativeDestroyThread (t);
}

ThreadLocal *
CreateThreadLocal (void)
{
	ThreadLocal *tl = HMalloc (sizeof (ThreadLocal));
	tl->flushSem = CreateSemaphore (0, "FlushGraphics", SYNC_CLASS_VIDEO);
	return tl;
}

void
DestroyThreadLocal (ThreadLocal *tl)
{
	DestroySemaphore (tl->flushSem);
	HFree (tl);
}

ThreadLocal *
GetMyThreadLocal (void)
{
	return NativeGetMyThreadLocal ();
}

void
WaitThread (Thread thread, int *status)
{
	NativeWaitThread (thread, status);
}

#ifdef DEBUG_SLEEP
extern uint32 mainThreadId;
extern uint32 SDL_ThreadID(void);
#endif  /* DEBUG_SLEEP */

void
HibernateThread (TimePeriod timePeriod)
{
#ifdef DEBUG_SLEEP
	if (SDL_ThreadID() == mainThreadId)
		log_add (log_Debug, "HibernateThread called from main thread.\n");
#endif  /* DEBUG_SLEEP */

	NativeSleepThread (timePeriod);
}

void
HibernateThreadUntil (TimeCount wakeTime)
{
#ifdef DEBUG_SLEEP
	if (SDL_ThreadID() == mainThreadId)
		log_add (log_Debug, "HibernateThreadUntil called from main "
				"thread.\n");
#endif  /* DEBUG_SLEEP */

	NativeSleepThreadUntil (wakeTime);
}

void
SleepThread (TimePeriod timePeriod)
{
	TimeCount now;

#ifdef DEBUG_SLEEP
	if (SDL_ThreadID() != mainThreadId)
		log_add (log_Debug, "SleepThread called from non-main "
				"thread.\n");
#endif  /* DEBUG_SLEEP */

	now = GetTimeCounter ();
	SleepThreadUntil (now + timePeriod);
}

// Sleep until wakeTime, but call asynchrounous operations until then.
void
SleepThreadUntil (TimeCount wakeTime)
{
#ifdef DEBUG_SLEEP
	if (SDL_ThreadID() != mainThreadId)
		log_add (log_Debug, "SleepThreadUntil called from non-main "
				"thread.\n");
#endif  /* DEBUG_SLEEP */

	for (;;) {
		uint32 nextTimeMs;
		TimeCount nextTime;
		TimeCount now;

		Async_process ();

		now = GetTimeCounter ();
		if (wakeTime <= now)
			return;
		
		nextTimeMs = Async_timeBeforeNextMs ();
		nextTime = (nextTimeMs / 1000) * ONE_SECOND +
				((nextTimeMs % 1000) * ONE_SECOND / 1000);
				// Overflow-safe conversion.
		if (wakeTime < nextTime)
			nextTime = wakeTime;

		NativeSleepThreadUntil (nextTime);
	}
}

void
TaskSwitch (void)
{
	NativeTaskSwitch ();
}

void
DestroyMutex (Mutex sem)
{
	NativeDestroyMutex (sem);
}

void
LockMutex (Mutex sem)
{
	NativeLockMutex (sem);
}

void
UnlockMutex (Mutex sem)
{
	NativeUnlockMutex (sem);
}

void
DestroySemaphore (Semaphore sem)
{
	NativeDestroySemaphore (sem);
}

void
SetSemaphore (Semaphore sem)
{
	NativeSetSemaphore (sem);
}

void
ClearSemaphore (Semaphore sem)
{
	NativeClearSemaphore (sem);
}

void
DestroyCondVar (CondVar cv)
{
	NativeDestroyCondVar (cv);
}

void
WaitCondVar (CondVar cv)
{
	NativeWaitCondVar (cv);
}

void
SignalCondVar (CondVar cv)
{
	NativeSignalCondVar (cv);
}

void
BroadcastCondVar (CondVar cv)
{
	NativeBroadcastCondVar (cv);
}

void
DestroyRecursiveMutex (RecursiveMutex mutex)
{
	NativeDestroyRecursiveMutex (mutex);
}

void
LockRecursiveMutex (RecursiveMutex mutex)
{
	NativeLockRecursiveMutex (mutex);
}

void
UnlockRecursiveMutex (RecursiveMutex mutex)
{
	NativeUnlockRecursiveMutex (mutex);
}

int
GetRecursiveMutexDepth (RecursiveMutex mutex)
{
	return NativeGetRecursiveMutexDepth (mutex);
}
