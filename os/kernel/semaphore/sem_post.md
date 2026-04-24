# `sem_post`

## Summary

`sem_post()` releases one semaphore count and wakes one waiting task when the semaphore still has blocked waiters.

## Behavior

- Enters a critical section before validating the semaphore object.
- Rejects a `NULL` or uninitialized semaphore with `EINVAL`.
- Releases holder ownership for the current task through `sem_releaseholder()`.
- Increments `semcount` and asserts that the value stays below `SEM_VALUE_MAX`.
- Calls `sem_unblock_task()` after incrementing the count.
- `sem_unblock_task()` scans the global waiting-for-semaphore list, finds the first task waiting on this semaphore, assigns holder ownership to that task, clears `waitsem`, and unblocks it.
- Restores holder priority or frees holder bookkeeping while the wakeup path still runs with scheduling locked when priority inheritance support is enabled.

## Inputs and Outputs

- `sem`: initialized semaphore object.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL`.

## Dependencies

- Uses `sem_releaseholder()` and `sem_unblock_task()` for bookkeeping and wakeup.
- Completes waits started by `sem_wait.md`, `sem_timedwait.md`, and `sem_tickwait.md`.

## Notes

- The implementation is explicitly written so it can be called from interrupt context.
- When no task is waiting, the function simply increments the stored count and returns.
