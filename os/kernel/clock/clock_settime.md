# `clock_settime`

## Summary

`clock_settime()` updates the realtime base offset used by `clock_gettime(CLOCK_REALTIME)` and `gettimeofday()`.

## Behavior

- Rejects a `NULL` `tp` pointer with `EINVAL`.
- Rejects every `clockid` except `CLOCK_REALTIME` with `EINVAL`.
- Enters a critical section before updating the global realtime base.
- Copies the requested wall-clock time into `g_basetime`.
- Reads the current bias from `clock_systimespec()` and subtracts that value from `g_basetime`.
- Leaves later clock reads using the adjusted `g_basetime` through the same helper path.

## Inputs and Outputs

- `clockid`: must be `CLOCK_REALTIME`.
- `tp`: requested realtime value.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL` on invalid input.

## Dependencies

- Uses `clock_systimespec()` to obtain the current helper bias before rebuilding `g_basetime`.
- Updates the shared `g_basetime` state consumed by `clock_gettime()` and `gettimeofday()`.

## Notes

- The implementation does not call `up_rtc_settime()`, so setting realtime only adjusts the in-memory base used by later reads.
- In hi-res RTC builds, the helper used for the bias calculation may still read RTC hardware instead of a pure uptime counter.
- Do not assume `CLOCK_MONOTONIC` is isolated from this call across all backends; both clock IDs share `clock_systimespec()`.
