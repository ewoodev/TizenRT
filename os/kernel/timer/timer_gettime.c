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
 * kernel/timer/timer_gettime.c
 *
 *   Copyright (C) 2007 Gregory Nutt. All rights reserved.
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

#include <time.h>
#include <errno.h>

#include "clock/clock.h"
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
 * Private Functions
 ********************************************************************************/

/********************************************************************************
 * Public Functions
 ********************************************************************************/

/********************************************************************************
 * Name: timer_gettime
 *
 * Description:
 *  Store the remaining watchdog delay in value->it_value and the module's last
 *  armed watchdog delay in value->it_interval. This reflects internal timer
 *  state, not necessarily the caller's last it_interval value.
 *
 * Parameters:
 *   timerid - The timer returned by timer_create().
 *   value - Receives the remaining time and the last armed watchdog delay.
 *
 * Return Value:
 *   Returns OK on success. Returns ERROR and sets errno to EINVAL when the
 *   timer handle or output pointer is invalid.
 *
 * Assumptions/Limitations:
 *   The watchdog may continue counting while this function runs, so the
 *   reported remaining time is only a snapshot.
 *
 ********************************************************************************/

int timer_gettime(timer_t timerid, FAR struct itimerspec *value)
{
	FAR struct posix_timer_s *timer = (FAR struct posix_timer_s *)timerid;
	int ticks;

	if (!PT_ISVALID(timer) || !value) {
		set_errno(EINVAL);
		return ERROR;
	}

	/* Get the number of ticks before the underlying watchdog expires */

	ticks = wd_gettime(timer->pt_wdog);

	/* Convert that to a struct timespec and return it */

	(void)clock_ticks2time(ticks, &value->it_value);
	(void)clock_ticks2time(timer->pt_last, &value->it_interval);
	return OK;
}

#endif							/* CONFIG_DISABLE_POSIX_TIMERS */
