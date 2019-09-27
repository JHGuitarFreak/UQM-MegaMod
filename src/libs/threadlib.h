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

/* By Serge van den Boom, 2002-09-12
 */

#ifndef LIBS_THREADLIB_H_
#define LIBS_THREADLIB_H_

#define NAMED_SYNCHRO           /* Should synchronizable objects have names? */
#define TRACK_CONTENTION       /* Should we report when a thread sleeps on synchronize? */

/* TRACK_CONTENTION implies NAMED_SYNCHRO. */
#ifdef TRACK_CONTENTION
#	ifndef NAMED_SYNCHRO
#		define NAMED_SYNCHRO
#	endif
#endif /* TRACK_CONTENTION */

#ifdef DEBUG
#	ifndef DEBUG_THREADS
#		define DEBUG_THREADS
#	endif
#endif  /* DEBUG */

#ifdef DEBUG_THREADS
//#	ifndef PROFILE_THREADS
//#		define PROFILE_THREADS
//#	endif
#endif  /* DEBUG_THREADS */

#include <sys/types.h>
#include "libs/timelib.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined (PROFILE_THREADS) || defined (DEBUG_THREADS)
#define THREAD_NAMES
#endif

void InitThreadSystem (void);
void UnInitThreadSystem (void);

typedef int (*ThreadFunction) (void *);

typedef void *Thread;
typedef void *Mutex;
typedef void *Semaphore;
typedef void *RecursiveMutex;
typedef void *CondVar;

/* Local data associated with each thread */
typedef struct _threadLocal {
	Semaphore flushSem;
} ThreadLocal;

/* The classes of synchronization objects */

enum 
{
	SYNC_CLASS_TOPLEVEL = (1 << 0),    /* Exposed to the game logic */
	SYNC_CLASS_AUDIO    = (1 << 1),    /* Involves the audio system */
	SYNC_CLASS_VIDEO    = (1 << 2),    /* Involves the video system.  Very noisy because of FlushGraphics(). */
	SYNC_CLASS_RESOURCE = (1 << 3)     /* Involves system resources (_MemoryLock) */
};

/* Note.  NEVER call CreateThread from the main thread, or deadlocks
   are guaranteed.  Use StartThread instead (which doesn't wait around
   for the main thread to actually create the thread and return
   it). */

#ifdef NAMED_SYNCHRO
/* Logical OR of all classes we want to track. */
#define TRACK_CONTENTION_CLASSES (SYNC_CLASS_TOPLEVEL)

/* Prototypes with the "name" field */

Thread CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize, const char *name);
void StartThread_Core (ThreadFunction func, void *data, SDWORD stackSize, const char *name);
Semaphore CreateSemaphore_Core (DWORD initial, const char *name, DWORD syncClass);
Mutex CreateMutex_Core (const char *name, DWORD syncClass);
RecursiveMutex CreateRecursiveMutex_Core (const char *name, DWORD syncClass);
CondVar CreateCondVar_Core (const char *name, DWORD syncClass);

/* Preprocessor directives to forward to the appropriate routines */

#define CreateThread(func, data, stackSize, name) \
	CreateThread_Core ((func), (data), (stackSize), (name))
#define StartThread(func, data, stackSize, name) \
	StartThread_Core ((func), (data), (stackSize), (name))
#define CreateSemaphore(initial, name, syncClass) \
	CreateSemaphore_Core ((initial), (name), (syncClass))
#define CreateMutex(name, syncClass) \
	CreateMutex_Core ((name), (syncClass))
#define CreateRecursiveMutex(name, syncClass) \
	CreateRecursiveMutex_Core((name), (syncClass))
#define CreateCondVar(name, syncClass) \
	CreateCondVar_Core ((name), (syncClass))

#else

/* Prototypes without the "name" field. */
Thread CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize);
void StartThread_Core (ThreadFunction func, void *data, SDWORD stackSize);
Semaphore CreateSemaphore_Core (DWORD initial);
Mutex CreateMutex_Core (void);
RecursiveMutex CreateRecursiveMutex_Core (void);
CondVar CreateCondVar_Core (void);


/* Preprocessor directives to forward to the appropriate routines.
   The "name" field is stripped away in preprocessing. */

#define CreateThread(func, data, stackSize, name) \
	CreateThread_Core ((func), (data), (stackSize))
#define StartThread(func, data, stackSize, name) \
	StartThread_Core ((func), (data), (stackSize))
#define CreateSemaphore(initial, name, syncClass) \
	CreateSemaphore_Core ((initial))
#define CreateMutex(name, syncClass) \
	CreateMutex_Core ()
#define CreateRecursiveMutex(name, syncClass) \
	CreateRecursiveMutex_Core()
#define CreateCondVar(name, syncClass) \
	CreateCondVar_Core ()

#endif

ThreadLocal *CreateThreadLocal (void);
void DestroyThreadLocal (ThreadLocal *tl);
ThreadLocal *GetMyThreadLocal (void);

void HibernateThread (TimePeriod timePeriod);
void HibernateThreadUntil (TimeCount wakeTime);
void SleepThread (TimePeriod timePeriod);
void SleepThreadUntil (TimeCount wakeTime);
void DestroyThread (Thread);
void TaskSwitch (void);
void WaitThread (Thread thread, int *status);

void FinishThread (Thread);
void ProcessThreadLifecycles (void);

#ifdef PROFILE_THREADS
void PrintThreadsStats (void);
#endif  /* PROFILE_THREADS */


void DestroySemaphore (Semaphore sem);
void SetSemaphore (Semaphore sem);
void ClearSemaphore (Semaphore sem);

void DestroyMutex (Mutex sem);
void LockMutex (Mutex sem);
void UnlockMutex (Mutex sem);

void DestroyRecursiveMutex (RecursiveMutex m);
void LockRecursiveMutex (RecursiveMutex m);
void UnlockRecursiveMutex (RecursiveMutex m);
int  GetRecursiveMutexDepth (RecursiveMutex m);

void DestroyCondVar (CondVar);
void WaitCondVar (CondVar);
void SignalCondVar (CondVar);
void BroadcastCondVar (CondVar);

#if defined(__cplusplus)
}
#endif

#endif  /* LIBS_THREADLIB_H_ */

