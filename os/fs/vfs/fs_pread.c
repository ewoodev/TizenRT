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
 * fs/vfs/fs_pread.c
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

#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <tinyara/cancelpt.h>
#include <tinyara/fs/fs.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: file_pread
 *
 * Description:
 *   Equivalent to the standard pread function except that is accepts a
 *   struct file instance instead of a file descriptor.  Currently used
 *   only by aio_read();
 *
 ****************************************************************************/

ssize_t file_pread(FAR struct file *filep, FAR void *buf, size_t nbytes, off_t offset)
{
	off_t savepos;
	off_t pos;
	ssize_t ret;
	int errcode;

	/* Perform the seek to the current position.  This will not move the
	 * file pointer, but will return its current setting
	 */

	savepos = file_seek(filep, 0, SEEK_CUR);
	if (savepos == (off_t)-1) {
		/* file_seek might fail if this if the media is not seekable */

		return ERROR;
	}

	/* Then seek to the requested position in the file */

	pos = file_seek(filep, offset, SEEK_SET);
	if (pos == (off_t)-1) {
		/* This fails when file_seek() rejects the target position */

		return ERROR;
	}

	/* Then perform the read operation */

	ret = file_read(filep, buf, nbytes);
	if (ret < 0) {
		errcode = -ret;
		ret = ERROR;
	}

	/* Restore the file position */

	pos = file_seek(filep, savepos, SEEK_SET);
	if (pos == (off_t)-1 && ret >= 0) {
		/* This really should not fail */

		return ERROR;
	}
	if (ret == ERROR) {
		set_errno(errcode);
	}
	return ret;
}

/****************************************************************************
 * Name: pread
 *
 * Description:
 *   Save the current file position with file_seek(), seek to offset,
 *   perform the read, then attempt to restore the saved position. The
 *   descriptor therefore has to support the VFS seek path. Several helper
 *   failures are collapsed to plain ERROR before the wrapper maps them back
 *   into errno. A restore failure after a successful read still turns the
 *   public result into ERROR.
 *
 *   NOTE: This function could have been wholly implemented within libc but
 *   it is not.  Why?  Because if pread were implemented in libc, it would
 *   require four system calls.  If it is implemented within the kernel,
 *   only three.
 *
 * Parameters:
 *   fd       File descriptor
 *   buf      User-provided to save the data
 *   nbytes   The maximum size of the user-provided buffer
 *   offset   The file offset
 *
 * Return:
 *   The positive non-zero number of bytes read on success, 0 on end-of-file,
 *   or ERROR on failure. The current wrapper may lose detailed errno values
 *   on seek or restore failures that collapse to plain ERROR, and a restore
 *   failure can leave the file position changed.
 *
 ****************************************************************************/

ssize_t pread(int fd, FAR void *buf, size_t nbytes, off_t offset)
{
	FAR struct file *filep;
	ssize_t ret;

	/* pread() is a cancellation point */

	(void)enter_cancellation_point();

	/* Get the file structure corresponding to the file descriptor. */

	ret = (ssize_t)fs_getfilep(fd, &filep);
	if (ret < 0) {
		goto errout;
	}

	DEBUGASSERT(filep != NULL);

	/* Let file_pread do the real work */

	ret = file_pread(filep, buf, nbytes, offset);
	if (ret < 0) {
		goto errout;
	}

	leave_cancellation_point();
	return ret;

errout:
	set_errno((int)-ret);
	leave_cancellation_point();
	return (ssize_t)ERROR;
}
