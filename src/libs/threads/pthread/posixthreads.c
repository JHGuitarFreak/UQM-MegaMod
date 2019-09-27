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

#include <stdlib.h>
#include "libs/misc.h"
#include "libs/memlib.h"
#include "posixthreads.h"
#include <pthread.h>
#include <unistd.h>

#include <semaphore.h>

#include "libs/log/uqmlog.h"

typedef struct _thread {
	pthread_t native;
#ifdef NAMED_SYNCHRO
	const char *name;
#endif
	ThreadLocal *localData;
	struct _thread *next;
} *TrueThread;

static volatile TrueThread threadQueue = NULL;
static pthread_mutex_t threadQueueMutex;

struct ThreadStartInfo
{
	ThreadFunction func;
	void *data;
	sem_t sem;
	TrueThread thread;
};

void
InitThreadSystem_PT (void)
{
	pthread_mutex_init (&threadQueueMutex, NULL);
}

void
UnInitThreadSystem_PT (void)
{
	pthread_mutex_destroy (&threadQueueMutex);
}

static void
QueueThread (TrueThread thread)
{
	pthread_mutex_lock (&threadQueueMutex);
	thread->next = threadQueue;
	threadQueue = thread;
	pthread_mutex_unlock (&threadQueueMutex);
}

static void
UnQueueThread (TrueThread thread)
{
	volatile TrueThread *ptr;

	pthread_mutex_lock (&threadQueueMutex);
	ptr = &threadQueue;
	while (*ptr != thread)
	{
#ifdef DEBUG_THREADS
		if (*ptr == NULL)
		{
			// Should not happen.
			log_add (log_Debug, "Error: Trying to remove non-present thread "
					"from thread queue.");
			fflush (stderr);
			explode ();
		}
#endif  /* DEBUG_THREADS */
		ptr = &(*ptr)->next;
	}
	*ptr = (*ptr)->next;
	pthread_mutex_unlock (&threadQueueMutex);
}

static TrueThread
FindThreadInfo (pthread_t threadID)
{
	TrueThread ptr;

	pthread_mutex_lock (&threadQueueMutex);
	ptr = threadQueue;
	while (ptr)
	{
		if (ptr->native == threadID)
		{
			pthread_mutex_unlock (&threadQueueMutex);
			return ptr;
		}
		ptr = ptr->next;
	}
	pthread_mutex_unlock (&threadQueueMutex);
	return NULL;
}

#ifdef NAMED_SYNCHRO
static const char *
MyThreadName (void)
{
	TrueThread t = FindThreadInfo (pthread_self());
	return t ? t->name : "Unknown (probably renderer)";
}
#endif

static void *
ThreadHelper (void *startInfo) {
	ThreadFunction func;
	void *data;
	sem_t *sem;
	TrueThread thread;
	int result;
	
	//log_add (log_Debug, "ThreadHelper()");
	
	func = ((struct ThreadStartInfo *) startInfo)->func;
	data = ((struct ThreadStartInfo *) startInfo)->data;
	sem  = &((struct ThreadStartInfo *) startInfo)->sem;

	// Wait until the Thread structure is available.
	if (sem_wait (sem))
	{
		log_add(log_Fatal, "ThreadHelper sem_wait fail");
		exit(EXIT_FAILURE);
	}
	if (sem_destroy (sem))
	{
		log_add(log_Fatal, "ThreadHelper sem_destroy fail");
		exit(EXIT_FAILURE);
	}
	
	thread = ((struct ThreadStartInfo *) startInfo)->thread;
	HFree (startInfo);

	result = (*func) (data);

#ifdef DEBUG_THREADS
	log_add (log_Debug, "Thread '%s' done (returned %d).",
			thread->name, result);
	fflush (stderr);
#endif

	UnQueueThread (thread);
	DestroyThreadLocal (thread->localData);
	FinishThread (thread);
	/* Destroying the thread is the responsibility of ProcessThreadLifecycles() */
	return (void*)result;
}

void
DestroyThread_PT (Thread t)
{
	HFree (t);
}	

