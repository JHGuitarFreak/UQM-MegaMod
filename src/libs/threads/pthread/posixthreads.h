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

#ifndef LIBS_THREADS_PTHREAD_POSIXTHREADS_H_
#define LIBS_THREADS_PTHREAD_POSIXTHREADS_H_

#include "port.h"
#include "libs/threadlib.h"

void InitThreadSystem_PT (void);
void UnInitThreadSystem_PT (void);

#ifdef NAMED_SYNCHRO
/* Prototypes with the "name" field */
Thread CreateThread_PT (ThreadFunction func, void *data, SDWORD stackSize, const char *name);
Mutex CreateMutex_PT (const char *name, DWORD syncClass);
Semaphore CreateSemaphore_PT (DWORD initial, const char *name, DWORD syncClass);
RecursiveMutex CreateRecursiveMutex_PT (const char *name, DWORD syncClass);
CondVar CreateCondVar_PT (const char *name, DWORD syncClass);
#else
/* Prototypes without the "name" field. */
Thread CreateThread_PT (ThreadFunction func, void *data, SDWORD stackSize);
Mutex CreateMutex_PT (void);
Semaphore CreateSemaphore_PT (DWORD initial);
RecursiveMutex CreateRecursiveMutex_PT (void);
CondVar CreateCondVar_PT (void);
#endif

ThreadLocal *GetMyThreadLocal_PT (void);

void SleepThread_PT (TimeCount sleepTime);
void SleepThreadUntil_PT (TimeCount wakeTime);
void TaskSwitch_PT (void);
void WaitThread_PT (Thread thread, int *status);
void DestroyThread_PT (Thread thread);

void DestroyMutex_PT (Mutex m);
void LockMutex_PT (Mutex m);
void UnlockMutex_PT (Mutex m);

void DestroySemaphore_PT (Semaphore sem);
void SetSemaphore_PT (Semaphore sem);
void ClearSemaphore_PT (Semaphore sem);

void DestroyCondVar_PT (CondVar c);
void WaitCondVar_PT (CondVar c);
void SignalCondVar_PT (CondVar c);
void BroadcastCondVar_PT (CondVar c);

void DestroyRecursiveMutex_PT (RecursiveMutex m);
void LockRecursiveMutex_PT (RecursiveMutex m);
void UnlockRecursiveMutex_PT (RecursiveMutex m);
int  GetRecursiveMutexDepth_PT (RecursiveMutex m);

#define NativeInitThreadSystem InitThreadSystem_PT
#define NativeUnInitThreadSystem UnInitThreadSystem_PT

#define NativeGetMyThreadLocal GetMyThreadLocal_PT

#define NativeCreateThread CreateThread_PT
#define NativeSleepThread SleepThread_PT
#define NativeSleepThreadUntil SleepThreadUntil_PT
#define NativeTaskSwitch TaskSwitch_PT
#define NativeWaitThread WaitThread_PT
#define NativeDestroyThread DestroyThread_PT

#define NativeCreateMutex CreateMutex_PT
#define NativeDestroyMutex DestroyMutex_PT
#define NativeLockMutex LockMutex_PT
#define NativeUnlockMutex UnlockMutex_PT

#define NativeCreateSemaphore CreateSemaphore_PT
#define NativeDestroySemaphore DestroySemaphore_PT
#define NativeSetSemaphore SetSemaphore_PT
#define NativeClearSemaphore ClearSemaphore_PT

#define NativeCreateCondVar CreateCondVar_PT
#define NativeDestroyCondVar DestroyCondVar_PT
#define NativeWaitCondVar WaitCondVar_PT
#define NativeSignalCondVar SignalCondVar_PT
#define NativeBroadcastCondVar BroadcastCondVar_PT

#define NativeCreateRecursiveMutex CreateRecursiveMutex_PT
#define NativeDestroyRecursiveMutex DestroyRecursiveMutex_PT
#define NativeLockRecursiveMutex LockRecursiveMutex_PT
#define NativeUnlockRecursiveMutex UnlockRecursiveMutex_PT
#define NativeGetRecursiveMutexDepth GetRecursiveMutexDepth_PT

#endif  /* _PTTHREAD_H */

