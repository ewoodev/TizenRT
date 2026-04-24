# `sigaction`

## Summary

`sigaction()` examines or replaces the calling task's signal disposition for one signal number.

## Behavior

- Validates `signo` with `GOOD_SIGNO()` and fails with `EINVAL` for out-of-range signals.
- Reads and updates only the calling task's signal state through `this_task()`.
- For ordinary signals, looks up an existing action with `sig_findaction()`.
- Copies the previous action to `oact` when requested; if no action is installed, returns an empty record with a `NULL` handler, empty mask, and zero flags.
- Treats `SIG_DFL` like `SIG_IGN` because this owner implementation does not provide default actions.
- Removes the existing queue entry when the requested action is ignore/default.
- Otherwise reuses the existing queue entry or allocates a new one with `sig_allocateaction()`, then copies the caller's `struct sigaction`.
- When `CONFIG_SCHED_HAVE_PARENT` and `CONFIG_SCHED_CHILD_STATUS` are both enabled, a `SIGCHLD` action with `SA_NOCLDWAIT` also sets the group's no-child-status-retention flag and drops pending child-exit records.
- When `CONFIG_SIGKILL_HANDLER` is enabled and `signo == SIGKILL`, bypasses the normal sigaction queue and reads or writes `rtcb->sigkillusrhandler` directly through the `sa_sigaction` field.

## Inputs and Outputs

- `signo`: signal number to query or replace.
- `act`: replacement action, or `NULL` to query only.
- `oact`: destination for the previous action, or `NULL` when the caller does not need it.
- Return value: `0` on success, or `-1` with `errno` set on failure.

## Dependencies

- Depends on the current task returned by `this_task()`.
- Uses `sig_findaction()`, `sig_allocateaction()`, and `sig_releaseaction()` from the same folder.
- Uses the calling task's `sigactionq` and, for the optional SIGKILL path, `sigkillusrhandler` in the TCB.

## Errors and Limits

- Returns `EINVAL` if `signo` is outside the supported signal range.
- Returns `ENOMEM` if a new queue entry is required and allocation fails.
- Ignores most `sa_flags`; only the `SIGCHLD` / `SA_NOCLDWAIT` case has owner-specific behavior here.

## Notes

- This is task-local state, not a process-wide disposition table.
- The current owner implementation stores queued actions directly in kernel-managed TCB structures.
- This tranche fixes the `CONFIG_SIGKILL_HANDLER` query path so `sigaction(SIGKILL, NULL, oact)` no longer dereferences `act`.
