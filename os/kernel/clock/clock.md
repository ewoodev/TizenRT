# `clock`

## Summary

`clock()` returns the current system timer count in clock ticks.

## Behavior

- Calls `clock_systimer()` and returns that value directly.
- Does not inspect task state or accumulate per-process CPU usage.

## Inputs and Outputs

- Inputs: none.
- Return value: the current system timer count in clock ticks.

## Dependencies

- Uses `clock_systimer()`, which reads either the global tick counter or a platform timer depending on build configuration.

## Notes

- This behavior is narrower than the usual POSIX interpretation of `clock()`: it reflects system uptime ticks, not process CPU consumption.
- In tickless builds, the returned value comes from platform timer time converted back into tick units.
