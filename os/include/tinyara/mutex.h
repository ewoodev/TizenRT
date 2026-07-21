/****************************************************************************
 *
 * Copyright 2026 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * include/tinyara/mutex.h
 *
 * Based on NuttX include/nuttx/mutex.h
 *
 *   Copyright (C) 2018 Xiaomi Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_TINYARA_MUTEX_H
#define __INCLUDE_TINYARA_MUTEX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <sys/types.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <semaphore.h>
#include <tinyara/semaphore.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The holder value when a mutex is not taken */

#define NXMUTEX_NO_HOLDER      ((pid_t)-1)

/* Static initializers.
 *
 * NOTE : These should NOT be used in kernel space because they do not call
 * sem_init().  In app separation, all kernel semaphores are registered to
 * a list in sem_init() and recovered when a fault occurs, so kernel mutexes
 * must be initialized by nxmutex_init() for recovery.
 */

#define NXMUTEX_INITIALIZER    {MUTEX_SEM_INITIALIZER(1), NXMUTEX_NO_HOLDER}
#define NXRMUTEX_INITIALIZER   {NXMUTEX_INITIALIZER, 0}

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* A mutex is a semaphore with ownership: it is acquired and released by
 * the same task, always with an initial count of one.  Unlike a plain
 * semaphore, the kernel knows exactly which task holds it, so misuse
 * (release by a non-holder) is detectable and priority inheritance has a
 * well-defined boost target.
 */

struct mutex_s {
	sem_t sem;                  /* Underlying semaphore */
	pid_t holder;               /* PID of the current holder, or NXMUTEX_NO_HOLDER */
};

typedef struct mutex_s mutex_t;

/* A recursive mutex may be locked repeatedly by the holder.  It is
 * released only when unlock has been called the same number of times.
 */

struct rmutex_s {
	mutex_t mutex;              /* Underlying non-recursive mutex */
	unsigned int count;         /* Current recursion depth */
};

typedef struct rmutex_s rmutex_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: nxmutex_init
 *
 * Description:
 *   Initialize the mutex to the unlocked state.
 *
 * Parameters:
 *   mutex - Mutex to be initialized
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxmutex_init(FAR mutex_t *mutex)
{
	int ret;

	ret = sem_init(&mutex->sem, 0, 1);
	if (ret != OK) {
		return ret;
	}

	mutex->sem.flags |= FLAGS_SEM_MUTEX;
	mutex->holder = NXMUTEX_NO_HOLDER;

#ifdef CONFIG_PRIORITY_INHERITANCE
	/* A mutex always has exactly one well-defined holder, so it is the
	 * one primitive where priority inheritance is meaningful.  Opt in
	 * explicitly so that this keeps working when semaphore PI tracking
	 * becomes opt-in.
	 */

	ret = sem_setprotocol(&mutex->sem, SEM_PRIO_INHERIT);
#endif

	return ret;
}

/****************************************************************************
 * Name: nxmutex_destroy
 *
 * Description:
 *   Destroy the mutex.
 *
 * Parameters:
 *   mutex - Mutex to be destroyed
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxmutex_destroy(FAR mutex_t *mutex)
{
	mutex->holder = NXMUTEX_NO_HOLDER;
	return sem_destroy(&mutex->sem);
}

/****************************************************************************
 * Name: nxmutex_is_hold
 *
 * Description:
 *   Return true if the calling task holds the mutex.
 *
 * Parameters:
 *   mutex - Mutex to be tested
 *
 ****************************************************************************/

static inline bool nxmutex_is_hold(FAR mutex_t *mutex)
{
	return mutex->holder == getpid();
}

/****************************************************************************
 * Name: nxmutex_is_locked
 *
 * Description:
 *   Return true if some task holds the mutex.
 *
 * Parameters:
 *   mutex - Mutex to be tested
 *
 ****************************************************************************/

static inline bool nxmutex_is_locked(FAR mutex_t *mutex)
{
	return mutex->holder != NXMUTEX_NO_HOLDER;
}

/****************************************************************************
 * Name: nxmutex_lock
 *
 * Description:
 *   Acquire the mutex.  This function may block until the holder releases
 *   the mutex.  Unlike sem_wait(), it is not a cancellation point and it
 *   restarts automatically when interrupted by a signal, so the caller
 *   does not need an EINTR retry loop.
 *
 * Parameters:
 *   mutex - Mutex to be acquired
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxmutex_lock(FAR mutex_t *mutex)
{
	/* The calling task must not already be the holder: a plain mutex
	 * is not recursive, so locking it again would self-deadlock.
	 */

	DEBUGASSERT(!nxmutex_is_hold(mutex));

	while (sem_wait(&mutex->sem) != OK) {
		/* EINTR is the only recoverable failure.  Note that -1 is used
		 * instead of the ERROR macro because ERROR is not defined for
		 * C++ (see sys/types.h) and this header is included from C++.
		 */

		DEBUGASSERT(get_errno() == EINTR);
		if (get_errno() != EINTR) {
			return -1;
		}
	}

	mutex->holder = getpid();
	return OK;
}

