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
 * fs/aio/aio_cancel.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <aio.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>

#include <tinyara/wqueue.h>

#include "aio/aio.h"

#ifdef CONFIG_FS_AIO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: aio_cancel
 *
 * Description:
 *   Attempt to cancel queued low-priority work items created by the
 *   `os/fs/aio` submit APIs.  Only requests that are still queued on LPWORK
 *   can be cancelled here; once a worker has dequeued the request, normal
 *   completion must run to finish it.
 *
 *   When aiocbp is non-NULL, the implementation searches by control-block
 *   pointer and ignores fildes.  When aiocbp is NULL, the implementation
 *   cancels every queued request whose `aio_fildes` matches fildes.
 *
 *   For successfully cancelled requests, `aio_result` is updated to
 *   `-ECANCELED`. Requests that cannot be cancelled are left on the pending
 *   list for their worker to complete normally.
 *
 * Input Parameters:
 *   fildes - Descriptor filter used only when aiocbp is NULL.
 *   aiocbp - Specific asynchronous I/O control block to cancel, or NULL.
 *
 * Returned Value:
 *    The aio_cancel() function will return the value AIO_CANCELED if the
 *    requested operation(s) were cancelled. The value AIO_NOTCANCELED will
 *    be returned if at least one of the requested operation(s) cannot be
 *    cancelled because it is in progress. In this case, the state of the
 *    other operations, if any, referenced in the call to aio_cancel() is
 *    not indicated by the return value of aio_cancel(). The application
 *    may determine the state of affairs for these operations by using
 *    aio_error(). The value AIO_ALLDONE is returned if all of the
 *    operations have already completed. Otherwise, the function will return
 *    -1 and set errno to indicate the error.
 *
 ****************************************************************************/

int aio_cancel(int fildes, FAR struct aiocb *aiocbp)
{
	FAR struct aio_container_s *aioc;
	FAR struct aio_container_s *next;
	int status;
	int ret;

	/* Check if a non-NULL aiocbp was provided */

	/* Lock the scheduler so that no I/O events can complete on the worker
	 * thread until we set complete this operation.
	 */

	ret = AIO_ALLDONE;
	sched_lock();
	aio_lock();

	if (aiocbp) {
		/* Check if the I/O has completed */

		if (aiocbp->aio_result == -EINPROGRESS) {
			/* No.. Find the container for this AIO control block */

			for (aioc = (FAR struct aio_container_s *)g_aio_pending.head; aioc && aioc->aioc_aiocbp != aiocbp; aioc = (FAR struct aio_container_s *)aioc->aioc_link.flink) ;

			/* Did we find a container for this fildes?  We should; the aio_result says
			 * that the transfer is pending.  If not we return AIO_ALLDONE.
			 */

			if (aioc) {
				/* Yes... attempt to cancel the I/O.  There are two
				 * possibilities:* (1) the work has already been started and
				 * is no longer queued, or (2) the work has not been started
				 * and is still in the work queue.  Only the second case can
				 * be cancelled.  work_cancel() will return -ENOENT in the
				 * first case.
				 */

				status = work_cancel(LPWORK, &aioc->aioc_work);
				if (status >= 0) {
					aiocbp->aio_result = -ECANCELED;
					ret = AIO_CANCELED;
					(void)aioc_decant(aioc);
				} else {
					ret = AIO_NOTCANCELED;
				}
			}
		}
	} else {
		/* No aiocbp.. cancel all outstanding I/O for the fildes */

		next = (FAR struct aio_container_s *)g_aio_pending.head;
		do {
			/* Find the next container with this AIO control block */

			for (aioc = next; aioc && aioc->aioc_aiocbp->aio_fildes != fildes; aioc = (FAR struct aio_container_s *)aioc->aioc_link.flink) ;

			/* Did we find the container?  We should; the aio_result says
			 * that the transfer is pending.  If not we return AIO_ALLDONE.
			 */

			if (aioc) {
				/* Yes... attempt to cancel the I/O.  There are two
				 * possibilities:* (1) the work has already been started and
				 * is no longer queued, or (2) the work has not been started
				 * and is still in the work queue.  Only the second case can
				 * be cancelled.  work_cancel() will return -ENOENT in the
				 * first case.
				 */

				status = work_cancel(LPWORK, &aioc->aioc_work);

				/* Remove the container from the list of pending transfers */

				next = (FAR struct aio_container_s *)aioc->aioc_link.flink;
				if (status >= 0) {
					aiocbp = aioc_decant(aioc);
					DEBUGASSERT(aiocbp);
					aiocbp->aio_result = -ECANCELED;
					if (ret != AIO_NOTCANCELED) {
						ret = AIO_CANCELED;
					}
				} else {
					ret = AIO_NOTCANCELED;
				}
			}
		} while (aioc);
	}

	aio_unlock();
	sched_unlock();
	return ret;
}

#endif							/* CONFIG_FS_AIO */
