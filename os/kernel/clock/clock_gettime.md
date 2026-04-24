# `clock_gettime`

## Summary

`clock_gettime()` returns either the current realtime value or, when enabled, the `CLOCK_MONOTONIC` value produced by the shared clock helper.

## Behavior

- Rejects a `NULL` `tp` pointer with `EINVAL`.
- When `CONFIG_CLOCK_MONOTONIC` is enabled and `clockid == CLOCK_MONOTONIC`, returns the `clock_systimespec()` result directly.
- When `clockid == CLOCK_REALTIME`, reads the current bias with `clock_systimespec()`, adds `g_basetime`, normalizes nanosecond carry, and stores the result in `tp`.
- Rejects unsupported clock IDs with `EINVAL`.
- Converts negative helper results into `errno` before returning `ERROR`.

## Inputs and Outputs

- `clockid`: `CLOCK_REALTIME`, or `CLOCK_MONOTONIC` when that option is enabled.
- `tp`: destination buffer for the resulting `timespec`.
- Return value: `OK` on success, or `ERROR` with `errno` set from validation or a negative helper result.

## Dependencies

- Uses `clock_systimespec()` for the shared clock helper result.
- Uses `g_basetime` as the realtime offset.
- `gettimeofday.md` depends on this function for its wall-clock source.

## Notes

- `CLOCK_REALTIME` can jump when `clock_settime()` changes `g_basetime`.
- `CLOCK_MONOTONIC` is compiled in only when `CONFIG_CLOCK_MONOTONIC` is enabled.
- The `CLOCK_MONOTONIC` branch still shares `clock_systimespec()`, so backend details matter when judging how isolated it is from realtime adjustments.
- With hi-res RTC support, the helper path may read RTC hardware and can surface negative errno-style failures before this wrapper maps them to `errno`.
