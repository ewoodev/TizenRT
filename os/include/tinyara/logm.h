/****************************************************************************
 *
 * Copyright 2016-2017 Samsung Electronics All Rights Reserved.
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
 /**
  * @defgroup LOGM_KERNEL LOGM
  * @ingroup KERNEL
  */

///@file tinyara/logm.h
///@brief logm APIs

#ifndef __OS_INCLUDE_TINYARA_LOGM_H
#define __OS_INCLUDE_TINYARA_LOGM_H

#include <stdarg.h>

#define LOGM_DEF_PRIORITY (7)
/* Log priority levels in logm */

enum logm_loglevel_e {
	LOGM_EMR, /* Emergency */
	LOGM_ART, /* Alert */
	LOGM_CRT, /* Critical */
	LOGM_ERR, /* Error */
	LOGM_WRN, /* Warning */
	LOGM_NTCE,/* Notice */
	LOGM_INF, /* Info   */
	LOGM_DBG, /* Debug */
	LOGM_OFF  /* Is this needed? */
};

enum logm_param_type_e {
	LOGM_BUFSIZE,
	LOGM_INTERVAL,
	LOGM_PRIORITY
	/* This would grow later */
};

/* Log flags for somethings related to logging */
enum logm_logflag_e {
	LOGM_NORMAL,
	LOGM_LOWPUT
};

/* Log index means where messages are from */
enum logm_logindex_e {
	LOGM_UNKNOWN
	/* Not supported yet. This would be updated later */
};

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#  define EXTERN extern
#endif
/**
 * @cond
 * @internal
 */
/**
 * @brief Start the background logger task.
 *
 * @details
 * Creates the `logm` kernel thread with the priority and stack size selected
 * by Kconfig. When `CONFIG_TASH` is enabled, the function also registers the
 * `logm` shell command after the thread has been created successfully.
 *
 * If the thread creation fails, the function returns without changing logger
 * state or reporting an error.
 */
void logm_start(void);
/**
 * @brief Format a log message and route it to the active logger backend.
 *
 * @details
 * When the logger task is ready and the call is made from normal thread
 * context, the function appends the formatted message to the ring buffer and
 * optionally prepends a timestamp. During early boot, buffer resize, or
 * interrupt context, it falls back to the low-level output path when that
 * backend is available.
 *
 * If the ring buffer is already marked as overflowed, the message is dropped
 * and the drop counter is incremented.
 *
 * @return
 * Returns the formatted character count produced by `lib_vsprintf()` on the
 * active backend, or `0` when the message is dropped before formatting.
 */
int logm_internal(int flag, int indx, int priority, const char *fmt, va_list valst);
/**
 * @brief Variadic wrapper around `logm_internal()`.
 *
 * @details
 * Collects the variable argument list and forwards the formatted log request
 * to `logm_internal()`.
 *
 * @return
 * Returns the value produced by `logm_internal()`.
 */
int logm(int flag, int indx, int priority, const char *fmt, ...);
/**
 * @brief Update a runtime logger parameter.
 *
 * @details
 * `LOGM_BUFSIZE` stores a resize request rounded up to a 4-byte boundary.
 * `LOGM_INTERVAL` updates the background flush interval in microseconds.
 * This function does not set `LOGM_BUFFER_RESIZE_REQ`; callers that want the
 * new buffer size to take effect must arrange that signal separately. Unknown
 * parameter types are ignored.
 *
 * @return
 * Always returns `0`.
 */
int logm_set_values(enum logm_param_type_e type, int value);
/**
 * @brief Read a runtime logger parameter.
 *
 * @details
 * `LOGM_BUFSIZE` returns the current ring-buffer size in bytes.
 * `LOGM_INTERVAL` returns the current flush interval in milliseconds.
 * Unknown parameter types are ignored.
 *
 * @return
 * Always returns `0`.
 */
int logm_get_values(enum logm_param_type_e type, int *value);
/**
 * @endcond
 */
#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __OS_INCLUDE_TINYARA_LOGM_H */
