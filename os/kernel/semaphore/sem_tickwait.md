# `sem_tickwait`

## Summary

`sem_tickwait()` waits for a semaphore for at most a caller-supplied number of system ticks, adjusted relative to an earlier start tick.

## Behavior

- Creates a watchdog reservation before entering the wait path and returns `ENOMEM` if allocation fails.
- Enters a critical section and first attempts `sem_trywait()`.
- Returns immediately on that fast-path success.
- When `delay == 0`, returns the current `sem_trywait()` error without arming a watchdog.
- Computes elapsed ticks as `clock_systimer() - start` and reduces the remaining delay by that elapsed time.
- Returns `ETIMEDOUT` immediately if the elapsed time already consumed the entire delay budget.
- Arms the watchdog with the adjusted delay and blocks through `sem_wait()`.
- Cancels and deletes the watchdog after the wait, preserving the earlier error code because `wd_delete()` can overwrite `errno`.

## Inputs and Outputs

- `sem`: semaphore object to acquire.
- `start`: earlier system-tick snapshot used to compensate for setup latency.
- `delay`: total relative timeout budget in system ticks.
- Return value: `OK` on success, or `ERROR` with `errno` from `sem_trywait()`, `sem_wait()`, `sem_timeout()`, or `ENOMEM`.

## Dependencies

- Uses `sem_trywait.md` for the non-blocking fast path.
- Uses `sem_wait.md` for the blocking path.
- Uses `sem_timeout.md` as the watchdog callback.
- Uses `clock_systimer()` to compute the remaining relative delay.

## Notes

- This API is RTOS-specific and is declared in `tinyara/semaphore.h`, not in the POSIX `semaphore.h` header.
- The function does not normalize the caller's `start` tick; it trusts the caller to pass a compatible `clock_systimer()` snapshot.
