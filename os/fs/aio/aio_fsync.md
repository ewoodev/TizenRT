# `aio_fsync`

## Purpose

`aio_fsync()` queues one low-priority worker request that calls `file_fsync()` for the file descriptor stored in the caller's `aiocb`.

## Behavior

- Uses only `DEBUGASSERT()` to validate `op` and `aiocbp`.
- Sets `aiocbp->aio_result` to `-EINPROGRESS` before attempting any container allocation or queueing.
- Clears `aiocbp->aio_priv` before submission.
- Resolves `aiocbp->aio_fildes` through `aio_contain()`, which currently requires `fs_getfilep()` success and may wait for a free pre-allocated AIO container.
- Defers the actual `file_fsync()` call to LPWORK through `aio_queue()`.
- In the worker, stores `OK` in `aio_result` on success or a negated errno on failure.
- Signals completion through `aio_signal()` after the worker finishes.
- Ignores `op` at runtime and does not distinguish `O_SYNC` from `O_DSYNC`.

## Inputs and Outputs

- `op`: compatibility argument only; the current implementation ignores it.
- `aiocbp`: caller-owned asynchronous I/O control block that must remain valid until completion.
- Return value: `OK` when the request is queued, or `ERROR` with `errno` set when descriptor resolution or work-queue submission fails.

## Dependencies

- Uses `aio_contain()` to resolve the descriptor and allocate a container.
- Uses `aio_queue()` to submit the worker to LPWORK.
- Uses `aio_signal()` on completion.
- Worker-side I/O is performed by `file_fsync()`.

## Notes

- Submission is not guaranteed to be non-blocking. The call can wait for a free container from the `CONFIG_FS_NAIOC` pool before it returns.
- This folder owns only the submit side. Status-query APIs such as `aio_error()` and `aio_return()` live in `lib/libc/aio`.
