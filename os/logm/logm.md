# `logm`

## Purpose

Provides the public variadic logging entry point for the logger module.

## Behavior

- Opens a `va_list` from the caller arguments.
- Forwards the request to `logm_internal()`.
- Closes the `va_list` before returning.

## Parameters

- `flag`: selects the logging path
- `indx`: source index placeholder
- `priority`: priority placeholder
- `fmt`: format string
- `...`: format arguments

## Return Value

Returns the value from `logm_internal()`.

## Notes

All routing, buffering, timestamping, and overflow handling live in `logm_internal()`. This wrapper only manages the variadic interface.
