/****************************************************************************
 *
 * Copyright 2026 Samsung Electronics All Rights Reserved.
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
 * Kernel side of the heap use-after-free guard test.
 *
 * The memory it abuses is the kernel heap and the report comes out of the data
 * abort handler, so the test has to run in kernel space.  The front end is
 * apps/examples/mm_guard, which reaches this through the OS API test driver.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/kmalloc.h>
#include <tinyara/mm/mm.h>
#include <tinyara/os_api_test_drv.h>

#ifdef CONFIG_MM_GUARD_FREED_PAGES

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* A chunk big enough that its payload contains whole pages whatever its
 * alignment, and an offset that is inside the first of them.
 */

#define HEAP_UAF_SIZE		(64 * 1024)
#define HEAP_UAF_OFFSET		(8 * 1024)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __heap_uaf_run
 *
 * Description:
 *   Use freed kernel heap memory on purpose.
 *
 *   This is not expected to return.  The guard panics on a violation, so a
 *   correct system dies here with the use-after-free report naming this function
 *   and the pid that freed the chunk.  Returning at all means the access was not
 *   trapped, which is the failure this test looks for.
 *
 ****************************************************************************/

static int __heap_uaf_run(void)
{
	FAR volatile uint8_t *mem;
	uint8_t value;

	mem = (FAR volatile uint8_t *)kmm_malloc(HEAP_UAF_SIZE);
	if (mem == NULL) {
		lldbg("Could not allocate %d bytes from the kernel heap\n", HEAP_UAF_SIZE);
		return -ENOMEM;
	}

	lldbg("Allocated 0x%08x, will read offset %d of it after freeing it\n", mem, HEAP_UAF_OFFSET);

	kmm_free((FAR void *)mem);

	/* The chunk is gone.  Reading it is the bug this whole feature exists to
	 * catch, and the guard traps reads as well as writes, so the next line should
	 * not complete.
	 */

	value = mem[HEAP_UAF_OFFSET];

	lldbg("Read 0x%02x from freed memory: the guard did NOT trap the access\n", value);

	return -EFAULT;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_mm_guard
 *
 * Description:
 *   ioctl entry point.  arg selects what to run, see TESTIOC_MM_GUARD_TEST.
 *
 ****************************************************************************/

int test_mm_guard(int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	if (cmd != TESTIOC_MM_GUARD_TEST) {
		return -EINVAL;
	}

	switch (arg) {
	case MM_GUARD_TEST_DUMP:
		mm_guard_dump();
		ret = OK;
		break;

	case MM_GUARD_TEST_HEAP_UAF:
		ret = __heap_uaf_run();
		break;

	default:
		break;
	}

	return ret;
}

#endif							/* CONFIG_MM_GUARD_FREED_PAGES */
