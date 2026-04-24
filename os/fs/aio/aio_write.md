# `aio_write`

## Purpose

`aio_write()` queues one low-priority worker request that writes the caller's buffer to a file descriptor, using either `pwrite`-style or append-style behavior.

## Behavior

- Uses only `DEBUGASSERT()` to validate `aiocbp`.
- Sets `aiocbp->aio_result` to `-EINPROGRESS` and clears `aiocbp->aio_priv` before submission.
- Resolves `aiocbp->aio_fildes` through `aio_contain()`, which currently accepts only descriptors that `fs_getfilep()` can translate into a `struct file *`.
- May wait for a free pre-allocated AIO container before the request can be queued.
- Submits the worker to LPWORK through `aio_queue()`.
- In the worker, fetches the descriptor flags with `F_GETFL`.
- Uses `file_pwrite()` at `aio_offset` when `O_APPEND` is clear.
- Uses `file_write()` and ignores `aio_offset` when `O_APPEND` is set.
- Stores either the non-negative byte count or a negated errno in `aiocbp->aio_result`, then signals completion through `aio_signal()`.

## Inputs and Outputs

- `aiocbp`: caller-owned asynchronous I/O control block. The control block and `aio_buf` must stay valid until completion.
- Return value: `OK` when the request is queued, or `ERROR` with `errno` set when descriptor resolution or work-queue submission fails.

## Dependencies

- Uses `aio_contain()` to resolve the file descriptor and allocate a container.
- Uses `aio_queue()` to submit the worker to LPWORK.
- Uses `aio_signal()` on completion.
- Worker-side I/O is performed by `file_fcntl()`, `file_pwrite()`, and `file_write()`.

## Notes

- The implementation ignores `aio_reqprio` and `aio_lio_opcode`.
- Append-mode writes rely on the underlying `file_write()` path. This module does not add extra serialization or ordering guarantees beyond the file implementation and LPWORK scheduling.
- Socket descriptors are not handled here even though `aio_fildes` is typed as a generic descriptor.
