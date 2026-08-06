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
 * Front end for the heap use-after-free guard test.
 *
 * The test runs in kernel space and prints to the system log; this task only
 * selects what to run and reports the return code.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <tinyara/os_api_test_drv.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void __usage(const char *progname)
{
	printf("Usage: %s [uaf|dump]\n", progname);
	printf("  uaf  : allocate from the kernel heap, free it, then read it (default)\n");
	printf("         This is expected to CRASH the board with a use-after-free\n");
	printf("         report naming the faulting instruction and the task that\n");
	printf("         allocated and freed the memory.  Anything else means the\n");
	printf("         access was not trapped.\n");
	printf("  dump : print the guard state and the violations recorded so far\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int mm_guard_main(int argc, char *argv[])
#endif
{
	unsigned long arg = MM_GUARD_TEST_HEAP_UAF;
	int fd;
	int ret;

	if (argc > 1) {
		if (strcmp(argv[1], "uaf") == 0) {
			arg = MM_GUARD_TEST_HEAP_UAF;
		} else if (strcmp(argv[1], "dump") == 0) {
			arg = MM_GUARD_TEST_DUMP;
		} else {
			__usage(argv[0]);
			return ERROR;
		}
	}

	fd = open(OS_API_TEST_DRVPATH, O_WRONLY);
	if (fd < 0) {
		printf("Failed to open %s: %d\n", OS_API_TEST_DRVPATH, errno);
		return ERROR;
	}

	ret = ioctl(fd, TESTIOC_MM_GUARD_TEST, arg);

	close(fd);

	if (ret < 0) {
		if (arg == MM_GUARD_TEST_HEAP_UAF) {
			printf("The use-after-free was NOT trapped: %d (see the system log)\n", ret);
		} else {
			printf("Heap guard test reported a failure: %d (see the system log)\n", ret);
		}

		return ERROR;
	}

	printf("Done, see the system log for the report\n");

	return OK;
}