Thread
CreateThread_PT (ThreadFunction func, void *data, SDWORD stackSize
#ifdef NAMED_SYNCHRO
		  , const char *name
#endif
	)
{
	TrueThread thread;
	struct ThreadStartInfo *startInfo;
	pthread_attr_t attr;

	
	//log_add (log_Debug, "CreateThread_PT '%s'", name);
	
	thread = (struct _thread *) HMalloc (sizeof *thread);
#ifdef NAMED_SYNCHRO
	thread->name = name;
#endif

	thread->localData = CreateThreadLocal ();

	startInfo = (struct ThreadStartInfo *) HMalloc (sizeof (*startInfo));
	startInfo->func = func;
	startInfo->data = data;
	if (sem_init(&startInfo->sem, 0, 0) < 0)
	{
		log_add (log_Fatal, "createthread seminit fail");
		exit(EXIT_FAILURE);
	}
	startInfo->thread = thread;
		
	pthread_attr_init(&attr);
 	if (pthread_attr_setstacksize(&attr, 75000))
 	{
 		log_add (log_Debug, "pthread stacksize fail");
 	}
	if (pthread_create(&thread->native, &attr, ThreadHelper, (void *)startInfo))
	{
		log_add (log_Debug, "pthread create fail");
		DestroyThreadLocal (thread->localData);
		HFree (startInfo);
		HFree (thread);
		return NULL;
	}
	// The responsibility to free 'startInfo' and 'thread' is now by the new
	// thread.
	
	QueueThread (thread);

#ifdef DEBUG_THREADS
//#if 0	
	log_add (log_Debug, "Thread '%s' created.", thread->name);
	fflush (stderr);
//#endif
#endif

	// Signal to the new thread that the thread structure is ready
	// and it can begin to use it.
	if (sem_post (&startInfo->sem))
	{
		log_add(log_Fatal, "CreateThread sem_post fail");
		exit(EXIT_FAILURE);
	}

	(void) stackSize;  /* Satisfying compiler (unused parameter) */
	return thread;
}

void
SleepThread_PT (TimeCount sleepTime)
{
	usleep (sleepTime * 1000000 / ONE_SECOND);
}

void
SleepThreadUntil_PT (TimeCount wakeTime) {
	TimeCount now;

	now = GetTimeCounter ();
	if (wakeTime <= now)
		TaskSwitch_PT ();
	else
		usleep ((wakeTime - now) * 1000000 / ONE_SECOND);
}

void
TaskSwitch_PT (void) {
	usleep (1000);
}

void
WaitThread_PT (Thread thread, int *status) {
	//log_add(log_Debug, "WaitThread_PT '%s', status %x", ((TrueThread)thread)->name, status);
	//pthread_join(((TrueThread)thread)->native, status);
	pthread_join(((TrueThread)thread)->native, NULL);
	//log_add(log_Debug, "WaitThread_PT '%s' complete", ((TrueThread)thread)->name);
}

ThreadLocal *
GetMyThreadLocal_PT (void)
{
	TrueThread t = FindThreadInfo (pthread_self());
	return t ? t->localData : NULL;
}

/* These are the pthread implementations of the UQM synchronization objects. */

/* Mutexes. */
/* TODO.  The w_memlib uses Mutexes right now, so we can't use HMalloc
 * or HFree. Once that goes, this needs to change. */

typedef struct _mutex {
	pthread_mutex_t mutex;
#ifdef TRACK_CONTENTION
	pthread_t owner;
#endif
#ifdef NAMED_SYNCHRO
	const char *name;
	DWORD syncClass;
#endif
} Mut;
	

Mutex
#ifdef NAMED_SYNCHRO
CreateMutex_PT (const char *name, DWORD syncClass)
#else
CreateMutex_PT (void)
#endif
{
	Mut *mutex = malloc (sizeof (Mut));
	
	if (mutex != NULL)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		if (pthread_mutex_init(&mutex->mutex, &attr))
		{
#ifdef NAMED_SYNCHRO
			/* logging depends on Mutexes, so we have to use the
			 * non-threaded version instead */
			log_add_nothread (log_Fatal, "Could not initialize mutex '%s':"
				"aborting.", name);
#else
			log_add_nothread (log_Fatal, "Could not initialize mutex:"
					"aborting.");
#endif
			exit (EXIT_FAILURE);			
		}
		pthread_mutexattr_destroy(&attr);
		 
#ifdef TRACK_CONTENTION
		mutex->owner = 0;
#endif
#ifdef NAMED_SYNCHRO
		mutex->name = name;
		mutex->syncClass = syncClass;
#endif
	}

	return mutex;
}

void
DestroyMutex_PT (Mutex m)
{
	Mut *mutex = (Mut *)m;
	//log_add_nothread(log_Debug, "Destroying mutex '%s'", mutex->name);
	pthread_mutex_destroy (&mutex->mutex);
	free (mutex);
}

