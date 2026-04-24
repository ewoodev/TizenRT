# `sigsuspend`

## Summary

`sigsuspend()` temporarily replaces the calling task's blocked-signal mask and waits until an unmasked signal is delivered.

## Behavior

- Marks `sigsuspend()` as a cancellation point.
- Reads the calling task through `this_task()` and inspects the task-group pending queue through `sig_pendingset()`.
- With interrupts disabled, saves the current task-local `sigprocmask`, installs `*set` as the temporary mask, and clears `sigwaitmask`.
- If the temporary mask unblocks any already pending signal, dispatches that pending work immediately through `sig_unmaskpendingsignal()` and skips the blocking sleep.
- Otherwise blocks the task in `TSTATE_WAIT_SIG`.
- While suspended, any signal that is unmasked under the temporary mask can unblock the task through `sig_dispatch()`; this implementation does not require the signal to have a user handler or terminating default action.
- After the task runs again, restores the original mask and calls `sig_unmaskpendingsignal()` so signals that become unblocked again under the restored mask can be processed before returning.
- Sets `errno` to `EINTR` and always returns `ERROR`.

## Inputs and Outputs

- `set`: temporary blocked-signal mask to use while suspended.
- Return value: `ERROR` on every return path.

## Dependencies

- Depends on the calling task returned by `this_task()`.
- Uses `sig_pendingset()` and `sig_unmaskpendingsignal()` for the immediate pending-signal path.
- Uses `up_block_task()` to enter `TSTATE_WAIT_SIG`.
- Uses `sig_unmaskpendingsignal()` after restoring the original mask.

## Errors and Limits

- There is no local validation for `set`; the implementation dereferences it unconditionally.
- Pending signals come from the task group's shared pending queue, while the temporary signal mask itself is task-local.
- There is no `siginfo_t` output path; callers learn only that the wait was interrupted.
- The wait ends on delivery of any signal that is unmasked under the temporary mask, even if that signal would otherwise be ignored.

## Notes

- Waiting with an empty mask effectively sleeps until any signal is delivered to the task.
- This tranche fixes the immediate pending-signal path so an already pending signal is dispatched instead of being dropped, and now assigns `errno = EINTR` on return.
