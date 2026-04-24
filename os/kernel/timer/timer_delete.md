# `timer_delete`

## Summary

`timer_delete()` releases a timer handle previously returned by `timer_create()`.

## Behavior

- Passes the handle to the internal `timer_release()` helper.
- Deletes the watchdog and invalidates the timer immediately when no other internal reference remains.
- Leaves final cleanup deferred when the timeout path still holds a reference.
- Returns `ERROR` only when the handle is invalid for `timer_release()`.

## Inputs and Outputs

- `timerid`: timer handle from `timer_create()`.
- Return value: `OK` on success, or `ERROR` with `errno` set from the negated `timer_release()` result.

## Dependencies

- `timer_create.md` describes handle creation.
- `timer_settime.md` describes arming and rearming before deletion.
- Internal cleanup happens in `timer_release()` and may return the timer to the `CONFIG_PREALLOC_TIMERS` pool.

## Notes

- `timer_deleteall()` uses the same release path to clean up timers owned by an exiting task.
- Callers must not use the handle again after a successful delete request, even though final teardown can be deferred until the timeout path drops its last internal reference.
