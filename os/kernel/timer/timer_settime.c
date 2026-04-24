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
 * kernel/timer/timer_settime.c
 *
 *   Copyright (C) 2007-2010, 2013-2014 Gregory Nutt. All rights reserved.
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

/********************************************************************************
 * Included Files
 ********************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "clock/clock.h"
#include "signal/signal.h"
#include "timer/timer.h"

#ifndef CONFIG_DISABLE_POSIX_TIMERS

/********************************************************************************
 * Definitions
 ********************************************************************************/

/********************************************************************************
 * Private Data
 ********************************************************************************/

/********************************************************************************
 * Public Data
 ********************************************************************************/

/********************************************************************************
 * Private Function Prototypes
 ********************************************************************************/

static inline void timer_sigqueue(FAR struct posix_timer_s *timer);
static inline void timer_restart(FAR struct posix_timer_s *timer, uint32_t itimer);
static void timer_timeout(int argc, uint32_t itimer);

/********************************************************************************
 * Private Functions
 ********************************************************************************/

/********************************************************************************
 * Name: timer_sigqueue
 *
 * Description:
 *   This function basically reimplements sigqueue() so that the si_code can
 *   be correctly set to SI_TIMER
 *
 * Parameters:
 *   timer - A reference to the POSIX timer that just timed out
 *
 * Return Value:
 *   None
 *
 * Assumptions:
 *   This function executes in the context of the watchod timer interrupt.
 *
 ********************************************************************************/

static inline void timer_sigqueue(FAR struct posix_timer_s *timer)
{
	siginfo_t info;

	/* Create the siginfo structure */

	info.si_signo = timer->pt_signo;
	info.si_code = SI_TIMER;
#ifdef CONFIG_CAN_PASS_STRUCTS
	info.si_value = timer->pt_value;
#else
	info.si_value.sival_ptr = timer->pt_value.sival_ptr;
#endif
#ifdef CONFIG_SCHED_HAVE_PARENT
	info.si_pid = 0;			/* Not applicable */
	info.si_status = OK;
#endif

	/* Send the signal */

	(void)sig_dispatch(timer->pt_owner, &info);
}

/********************************************************************************
 * Name: timer_restart
 *
 * Description:
 *   If a periodic timer has been selected, then restart the watchdog.
 *
 * Parameters:
 *   timer - A reference to the POSIX timer that just timed out
 *
 * Return Value:
 *   None
 *
 * Assumptions:
 *   This function executes in the context of the watchod timer interrupt.
 *
 ********************************************************************************/

static inline void timer_restart(FAR struct posix_timer_s *timer, uint32_t itimer)
{
	/* If this is a repetitive timer, then restart the watchdog */

	if (timer->pt_delay) {
		timer->pt_last = timer->pt_delay;
		(void)wd_start(timer->pt_wdog, timer->pt_delay, (wdentry_t)timer_timeout, 1, itimer);
	}
}

/********************************************************************************
 * Name: timer_timeout
 *
 * Description:
 *   This function is called if the timeout elapses before the condition is
 *   signaled.
 *
 * Parameters:
 *   argc   - the number of arguments (should be 1)
 *   itimer - A reference to the POSIX timer that just timed out
 *   signo  - The signal to use to wake up the task
 *
 * Return Value:
 *   None
 *
 * Assumptions:
 *   This function executes in the context of the watchod timer interrupt.
 *
 ********************************************************************************/

static void timer_timeout(int argc, uint32_t itimer)
{
#ifndef CONFIG_CAN_PASS_STRUCTS
	/* On many small machines, pointers are encoded and cannot be simply cast from
	 * uint32_t to struct tcb_s*.  The following union works around this (see wdogparm_t).
	 */

	union {
		FAR struct posix_timer_s *timer;
		uint32_t itimer;
	} u;

	u.itimer = itimer;

	/* Send the specified signal to the specified task.   Increment the reference
	 * count on the timer first so that will not be deleted until after the
	 * signal handler returns.
	 */

	u.timer->pt_crefs++;
	timer_sigqueue(u.timer);

	/* Release the reference.  timer_release will return nonzero if the timer
	 * was not deleted.
	 */

	if (timer_release(u.timer)) {
		/* If this is a repetitive timer, the restart the watchdog */

		timer_restart(u.timer, itimer);
	}
#else
	/* (casting to uintptr_t first eliminates complaints on some architectures
	 *  where the sizeof uint32_t is different from the size of a pointer).
	 */

	FAR struct posix_timer_s *timer = (FAR struct posix_timer_s *)((uintptr_t)itimer);

	/* Send the specified signal to the specified task.   Increment the reference
	 * count on the timer first so that will not be deleted until after the
	 * signal handler returns.
	 */

	timer->pt_crefs++;
	timer_sigqueue(timer);

	/* Release the reference.  timer_release will return nonzero if the timer
	 * was not deleted.
	 */

	if (timer_release(timer)) {
		/* If this is a repetitive timer, the restart the watchdog */

		timer_restart(timer, itimer);
	}
#endif
}

