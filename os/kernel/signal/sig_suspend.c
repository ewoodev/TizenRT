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
 * kernel/signal/sig_suspend.c
 *
 *   Copyright (C) 2007-2009, 2013 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <signal.h>
#include <errno.h>
#include <sched.h>

#include <tinyara/arch.h>
#include <tinyara/cancelpt.h>

#include "sched/sched.h"
#include "signal/signal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigsuspend
 *
 * Description:
 *
 *   The sigsuspend() function replaces the signal mask of the calling task
 *   with the set of signals pointed to by the argument 'set' and then
 *   suspends the task until delivery of an unmasked signal.
 *
 *   If the effect of the set argument is to unblock an already pending
 *   signal, then that signal is dispatched immediately and no blocking wait
 *   is performed.
 *
 *   The original signal mask is restored before this function returns.
 *
 *   Waiting with an empty temporary mask stops a task without freeing any
 *   resources.
 *
 * Parameters:
 *   set - signal mask to use while suspended.
 *
 * Return Value:
 *   -1 (ERROR) always with errno set to EINTR after signal delivery
 *
 * Assumptions:
 *
 * POSIX Compatibility:
 *   int sigsuspend(const sigset_t *set);
 *
 *   POSIX states that sigsuspend() "suspends the process until delivery of
 *   a signal whose action is either to execute a signal-catching function
 *   or to terminate the process."  Only delivery of an unmasked signal is
 *   required in the present implementation (even if the signal is ignored).
 *
 ****************************************************************************/

int sigsuspend(FAR const sigset_t *set)
{
	FAR struct tcb_s *rtcb = this_task();
	sigset_t saved_sigprocmask;
	irqstate_t saved_state;

	/* sigsuspend() is a cancellation point */
	(void)enter_cancellation_point();

	/* Several operations must be performed below:  We must determine if any
	 * signal is pending and, if not, wait for the signal.  Since signals can
	 * be posted from the interrupt level, there is a race condition that
	 * can only be eliminated by disabling interrupts!
	 */

	sched_lock();				/* Not necessary */
	saved_state = enter_critical_section();

	/* Save the old sigprocmask and install the new temporary sigprocmask. */

	saved_sigprocmask = rtcb->sigprocmask;
	rtcb->sigprocmask = *set;
	rtcb->sigwaitmask = NULL_SIGNAL_SET;

	/* If the temporary mask unblocks a pending signal, dispatch it now.
	 * Otherwise wait until an unmasked signal is posted.
	 */

	if ((~(*set) & sig_pendingset(rtcb)) != NULL_SIGNAL_SET) {
		leave_critical_section(saved_state);
		sig_unmaskpendingsignal();
	} else {
		up_block_task(rtcb, TSTATE_WAIT_SIG);
		leave_critical_section(saved_state);
	}

	/* Restore the original mask, then process any signals that were pending
	 * only because the temporary sigprocmask blocked them.
	 */

	saved_state = enter_critical_section();
	rtcb->sigprocmask = saved_sigprocmask;
	leave_critical_section(saved_state);
	sig_unmaskpendingsignal();

	sched_unlock();
	set_errno(EINTR);
	leave_cancellation_point();
	return ERROR;
}
