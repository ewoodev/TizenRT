# `sigwaitinfo`

## Summary

`sigwaitinfo()` is the untimed wrapper around `sigtimedwait()`: it waits indefinitely for the lowest-numbered pending signal selected by `set`.

## Behavior

- Marks `sigwaitinfo()` as a cancellation point.
- Delegates directly to `sigtimedwait(set, info, NULL)` and returns that result unchanged.
- Therefore first consumes the lowest-numbered matching signal already pending in the task-group pending queue, before doing any blocking wait.
- Otherwise blocks the calling task in `TSTATE_WAIT_SIG` with `sigwaitmask = *set` until a matching signal arrives or some other unmasked signal interrupts the wait.
- Copies the selected or wakeup `siginfo_t` into `info` only when the caller passes a non-`NULL` output pointer.

## Inputs and Outputs

- `set`: signal set to wait for.
- `info`: optional destination for the delivered signal details.
- Return value: the selected signal number on success, or `ERROR` with `errno` set on failure.

## Dependencies

- Depends entirely on `sigtimedwait()` for queue inspection, sleeping, and `siginfo_t` copying.
- Inherits the same `sigwaitmask` / `sigunbinfo` wakeup path used by `sig_dispatch()`.

## Errors and Limits

- Returns `EINTR` when the wait is interrupted by an unmasked signal that is not a member of `set`.
- Unlike `sigtimedwait()`, there is no timeout result here, so this wrapper does not produce `EAGAIN`.
- There is no local validation for `set`; the wrapper forwards the pointer directly to `sigtimedwait()`.
- Pending-signal selection still comes from the task group's shared pending queue, and only the lowest-numbered matching signal is returned.

## Notes

- The caller should block the signals in `set` beforehand if it needs handler-free consumption; this wrapper does not enforce that POSIX usage rule itself.
- Passing an empty `set` can never succeed in this implementation; the wait can end only when another unmasked signal interrupts it.
- `pause()`, `waitid()`, and `waitpid()` reuse this helper for signal-driven wakeups.
