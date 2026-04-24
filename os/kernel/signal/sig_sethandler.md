# `sig_sethandler`

## Summary

`sig_sethandler()` installs or replaces one signal action entry on a caller-specified target task.

## Behavior

- Validates `tcb`, `act`, and `signo` before touching the target queue.
- Locks the scheduler while it updates the target task's `sigactionq`.
- Searches the target queue for an existing entry with the same signal number.
- Reuses that entry when present so repeated registrations replace the previous handler instead of silently appending a duplicate.
- Allocates a new `sigactq_t` with `sig_allocateaction()` only when no entry exists yet.
- Copies the caller-supplied `struct sigaction` into the selected queue entry with `COPY_SIGACTION()`.

## Inputs and Outputs

- `tcb`: target task whose signal-action queue will be updated.
- `signo`: signal number to install or replace.
- `act`: replacement action to copy.
- Return value: `OK` on success, or `ERROR` with `errno` set on failure.

## Dependencies

- Uses the target task's `sigactionq` directly.
- Depends on `sig_allocateaction()` from the same folder for new queue entries.
- Uses `COPY_SIGACTION()` to copy the public action structure into kernel-owned storage.

## Errors and Limits

- Returns `EINVAL` if `tcb` is `NULL`, `act` is `NULL`, or `signo` is outside the supported signal range.
- Returns `ENOMEM` if a new queue entry is required and allocation fails.
- Does not implement `SIG_IGN` removal semantics; it only installs or replaces queue entries.

## Notes

- This helper operates on an arbitrary target TCB, unlike `sigaction()` which always works on the calling task.
- This tranche fixes two real bugs: repeated registrations now replace the existing entry instead of leaving the older handler active, and the `ENOMEM` path now releases the scheduler lock before returning.
