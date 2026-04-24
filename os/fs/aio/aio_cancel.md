# `aio_cancel`

## Purpose

`aio_cancel()` attempts to stop queued `CONFIG_FS_AIO` work items before an LPWORK worker thread begins executing them.

## Behavior

- Returns `AIO_ALLDONE` by default and changes that status only when it finds matching pending requests.
- When `aiocbp` is non-NULL, searches the pending list by control-block pointer and ignores `fildes`.
- When `aiocbp` is NULL, scans the pending list for requests whose `aio_fildes` matches `fildes`.
- Treats `aio_result == -EINPROGRESS` plus a matching pending-list entry as the only cancelable state it can inspect directly.
- Calls `work_cancel(LPWORK, ...)` for each matching request and considers the request cancelable only when `work_cancel()` succeeds.
- On successful cancellation, removes the container from the pending list, returns it to the free pool, and sets the corresponding `aiocb->aio_result` to `-ECANCELED`.
- Leaves not-cancelable requests on the pending list so their worker can complete normally.
- Does not validate `fildes` up front and does not set `errno` for the common “nothing matched” path.

## Inputs and Outputs

- `fildes`: descriptor filter used only when `aiocbp` is `NULL`.
- `aiocbp`: specific request to cancel, or `NULL` to cancel queued requests that match `fildes`.
- Return value:
- `AIO_CANCELED` when every matched request it examined was still cancelable.
- `AIO_NOTCANCELED` when at least one matched request had already moved past the queueable state.
- `AIO_ALLDONE` when no matched request remained pending.

## Dependencies

- Uses the global pending list guarded by `aio_lock()`.
- Uses `work_cancel()` to remove requests from LPWORK.
- Uses `aioc_decant()` to detach successfully cancelled requests from the pending list and return containers to the free pool.

## Notes

- Successful cancellation updates `aio_result`, but this implementation does not call `aio_signal()` for canceled requests.
- The function can only cancel requests that are still queued on LPWORK. Once a worker has taken ownership, normal completion must run.
