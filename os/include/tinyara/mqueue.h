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
 *   Copyright (C) 2007, 2009, 2011, 2014-2016 Gregory Nutt. All rights reserved.
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

#ifndef ___INCLUDE_MQUEUE_H
#define ___INCLUDE_MQUEUE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#ifndef NXFUSE_HOST_BUILD
#include <tinyara/compiler.h>
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <mqueue.h>
#include <queue.h>
#include <signal.h>

#if CONFIG_MQ_MAXMSGSIZE > 0

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Global Type Declarations
 ****************************************************************************/

/* This structure defines a message queue */

struct mq_des;					/* forward reference */

struct mqueue_inode_s {
	FAR struct inode *inode;	/* Containing inode */
	sq_queue_t msglist;			/* Prioritized message list */
	uint16_t maxmsgs;			/* Maximum number of messages in the queue */
	uint16_t nmsgs;				/* Number of message in the queue */
	int16_t nwaitnotfull;		/* Number tasks waiting for not full */
	int16_t nwaitnotempty;		/* Number tasks waiting for not empty */
	size_t maxmsgsize;			/* Max size of message in message queue */
#ifndef CONFIG_DISABLE_SIGNALS
	FAR struct mq_des *ntmqdes;	/* Notification: Owning mqdes (NULL if none) */
	pid_t ntpid;				/* Notification: Receiving Task's PID */
	int ntsigno;				/* Notification: Signal number */
	union sigval ntvalue;		/* Notification: Signal value */
#endif
};

/* This describes the message queue descriptor that is held in the
 * task's TCB
 */

struct mq_des {
	FAR struct mq_des *flink;	/* Forward link to next message descriptor */
	FAR struct mqueue_inode_s *msgq;	/* Pointer to associated message queue */
	int oflags;					/* Flags set when message queue was opened */
};

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

struct mq_attr;       /* Forward reference */
struct tcb_s;         /* Forward reference */
struct task_group_s;  /* Forward reference */

/************************************************************************
 * Name: mq_msgqfree
 *
 * Description:
 *   This function deallocates an initialized message queue structure
 *   after named-queue lifetime management has already made it
 *   unreachable.  It first returns every queued message through
 *   mq_msgfree(), then releases the queue object itself.
 *
 * Inputs:
 *   msgq - Named message queue to be freed
 *
 * Return Value:
 *   None
 *
 ************************************************************************/

void mq_msgqfree(FAR struct mqueue_inode_s *msgq);

/****************************************************************************
 * Name: mq_msgqalloc
 *
 * Description:
 *   This function implements the queue-object allocation step of the
 *   POSIX message queue open logic.  It allocates a zeroed
 *   struct mqueue_inode_s, initializes its message list, copies queue
 *   limits from @p attr when present, and otherwise falls back to the
 *   internal defaults.
 *
 * Parameters:
 *   mode   - mode_t value is accepted for API compatibility but ignored
 *   attr   - Optional creation attributes.  `mq_maxmsg` and `mq_msgsize`
 *            are copied into the new queue object.  `mq_flags` and
 *            `mq_curmsgs` are ignored on input.
 *
 * Return Value:
 *   The allocated and initialized message queue structure or NULL on
 *   failure.  The helper rejects `attr->mq_msgsize > MQ_MAX_BYTES` by
 *   returning NULL without setting errno; callers provide the public
 *   error mapping.
 *
 ****************************************************************************/

FAR struct mqueue_inode_s *mq_msgqalloc(mode_t mode, FAR struct mq_attr *attr);

/****************************************************************************
 * Name: mq_descreate
 *
 * Description:
 *   Create a message queue descriptor for the supplied task's group and
 *   append it to that group's descriptor list.  When @p mtcb is NULL,
 *   the helper uses the currently executing task.
 *
 * Inputs:
 *   TCB - Task that needs the descriptor, or NULL for the current task.
 *   msgq - Backing message queue object referenced by the descriptor.
 *   oflags - Access rights and status flags stored in the descriptor.
 *
 * Return Value:
 *   On success, the message queue descriptor is returned.  NULL is
 *   returned when no descriptor can be allocated; this helper does not
 *   set errno on that failure path.
 *
 ****************************************************************************/

mqd_t mq_descreate(FAR struct tcb_s *mtcb, FAR struct mqueue_inode_s *msgq, int oflags);

/****************************************************************************
 * Name: mq_close_group
 *
 * Description:
 *   Close one message queue descriptor held by the supplied task group.
 *   The helper removes the descriptor, clears descriptor-owned message
 *   queue notification state, and drops the queue inode reference.  The
 *   queue object itself is freed only after it has already been unlinked
 *   and this close drops the final reference.
 *
 * Parameters:
 *   mqdes - Message queue descriptor.
 *   group - Group that has the open descriptor.
 *
 * Return Value:
 *   0 (OK) if the message queue descriptor is closed successfully,
 *   otherwise, -1 (ERROR).  `errno` is set to `EBADF` when @p mqdes is
 *   NULL or does not belong to @p group, or `EINVAL` when @p group is
 *   NULL.
 *
 ****************************************************************************/

int mq_close_group(mqd_t mqdes, FAR struct task_group_s *group);

/****************************************************************************
 * Name: mq_desclose_group
 *
 * Description:
 *   This function performs the descriptor-local portion of mq_close().
 *   It removes @p mqdes from @p group, clears queue-global notification
 *   state only when that notification is owned by @p mqdes, and returns
 *   the descriptor storage to the global free list.
 *
 * Parameters:
 *   mqdes - Message queue descriptor.
 *   group - Group that has the open descriptor.
 *
 * Return Value:
 *   None.
 *
 * Assumptions:
 * - The caller has already validated @p mqdes and @p group.
 * - Called only from mq_close() with the scheduler locked.
 *
 ****************************************************************************/

void mq_desclose_group(mqd_t mqdes, FAR struct task_group_s *group);


#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* CONFIG_MQ_MAXMSGSIZE > 0 */
#endif							/* ___INCLUDE_MQUEUE_H */
