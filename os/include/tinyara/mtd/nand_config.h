/****************************************************************************
 *
 * Copyright 2025 Samsung Electronics All Rights Reserved.
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
 * include/nuttx/mtd/nand_config.h
 *
 *   Copyright (c) 2012, Atmel Corporation
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
 * 3. Neither the names NuttX nor Atmel nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
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

#ifndef __INCLUDE_MTD_NAND_CONFIG_H
#define __INCLUDE_MTD_NAND_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Maximum number of blocks in a device */

#ifndef CONFIG_MTD_NAND_MAXNUMBLOCKS
#  define CONFIG_MTD_NAND_MAXNUMBLOCKS        1024
#endif

/* Maximum number of pages in one block */

#ifndef CONFIG_MTD_NAND_MAXNUMPAGESPERBLOCK
#  define CONFIG_MTD_NAND_MAXNUMPAGESPERBLOCK 256
#endif

/* Maximum size of the data area of one page, in bytes. */

#ifndef CONFIG_MTD_NAND_MAXPAGEDATASIZE
#  define CONFIG_MTD_NAND_MAXPAGEDATASIZE     4096
#endif

/* Maximum size of the spare area of one page, in bytes. */

#ifndef CONFIG_MTD_NAND_MAXPAGESPARESIZE
#  define CONFIG_MTD_NAND_MAXPAGESPARESIZE    256
#endif

/* Maximum number of ecc bytes stored in the spare for one single page. */

#ifndef CONFIG_MTD_NAND_MAXSPAREECCBYTES
#  define CONFIG_MTD_NAND_MAXSPAREECCBYTES    48
#endif

/* Maximum number of extra free bytes inside the spare area of a page. */

#ifndef CONFIG_MTD_NAND_MAXSPAREEXTRABYTES
#  define CONFIG_MTD_NAND_MAXSPAREEXTRABYTES  206
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __INCLUDE_MTD_NAND_CONFIG_H */
