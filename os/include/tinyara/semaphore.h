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
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#ifndef __INCLUDE_TINYARA_SEMAPHORE_H
#define __INCLUDE_TINYARA_SEMAPHORE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <semaphore.h>
#include <tinyara/fs/fs.h>
#include <tinyara/clock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Values for protocol attribute */

#define SEM_PRIO_NONE		0
#define SEM_PRIO_INHERIT	1
#define SEM_PRIO_PROTECT	2

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_FS_NAMED_SEMAPHORES
/* This is the named semaphore inode */

struct inode;
struct nsem_inode_s {
	/*
	 * This must be the first element of the structure. In sem_close()
	 * this structure must be cast compatible with sem_t.
	 */
	sem_t ns_sem;			/* The contained semaphore */

	/* Inode payload unique to named semaphores */

	FAR struct inode *ns_inode;	/* Containing inode */
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: sem_tickwait
 *
 * Description:
 *   This function is a lighter weight, RTOS-specific timeout wait helper.
 *   It first performs a non-blocking `sem_trywait()`, then waits for at most
 *   `delay` ticks after compensating for time already elapsed since `start`.
 *
 * Parameters:
 *   sem     - Semaphore object
 *   start   - Base system tick used to compensate for time already elapsed
 *             before the wait begins.
 *   delay   - Ticks to wait from `start` until the semaphore is posted.  If
 *             `delay` is zero, then this function is equivalent
 *             to sem_trywait().
 *
 * Return Value:
 *   Zero (OK) is returned on success.
 *   On failure, -1 (ERROR) is returned and the errno
 *   is set appropriately:
 *
 *   EINVAL    Invalid semaphore state propagated from `sem_trywait()` or
 *             `sem_wait()`.
 *   EAGAIN    No count was available when `delay` was zero.
 *   ECANCELED The wait was canceled while blocked in `sem_wait()`.
 *   ETIMEDOUT The semaphore could not be locked before the adjusted timeout
 *             expired.
 *   ENOMEM    Out of memory
 *
 ****************************************************************************/

int sem_tickwait(FAR sem_t *sem, clock_t start, uint32_t delay);

/****************************************************************************
 * Name: sem_reset
 *
 * Description:
 *   Reset a semaphore to a specific non-negative count.  If threads are
 *   already waiting, the implementation hands counts to waiters first by
 *   calling `sem_post()` before storing any remaining count.
 *
 * Parameters:
 *   sem   - Semaphore descriptor to be reset
 *   count - The requested semaphore count
 *
 * Return Value:
 *   0 (OK) on success. Otherwise, -1 (ERROR) is returned and errno is set.
 *
 ****************************************************************************/

int sem_reset(FAR sem_t *sem, int16_t count);

/****************************************************************************
 * Function: sem_getprotocol
 *
 * Description:
 *    Return the current semaphore protocol mode.
 *
 *    When priority inheritance support is disabled globally, this function
 *    always reports `SEM_PRIO_NONE`. Otherwise it reports whether priority
 *    inheritance is enabled for the specific semaphore instance.
 *
 * Parameters:
 *    sem      - A pointer to the semaphore whose attributes are to be
 *               queried.
 *    protocol - The user provided location in which to store the protocol
 *               value.
 *
 * Return Value:
 *   0 if successful.  Otherwise, -1 is returned and the errno value is set
 *   appropriately.
 *
 ****************************************************************************/

int sem_getprotocol(FAR sem_t *sem, FAR int *protocol);

/****************************************************************************
 * Function: sem_setprotocol
 *
 * Description:
 *    Set the semaphore protocol mode.
 *
 *    `SEM_PRIO_NONE` disables priority inheritance for the semaphore.
 *    `SEM_PRIO_INHERIT` re-enables priority inheritance when that kernel
 *    feature is built. `SEM_PRIO_PROTECT` is not supported.
 *
 *    When priority inheritance support is disabled globally, only
 *    `SEM_PRIO_NONE` succeeds.
 *
 * Parameters:
 *    sem      - A pointer to the semaphore whose attributes are to be
 *               modified
 *    protocol - The new protocol to use
 *
 * Return Value:
 *   0 if successful.  Otherwise, -1 is returned and the errno value is set
 *   appropriately.
 *
 ****************************************************************************/

int sem_setprotocol(FAR sem_t *sem, int protocol);

#ifdef CONFIG_BINMGR_RECOVERY
/****************************************************************************
 * Name: sem_register
 *
 * Description:
 *   Register a kernel semaphore in the binary-manager recovery list.
 *
 * Parameters:
 *   sem - Semaphore descriptor
 *
 * Return Value:
 *   None
 *
 * Assumptions:
 *   This function may be called when semaphore in kernel region is initialized.
 *
 ****************************************************************************/
void sem_register(FAR sem_t *sem);

/****************************************************************************
 * Name: sem_unregister
 *
 * Description:
 *   Unregister a kernel semaphore from the binary-manager recovery list.
 *
 * Parameters:
 *   sem - Semaphore descriptor
 *
 * Return Value:
 *   None
 *
 * Assumptions:
 *   This function may be called when semaphore in kernel region is destroyed.
 *
 ****************************************************************************/
void sem_unregister(FAR sem_t *sem);
#endif


#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* __ASSEMBLY__ */
#endif							/* __INCLUDE_TINYARA_SEMAPHORE_H */
