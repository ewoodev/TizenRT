# `sem_wait`

## Summary

`sem_wait()` acquires one semaphore count, blocking the caller when no count is currently available.

## Behavior

- Rejects uninitialized semaphores with `EINVAL`.
- Treats the call as a cancellation point and returns `ECANCELED` when a deferred cancellation is already pending before the wait starts.
- Takes the fast path when `semcount > 0`: decrements the count, records holder ownership, clears `waitsem`, and returns `OK`.
- When no count is available, decrements `semcount`, stores the semaphore in `rtcb->waitsem`, and blocks the current task in `TSTATE_WAIT_SEM`.
- Boosts holder priority before blocking when `CONFIG_PRIORITY_INHERITANCE` is enabled and the semaphore has not disabled inheritance.
- Returns `OK` after unblock unless the wakeup path set `errno` to `EINTR` or `ETIMEDOUT`.
- Leaves cleanup of interrupted or timed-out waits to `sem_waitirq()`, which restores `semcount`, clears `waitsem`, and restores holder priority before the task resumes here.

## Inputs and Outputs

- `sem`: initialized semaphore object.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL`, `ECANCELED`, `EINTR`, or `ETIMEDOUT`.

## Dependencies

- Uses `sem_addholder()` on the fast path.
- Uses `sem_boostpriority()` and `sem_canceled()` indirectly through the block/unblock flow when priority inheritance is active.
- `sem_post.md`, `sem_timedwait.md`, and `sem_tickwait.md` drive the wakeup paths that feed back into this function.

## Notes

- The function is not intended for normal interrupt-context use. A debug-only abort-mode path can return `OK` from interrupt context during special recovery flows, but that is not the general API contract.
- `semcount` can become negative while the task is blocked. The magnitude of that negative value represents the number of waiting threads.
