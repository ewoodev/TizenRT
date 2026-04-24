# `gettimeofday`

## Summary

`gettimeofday()` returns the current realtime clock value as a `timeval`.

## Behavior

- Calls `clock_gettime(CLOCK_REALTIME, &ts)` to fetch the current wall-clock time.
- Converts the successful `timespec` result into `timeval` by copying seconds
  and truncating nanoseconds to microseconds.
- Ignores the `tz` argument.

## Inputs and Outputs

- `tv`: destination buffer for the current realtime value.
- `tz`: accepted for compatibility, but unused.
- Return value: `OK` on success, or `ERROR` when `clock_gettime()` fails.

## Error Handling

- When `CONFIG_DEBUG` is enabled and `tv` is `NULL`, the function sets
  `errno` to `EINVAL` and returns `ERROR`.
- Otherwise it propagates the result from `clock_gettime()`.

## Dependencies

- `clock_gettime(CLOCK_REALTIME, ...)`
- `NSEC_PER_USEC`

## Notes

- The function is a thin wrapper over `clock_gettime()` rather than an
  independent time source.
- Because `tz` is ignored, callers should not expect timezone data to be
  populated.
