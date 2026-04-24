# `sem_destroy`

## Summary

`sem_destroy()` marks an unnamed semaphore invalid and tears down holder and recovery bookkeeping. It does not actively wake threads that are already waiting on the semaphore.

## Behavior

- Rejects a `NULL` semaphore pointer with `EINVAL`.
- Returns `OK` immediately when the semaphore is already uninitialized.
- Enters a critical section before changing the semaphore state.
- Resets `semcount` to `1` only when the current count is non-negative.
- Leaves a negative `semcount` unchanged, even if that means waiters are still recorded.
- Calls `sem_destroyholder()` to clear holder-tracking state.
- Calls `sem_unregister()` for non-signal semaphores when `CONFIG_BINMGR_RECOVERY` is enabled.
- Clears `sem->flags` and returns `OK`.

## Inputs and Outputs

- `sem`: semaphore object previously initialized with `sem_init()`.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL` for a `NULL` pointer.

## Dependencies

- Uses `sem_destroyholder()` for holder cleanup.
- Uses `sem_unregister()` when binary-manager recovery tracks kernel semaphores.
- Pairs with the public `sem_init()` entry point, whose implementation currently lives in `lib/libc/semaphore/sem_init.c`.

## Notes

- The source comment says waiter behavior is undefined, and the implementation follows that narrow contract: it still returns success without waking or reassigning blocked waiters.
- After `flags` is cleared, subsequent semaphore operations treat the object as invalid until it is reinitialized.
