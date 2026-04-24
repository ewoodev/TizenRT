# `sem_trywait`

## Summary

`sem_trywait()` attempts to acquire one semaphore count without blocking.

## Behavior

- Rejects uninitialized semaphores with `EINVAL`.
- Returns immediately with `EAGAIN` when `semcount <= 0`.
- On success, decrements `semcount`, records holder ownership, clears `waitsem`, and returns `OK`.
- Executes the acquire path inside a critical section because `sem_post()` may run from interrupt context.
- Checks mutex-style semaphores with a debug assertion that the count never exceeds one.

## Inputs and Outputs

- `sem`: initialized semaphore object.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL` or `EAGAIN`.

## Dependencies

- Uses `sem_addholder()` for holder bookkeeping.
- Serves as the fast path for `sem_timedwait.md` and `sem_tickwait.md`.

## Notes

- The implementation relies on a debug assertion to reject interrupt-context callers. In non-debug builds the public contract is still non-blocking thread context, not ISR use.
