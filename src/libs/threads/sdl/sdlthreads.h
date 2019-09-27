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

#ifndef LIBS_THREADS_SDL_SDLTHREADS_H_
#define LIBS_THREADS_SDL_SDLTHREADS_H_

#include "port.h"
#include SDL_INCLUDE(SDL.h)
#include SDL_INCLUDE(SDL_thread.h)
#include "libs/threadlib.h"
#include "libs/timelib.h"

void InitThreadSystem_SDL (void);
void UnInitThreadSystem_SDL (void);

#ifdef NAMED_SYNCHRO
/* Prototypes with the "name" field */
Thread CreateThread_SDL (ThreadFunction func, void *data, SDWORD stackSize, const char *name);
Mutex CreateMutex_SDL (const char *name, DWORD syncClass);
Semaphore CreateSemaphore_SDL (DWORD initial, const char *name, DWORD syncClass);
RecursiveMutex CreateRecursiveMutex_SDL (const char *name, DWORD syncClass);
CondVar CreateCondVar_SDL (const char *name, DWORD syncClass);
#else
/* Prototypes without the "name" field. */
Thread CreateThread_SDL (ThreadFunction func, void *data, SDWORD stackSize);
Mutex CreateMutex_SDL (void);
Semaphore CreateSemaphore_SDL (DWORD initial);
RecursiveMutex CreateRecursiveMutex_SDL (void);
CondVar CreateCondVar_SDL (void);
#endif

ThreadLocal *GetMyThreadLocal_SDL (void);

void SleepThread_SDL (TimeCount sleepTime);
void SleepThreadUntil_SDL (TimeCount wakeTime);
void TaskSwitch_SDL (void);
void WaitThread_SDL (Thread thread, int *status);
void DestroyThread_SDL (Thread thread);

void DestroyMutex_SDL (Mutex m);
void LockMutex_SDL (Mutex m);
void UnlockMutex_SDL (Mutex m);

void DestroySemaphore_SDL (Semaphore sem);
void SetSemaphore_SDL (Semaphore sem);
void ClearSemaphore_SDL (Semaphore sem);

void DestroyCondVar_SDL (CondVar c);
void WaitCondVar_SDL (CondVar c);
void SignalCondVar_SDL (CondVar c);
void BroadcastCondVar_SDL (CondVar c);

void DestroyRecursiveMutex_SDL (RecursiveMutex m);
void LockRecursiveMutex_SDL (RecursiveMutex m);
void UnlockRecursiveMutex_SDL (RecursiveMutex m);
int  GetRecursiveMutexDepth_SDL (RecursiveMutex m);

#define NativeInitThreadSystem InitThreadSystem_SDL
#define NativeUnInitThreadSystem UnInitThreadSystem_SDL

#define NativeGetMyThreadLocal GetMyThreadLocal_SDL

#define NativeCreateThread CreateThread_SDL
#define NativeSleepThread SleepThread_SDL
#define NativeSleepThreadUntil SleepThreadUntil_SDL
#define NativeTaskSwitch TaskSwitch_SDL
#define NativeWaitThread WaitThread_SDL
#define NativeDestroyThread DestroyThread_SDL

#define NativeCreateMutex CreateMutex_SDL
#define NativeDestroyMutex DestroyMutex_SDL
#define NativeLockMutex LockMutex_SDL
#define NativeUnlockMutex UnlockMutex_SDL

#define NativeCreateSemaphore CreateSemaphore_SDL
#define NativeDestroySemaphore DestroySemaphore_SDL
#define NativeSetSemaphore SetSemaphore_SDL
#define NativeClearSemaphore ClearSemaphore_SDL

#define NativeCreateCondVar CreateCondVar_SDL
#define NativeDestroyCondVar DestroyCondVar_SDL
#define NativeWaitCondVar WaitCondVar_SDL
#define NativeSignalCondVar SignalCondVar_SDL
#define NativeBroadcastCondVar BroadcastCondVar_SDL

#define NativeCreateRecursiveMutex CreateRecursiveMutex_SDL
#define NativeDestroyRecursiveMutex DestroyRecursiveMutex_SDL
#define NativeLockRecursiveMutex LockRecursiveMutex_SDL
#define NativeUnlockRecursiveMutex UnlockRecursiveMutex_SDL
#define NativeGetRecursiveMutexDepth GetRecursiveMutexDepth_SDL

#endif  /* LIBS_THREADS_SDL_SDLTHREADS_H_ */

