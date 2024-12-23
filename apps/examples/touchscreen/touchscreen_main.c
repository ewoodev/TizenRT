/****************************************************************************
 *
 * Copyright 2024 Samsung Electronics All Rights Reserved.
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
#include <tinyara/config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#if !defined(CONFIG_DISABLE_POLL)
#include <poll.h>
#endif

#include <tinyara/input/touchscreen.h>

static bool g_terminated;

static void touch_test(void)
{
	/* read first 10 events */
	int ret;
	int read_size;
	struct touch_point_s buf[15];
	struct pollfd fds[1];

	int fd = open(TOUCH_DEV_PATH, O_RDONLY);
	if (fd < 0) {
		printf("Error: Failed to open /dev/input0, errno : %d\n", get_errno());
		return;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (!g_terminated) {
		poll(fds, 1, 5000);
		if (fds[0].revents & POLLIN) {
			ret = read(fd, buf, sizeof(struct touch_point_s) * 15);
			if (ret > 0) {
				DEBUGASSERT(ret <= sizeof(struct touch_point_s) * 15);

				read_size = ret / sizeof(struct touch_point_s);
				printf("Total touch points %d\n", read_size);
				for (int i = 0; i < read_size; i++) {
					printf("coordinates id: %d, x : %d y : %d touch type: %d\n", buf[i].id, buf[i].x, buf[i].y, buf[i].flags);
					if (buf[i].flags == TOUCH_DOWN) {
						printf("Touch press event \n");
					} else if (buf[i].flags == TOUCH_MOVE) {
						printf("Touch hold/move event \n");
					} else if (buf[i].flags == TOUCH_UP) {
						printf("Touch release event \n");
					}
				}
			}
		}
	}
	close(fd);
}

static int touchsceen_test_start(void)
{
	printf("touchscreen test start\n");
	int touch = task_create("touch", SCHED_PRIORITY_DEFAULT, 8096, (main_t)touch_test, NULL);
	if (touch < 0) {
		printf("Error: Failed to create touch reader, error : %d\n", get_errno());
		return ERROR;
	}
	g_terminated = false;
	return OK;
}

static int touchsceen_test_stop(void)
{
	g_terminated = true;
	printf("touchscreen test stop\n");
	return OK;
}

static int touchsceen_specific_cmd(int argc, char*argv[])
{
	struct touchscreen_cmd_s args;
	int fd = open(TOUCH_DEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("Fail to open %s, errno:%d\n", TOUCH_DEV_PATH, get_errno());
		return ERROR;
	}
	args.argc = argc;
	args.argv = argv;
	if (ioctl(fd, TSIOC_CMD, (unsigned long)&args) != OK) {
		printf("Fail to ioctl %s, errno:%d\n", TOUCH_DEV_PATH, get_errno());
	}

	close(fd);
	return OK;
}


static void show_usage(void)
{
	printf("usage: touchscreen <command #>\n");
	printf("Excute touchscreen testing or controling.\n\n");
	printf("The touchscreen basic test command which printing coordinates and types:\n");
	printf("    start: Start the touchscreen basic test \n");
	printf("    stop : Stop  the touchscreen basic test\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int touchscreen_main(int argc, char *argv[])
#endif
{
	if (argc <= 1 || !strncmp(argv[1], "-h", 2) || !strncmp(argv[1], "--help", 6)) {
		show_usage();
		touchsceen_specific_cmd(0, NULL);
		return OK;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "start")) {
			return touchsceen_test_start();
		} else if (!strcmp(argv[1], "stop")) {
			return touchsceen_test_stop();
		}
	}

	return touchsceen_specific_cmd(argc, argv);
}
