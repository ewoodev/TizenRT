/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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
 * include/semaphore.h
 *
 *   Copyright (C) 2007-2009, 2012-2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/**
 * @defgroup SEMAPHORE_KERNEL SEMAPHORE
 * @brief Provides APIs for Semaphore
 * @ingroup KERNEL
 */

/// @file semaphore.h
/// @brief Semaphore APIs

#ifndef __INCLUDE_SEMAPHORE_H
#define __INCLUDE_SEMAPHORE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <limits.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Save semaphore holder data when priority inheritance or binary manager is enabled. */
#if defined(CONFIG_PRIORITY_INHERITANCE) || defined(CONFIG_BINARY_MANAGER)
#define SAVE_SEM_HOLDER 1
#endif

/* Bit definitions for the struct sem_s flags field */

#define PRIOINHERIT_FLAGS_DISABLE (1 << 0) /* Bit 0: Priority inheritance
					    * is disabled for this semaphore */
#define FLAGS_INITIALIZED         (1 << 1) /* Bit 1: This semaphore initialized */
#define FLAGS_SIGSEM              (1 << 2) /* Bit 2: The semaphore for signaling */
#define FLAGS_SEM_MUTEX		  (1 << 3) /* Bit 3: The semaphore is used to implement mutex */

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

/* This structure contains information about the holder of a semaphore */

#ifdef SAVE_SEM_HOLDER
struct tcb_s;					/* Forward reference */
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Structure of semholder
 */
struct semholder_s {
#if CONFIG_SEM_PREALLOCHOLDERS > 0
	struct semholder_s *flink;	/* Implements singly linked list */
#endif
	FAR struct tcb_s *htcb;		/* Holder TCB */
	int16_t counts;				/* Number of counts owned by this holder */
};

#if CONFIG_SEM_PREALLOCHOLDERS > 0
#define SEMHOLDER_INITIALIZER {NULL, NULL, 0}
#else
#define SEMHOLDER_INITIALIZER {NULL, 0}
#endif
#endif							/* SAVE_SEM_HOLDER */

/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Structure of generic semaphore
 */
struct sem_s {
#ifdef CONFIG_BINARY_MANAGER
	struct sem_s *flink;		/* Support for singly linked lists. */
#endif
	int16_t semcount;			/* >0 -> Num counts available */
	/* <0 -> Num tasks waiting for semaphore */
	/* If priority inheritance is enabled, then we have to keep track of which
	 * tasks hold references to the semaphore.
	 */

	uint8_t flags;			/* See definitions for the struct sem_s flags */
#ifdef SAVE_SEM_HOLDER
#if CONFIG_SEM_PREALLOCHOLDERS > 0
	FAR struct semholder_s *hhead;	/* List of holders of semaphore counts */
#else
	struct semholder_s holder;	/* Single holder */
#endif
#endif
};

typedef struct sem_s sem_t;

/* Initializers */
/* NOTE : It should NOT be used in kernel space because it doesn't call sem_init.
 * In app separtion, all kernel semaphores are registered to a list in sem_init and recoverd when fault occurs.
 * So they should be initialized by sem_init for recovery.
*/

/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Sem initializer
 */
#ifdef SAVE_SEM_HOLDER
#ifdef CONFIG_BINARY_MANAGER
#if CONFIG_SEM_PREALLOCHOLDERS > 0
#define SEM_INITIALIZER(c) {NULL, (c), FLAGS_INITIALIZED, NULL} /* flink, semcount, flags, hhead */
#define MUTEX_SEM_INITIALIZER(c) {NULL, (c), FLAGS_INITIALIZED | FLAGS_SEM_MUTEX, NULL} /* flink, semcount, flags, hhead */
#else
#define SEM_INITIALIZER(c) {NULL, (c), FLAGS_INITIALIZED, SEMHOLDER_INITIALIZER} /* flink, semcount, flags, holder */
#define MUTEX_SEM_INITIALIZER(c) {NULL, (c), FLAGS_INITIALIZED | FLAGS_SEM_MUTEX, SEMHOLDER_INITIALIZER} /* flink, semcount, flags, holder */
#endif
#else // CONFIG_BINARY_MANAGER
#if CONFIG_SEM_PREALLOCHOLDERS > 0
#define SEM_INITIALIZER(c) {(c), FLAGS_INITIALIZED, NULL} /* semcount, flags, hhead */
#define MUTEX_SEM_INITIALIZER(c) {(c), FLAGS_INITIALIZED | FLAGS_SEM_MUTEX, NULL} /* semcount, flags, hhead */
#else
#define SEM_INITIALIZER(c) {(c), FLAGS_INITIALIZED, SEMHOLDER_INITIALIZER} /* semcount, flags, holder */
#define MUTEX_SEM_INITIALIZER(c) {(c), FLAGS_INITIALIZED | FLAGS_SEM_MUTEX, SEMHOLDER_INITIALIZER} /* semcount, flags, holder */
#endif
#endif
#else
#define SEM_INITIALIZER(c) {(c), FLAGS_INITIALIZED}	/* semcount, flags */
#define MUTEX_SEM_INITIALIZER(c) {(c), FLAGS_INITIALIZED | FLAGS_SEM_MUTEX} /* semcount, flags */
#endif

