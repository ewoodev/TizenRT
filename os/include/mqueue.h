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
/********************************************************************************
 * include/mqueue.h
 *
 *   Copyright (C) 2007, 2008 Gregory Nutt. All rights reserved.
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
 ********************************************************************************/
/**
 * @defgroup MQUEUE_KERNEL MQUEUE
 * @brief Provides APIs for Message Queue
 * @ingroup KERNEL
 *
 * @{
 */

/// @file mqueue.h
/// @brief Mqueue APIs

#ifndef __INCLUDE_MQUEUE_H
#define __INCLUDE_MQUEUE_H

/********************************************************************************
 * Included Files
 ********************************************************************************/

#include <sys/types.h>
#include <signal.h>
#include "queue.h"

/********************************************************************************
 * Pre-processor Definitions
 ********************************************************************************/

#define MQ_NONBLOCK O_NONBLOCK

/********************************************************************************
 * Global Type Declarations
 ********************************************************************************/

/* Message queue attributes */

/** @brief sturcutre of mqueue attritube */
struct mq_attr {
	uint16_t mq_maxmsg;			/* Max number of messages in queue */
	size_t mq_msgsize;			/* Max message size */
	unsigned mq_flags;			/* Queue flags */
	uint16_t mq_curmsgs;			/* Number of messages currently in queue */
};

/* Message queue descriptor */

typedef FAR struct mq_des *mqd_t;

/********************************************************************************
 * Public Data
 ********************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/********************************************************************************
 * Public Function Prototypes
 ********************************************************************************/
/**
 * @brief Open or create a named message queue descriptor.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Resolves @p mq_name below `CONFIG_FS_MQUEUE_MPATH` and returns a
 * descriptor owned by the caller's current task group. When `O_CREAT`
 * creates a new queue, the implementation consumes `mode_t` and
 * `struct mq_attr *`, but it currently ignores `mode` and uses `attr`
 * only for the first creation.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
mqd_t mq_open(FAR const char *mq_name, int oflags, ...);
/**
 * @brief Close one message queue descriptor for the current task group.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Releases the descriptor and any notification registered through that
 * descriptor. The named queue object itself remains alive until it has
 * been unlinked and the last reference is dropped.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_close(mqd_t mqdes);
/**
 * @brief Remove a named message queue from the configured namespace.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Removes the entry for @p mq_name below `CONFIG_FS_MQUEUE_MPATH`.
 * Existing descriptors keep the queue alive until the final close drops
 * the last inode reference.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_unlink(FAR const char *mq_name);
/**
 * @brief Queue one message on a message queue descriptor.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Copies `msglen` bytes from `msg`, inserts the new message ahead of
 * lower-priority entries and after older messages with the same
 * priority, and waits for queue space unless the descriptor currently
 * has `O_NONBLOCK` set. Successful sends also drive the queue-global
 * one-shot notification state and wake one task waiting for the queue
 * to become non-empty. This implementation treats the call as a
 * cancellation point.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_send(mqd_t mqdes, FAR const char *msg, size_t msglen, int prio);
/**
 * @brief Queue one message with an absolute `CLOCK_REALTIME` timeout.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Behaves like `mq_send()`, but when the queue is full and the
 * descriptor is blocking, the wait is limited by the absolute
 * `CLOCK_REALTIME` deadline in `abstime`. The implementation validates
 * `abstime`, reserves a wait watchdog up front, and still returns
 * immediately with `EAGAIN` when the descriptor has `O_NONBLOCK` set.
 * This implementation treats the call as a cancellation point.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_timedsend(mqd_t mqdes, FAR const char *msg, size_t msglen, int prio, FAR const struct timespec *abstime);
/**
 * @brief Receive the oldest message from the highest-priority queue band.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Blocks until one message becomes available unless the descriptor has
 * `O_NONBLOCK` set, then removes the selected message from the queue and
 * copies it into the caller's buffer.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
ssize_t mq_receive(mqd_t mqdes, FAR char *msg, size_t msglen, FAR int *prio);
/**
 * @brief Receive one message with an absolute `CLOCK_REALTIME` timeout.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * Behaves like `mq_receive()` but limits the blocking wait with the
 * absolute time supplied in `abstime` when the descriptor is not
 * non-blocking.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
ssize_t mq_timedreceive(mqd_t mqdes, FAR char *msg, size_t msglen, FAR int *prio, FAR const struct timespec *abstime);
/**
 * @brief Register or remove one-shot notification for a message queue.
 * @details @b #include <mqueue.h> \n
 * SYSTEM CALL API \n
 * The registration is queue-global and is removed after the first
 * successful not-empty notification. The current implementation stores
 * `sigev_signo` and `sigev_value` but ignores `sigev_notify`.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_notify(mqd_t mqdes, const struct sigevent *notification);

/**
 * @brief Update the descriptor attributes of a message queue.
 * @details @b #include <mqueue.h> \n
 * Only the `O_NONBLOCK` bit in the descriptor flags is writable through
 * this API. When `oldstat` is non-NULL, the previous descriptor snapshot
 * is returned as if by `mq_getattr()`.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_setattr(mqd_t mqdes, FAR const struct mq_attr *mq_stat, FAR struct mq_attr *oldstat);
/**
 * @brief Get a snapshot of queue limits, occupancy, and descriptor flags.
 * @details @b #include <mqueue.h> \n
 * Returns the backing queue's `mq_maxmsg`, `mq_msgsize`, and `mq_curmsgs`
 * values together with the calling descriptor's current flag word.
 * POSIX API (refer to : http://pubs.opengroup.org/onlinepubs/9699919799/)
 * @since TizenRT v1.0
 */
int mq_getattr(mqd_t mqdes, FAR struct mq_attr *mq_stat);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* __INCLUDE_MQUEUE_H */
/**
 * @}
 */
