# `timer_create`

## Summary

`timer_create()` allocates a watchdog-backed POSIX timer object and returns it in the disarmed state.

## Behavior

- Rejects any clock other than `CLOCK_REALTIME`.
- Rejects a NULL `timerid` output pointer.
- Allocates one watchdog with `wd_create()` and one timer structure from the preallocated pool or the kernel heap.
- Records the caller PID as the timer owner so `timer_deleteall()` can clean it up on task exit.
- Copies `sigev_signo` and `sigev_value` from `evp` when provided.
- Defaults to `SIGALRM` and stores the timer handle in `sigev_value.sival_ptr` when `evp` is NULL.
- Leaves the timer disarmed until `timer_settime()` arms it.

## Inputs and Outputs

- `clockid`: must be `CLOCK_REALTIME`.
- `evp`: optional notification template copied into the timer object.
- `timerid`: output location for the new timer handle.
- Return value: `OK` on success, or `ERROR` with `errno` set to `EINVAL` or `EAGAIN`.

## Dependencies

- `wd_create()` allocates the underlying watchdog timer.
- `timer_allocate()` pulls a timer object from `CONFIG_PREALLOC_TIMERS` storage or `kmm_malloc()`.
- `timer_settime.md` arms the timer after creation.
- `timer_delete.md` releases the handle.

## Notes

- The handle is a pointer to the internal `struct posix_timer_s`.
- The implementation does not support other POSIX clock IDs.
