# `sigqueue`

## Summary

`sigqueue()` sends one signal plus a caller-supplied value to the task identified by `pid`.

## Behavior

- Rejects invalid signal numbers and also rejects `signo == 0`.
- Builds a `siginfo_t` tagged with `SI_QUEUE`.
- Copies the caller-supplied `union sigval` or pointer payload into `si_value`, depending on `CONFIG_CAN_PASS_STRUCTS`.
- Fills the sender pid when `CONFIG_SCHED_HAVE_PARENT` is enabled.
- Dispatches the request through `sig_dispatch()` while the scheduler is locked.
- Converts negative internal dispatch results into `errno` before returning `ERROR`.

## Inputs and Outputs

- `pid`: destination pid.
- `signo`: signal number to queue; `0` is rejected.
- `value` / `sival_ptr`: payload stored in `siginfo_t.si_value`.
- Return value: `OK` on success, or `ERROR` with `errno` set on failure.

## Dependencies

- Depends on `sig_dispatch()` for actual delivery and pending-queue behavior.
- Uses the standard `siginfo_t` path shared with other owner-local signal senders.

## Errors and Limits

- Returns `EINVAL` for invalid signal numbers and for `signo == 0`.
- Returns `ENOMEM` when the downstream dispatch path cannot allocate pending signal-action storage.
- Returns `ESRCH` when the destination pid cannot be resolved.

## Notes

- Only one pending non-real-time signal per signal number is retained by the downstream signal infrastructure.
- This tranche corrects the public/source documentation to match the real error surface from `sig_dispatch()`, which does not report `EAGAIN` or permission failures here.
