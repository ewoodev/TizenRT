# `logm_set_values`

## Purpose

Updates runtime configuration values used by the logger task.

## Supported Parameters

- `LOGM_BUFSIZE`
  Stores a pending resize target in `new_logm_bufsize` after rounding the requested size up to a 4-byte boundary.
- `LOGM_INTERVAL`
  Converts the supplied millisecond interval to microseconds and stores it in `logm_print_interval`.

## Behavior

- Does not resize the buffer directly.
- Does not set `LOGM_BUFFER_RESIZE_REQ` by itself.
- Leaves unsupported parameter types unchanged.
- Always returns `0`.

## Notes

Buffer resizing is deferred. The logger task performs the actual
`kmm_realloc()` only after some other path, such as the TASH command handler,
sets `LOGM_BUFFER_RESIZE_REQ`.
