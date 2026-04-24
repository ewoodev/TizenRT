# `timer_settime`

## Summary

`timer_settime()` disarms, arms, or reprograms a timer by translating the requested timeout into watchdog ticks.

## Behavior

- Rejects invalid timer handles and a NULL `value` pointer with `EINVAL`.
- Cancels any watchdog instance already armed for the timer.
- Optionally marks the watchdog as a wakeup source when `TIMER_WAKEUPSOURCE` is set and `CONFIG_SCHED_WAKEUPSOURCE` is enabled.
- Returns immediately with `OK` when both `value->it_value.tv_sec` and `value->it_value.tv_nsec` are non-positive, leaving the timer disarmed.
- Stores a periodic reload delay in `pt_delay` only when `value->it_interval` is non-zero and the call is not handled by that early disarm return.
- Interprets `value->it_value` as an absolute `CLOCK_REALTIME` deadline when `TIMER_ABSTIME` is set, otherwise as a relative delay.
- Falls back to the stored periodic delay when the computed deadline is already in the past.
- Starts the watchdog only when the final delay is positive.

## Inputs and Outputs

- `timerid`: timer handle from `timer_create()`.
- `flags`: `TIMER_ABSTIME` for absolute deadlines and `TIMER_WAKEUPSOURCE` for watchdog wakeup registration when supported.
- `value`: requested first expiration and optional periodic interval.
- `ovalue`: accepted but ignored by the current implementation.
- Return value: `OK` on success, or the watchdog helper failure result on start or wakeup-source configuration failure.

## Dependencies

- `timer_create.md` provides the handle and signal configuration consumed here.
- Uses `clock_time2ticks()` and `clock_abstime2ticks()` to convert `struct timespec` values into watchdog ticks.
- Uses `wd_cancel()`, `wd_setwakeupsource()`, and `wd_start()` to control the underlying watchdog.
- `timer_gettime.md` reports the state written here through `pt_last` and `pt_delay`.

## Notes

- A past absolute deadline does not trigger an immediate one-shot expiration. The code only rearms from `pt_delay`, so a non-periodic timer can end up disarmed with a successful return.
- Because `ovalue` is ignored, callers cannot read the previous timer state through this API in the current implementation.
