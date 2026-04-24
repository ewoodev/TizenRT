# `sem_setprotocol`

## Summary

`sem_setprotocol()` changes whether semaphore priority inheritance is enabled for a specific semaphore instance.

## Behavior

- This source file is compiled only when `CONFIG_PRIORITY_INHERITANCE` is enabled.
- Rejects a `NULL` or uninitialized semaphore with `EINVAL`.
- Accepts `SEM_PRIO_NONE` and sets `PRIOINHERIT_FLAGS_DISABLE` on the semaphore.
- When switching to `SEM_PRIO_NONE`, destroys current holder bookkeeping unless `CONFIG_BINMGR_RECOVERY` requires that holder list to remain available for recovery.
- Accepts `SEM_PRIO_INHERIT` and clears `PRIOINHERIT_FLAGS_DISABLE`.
- Rejects `SEM_PRIO_PROTECT` with `ENOSYS`.
- Rejects any other protocol value with `EINVAL`.

## Inputs and Outputs

- `sem`: initialized semaphore object.
- `protocol`: requested protocol mode.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL` or `ENOSYS`.

## Dependencies

- Uses `sem_destroyholder()` when disabling inheritance and recovery mode does not need holder tracking.
- Works with `sem_getprotocol()`, whose current public implementation lives in `lib/libc/semaphore/sem_getprotocol.c`.

## Notes

- In builds without `CONFIG_PRIORITY_INHERITANCE`, the public `sem_setprotocol()` symbol comes from `lib/libc/semaphore/sem_setprotocol.c` instead. That fallback accepts only `SEM_PRIO_NONE`.
