# `logm_get_values`

## Purpose

Reads runtime configuration values from the logger module.

## Supported Parameters

- `LOGM_BUFSIZE`
  Returns the current active ring-buffer size in bytes.
- `LOGM_INTERVAL`
  Returns the current flush interval in milliseconds.

## Behavior

- Reads from the active runtime state, not from the pending resize request.
- Leaves unsupported parameter types unchanged.
- Always returns `0`.

## Notes

The interval is stored internally in microseconds. This API converts it back to milliseconds before returning it.
