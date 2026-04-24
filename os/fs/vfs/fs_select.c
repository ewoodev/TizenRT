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
 * fs/vfs/fs_select.c
 *
 *   Copyright (C) 2008-2009, 2012-2013 Gregory Nutt. All rights reserved.
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

#include <sys/select.h>
#include <sys/time.h>

#include <string.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/kmalloc.h>
#include <tinyara/cancelpt.h>
#include <tinyara/fs/fs.h>

#include "inode/inode.h"

#ifndef CONFIG_DISABLE_POLL

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static int _set_timeout(FAR struct timeval *timeout)
{
	/* Convert the timeval into milliseconds for poll().
	 *
	 * Microseconds are truncated. Negative or out-of-range timeval fields are
	 * not validated here, so a negative result remains the "wait forever" case
	 * that poll() understands.
	 */
	int msec = -1;
	if (timeout) {
		msec = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
	}

	return msec;
}

static int _init_desc_list(int nfds,
						  FAR fd_set *readfds,
						  FAR fd_set *writefds,
						  FAR fd_set *exceptfds,
						  struct pollfd *pollset)
{
	int fd;
	int ndx;

	for (fd = 0, ndx = 0; fd < nfds; fd++) {
		int incr = 0;

		/* Translate readfds into POLLIN. POLLHUP is treated as readable so
		 * callers can drain the remaining data before EOF.
		 */
		if (readfds && FD_ISSET(fd, readfds)) {
			pollset[ndx].fd = fd;
			pollset[ndx].events |= POLLIN;
			incr = 1;
		}

		/* Translate writefds into POLLOUT. */
		if (writefds && FD_ISSET(fd, writefds)) {
			pollset[ndx].fd = fd;
			pollset[ndx].events |= POLLOUT;
			incr = 1;
		}

		/* Translate exceptfds into POLLERR. */
		if (exceptfds && FD_ISSET(fd, exceptfds)) {
			pollset[ndx].fd = fd;
			pollset[ndx].events |= POLLERR;
			incr = 1;
		}

		ndx += incr;
	}

	return ndx;
}

static void _reset_fds(FAR fd_set *readfds, FAR fd_set *writefds, FAR fd_set *exceptfds)
{
	if (readfds) {
		memset(readfds, 0, sizeof(fd_set));
	}
	if (writefds) {
		memset(writefds, 0, sizeof(fd_set));
	}
	if (exceptfds) {
		memset(exceptfds, 0, sizeof(fd_set));
	}
}

static int _back_desc_list(int npfds,
						  FAR fd_set *readfds,
						  FAR fd_set *writefds,
						  FAR fd_set *exceptfds,
						  struct pollfd *pollset)
{
	int ndx;
	int ret = 0;
	for (ndx = 0; ndx < npfds; ndx++) {
		/* Copy poll() readiness bits back into the fd_sets.
		 *
		 * POLLHUP is treated as readable because a non-blocking read can still
		 * drain remaining data before EOF.
		 */
		if (readfds) {
			if (pollset[ndx].revents & (POLLIN | POLLHUP)) {
				FD_SET(pollset[ndx].fd, readfds);
				ret++;
			}
		}

		/* Copy POLLOUT back into writefds. */
		if (writefds) {
			if (pollset[ndx].revents & POLLOUT) {
				FD_SET(pollset[ndx].fd, writefds);
				ret++;
			}
		}

		/* Copy POLLERR back into exceptfds. */
		if (exceptfds) {
			if (pollset[ndx].revents & POLLERR) {
				FD_SET(pollset[ndx].fd, exceptfds);
				ret++;
			}
		}
	}

	return ret;
}

/****************************************************************************
 * Name: select
 *
 * Description:
 *   select() is a compatibility wrapper over poll(). It scans the input
 *   fd_sets, builds a temporary pollfd list, waits for readiness, and then
 *   copies the resulting bits back into the caller's fd_sets.
 *
 * Input parameters:
 *   nfds - the highest descriptor number plus one.
 *   readfds - descriptors to monitor for read readiness
 *   writefds - descriptors to monitor for write readiness
 *   exceptfds - descriptors to monitor for exception readiness
 *   timeout - relative wait interval. A negative converted timeout means
 *     wait forever.
 *
 *  Return:
 *   0: Timer expired
 *  >0: The number of readiness bits set across the output fd_sets
 *  -1: An error occurred (errno will be set appropriately)
 *
 ****************************************************************************/

int select(int nfds, FAR fd_set *readfds, FAR fd_set *writefds, FAR fd_set *exceptfds, FAR struct timeval *timeout)
{
	struct pollfd *pollset = NULL;
	int errcode = OK;
	int npfds;
	int fd;
	int ret;

	/* select() is a cancellation point */
	(void)enter_cancellation_point();

	/* Count the requested descriptors before allocating the pollfd list. */
	for (fd = 0, npfds = 0; fd < nfds; fd++) {
		/* Check whether this fd appears in any of the input sets. */
		if ((readfds && FD_ISSET(fd, readfds)) || (writefds && FD_ISSET(fd, writefds)) || (exceptfds && FD_ISSET(fd, exceptfds))) {
			/* Yes, this fd needs one pollfd entry. */
			npfds++;
		}
	}

	if (npfds > 0) {
		/* Allocate the temporary pollfd list. */
		pollset = (struct pollfd *)kmm_zalloc(npfds * sizeof(struct pollfd));
		if (!pollset) {
			set_errno(ENOMEM);
			leave_cancellation_point();
			return ERROR;
		}
	}

	/* Translate the input fd_sets into a pollfd list and count entries. */
	int ndx = _init_desc_list(nfds, readfds, writefds, exceptfds, pollset);

	DEBUGASSERT(ndx == npfds);
	if (ndx != npfds) {
		set_errno(EBADF);
		return ERROR;
	}

	/* Then let poll do the real work. timeout is in milliseconds. */
	ret = poll(pollset, npfds, _set_timeout(timeout));
	if (ret < 0) {
		/* poll() failed! Save the errno value */
		errcode = get_errno();
	}

	_reset_fds(readfds, writefds, exceptfds);

	/* Convert the poll descriptor list back into the three fd_sets. */
	if (ret > 0) {
		ret = _back_desc_list(npfds, readfds, writefds, exceptfds, pollset);
	}

	if (pollset) {
		kmm_free(pollset);
	}

	/* Did poll() fail above? */
	if (ret < 0) {
		/* Yes.. restore the errno value */
		set_errno(errcode);
	}

	leave_cancellation_point();
	return ret;
}

#endif							/* CONFIG_DISABLE_POLL */