/****************************************************************************
 * Public Variables
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/* Forward references needed by some prototypes */

struct timespec;				/* Defined in time.h */

/* Counting Semaphore Interfaces (based on POSIX APIs) */
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Initialize an unnamed semaphore with an initial count.
 * @details @b #include <semaphore.h> \n
 * The current implementation accepts `pshared` for API compatibility but does
 * not use it when initializing the semaphore state.
 * @since TizenRT v1.0
 */
int sem_init(FAR sem_t *sem, int pshared, unsigned int value);

/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Destroy an unnamed semaphore.
 * @details @b #include <semaphore.h> \n
 * The current implementation clears the initialized flag and tears down holder
 * bookkeeping. It does not wake waiting threads before returning.
 * @since TizenRT v1.0
 */
int sem_destroy(FAR sem_t *sem);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Wait until a semaphore count becomes available.
 * @details @b #include <semaphore.h> \n
 * This call blocks the caller when the current count is zero or negative and
 * may return early when the wait is interrupted or canceled.
 * @since TizenRT v1.0
 */
int sem_wait(FAR sem_t *sem);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Wait for a semaphore until an absolute timeout expires.
 * @details @b #include <semaphore.h> \n
 * The timeout is interpreted against `CLOCK_REALTIME` and the implementation
 * first attempts a non-blocking acquire before arming a watchdog.
 * @since TizenRT v1.0
 */
int sem_timedwait(FAR sem_t *sem, FAR const struct timespec *abstime);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Try to acquire a semaphore without blocking.
 * @details @b #include <semaphore.h> \n
 * This call returns immediately with `EAGAIN` when no count is available.
 * @since TizenRT v1.0
 */
int sem_trywait(FAR sem_t *sem);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Release one semaphore count.
 * @details @b #include <semaphore.h> \n
 * The implementation increments the count and wakes one waiting thread when
 * present. This API may be called from interrupt context.
 * @since TizenRT v1.0
 */
int sem_post(FAR sem_t *sem);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Read the current internal semaphore count.
 * @details @b #include <semaphore.h> \n
 * The current implementation stores the raw `semcount` value in `sval`.
 * Negative values therefore reflect waiting threads rather than being
 * normalized to zero.
 * @since TizenRT v1.0
 */
int sem_getvalue(FAR sem_t *sem, FAR int *sval);

/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Timeout callback used by semaphore wait watchdogs.
 * @details This helper is the watchdog entry point used by `sem_timedwait()`
 * and `sem_tickwait()` to terminate a blocked wait with `ETIMEDOUT`.
 */
void sem_timeout(int argc, uint32_t pid);

#ifdef CONFIG_FS_NAMED_SEMAPHORES
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Open or create a named semaphore.
 * @details @b #include <semaphore.h> \n
 * The current implementation resolves the name under
 * `CONFIG_FS_NAMED_SEMPATH`, reuses the existing named semaphore when it is
 * already present, and ignores the `mode_t` creation argument. Callers must
 * provide a non-NULL name that fits in the fixed `MAX_SEMPATH` buffer once
 * the configured path prefix is added.
 */
FAR sem_t *sem_open(FAR const char *name, int oflag, ...);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Close one open reference to a named semaphore.
 * @details The named semaphore object is only destroyed after it has been
 * unlinked and the final open reference is closed.
 */
int sem_close(FAR sem_t *sem);
/**
 * @ingroup SEMAPHORE_KERNEL
 * @brief Remove a named semaphore from the namespace.
 * @details This call detaches the name immediately, but the underlying object
 * remains alive until the last open reference is closed. The current
 * implementation expects a non-NULL name and can fail before unlinking when
 * the path does not resolve to a named semaphore inode.
 */
int sem_unlink(FAR const char *name);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* __INCLUDE_SEMAPHORE_H */
