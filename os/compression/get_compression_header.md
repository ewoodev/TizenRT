# `get_compression_header`

## Summary

`get_compression_header()` exposes the header object cached by the current decompression session.

## Behavior

- Returns the module-global `compression_header` pointer directly.
- Does not copy the structure or transfer ownership.

## Inputs and Outputs

- No parameters.
- Return value: the active `struct s_header *`, or `NULL` when no session is initialized.

## Notes

- The returned pointer becomes invalid after `compress_uninit()`.
- Callers are expected to treat the header as owned by the compression module.
