/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
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
 * fs/vfs/fs_truncate.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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


#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <tinyara/config.h>
#include <tinyara/fs/fs.h>

#include "inode/inode.h"

#ifndef CONFIG_DISABLE_MOUNTPOINT

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: file_truncate
 *
 * Description:
 *   File-only truncate helper. This helper accepts a struct file instance
 *   instead of a file descriptor, requires write access, and delegates to
 *   the mountpoint truncate operation without setting errno itself.
 *
 ****************************************************************************/

int file_truncate(FAR struct file *filep, off_t length)
{
	struct inode *inode;

	/* Was this file opened for write access? */

	if ((filep->f_oflags & O_WROK) == 0) {
		return -EBADF;
	}

	/* Is this inode a registered mountpoint? Does it support the
	 * truncate operations may be relevant to device drivers but only
	 * the mountpoint operations vtable contains a truncate method.
	 */

	inode = filep->f_inode;
	if (inode == NULL || !INODE_IS_MOUNTPT(inode) || inode->u.i_mops == NULL) {
		return -EINVAL;
	}

	/* A NULL write() method is an indicator of a read-only file system (but
	 * possible not the only indicator -- sufficient, but not necessary")
	 */

	if (inode->u.i_mops->write == NULL)	{
		return -EROFS;
	}

	/* Does the file system support the truncate method?  It should if it is
	 * a write-able file system.
	 */

	if (inode->u.i_mops->truncate == NULL) {
		return -ENOSYS;
	}

	/* Yes, then tell the file system to truncate this file */
	return inode->u.i_mops->truncate(filep, length);
}

/****************************************************************************
 * Name: ftruncate
 *
 * Description:
 *   Reject negative lengths, resolve the descriptor into a VFS file object,
 *   then call file_truncate(). The current implementation is mountpoint-only:
 *   it requires a descriptor opened for writing, a writable mountpoint, and
 *   a mountpoint truncate method.
 *
 * Input Parameters:
 *   fd     - File descriptor to truncate
 *   length - New length, which must be non-negative
 *
 * Returned Value:
 *   0 on success; ERROR on failure with errno set from fs_getfilep() or
 *   file_truncate().
 *
 ****************************************************************************/

int ftruncate(int fd, off_t length)
{
	FAR struct file *filep;
	int ret;

	if (length < 0) {
		ret = -EINVAL;
		goto errout;
	}

	/* Get the file structure corresponding to the file descriptor. */	
	ret = fs_getfilep(fd, &filep);
	if (ret < 0) {
		goto errout;
	}

	DEBUGASSERT(filep != NULL);

	/* Perform the truncate operation */

	ret = file_truncate(filep, length);
	if (ret >= 0) {
		return 0;
	}

errout:
	set_errno(-ret);
	return ERROR;
}

#endif /* !CONFIG_DISABLE_MOUNTPOINT */