void
LockMutex_PT (Mutex m)
{
	Mut *mutex = (Mut *)m;
#ifdef TRACK_CONTENTION
	/* This code isn't really quite right; race conditions between
	 * check and lock remain and can produce reports of contention
	 * where the thread never sleeps, or fail to report in
	 * situations where it does.  If tracking with perfect
	 * accuracy becomes important, the TRACK_CONTENTION mutex will
	 * need to handle its own wake/sleep cycles with condition
	 * variables (check the history of this file for the
	 * CrossThreadMutex code).  This almost-measure is being added
	 * because for the most part it should suffice. */
	if (mutex->owner && (mutex->syncClass & TRACK_CONTENTION_CLASSES))
	{	/* logging depends on Mutexes, so we have to use the
		 * non-threaded version instead */
		log_add_nothread (log_Debug, "Thread '%s' blocking on mutex '%s'",
				MyThreadName (), mutex->name);
	}
#endif

	while (pthread_mutex_lock (&mutex->mutex) != 0)
	{		
		//log_add_nothread (log_Debug, "Attempt to acquire mutex '%s' failretry", mutex->name);
		TaskSwitch_PT ();
	}
#ifdef TRACK_CONTENTION
	mutex->owner = pthread_self();
#endif
}

void
UnlockMutex_PT (Mutex m)
{
	Mut *mutex = (Mut *)m;
#ifdef TRACK_CONTENTION
	mutex->owner = 0;
#endif
	while (pthread_mutex_unlock (&mutex->mutex) != 0)
	{
		TaskSwitch_PT ();
	}
}

/* Semaphores. */

typedef struct _sem {
	sem_t sem;
#ifdef NAMED_SYNCHRO
	const char *name;
	DWORD syncClass;
#endif
} Sem;

Semaphore
CreateSemaphore_PT (DWORD initial
#ifdef NAMED_SYNCHRO
		  , const char *name, DWORD syncClass
#endif
	)
{
	Sem *sem = (Sem *) HMalloc (sizeof (struct _sem));
#ifdef NAMED_SYNCHRO
	sem->name = name;
	sem->syncClass = syncClass;
#endif
	
	//log_add (log_Debug, "Creating semaphore '%s'", sem->name);
	
	if (sem_init(&sem->sem, 0, initial) < 0)
	{
#ifdef NAMED_SYNCHRO
		log_add (log_Fatal, "Could not initialize semaphore '%s':"
				" aborting.", name);
#else
		log_add (log_Fatal, "Could not initialize semaphore:"
				" aborting.");
#endif
		exit (EXIT_FAILURE);
	}
	//log_add (log_Debug, "Creating semaphore '%s' success", sem->name);
	return sem;
}

void
DestroySemaphore_PT (Semaphore s)
{
	Sem *sem = (Sem *)s;
	//log_add (log_Debug, "Destroying semaphore '%s'", sem->name);
	if (sem_destroy (&sem->sem))
	{
		log_add (log_Debug, "Destroying semaphore '%s' failed", sem->name);
	}
	HFree (sem);
}

void
SetSemaphore_PT (Semaphore s)
{
	Sem *sem = (Sem *)s;
#ifdef TRACK_CONTENTION
	int contention = 0;
	sem_getvalue(&sem->sem, &contention);
	contention = !contention;
	if (contention && (sem->syncClass & TRACK_CONTENTION_CLASSES))
	{
		log_add (log_Debug, "Thread '%s' blocking on semaphore '%s'",
				MyThreadName (), sem->name);
	}
#endif
	//log_add (log_Debug, "Attempt to set semaphore '%s'", sem->name);
	while (sem_wait (&sem->sem) == -1)
	{
		//log_add (log_Debug, "Attempt to set semaphore '%s' failretry", sem->name);
		TaskSwitch_PT ();
	}
	//log_add (log_Debug, "Attempt to set semaphore '%s' success", sem->name);
#ifdef TRACK_CONTENTION
	if (contention && (sem->syncClass & TRACK_CONTENTION_CLASSES))
	{
		log_add (log_Debug, "Thread '%s' awakens,"
				" released from semaphore '%s'", MyThreadName (), sem->name);
	}
#endif
}

void
ClearSemaphore_PT (Semaphore s)
{
	Sem *sem = (Sem *)s;
	//log_add (log_Debug, "Attempt to clear semaphore '%s' %x", sem->name, sem);
	while (sem_post (&sem->sem) == -1)
	{
		//log_add (log_Debug, "Attempt to clear semaphore %x failretry", sem);
		TaskSwitch_PT ();
	}
	//log_add (log_Debug, "Attempt to clear semaphore %x success", sem);
}

/* Recursive mutexes. Adapted from mixSDL code, which was adapted from
   the original DCQ code. */

typedef struct _recm {
	pthread_mutex_t mutex;
	pthread_t thread_id;
	unsigned int locks;
#ifdef NAMED_SYNCHRO
	const char *name;
	DWORD syncClass;
#endif
} RecM;

