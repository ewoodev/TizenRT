# `sem_unregister`

## Summary

`sem_unregister()` removes a kernel semaphore from the global recovery list.

## Behavior

- Enters a critical section before mutating the global queue.
- Removes the semaphore from `g_sem_list` with `sq_rem()`.
- Returns no status and does not report whether the semaphore was actually present in the list.

## Inputs and Outputs

- `sem`: kernel semaphore to stop tracking.
- Return value: none.

## Dependencies

- Operates on the global `g_sem_list` queue defined in `sem_list.c`.
- Reached from `sem_destroy.md` when binary-manager recovery is enabled and the semaphore is not marked as a signal semaphore.
- Paired with `sem_register.md`.

## Notes

- The prototype is exposed only when `CONFIG_BINMGR_RECOVERY` is enabled.
