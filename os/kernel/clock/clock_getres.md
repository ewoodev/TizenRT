# `clock_getres`

## Summary

`clock_getres()` reports the resolution used for supported realtime clock reads.

## Behavior

- Rejects a `NULL` `res` pointer with `EINVAL`.
- Accepts only `CLOCK_REALTIME`.
- Fills `res` with `{ tv_sec = 0, tv_nsec = NSEC_PER_TICK }`.
- Rejects every other clock ID with `EINVAL`, even when `CLOCK_MONOTONIC` support is compiled into `clock_gettime()`.

## Inputs and Outputs

- `clockid`: must be `CLOCK_REALTIME`.
- `res`: destination buffer for the reported resolution.
- Return value: `OK` on success, or `ERROR` with `errno = EINVAL` on invalid input.

## Dependencies

- Uses the build-time tick resolution constant `NSEC_PER_TICK`.

## Notes

- The resolution reported here is tied to the system tick configuration, not to any separate RTC hardware capability.
