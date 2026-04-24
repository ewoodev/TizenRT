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
 * fs/aio/aio_fsync.c
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

#include <unistd.h>
#include <fcntl.h>
#include <aio.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/fs/fs.h>

#include "aio/aio.h"

#ifdef CONFIG_FS_AIO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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
 * Name: aio_fsync_worker
 *
 * Description:
 *   This function executes on the worker thread and performs the
 *   asynchronous I/O operation.
 *
 * Input Parameters:
 *   arg - Worker argument.  In this case, a pointer to struct
 *         aio_container_s cast to void *.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void aio_fsync_worker(FAR void *arg)
{
	FAR struct aio_container_s *aioc = (FAR struct aio_container_s *)arg;
	FAR struct aiocb *aiocbp;
#ifdef AIO_HAVE_FILEP
	FAR struct file *filep;
#endif
	pid_t pid;
#ifdef CONFIG_PRIORITY_INHERITANCE
	uint8_t prio;
#endif
	int ret;

	/* Get the information from the container, decant the AIO control block,
	 * and free the container before starting any I/O.  That will minimize
	 * the delays by any other threads waiting for a pre-allocated container.
	 */

	DEBUGASSERT(aioc && aioc->aioc_aiocbp);
	pid = aioc->aioc_pid;
#ifdef CONFIG_PRIORITY_INHERITANCE
	prio = aioc->aioc_prio;
#endif
#ifdef AIO_HAVE_FILEP
	filep = aioc->u.aioc_filep;
#endif
	aiocbp = aioc_decant(aioc);

	/* Perform the fsync using the resolved file structure. */

	ret = file_fsync(filep);
	if (ret < 0) {
		int errcode = get_errno();
		fdbg("ERROR: fsync failed: %d\n", errcode);
		DEBUGASSERT(errcode > 0);
		aiocbp->aio_result = -errcode;
	} else {
		aiocbp->aio_result = OK;
	}

	/* Signal the client */

	(void)aio_signal(pid, aiocbp);

#ifdef CONFIG_PRIORITY_INHERITANCE
	/* Restore the low priority worker thread default priority */

	lpwork_restorepriority(prio);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: aio_fsync
 *
 * Description:
 *   Queue one low-priority worker request that calls `file_fsync()` for the
 *   file descriptor stored in aiocbp->aio_fildes.
 *
 *   The descriptor is resolved up front through `fs_getfilep()`, so this
 *   entry point currently handles only file-descriptor paths that have a
 *   `struct file` representation.  The caller's aiocb remains owned by the
 *   caller and is used in-place until completion.
 *
 *   The current implementation ignores op and does not distinguish O_SYNC
 *   from O_DSYNC.
 *
 * Input Parameters:
 *   op     - Compatibility argument.  Currently ignored.
 *   aiocbp - A pointer to an instance of struct aiocb
 *
 * Returned Value:
 *   The aio_fsync() function will return the value 0 if the I/O operation is
 *   successfully queued; otherwise, the function will return the value -1 and
 *   set errno to indicate the error.
 *
 *   Queueing can still block while waiting for one of the pre-allocated
 *   AIO containers to become available.
 *
 ****************************************************************************/

int aio_fsync(int op, FAR struct aiocb *aiocbp)
{
	FAR struct aio_container_s *aioc;
	int ret;

	DEBUGASSERT(op == O_SYNC /* || op == O_DSYNC */);
	DEBUGASSERT(aiocbp);

	/* The result -EINPROGRESS means that the transfer has not yet completed */

	aiocbp->aio_result = -EINPROGRESS;
	aiocbp->aio_priv = NULL;

	/* Create a container for the AIO control block.  This may cause us to
	 * block if there are insufficient resources to satisfy the request.
	 */

	aioc = aio_contain(aiocbp);
	if (!aioc) {
		/* The errno has already been set (probably EBADF) */

		aiocbp->aio_result = -get_errno();
		return ERROR;
	}

	/* Defer the work to the worker thread */

	ret = aio_queue(aioc, aio_fsync_worker);
	if (ret < 0) {
		/* The result and the errno have already been set */

		return ERROR;
	}

	return OK;
}

#endif							/* CONFIG_FS_AIO */
