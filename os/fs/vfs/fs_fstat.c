/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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
 * fs/vfs/fs_fstat.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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
#include <sched.h>
#include <errno.h>

#include <tinyara/fs/fs.h>
#include "inode/inode.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstat
 *
 * Description:
 *   Return metadata through a file-table descriptor slot. File descriptors
 *   that resolve to mountpoints use the mountpoint fstat() hook.
 *   Pseudo-filesystem descriptors reuse inode_stat() instead. There is no
 *   socket fallback, and this path assumes the resolved slot already refers
 *   to an initialized VFS file.
 *
 * Input Parameters:
 *   fd  - The file descriptor slot associated with the file of interest
 *   buf - The caller-provided location in which to return metadata
 *
 * Returned Value:
 *   OK on success, or ERROR on failure with errno set.
 *
 ****************************************************************************/

int fstat(int fd, FAR struct stat *buf)
{
	FAR struct file *filep;
	FAR struct inode *inode;
	int ret;

	/* Did we get a valid file descriptor? */
	if ((unsigned int)fd >= CONFIG_NFILE_DESCRIPTORS) {
		/* No networking... it is a bad descriptor in any event */
		set_errno(EBADF);
		return ERROR;
	}

	/* The descriptor is in a valid range for a file descriptor. First, get
	 * the file structure pointer for this descriptor slot. On failure,
	 * fs_getfilep() returns a negated errno value.
	 */

	ret = fs_getfilep(fd, &filep);
	if (ret < 0) {
		goto errout;
	}

	/* Get the inode from the file structure. The current path assumes the
	 * descriptor slot already refers to an initialized VFS file.
	 */
	inode = filep->f_inode;
	DEBUGASSERT(inode != NULL);

	/* The way we handle the stat depends on the type of inode that we
	 * are dealing with. */
#ifndef CONFIG_DISABLE_MOUNTPOINT
	if (INODE_IS_MOUNTPT(inode)) {
		/* The inode is a file system mointpoint. Verify that the mountpoint
		 * supports the fstat() method */
		ret = -ENOSYS;
		if (inode->u.i_mops && inode->u.i_mops->fstat) {
			/* Perform the fstat() operation */
			ret = inode->u.i_mops->fstat(filep, buf);
		}
	} else
#endif
	{
		/* The inode is part of the root pseudo file system. */
		ret = inode_stat(inode, buf);
	}

	/* Check if the fstat operation was successful */
	if (ret < 0) {
		set_errno(-ret);
		return ERROR;
	}

	/* Successfully fstat'ed the file */
	return OK;
errout:
	set_errno(-ret);
	return ERROR;
}
