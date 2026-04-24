# `sigtimedwait`

## Summary

`sigtimedwait()` returns the lowest-numbered pending signal in `set`, or waits up to `timeout` for one to arrive.

## Behavior

- Marks `sigtimedwait()` as a cancellation point and operates on the calling task returned by `this_task()`.
- Rejects a non-`NULL` timeout with `tv_nsec < 0` or `tv_nsec >= NSEC_PER_SEC` with `EINVAL`.
- With interrupts disabled, intersects `*set` with the task-group pending queue returned by `sig_pendingset()`.
- If a matching pending signal already exists, removes the lowest-numbered one from the queue, copies its `siginfo_t` to `info` when requested, releases the queue entry, and returns that signal number immediately.
- Otherwise stores `*set` in the task-local `sigwaitmask` and blocks the task in `TSTATE_WAIT_SIG`.
- When `timeout` is non-`NULL`, converts the relative interval to system ticks by rounding up, allocates a watchdog, and uses that watchdog to unblock the task with a synthetic timeout record in `sigunbinfo`.
- When a signal wakes the blocked task, `sig_dispatch()` copies that signal's `siginfo_t` into `sigunbinfo`, clears `sigwaitmask`, and unblocks the task.
- After wakeup, clears `sigwaitmask`, returns the signal number if the wakeup signal is a member of `set`, reports `EINTR` if another unmasked signal interrupted the wait, and reports `EAGAIN` if the watchdog timeout fired.
- Copies the final wakeup record from `sigunbinfo` into `info` when the caller provides a non-`NULL` output pointer, including the timeout sentinel record on `EAGAIN`.

## Inputs and Outputs

- `set`: signal set to select from or wait for.
- `info`: optional destination for the returned signal details.
- `timeout`: optional relative timeout; `NULL` means wait indefinitely.
- Return value: the selected signal number on success, or `ERROR` with `errno` set on failure.

## Dependencies

- Uses `sig_pendingset()`, `sig_removependingsignal()`, and `sig_releasependingsignal()` for the immediate pending-signal path.
- Uses `up_block_task()` plus the task-local `sigwaitmask` / `sigunbinfo` fields for the blocking wait.
- Uses `wd_create()`, `wd_start()`, and `wd_delete()` for timed waits.
- Relies on `sig_dispatch()` to wake a `TSTATE_WAIT_SIG` task when a signal is delivered.

## Errors and Limits

- Returns `EAGAIN` when `timeout` expires before any signal in `set` is delivered.
- Returns `EINTR` when the wait is interrupted by an unmasked signal that is not a member of `set`.
- Returns `EINVAL` when `timeout` is non-`NULL` and contains `tv_nsec < 0` or `tv_nsec >= NSEC_PER_SEC`.
- Returns `ENOMEM` when a timed wait cannot allocate its per-task watchdog.
- There is no public validation for `set`; the implementation dereferences it unconditionally.
- Timeout resolution is limited to system clock ticks, and the requested interval is rounded up so the wait is not shorter than requested.
- Pending signals come from the task group's shared pending queue, and the downstream pending-signal infrastructure retains only one pending non-real-time signal per signal number.

## Notes

- `timeout == NULL` makes this the same blocking path used by `sigwaitinfo()`.
- The caller should block the signals in `set` beforehand if it needs handler-free consumption; this owner does not enforce that and an unmasked signal in `set` can still follow the normal dispatch path.
- Passing an empty `set` cannot produce a successful signal return; the call can finish only by timeout or interruption from another unmasked signal.
- On timeout with `info != NULL`, the copied record carries the internal sentinel `si_signo = 0xff` and `si_code = SI_TIMER`.