/****************************************************************************
 * Name: nxmutex_trylock
 *
 * Description:
 *   Try to acquire the mutex without blocking.
 *
 * Parameters:
 *   mutex - Mutex to be acquired
 *
 * Return Value:
 *   OK on success; ERROR if the mutex is already locked or on failure,
 *   with the errno value set (EAGAIN when already locked).
 *
 ****************************************************************************/

static inline int nxmutex_trylock(FAR mutex_t *mutex)
{
	if (sem_trywait(&mutex->sem) != OK) {
		/* -1 instead of the ERROR macro: ERROR is not defined for C++ */

		return -1;
	}

	mutex->holder = getpid();
	return OK;
}

/****************************************************************************
 * Name: nxmutex_unlock
 *
 * Description:
 *   Release the mutex.  Only the task that acquired the mutex may release
 *   it; releasing from another task or from an interrupt handler is a
 *   usage error that is detected in debug builds.
 *
 * Parameters:
 *   mutex - Mutex to be released
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxmutex_unlock(FAR mutex_t *mutex)
{
	/* A mutex has ownership: release by a task that is not the holder
	 * indicates a locking discipline violation.
	 */

	DEBUGASSERT(nxmutex_is_hold(mutex));

	mutex->holder = NXMUTEX_NO_HOLDER;
	return sem_post(&mutex->sem);
}

/****************************************************************************
 * Name: nxrmutex_init
 *
 * Description:
 *   Initialize the recursive mutex to the unlocked state.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be initialized
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxrmutex_init(FAR rmutex_t *rmutex)
{
	rmutex->count = 0;
	return nxmutex_init(&rmutex->mutex);
}

/****************************************************************************
 * Name: nxrmutex_destroy
 *
 * Description:
 *   Destroy the recursive mutex.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be destroyed
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxrmutex_destroy(FAR rmutex_t *rmutex)
{
	rmutex->count = 0;
	return nxmutex_destroy(&rmutex->mutex);
}

/****************************************************************************
 * Name: nxrmutex_is_hold
 *
 * Description:
 *   Return true if the calling task holds the recursive mutex.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be tested
 *
 ****************************************************************************/

static inline bool nxrmutex_is_hold(FAR rmutex_t *rmutex)
{
	return nxmutex_is_hold(&rmutex->mutex);
}

/****************************************************************************
 * Name: nxrmutex_lock
 *
 * Description:
 *   Acquire the recursive mutex.  The holder may lock it again; each
 *   nested lock must be paired with an unlock.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be acquired
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxrmutex_lock(FAR rmutex_t *rmutex)
{
	int ret = OK;

	if (!nxrmutex_is_hold(rmutex)) {
		ret = nxmutex_lock(&rmutex->mutex);
	}

	if (ret == OK) {
		DEBUGASSERT(rmutex->count < UINT_MAX);
		rmutex->count++;
	}

	return ret;
}

/****************************************************************************
 * Name: nxrmutex_trylock
 *
 * Description:
 *   Try to acquire the recursive mutex without blocking.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be acquired
 *
 * Return Value:
 *   OK on success; ERROR if the mutex is held by another task or on
 *   failure, with the errno value set.
 *
 ****************************************************************************/

static inline int nxrmutex_trylock(FAR rmutex_t *rmutex)
{
	int ret = OK;

	if (!nxrmutex_is_hold(rmutex)) {
		ret = nxmutex_trylock(&rmutex->mutex);
	}

	if (ret == OK) {
		DEBUGASSERT(rmutex->count < UINT_MAX);
		rmutex->count++;
	}

	return ret;
}

/****************************************************************************
 * Name: nxrmutex_unlock
 *
 * Description:
 *   Release one level of the recursive mutex.  The mutex is released to
 *   other tasks when the recursion count returns to zero.
 *
 * Parameters:
 *   rmutex - Recursive mutex to be released
 *
 * Return Value:
 *   OK on success; ERROR on failure with the errno value set.
 *
 ****************************************************************************/

static inline int nxrmutex_unlock(FAR rmutex_t *rmutex)
{
	int ret = OK;

	DEBUGASSERT(nxrmutex_is_hold(rmutex));
	DEBUGASSERT(rmutex->count > 0);

	if (--rmutex->count == 0) {
		ret = nxmutex_unlock(&rmutex->mutex);
	}

	return ret;
}

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* __INCLUDE_TINYARA_MUTEX_H */
