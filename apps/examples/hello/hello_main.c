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
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
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
#include <tinyara/wqueue.h>
#include <stdio.h>
#include <stdlib.h>
/****************************************************************************
 * hello_main
 ****************************************************************************/

static clock_t start_time;

struct work_s works[1000];
struct work_s g_work_test_timer[3];

static void wq_test1(FAR void *arg)
{
	clock_t cur_time = 0;
	cur_time = clock();
	int count = (int)arg;
	printf("workqueue_test: count(%d), executed delay is (%llu) ticks.\n", count, (uint64_t)cur_time - (uint64_t)start_time);
	if(count < 5000){
		count++;
		work_queue(LPWORK, &works[count%1000], wq_test1, (void *)count, 0);
	}
}

static void work_queue_test_func(FAR void *arg)
{
	clock_t cur_time = 0;
	cur_time = clock();
	int count = (int)arg;
	printf("add by other thread: count(%d), executed delay is (%llu) ticks.\n", count, (uint64_t)cur_time - (uint64_t)start_time);
}

static int func_thread(int argc, char *argv[])
{
	int idx = atoi(argv[argc-1]);
	int tick= idx*10;

	usleep(tick*5000);
	for(int i= 0; i<5000;i++){
		int ret;
		ret = work_queue(LPWORK, &g_work_test_timer[idx-1], work_queue_test_func, i, 0);
		usleep(tick*1000);
	}
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int hello_main(int argc, char *argv[])
#endif
{
	pid_t pid1;
	pid_t pid2;
	pid_t pid3;
	
	printf("hello\n");

	start_time = clock();
	char *const arg1[] = { "1", NULL };
	pid1 = task_create("wq_test1", 100, 2048, func_thread, (char * const *)arg1);
	char *const arg2[] = { "2", NULL };
	pid2 = task_create("wq_test2", 100, 2048, func_thread, (char * const *)arg2);
	char *const arg3[] = { "3", NULL };
	pid3 = task_create("wq_test3", 100, 2048, func_thread, (char * const *)arg3);

	work_queue(LPWORK, &works[0], wq_test1, (void *)1, 0);
	sleep(1000);
	return 0;
}