/********************************************************************************
 * Public Functions
 ********************************************************************************/

/********************************************************************************
 * Name: timer_settime
 *
 * Description:
 *   Cancel the current watchdog state, then either leave the timer disarmed
 *   when value->it_value is non-positive or program a new expiration from
 *   value->it_value. TIMER_ABSTIME interprets it_value as an absolute
 *   CLOCK_REALTIME deadline. When the call continues past the disarm path, a
 *   non-zero value->it_interval is converted into the stored periodic delay.
 *   If the computed deadline is already in the past, the implementation falls
 *   back to the periodic delay; if no periodic delay exists, the call succeeds
 *   without queueing an immediate expiration. The current implementation
 *   ignores ovalue.
 *
 * Parameters:
 *   timerid - The timer returned by timer_create().
 *   flags - Selects relative or absolute programming, plus the optional wakeup
 *     source flag when enabled by configuration.
 *   value - Describes the new expiration and optional periodic interval.
 *   ovalue - Ignored by the current implementation.
 *
 * Return Value:
 *   Returns OK when the timer is successfully disarmed or armed. Returns ERROR
 *   with errno set to EINVAL when the timer handle or value pointer is invalid.
 *   If watchdog setup fails, the watchdog helper return value is passed through.
 *
 * Assumptions:
 *
 ********************************************************************************/

int timer_settime(timer_t timerid, int flags, FAR const struct itimerspec *value, FAR struct itimerspec *ovalue)
{
	FAR struct posix_timer_s *timer = (FAR struct posix_timer_s *)timerid;
	irqstate_t state;
	int delay;
	int ret = OK;

	/* Some sanity checks */

	if (!PT_ISVALID(timer) || !value) {
		set_errno(EINVAL);
		return ERROR;
	}

	/* Disarm the timer (in case the timer was already armed when timer_settime()
	 * is called).
	 */

	(void)wd_cancel(timer->pt_wdog);

#ifdef CONFIG_SCHED_WAKEUPSOURCE
	if (flags & TIMER_WAKEUPSOURCE) {
		if (wd_setwakeupsource(timer->pt_wdog) != OK) {
			return ERROR;
		}
	}
#endif

	/* If the it_value member of value is zero, the timer will not be re-armed */

	if (value->it_value.tv_sec <= 0 && value->it_value.tv_nsec <= 0) {
		return OK;
	}

	/* Setup up any repititive timer */

	if (value->it_interval.tv_sec > 0 || value->it_interval.tv_nsec > 0) {
		(void)clock_time2ticks(&value->it_interval, &timer->pt_delay);
	} else {
		timer->pt_delay = 0;
	}

	/* We need to disable timer interrupts through the following section so
	 * that the system timer is stable.
	 */

	state = enter_critical_section();

	/* Check if abstime is selected */

	if ((flags & TIMER_ABSTIME) != 0) {
		/* Calculate a delay corresponding to the absolute time in 'value'.
		 * NOTE:  We have internal knowledge the clock_abstime2ticks only
		 * returns an error if clockid != CLOCK_REALTIME.
		 */

		(void)clock_abstime2ticks(CLOCK_REALTIME, &value->it_value, &delay);
	} else {
		/* Calculate a delay assuming that 'value' holds the relative time
		 * to wait.  We have internal knowledge that clock_time2ticks always
		 * returns success.
		 */

		(void)clock_time2ticks(&value->it_value, &delay);
	}

	/* If the time is in the past or now, then set up the next interval
	 * instead (assuming a repititive timer).
	 */

	if (delay <= 0) {
		delay = timer->pt_delay;
	}

	/* Then start the watchdog */

	if (delay > 0) {
		timer->pt_last = delay;
		ret = wd_start(timer->pt_wdog, delay, (wdentry_t)timer_timeout, 1, (uint32_t)((uintptr_t)timer));
	}

	leave_critical_section(state);
	return ret;
}

#endif							/* CONFIG_DISABLE_POSIX_TIMERS */
