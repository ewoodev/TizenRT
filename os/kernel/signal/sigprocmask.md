# `sigprocmask`

## Summary

`sigprocmask()` examines or updates the calling task's blocked-signal mask.

## Behavior

- Reads the current task from `this_task()` and snapshots the existing `sigprocmask`.
- Copies the previous mask to `oset` when requested.
- If `set` is `NULL`, acts only as a query and ignores `how`.
- Otherwise enters a critical section, applies the requested mask operation to `rtcb->sigprocmask`, then leaves the critical section.
- Supports `SIG_BLOCK`, `SIG_UNBLOCK`, and `SIG_SETMASK`.
- Calls `sig_unmaskpendingsignal()` only after a successful update so newly unblocked pending signals can be dispatched before returning.

## Inputs and Outputs

- `how`: update mode used only when `set` is non-`NULL`.
- `set`: signal-set operand for the update, or `NULL` for query-only behavior.
- `oset`: destination for the previous mask, or `NULL` if the caller does not need it.
- Return value: `0` on success, or `-1` with `errno` set on failure.

## Dependencies

- Depends on the current task returned by `this_task()`.
- Uses `enter_critical_section()` / `leave_critical_section()` to protect non-atomic mask updates against interrupt-side signal activity.
- Uses `sig_unmaskpendingsignal()` from the same folder to process signals that became deliverable.

## Errors and Limits

- Returns `EINVAL` if `set` is non-`NULL` and `how` is not one of `SIG_BLOCK`, `SIG_UNBLOCK`, or `SIG_SETMASK`.
- The function operates on task-local mask state, not a process-wide shared mask.

## Notes

- This tranche fixes the invalid-`how` path so it now sets `errno` to `EINVAL` before returning `ERROR` and skips the pending-signal dispatch pass on that failure path.
- The function snapshots the old mask before any modification, so `oset` reflects the pre-call state even when the caller also supplies a new mask.