RecursiveMutex
#ifdef NAMED_SYNCHRO
CreateRecursiveMutex_PT (const char *name, DWORD syncClass)
#else
CreateRecursiveMutex_PT (void)
#endif
{
	RecM *mtx = (RecM *) HMalloc (sizeof (struct _recm));

	mtx->thread_id = 0;
	if (pthread_mutex_init(&mtx->mutex, NULL))
	{
#ifdef NAMED_SYNCHRO
		log_add (log_Fatal, "Could not initialize recursive "
				"mutex '%s': aborting.", name);
#else
		log_add (log_Fatal, "Could not initialize recursive "
				"mutex: aborting.");
#endif
		exit (EXIT_FAILURE);
	}
#ifdef NAMED_SYNCHRO
	mtx->name = name;
	mtx->syncClass = syncClass;
#endif
	mtx->locks = 0;
	return (RecursiveMutex) mtx;
}

void
DestroyRecursiveMutex_PT (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	pthread_mutex_destroy(&mtx->mutex);
	HFree (mtx);
}

void
LockRecursiveMutex_PT (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	pthread_t thread_id = pthread_self();
	if (!mtx->locks || mtx->thread_id != thread_id)
	{
#ifdef TRACK_CONTENTION
		if (mtx->thread_id && (mtx->syncClass & TRACK_CONTENTION_CLASSES))
		{
			log_add (log_Debug, "Thread '%s' blocking on '%s'",
					MyThreadName (), mtx->name);
		}
#endif
		while (pthread_mutex_lock (&mtx->mutex))
			TaskSwitch_PT ();
		mtx->thread_id = thread_id;
	}
	mtx->locks++;
}

void
UnlockRecursiveMutex_PT (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	pthread_t thread_id = pthread_self();
	if (!mtx->locks || mtx->thread_id != thread_id)
	{
#ifdef NAMED_SYNCHRO
		log_add (log_Debug, "'%s' attempted to unlock %s when it "
				"didn't hold it", MyThreadName (), mtx->name);
#endif
	}
	else
	{
		mtx->locks--;
		if (!mtx->locks)
		{
			mtx->thread_id = 0;
			pthread_mutex_unlock (&mtx->mutex);
		}
	}
}

int
GetRecursiveMutexDepth_PT (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	return mtx->locks;
}

typedef struct _cond {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
#ifdef NAMED_SYNCHRO
	const char *name;
	DWORD syncClass;
#endif
} cvar;

CondVar
#ifdef NAMED_SYNCHRO
CreateCondVar_PT (const char *name, DWORD syncClass)
#else
CreateCondVar_PT (void)
#endif
{
	int err1, err2;
	cvar *cv = (cvar *) HMalloc (sizeof (cvar));
	err1 = pthread_cond_init(&cv->cond, NULL);
	err2 = pthread_mutex_init(&cv->mutex, NULL);
	if (err1 || err2)
	{
#ifdef NAMED_SYNCHRO
		log_add (log_Fatal, "Could not initialize condition variable '%s':"
				" aborting.", name);
#else
		log_add (log_Fatal, "Could not initialize condition variable:"
				" aborting.");
#endif
		exit (EXIT_FAILURE);
	}
#ifdef NAMED_SYNCHRO
	cv->name = name;
	cv->syncClass = syncClass;
#endif
	return cv;
}

void
DestroyCondVar_PT (CondVar c)
{
	cvar *cv = (cvar *) c;
	pthread_cond_destroy(&cv->cond);
	pthread_mutex_destroy(&cv->mutex);
	HFree (cv);
}

void
WaitCondVar_PT (CondVar c)
{
	cvar *cv = (cvar *) c;
	pthread_mutex_lock (&cv->mutex);
#ifdef TRACK_CONTENTION
	if (cv->syncClass & TRACK_CONTENTION_CLASSES)
	{
		log_add (log_Debug, "Thread '%s' waiting for signal from '%s'",
				MyThreadName (), cv->name);
	}
#endif
	while (pthread_cond_wait (&cv->cond, &cv->mutex) != 0)
	{
		TaskSwitch_PT ();
	}
#ifdef TRACK_CONTENTION
	if (cv->syncClass & TRACK_CONTENTION_CLASSES)
	{
		log_add (log_Debug, "Thread '%s' received signal from '%s',"
				" awakening.", MyThreadName (), cv->name);
	}
#endif
	pthread_mutex_unlock (&cv->mutex);
}

void
SignalCondVar_PT (CondVar c)
{
	cvar *cv = (cvar *) c;
	pthread_cond_signal(&cv->cond);
}

void
BroadcastCondVar_PT (CondVar c)
{
	cvar *cv = (cvar *) c;
	pthread_cond_broadcast(&cv->cond);
}
