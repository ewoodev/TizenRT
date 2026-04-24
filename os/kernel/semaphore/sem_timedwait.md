# `sem_timedwait`

## Summary

`sem_timedwait()` waits for a semaphore until an absolute `CLOCK_REALTIME` deadline expires.

## Behavior

- Treats the call as a cancellation point.
- In debug builds, rejects a `NULL` semaphore or `NULL` `abstime` pointer with `EINVAL`.
- Creates a per-thread watchdog before entering the blocking path; returns `ENOMEM` if no watchdog can be allocated.
- Starts with `sem_trywait()`. If that succeeds, the function deletes the watchdog reservation and returns `OK` without arming a timeout.
- Validates `abstime->tv_nsec` only on the blocking path and returns `EINVAL` when it is outside `[0, 1000000000)`.
- Converts the absolute deadline to ticks with `clock_abstime2ticks(CLOCK_REALTIME, ...)`.
- Returns `ETIMEDOUT` immediately when the converted tick delay is already non-positive.
- Arms the watchdog with `sem_timeout()` and then blocks through `sem_wait()`.
- Cancels and deletes the watchdog after the wait completes, then restores the saved `errno` value because `wd_delete()` can overwrite it during memory-free paths.

## Inputs and Outputs

- `sem`: semaphore object to acquire.
- `abstime`: absolute `CLOCK_REALTIME` deadline.
- Return value: `OK` on success, or `ERROR` with `errno = ENOMEM`, `EINVAL`, `ETIMEDOUT`, or the error propagated from `sem_wait()`.

## Dependencies

- Uses `sem_trywait.md` for the non-blocking fast path.
- Uses `sem_wait.md` for the blocking acquire path.
- Uses `sem_timeout.md` as the watchdog callback on timeout.
- Uses `clock_abstime2ticks()` and the watchdog API (`wd_create()`, `wd_start()`, `wd_cancel()`, `wd_delete()`).

## Notes

- The deadline is always interpreted against `CLOCK_REALTIME`; there is no per-call clock selector in this implementation.
- `abstime` is only checked for `NULL` inside a debug-only guard, so callers should supply a valid pointer even in release builds.
