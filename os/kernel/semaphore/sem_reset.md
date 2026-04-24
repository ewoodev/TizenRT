# `sem_reset`

## Summary

`sem_reset()` forces a semaphore to a new non-negative count, waking blocked waiters first when the old count is already negative.

## Behavior

- Rejects a `NULL` semaphore, an uninitialized semaphore, or a negative target count with `EINVAL`.
- Locks the scheduler and enters a critical section before modifying the semaphore.
- While the semaphore still has waiters (`semcount < 0`) and the requested new count is still positive, repeatedly calls `sem_post()` to hand counts directly to waiting threads.
- Decrements the requested count after each wakeup so only the remaining budget is written back.
- If no waiters remain, stores the remaining `count` directly into `sem->semcount`.
- Leaves the stored negative wait count unchanged when blocked waiters still remain after the requested count budget is exhausted.

## Inputs and Outputs

- `sem`: initialized semaphore object.
- `count`: new non-negative target count.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL`.

## Dependencies

- Uses `sem_post.md` to wake blocked waiters while preserving normal holder and wakeup bookkeeping.

## Notes

- This function is a recovery-style API. It does more than a raw count assignment because it must preserve the semantics of already-blocked waiters.
