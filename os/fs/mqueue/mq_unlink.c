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
 * fs/mqueue/mq_unlink.c
 *
 *   Copyright (C) 2007, 2009, 2014 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <mqueue.h>
#include <assert.h>
#include <errno.h>

#include <tinyara/mqueue.h>

#include "inode/inode.h"
#include "mqueue/mqueue.h"

/************************************************************************
 * Public Functions
 ************************************************************************/

/************************************************************************
 * Name: mq_unlink
 *
 * Description:
 *   Remove the named message queue from the CONFIG_FS_MQUEUE_MPATH
 *   namespace.  If one or more tasks still hold descriptors when
 *   mq_unlink() is called, namespace removal happens immediately but the
 *   queue object remains alive until all references are closed.
 *
 * Parameters:
 *   mq_name - Name of the message queue
 *
 * Return Value:
 *   0 (OK) on success; otherwise, -1 (ERROR) with errno set.
 *
 * Assumptions:
 *
 ************************************************************************/

int mq_unlink(FAR const char *mq_name)
{
	FAR struct inode *inode;
	FAR const char *relpath = NULL;
	char fullpath[MAX_MQUEUE_PATH];
	int errcode;
	int ret;

	if (!mq_name) {
		errcode = EINVAL;
		goto errout;
	}

	/* Get the full path to the message queue */

	ret = snprintf(fullpath, MAX_MQUEUE_PATH, CONFIG_FS_MQUEUE_MPATH "/%s", mq_name);
	if (ret < 0 || ret >= MAX_MQUEUE_PATH) {
		errcode = ENAMETOOLONG;
		goto errout;
	}

	/* Get the inode for this message queue. */

	sched_lock();
	inode = inode_find(fullpath, &relpath);
	if (!inode) {
		/* There is no inode at this path. */

		errcode = ENOENT;
		goto errout;
	}

	/* Verify that what we found is, indeed, a message queue */

	if (!INODE_IS_MQUEUE(inode)) {
		errcode = ENXIO;
		goto errout_with_inode;
	}

	/* Refuse to unlink the inode if it has children.  I.e., if it is
	 * functioning as a directory and the directory is not empty.
	 */

	inode_semtake();
	if (inode->i_child != NULL) {
		errcode = ENOTEMPTY;
		goto errout_with_semaphore;
	}

	/* Remove the old inode from the tree.  Because we hold a reference count
	 * on the inode, it will not be deleted now.  This will set the
	 * FSNODEFLAG_DELETED bit in the inode flags.
	 */

	ret = inode_remove(fullpath);

	/* inode_remove() usually fails with -EBUSY because we still hold a
	 * reference on the inode.  -EBUSY means that the inode was unlinked but
	 * could not be freed yet because references remain.
	 */

	DEBUGASSERT(ret >= 0 || ret == -EBUSY);
	if (ret < 0 && ret != -EBUSY) {
		errcode = -ret;
		goto errout_with_semaphore;
	}

	/* Now we do not release the reference count in the normal way (by calling
	 * inode_release()).  Rather, we call mq_inode_release().  mq_inode_release
	 * will decrement the reference count on the inode.  But it will also free
	 * the message queue if that reference count decrements to zero.  Since we
	 * hold one reference, that can only occur if the message queue is not
	 * in use.
	 */

	inode_semgive();
	mq_inode_release(inode);
	sched_unlock();
	return OK;

errout_with_semaphore:
	inode_semgive();
errout_with_inode:
	inode_release(inode);
errout:
	set_errno(errcode);
	sched_unlock();
	return ERROR;
}
