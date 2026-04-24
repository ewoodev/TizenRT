# `get_errno`

## Purpose

`get_errno()` returns the current errno value selected for the caller's execution context.

## Behavior

- Calls `get_errno_ptr()`.
- Dereferences the returned pointer.
- Returns the stored integer value without additional validation or translation.

## Inputs and Outputs

- Inputs: none
- Return value: current errno value

## Notes

- In builds with direct errno access enabled, most code uses the `errno` macro instead of this function.
- The function is still the real implementation behind the accessor path used when direct errno access is unavailable.
