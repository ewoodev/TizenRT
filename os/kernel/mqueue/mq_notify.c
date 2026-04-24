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
/************************************************************************
 *  kernel/mqueue/mq_notify.c
 *
 *   Copyright (C) 2007, 2009, 2011, 2013 Gregory Nutt. All rights reserved.
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
 ************************************************************************/

/************************************************************************
 * Included Files
 ************************************************************************/

#include <tinyara/config.h>

#include <signal.h>
#include <mqueue.h>
#include <sched.h>
#include <errno.h>

#include <tinyara/sched.h>

#include "sched/sched.h"
#include "mqueue/mqueue.h"

/************************************************************************
 * Pre-processor Definitions
 ************************************************************************/

/************************************************************************
 * Private Type Declarations
 ************************************************************************/

/************************************************************************
 * Public Variables
 ************************************************************************/

/************************************************************************
 * Private Variables
 ************************************************************************/

/************************************************************************
 * Private Functions
 ************************************************************************/

/************************************************************************
 * Public Functions
 ************************************************************************/

/************************************************************************
 * Name: mq_notify
 *
 * Description:
 *   Register or remove a one-shot notification on a message queue.
 *   At most one notification can be attached to the queue at a time.
 *
 *   If notification is non-NULL and no notification is currently
 *   attached, the current task becomes the owner and mq_notify() stores
 *   sigev_signo plus sigev_value on the queue.  The sigev_notify field is
 *   ignored by the current implementation.
 *
 *   If notification is NULL, the current task may detach its own
 *   registration.  When no registration exists, a NULL notification is a
 *   successful no-op.  When another task owns the registration, the call
 *   fails with EBUSY.
 *
 *   When a sender later observes the queue become non-empty, the sender
 *   path clears the stored registration and queues the signal exactly
 *   once.  Closing the owning descriptor also clears the stored
 *   registration.
 *
 * Parameters:
 *   mqdes - Message queue descriptor
 *   notification - Real-time signal structure containing:
 *      sigev_notify - Ignored by the current implementation
 *      sigev_signo - The signo to use for the notification
 *      sigev_value - Value associated with the signal
 *
 * Return Value:
 *   On success mq_notify() returns 0; on error, -1 is returned, with
 *   errno set to indicate the error.
 *
 *   EBADF The descriptor specified in mqdes is invalid.
 *   EBUSY Another task already owns the queue notification, or the
 *     current task attempts to replace an existing registration without
 *     detaching it first.
 *   EINVAL The supplied signal number is invalid.
 *
 * Assumptions:
 *
 * POSIX Compatibility:
 *   int mq_notify(mqd_t mqdes, const struct sigevent *notification);
 *
 *   The notification will be sent to the registered task even if another
 *   task is waiting for the message queue to become non-empty.  This is
 *   inconsistent with the POSIX specification which says, "If a process
 *   has registered for notification of message a arrival at a message
 *   queue and some process is blocked in mq_receive() waiting to receive
 *   a message when a message arrives at the queue, the arriving message
 *   message shall satisfy mq_receive()... The resulting behavior is as if
 *   the message queue remains empty, and no notification shall be sent."
 *
 ************************************************************************/

int mq_notify(mqd_t mqdes, const struct sigevent *notification)
{
	struct tcb_s *rtcb;
	struct mqueue_inode_s *msgq;
	int errval;
	irqstate_t flags;

	/* Was a valid message queue descriptor provided? */

	if (!mqdes) {
		/* No... return EBADF. */

		set_errno(EBADF);
		return ERROR;
	}

	flags = enter_critical_section();

	/* Get a pointer to the message queue */

	msgq = mqdes->msgq;
	if (!msgq) {
		errval = EBADF;
		goto errout;
	}

	/* Get the current process ID */

	rtcb = this_task();

	/* Is there already a notification attached? */

	if (!msgq->ntmqdes) {
		/* No... Have we been asked to establish one? */

		if (notification) {
			/* Yes... Was a valid signal number supplied? */

			if (!GOOD_SIGNO(notification->sigev_signo)) {
				/* No... Return EINVAL */

				errval = EINVAL;
				goto errout;
			}

			/* Yes... Assign it to the current task. */

			msgq->ntvalue.sival_ptr = notification->sigev_value.sival_ptr;
			msgq->ntsigno           = notification->sigev_signo;
			msgq->ntpid             = rtcb->pid;
			msgq->ntmqdes           = mqdes;
		}
	}

	/* Yes... a notification is attached.  Does this task own it?
	 * Is it trying to remove it?
	 */

	else if ((msgq->ntpid != rtcb->pid) || (notification)) {
		/* This task does not own the notification, or it is trying to
		 * replace an existing registration without detaching it first.
		 */

		errval = EBUSY;
		goto errout;
	} else {
		/* Yes, the notification belongs to this task.  Allow it to
		 * detach the notification.
		 */

		msgq->ntpid             = INVALID_PROCESS_ID;
		msgq->ntsigno           = 0;
		msgq->ntvalue.sival_ptr = NULL;
		msgq->ntmqdes           = NULL;
	}

	leave_critical_section(flags);
	return OK;

errout:
	set_errno(errval);
	leave_critical_section(flags);
	return ERROR;
}
