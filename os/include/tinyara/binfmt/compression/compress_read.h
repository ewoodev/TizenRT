/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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

#ifndef __INCLUDE_COMPRESS_READ_H
#define __INCLUDE_COMPRESS_READ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/compression.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Struct for buffers to be used for read/decompression */
struct s_buffer {
	unsigned char *read_buffer;
	unsigned char *out_buffer;
};

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: compress_uninit
 *
 * Description:
 *   Release the global decompression header and temporary block buffers that
 *   were allocated for the current compressed file session.
 *
 * Returned Value:
 *   None
 ****************************************************************************/
void compress_uninit(void);

/****************************************************************************
 * Name: compress_init
 *
 * Description:
 *   Parse the compression header for `filfd`, cache it as the active
 *   decompression session, allocate temporary block buffers, and return the
 *   uncompressed file size through `filelen`.
 *
 * Returned Value:
 *   `OK` on success, `-EBUSY` when another file is already active, or a
 *   negative errno value when header validation or allocation fails.
 ****************************************************************************/
int compress_init(int filfd, uint16_t offset, off_t *filelen);

/****************************************************************************
 * Name: compress_read
 *
 * Description:
 *   Read uncompressed bytes from the active compressed file session. The
 *   implementation loads only the compressed blocks that cover the requested
 *   range and copies the requested byte span into `buffer`.
 *
 * Returned Value:
 *   The number of bytes copied into `buffer`, or a negative failure value on
 *   error. Some paths return specific negative errno-style codes, while other
 *   failures return `ERROR` (`-1`).
 ****************************************************************************/
int compress_read(int filfd, uint16_t binary_header_size, FAR uint8_t *buffer, size_t readsize, off_t offset);

/****************************************************************************
 * Name: get_compression_header
 *
 * Description:
 *   Return the header object cached by the current `compress_init()` session.
 *
 * Returned Value:
 *   The active compression header pointer, or `NULL` when no session is
 *   initialized.
 ****************************************************************************/
struct s_header *get_compression_header(void);

#endif							/* __INCLUDE_COMPRESS_READ_H */
