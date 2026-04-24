# `os/fs/aio` Module Guide

## Purpose

`os/fs/aio` implements the owner-local submit side of the public asynchronous I/O APIs declared in `os/include/aio.h`. The module does not provide true device-native async I/O; instead, it packages requests into pre-allocated containers and defers synchronous file operations onto the low-priority work queue.

## Public APIs Covered in This Folder

- `aio_cancel()`
- `aio_fsync()`
- `aio_read()`
- `aio_write()`

Function-level behavior is documented beside the implementation sources in this folder.

## Build and Configuration

- `os/fs/aio/Kconfig` exposes `CONFIG_FS_AIO`, which enables the AIO facade declared in `os/include/aio.h`.
- `os/fs/aio/Kconfig` exposes `CONFIG_FS_NAIOC`, which controls how many pre-allocated AIO containers exist at once.
- `os/wqueue/Kconfig` requires `CONFIG_SCHED_LPWORK` for this module and raises the default `CONFIG_SCHED_LPNTHREADS` from `1` to `4` when `CONFIG_FS_AIO=y`.
- `os/kernel/Kconfig` ties `CONFIG_SIG_POLL` to `CONFIG_FS_AIO` because completion signaling always emits `SIGPOLL`.
- `os/fs/aio/Make.defs` builds this folder only when `CONFIG_FS_AIO=y`.
- The companion status/wait APIs `aio_error()`, `aio_return()`, `aio_suspend()`, and `lio_listio()` are built from `lib/libc/aio`, not from this folder.

## Internal Model

1. `aio_initialize()` creates a fixed pool of `struct aio_container_s` objects plus the pending-list and free-list synchronization primitives.
2. `aio_contain()` resolves the caller's descriptor through `fs_getfilep()`, waits for a free pre-allocated container if necessary, records the caller's `aiocb`, file pointer, and task identity, then appends the request to `g_aio_pending`.
3. `aio_queue()` optionally boosts LPWORK priority to the submitting task's priority, then submits the worker with zero delay.
4. `aio_read()`, `aio_write()`, and `aio_fsync()` only prepare the request and queue the worker; the actual file I/O still runs synchronously inside LPWORK.
5. The worker decants the request, performs `file_pread()`, `file_write()`/`file_pwrite()`, or `file_fsync()`, stores the result in the caller's `aiocb`, and notifies the client with `aio_signal()`.
6. `aio_cancel()` can remove only requests that are still queued on LPWORK; requests already owned by a worker remain pending until that worker completes them.

## Behavioral Constraints

- Descriptor resolution is file-only in this folder because `aio_contain()` uses `fs_getfilep()`. Socket descriptors are not handled here.
- Submitting a request can block while waiting for a free container from the `CONFIG_FS_NAIOC` pool.
- The caller keeps ownership of the `struct aiocb` and the data buffer. Both must remain valid until worker completion.
- `aio_reqprio` is not used to compute per-request worker priority. The current implementation only snapshots the submitting task's priority for LPWORK boosting.
- `aio_cancel()` updates `aio_result` for successfully canceled requests, but this folder does not emit `aio_signal()` for those canceled requests.
- `aio_write()` uses `file_write()` when `O_APPEND` is set and `file_pwrite()` otherwise, so append semantics depend on the underlying file path and LPWORK scheduling.

## Dependencies

- Low-priority work queue infrastructure from `tinyara/wqueue.h`
- File-descriptor resolution and file operations from the VFS layer
- Signal delivery for completion notification
- Companion AIO status/wait helpers in `lib/libc/aio`

## Scope Boundaries

- This folder owns only the submit/cancel side of the AIO API family.
- `aio_error()`, `aio_return()`, `aio_suspend()`, and `lio_listio()` are public APIs but their implementations live outside `os/`.
- Internal helpers such as `aio_contain()`, `aio_queue()`, `aio_signal()`, and the free/pending list machinery are private implementation details.

## Maintenance Notes

- Keep `os/include/aio.h` aligned with the actual work-queue model instead of copying broad POSIX guarantees verbatim.
- Any change to container lifetime or LPWORK queueing must be reviewed together with `aio_cancel()` because queue ownership determines what can be canceled safely.
- If future work adds socket-aware AIO or different completion signaling, update both the folder guide and the function-level docs together.
