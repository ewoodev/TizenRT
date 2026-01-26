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

#include <tinyara/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <poll.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define UART_PORT       "/dev/ttyS2"
#define UART_BAUDRATE   B115200
#define READ_BUF_SIZE   512
#define LINE_BUF_SIZE   1024

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_uart_fd = -1;
static volatile int g_running = 1;

/* RX line buffer */
static unsigned char g_line_buf[LINE_BUF_SIZE];
static int g_line_pos = 0;
static int g_prev_was_cr = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int configure_uart(int fd)
{
	struct termios tty;

	memset(&tty, 0, sizeof(tty));
	if (tcgetattr(fd, &tty) != 0) {
		return -1;
	}

	cfsetispeed(&tty, UART_BAUDRATE);
	cfsetospeed(&tty, UART_BAUDRATE);

	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag &= ~(PARENB | PARODD);
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
	tty.c_cflag |= (CREAD | CLOCAL);

	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
	tty.c_oflag &= ~OPOST;

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	tcflush(fd, TCIOFLUSH);
	return tcsetattr(fd, TCSANOW, &tty);
}

static void output_line(void)
{
	int i;

	if (g_line_pos == 0) {
		return;
	}

	for (i = 0; i < g_line_pos; i++) {
		unsigned char c = g_line_buf[i];
		if (c >= 0x20 && c <= 0x7E) {
			putchar(c);
		} else if (c == '\t') {
			putchar('\t');
		}
	}

	printf("\n");
	fflush(stdout);
	g_line_pos = 0;
}

static void process_rx(const unsigned char *buf, ssize_t len)
{
	ssize_t i;

	for (i = 0; i < len; i++) {
		unsigned char c = buf[i];

		/* Skip NULL, DEL, high-bit */
		if (c == 0x00 || c == 0x7F || c > 0x7F) {
			continue;
		}

		/* Backspace */
		if (c == 0x08) {
			if (g_line_pos > 0) g_line_pos--;
			continue;
		}

		/* CR */
		if (c == '\r') {
			output_line();
			g_prev_was_cr = 1;
			continue;
		}

		/* LF */
		if (c == '\n') {
			if (!g_prev_was_cr) output_line();
			g_prev_was_cr = 0;
			continue;
		}

		g_prev_was_cr = 0;

		/* Skip control chars except TAB */
		if (c < 0x20 && c != '\t') {
			continue;
		}

		/* Add to buffer */
		if (g_line_pos < LINE_BUF_SIZE - 1) {
			g_line_buf[g_line_pos++] = c;
		} else {
			output_line();
			g_line_buf[g_line_pos++] = c;
		}
	}
}

/* TX Thread */
static void *tx_thread(void *arg)
{
	int ch;
	int at_line_start = 1;      /* Start of new line */
	int got_exclaim = 0;        /* Got '!' at line start */

	while (g_running) {
		ch = getchar();
		if (ch == EOF) {
			break;
		}

		/* Check for "!q" at line start */
		if (got_exclaim) {
			if (ch == 'q') {
				/* "!q" at line start - exit */
				g_running = 0;
				break;
			} else {
				/* Not "!q", send the '!' we held back */
				write(g_uart_fd, "!", 1);
				got_exclaim = 0;
				at_line_start = 0;
			}
		}

		/* Check for '!' at line start */
		if (at_line_start && ch == '!') {
			got_exclaim = 1;
			continue;
		}

		/* Track line start (after CR or LF) */
		if (ch == '\r' || ch == '\n') {
			at_line_start = 1;
		} else {
			at_line_start = 0;
		}

		write(g_uart_fd, &ch, 1);
	}

	return NULL;
}

/* RX Thread */
static void *rx_thread(void *arg)
{
	unsigned char buf[READ_BUF_SIZE];
	struct pollfd fds;
	ssize_t len;
	int ret;

	fds.fd = g_uart_fd;
	fds.events = POLLIN;

	while (g_running) {
		ret = poll(&fds, 1, 100);  /* 100ms timeout */

		if (ret < 0) {
			if (errno == EINTR) continue;
			break;
		}

		if (ret == 0) {
			continue;  /* Timeout, check g_running */
		}

		if (fds.revents & POLLIN) {
			len = read(g_uart_fd, buf, sizeof(buf));
			if (len > 0) {
				process_rx(buf, len);
			}
		}
	}

	return NULL;
}

/****************************************************************************
 * hello_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int hello_main(int argc, char *argv[])
#endif
{
	pthread_t tx_tid, rx_tid;

	printf("\n");
	printf("=== UART Terminal ===\n");
	printf("Port: %s, 115200 8N1\n", UART_PORT);
	printf("Type !q to exit\n\n");

	/* Open UART */
	g_uart_fd = open(UART_PORT, O_RDWR | O_NOCTTY);
	if (g_uart_fd < 0) {
		printf("Error: Cannot open %s\n", UART_PORT);
		return -1;
	}

	if (configure_uart(g_uart_fd) != 0) {
		printf("Error: UART config failed\n");
		close(g_uart_fd);
		return -1;
	}

	/* Create threads */
	pthread_create(&rx_tid, NULL, rx_thread, NULL);
	pthread_create(&tx_tid, NULL, tx_thread, NULL);

	/* Wait */
	pthread_join(rx_tid, NULL);
	pthread_join(tx_tid, NULL);

	/* Cleanup */
	if (g_line_pos > 0) {
		output_line();
	}

	printf("\nBye!\n");
	close(g_uart_fd);

	return 0;
}
