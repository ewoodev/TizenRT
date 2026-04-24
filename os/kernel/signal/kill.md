# `kill`

## Summary

`kill()` sends one signal to the task identified by a positive pid, or performs an existence check when `signo == 0`.

## Behavior

- Rejects `pid <= 0` with `ENOSYS` because this owner implementation does not support process-group signaling.
- Treats `signo == 0` as a pure existence check.
- For the existence-check path, looks up the target TCB and, when group support is enabled, also accepts a surviving task-group owner lookup through `group_findbypid()`.
- For real signals, validates the signal number and builds a `siginfo_t` with `SI_USER`.
- Fills the sender pid when `CONFIG_SCHED_HAVE_PARENT` is enabled.
- Dispatches the signal through `sig_dispatch()` while the scheduler is locked.
- Converts negative internal dispatch results into `errno` before returning `ERROR`.

## Inputs and Outputs

- `pid`: positive pid of the destination task or task-group owner.
- `signo`: signal number to send, or `0` for existence checking only.
- Return value: `OK` on success, or `ERROR` with `errno` set on failure.

## Dependencies

- Depends on `sig_dispatch()` for actual delivery.
- Uses `sched_gettcb()` and, with group support, `group_findbypid()` for the `signo == 0` existence-check path.

## Errors and Limits

- Returns `ENOSYS` for `pid <= 0` because process-group signaling is not implemented here.
- Returns `EINVAL` for invalid non-zero signal numbers.
- Returns `ESRCH` when the target pid cannot be resolved.

## Notes

- This tranche fixes the `signo == 0` path so it no longer falls through into real signal dispatch.
- There is no permission model in this owner implementation, so the documented error surface is narrower than full POSIX `kill()`.
