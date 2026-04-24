# `net_checksd`

## Summary

`net_checksd()` is the socket-descriptor validation wrapper for the active netstack backend.

## Behavior

- Selects a backend through `get_netstack_byfd(sd)`, which uses the descriptor range to distinguish supported socket classes.
- Invokes the backend `checksd` hook through `NETSTACK_CALL_BYFD()`.
- Returns the backend result directly.
- Falls back to `-1` when no backend or no `checksd` hook is available for the descriptor.

## Inputs and Outputs

- `sd`: descriptor to validate.
- `oflags`: requested access/status flags passed through to the backend.
- Return value: backend-defined result, or `-1` when dispatch fails before a backend runs.

## Dependencies

- Depends on `get_netstack_byfd()` from `netstack.c`.
- Uses the backend `checksd` hook declared in `netmgr/netstack.h`.

## Notes

- The wrapper itself does not set `errno`.
- The current lwIP backend hook is still a stub that logs `"Not supported yet"` and returns `0`, so the wrapper currently does not enforce meaningful validation for that stack.
