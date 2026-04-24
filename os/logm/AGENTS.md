# `os/logm` Module Guide

## Purpose

`os/logm` implements a buffered logger that decouples log producers from stdout writes. Producers format messages into a circular buffer, and a dedicated background task flushes the buffer to standard output at a configurable interval.

## Public APIs

- `logm_start()`: launches the logger task and optional shell command registration
- `logm_internal()`: core formatter and router used by the public wrapper and debug macros
- `logm()`: variadic wrapper over `logm_internal()`
- `logm_set_values()`: updates runtime logger parameters
- `logm_get_values()`: reads runtime logger parameters

Function-level API notes live next to the sources:

- `logm_start.md`
- `logm_internal.md`
- `logm.md`
- `logm_set_values.md`
- `logm_get_values.md`

## Internal Flow

1. `logm_start()` creates the logger thread.
2. `logm_task()` allocates the ring buffer, marks the module ready, and enters the flush loop.
3. `logm_internal()` appends formatted messages to the ring buffer while the buffered path is available.
4. `logm_task()` drains the ring buffer to stdout and handles deferred buffer resizing.
5. TASH commands, when enabled, call `logm_set_values()` and `logm_get_values()` to inspect or adjust runtime settings.

## Kconfig

- `CONFIG_LOGM`: enables the module
- `CONFIG_PRINTF2LOGM`: routes `printf()` through the logger integration
- `CONFIG_SYSLOG2LOGM`: routes syslog traffic through the logger integration
- `CONFIG_LOGM_TIMESTAMP`: prepends timestamps to buffered messages
- `CONFIG_LOGM_BUFFER_SIZE`: initial ring-buffer size in bytes
- `CONFIG_LOGM_PRINT_INTERVAL`: background flush interval in milliseconds
- `CONFIG_LOGM_TASK_PRIORITY`: priority of the logger task
- `CONFIG_LOGM_TASK_STACKSIZE`: stack size of the logger task
- `CONFIG_LOGM_TEST`: enables logger test code

## Dependencies

- `kernel_thread()` for task creation
- `kmm_malloc()` and `kmm_realloc()` for buffer management
- `lib_vsprintf()` and stream helpers for formatting
- `stdout` for flush output
- `TASH` support when command registration is enabled

## Maintenance Notes

- The buffered path depends on shared head/tail indices and must stay synchronized with the critical-section usage in `logm_internal()` and the drain loop in `logm_task()`.
- `logm_set_values()` only stores the requested buffer size. The actual buffer reallocation happens inside `logm_task()` after another path sets `LOGM_BUFFER_RESIZE_REQ`.
- `priority` and `indx` are currently placeholders in the public APIs. If future changes make them meaningful, update both the code comments and the function-level Markdown.
