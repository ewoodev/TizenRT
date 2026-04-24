/****************************************************************************
 *
 * Copyright 2023 Samsung Electronics All Rights Reserved.
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
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>

#include <tinyara/fs/fs.h>
#include <tinyara/fs/ioctl.h>
#include <tinyara/compression.h>
#include <tinyara/binfmt/compression/compress_read.h>

#include <errno.h>
#include <debug.h>
#include <assert.h>
#include <stdio.h>

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

static int comp_ioctl(FAR struct file *filep, int cmd, unsigned long arg);
static ssize_t comp_read(FAR struct file *filep, FAR char *buffer, size_t len);
static ssize_t comp_write(FAR struct file *filep, FAR const char *buffer, size_t len);

/****************************************************************************
 * Data
 ****************************************************************************/
static const struct file_operations compress_fops = {
	0,                          /* open */
	0,                          /* close */
	comp_read,                  /* read */
	comp_write,                 /* write */
	0,                          /* seek */
	comp_ioctl              /* ioctl */
#ifndef CONFIG_DISABLE_POLL
	, 0                         /* poll */
#endif
};

static struct file_decomp_data_s {
	int fd;
	struct s_header *compression_header;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static ssize_t comp_read(FAR struct file *filep, FAR char *buffer, size_t len)
{
	return 0;
}

static ssize_t comp_write(FAR struct file *filep, FAR const char *buffer, size_t len)
{
	return 0;
}


/************************************************************************************
 * Name: comp_ioctl
 *
 * Description:
 *   Handles the `/dev/compress` ioctl interface.
 *
 *   Supported commands:
 *   - COMPIOC_COMPRESS / COMPIOC_DECOMPRESS:
 *     operate on a caller-provided `struct compress_header`.
 *   - COMPIOC_GET_COMP_TYPE / COMPIOC_GET_COMP_NAME:
 *     report the backend selected by `CONFIG_COMPRESSION_TYPE`.
 *   - COMPIOC_FCOMP_INIT / COMPIOC_FCOMP_GET_BUFSIZE /
 *     COMPIOC_FCOMP_DECOMPRESS / COMPIOC_FCOMP_DEINIT:
 *     expose the single-session compressed-file decompression flow built on
 *     top of `compress_init()`, `compress_read()`, and `compress_uninit()`.
 *
 *   The file-decompression sequence is stateful and must be closed with
 *   COMPIOC_FCOMP_DEINIT even on error.
 *
 ************************************************************************************/
static int comp_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	int ret = -ENOSYS;
	unsigned int size;
	off_t filelen;
	struct compress_header *comp_info = (struct compress_header *)arg;
	struct file_decomp_data_s *data;

	/* Handle built-in ioctl commands */
	switch (cmd) {
	case COMPIOC_COMPRESS:
		if (comp_info == NULL) {
			return -EINVAL;
		}
		if (comp_info->output_size < comp_info->input_size) {
			bcmpdbg("It is recommended to keep input size and output size equal during compression\n");
		}
		ret = compress_block(comp_info->output_buffer, &comp_info->output_size, comp_info->input_buffer, comp_info->input_size);
		break;
	case COMPIOC_GET_COMP_TYPE:
		/* CONFIG_COMPRESSION_TYPE 1 for LZMA and 2 for MINIZ */
		ret = CONFIG_COMPRESSION_TYPE;
		break;
	case COMPIOC_GET_COMP_NAME:
		if ((char *)arg == NULL) {
			return -EINVAL;
		}
		switch (CONFIG_COMPRESSION_TYPE) {
		case LZMA_TYPE:
			memcpy((char *)arg, LZMA_NAME, COMP_NAME_SIZE);
			break;
		case MINIZ_TYPE:
			memcpy((char *)arg, MINIZ_NAME, COMP_NAME_SIZE);
			break;
		}
		ret = OK;
		break;
 	case COMPIOC_DECOMPRESS:
		if (comp_info == NULL) {
			return -EINVAL;
		}
		ret = decompress_block(comp_info->output_buffer, &comp_info->output_size, comp_info->input_buffer, &comp_info->input_size);
		if (ret == -ENOMEM) {
			bcmpdbg("Output buffer allocated is not sufficient\n");
			return ret;
		}
		break;
	case COMPIOC_FCOMP_INIT:
		if ((char *)arg == NULL) {
			return -EINVAL;
		}
		data = (struct file_decomp_data_s *)kmm_zalloc(sizeof(struct file_decomp_data_s));
		if (!data) {
			bcmpdbg("Memory allocation fail for data storing\n");
			return -ENOMEM;
		}
		data->fd = open((char *)arg, "r");
		if (data->fd < 0) {
			bcmpdbg("Failed to open file = %s\n", (char *)arg);
			free(data);
			return -EINVAL;
		}
		ret = compress_init(data->fd, 0, &filelen);
		if (ret != OK) {
			if (ret == -EBUSY) {
				bcmpdbg("Another file decompression is in process\n");
				close(data->fd);
				free(data);
			}
			return ret;
		}
		DEBUGASSERT(filep);
		filep->f_priv = data;
		break;
	case COMPIOC_FCOMP_GET_BUFSIZE:
		DEBUGASSERT(filep);
		data = filep->f_priv;
		data->compression_header = get_compression_header();
		ret = data->compression_header->binary_size;
		if (ret <= 0) {
			bcmpdbg("Failed to get buffer size = %d\n", ret);
			return -EBADF;
		}
		break;
	case COMPIOC_FCOMP_DECOMPRESS:
		if ((uint8_t *)arg == NULL) {
			return -EINVAL;
		}
		DEBUGASSERT(filep);
		data = filep->f_priv;
		size = compress_read(data->fd, 0, (uint8_t *)arg, data->compression_header->binary_size, 0);
		ret = OK;
		if (size != data->compression_header->binary_size) {
			bcmpdbg("Compress header size mismatch");
			return -EINVAL;
		}
		break;
	case COMPIOC_FCOMP_DEINIT:
		DEBUGASSERT(filep);
		data = filep->f_priv;
		if (data) {
			if (data->compression_header) {
				compress_uninit();
			}
			close(data->fd);
			free(data);
			filep->f_priv = NULL;
		}
		ret = OK;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

/****************************************************************************
 * Name: compress_register
 *
 * Description:
 *   Register compress driver path, COMP_DRVPATH
 *
 ****************************************************************************/
void compress_register(void)
{
	(void)register_driver(COMP_DRVPATH, &compress_fops, 0666, NULL);
}
