# `set_errno`

## Purpose

`set_errno()` stores an errno value in the active errno slot for the current execution context.

## Behavior

- Calls `get_errno_ptr()`.
- Writes `errcode` to the returned location.
- Does not clamp, validate, or transform the supplied value.

## Inputs and Outputs

- `errcode`: value to store as errno
- Return value: none

## Notes

- In builds with direct errno access enabled, kernel code often uses the `set_errno` macro instead of this function.
- The actual storage target depends on context: running-task TCB in normal tasking, or the fallback interrupt/early-boot slot otherwise.
