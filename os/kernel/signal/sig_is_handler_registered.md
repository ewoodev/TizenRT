# `sig_is_handler_registered`

## Summary

`sig_is_handler_registered()` reports whether a target task currently has a signal action entry for one signal number.

## Behavior

- Validates the target `tcb` pointer and signal number first.
- Delegates the queue lookup to `sig_findaction()`.
- Returns `true` when `sig_findaction()` finds a matching entry in the target task's `sigactionq`.
- Returns `false` when the target is invalid, the signal number is invalid, or no entry exists.

## Inputs and Outputs

- `tcb`: target task to inspect.
- `signo`: signal number to look up.
- Return value: `true` if a queue entry exists for that signal, otherwise `false`.

## Dependencies

- Depends on `sig_findaction()` from the same folder.
- Reads the target task's `sigactionq` through that helper.

## Notes

- This helper reports queue-entry presence only; it does not verify whether the stored action points to a non-NULL callable handler.
- The check is task-local and does not imply anything about other tasks or global signal state.
