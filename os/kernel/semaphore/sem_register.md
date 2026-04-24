# `sem_register`

## Summary

`sem_register()` adds a kernel semaphore to the global recovery list used by binary-manager recovery logic.

## Behavior

- Enters a critical section before traversing or updating the global queue.
- Scans `g_sem_list` for the same semaphore pointer and returns immediately when it is already registered.
- Appends the semaphore to the tail of `g_sem_list` when it is not already present.
- Does not return a status value to indicate duplicate detection or insertion failure.

## Inputs and Outputs

- `sem`: kernel semaphore to track.
- Return value: none.

## Dependencies

- Operates on the global `g_sem_list` queue defined in `sem_list.c`.
- Reached from the public `sem_init()` implementation in `lib/libc/semaphore/sem_init.c` when kernel-space semaphores are initialized under binary-manager recovery.
- Paired with `sem_unregister.md`.

## Notes

- The prototype is exposed only when `CONFIG_BINMGR_RECOVERY` is enabled.
- The list membership check is pointer-based; the function does not compare semaphore contents.
