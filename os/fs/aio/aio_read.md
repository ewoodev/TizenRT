# `aio_read`

## Purpose

`aio_read()` queues one low-priority worker request that performs a positioned read into the caller's buffer.

## Behavior

- Uses only `DEBUGASSERT()` to validate `aiocbp`.
- Sets `aiocbp->aio_result` to `-EINPROGRESS` and clears `aiocbp->aio_priv` before submission.
- Resolves `aiocbp->aio_fildes` through `aio_contain()`, which currently accepts only descriptors that `fs_getfilep()` can translate into a `struct file *`.
- May wait for a free pre-allocated AIO container before the request can be queued.
- Submits the worker to LPWORK through `aio_queue()`.
- In the worker, performs `file_pread(filep, aio_buf, aio_nbytes, aio_offset)`.
- Stores either the non-negative byte count or a negated errno in `aiocbp->aio_result`.
- Signals completion through `aio_signal()` after the read finishes.

## Inputs and Outputs

- `aiocbp`: caller-owned asynchronous I/O control block. The control block and `aio_buf` must stay valid until completion.
- Return value: `OK` when the request is queued, or `ERROR` with `errno` set when descriptor resolution or work-queue submission fails.

## Dependencies

- Uses `aio_contain()` to resolve the file descriptor and allocate a container.
- Uses `aio_queue()` to submit the worker to LPWORK.
- Uses `aio_signal()` on completion.
- Worker-side I/O is performed by `file_pread()`.

## Notes

- The implementation ignores `aio_reqprio` and `aio_lio_opcode`.
- Because the worker uses `file_pread()`, the shared file position is not advanced by the read path itself.
- Socket descriptors are not handled here even though `aio_fildes` is typed as a generic descriptor.
