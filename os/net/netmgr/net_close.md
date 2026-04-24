# `net_close`

## Summary

`net_close()` is the socket-side close bridge. It resolves the active network stack from the descriptor and forwards the close request directly to that stack.

## Behavior

- Uses `NETSTACK_CALL_BYFD(sd, close, ...)` to select the active netstack with `get_netstack_byfd(sd)`.
- Returns the backend close result directly when a stack and `close` hook are present.
- Falls back to raw `-1` when no stack or no `close` hook is available for the descriptor.
- Does not add its own errno translation.

## Inputs and Outputs

- `sd`: socket descriptor to close.
- Return value: backend close result directly, or `-1` when the bridge cannot dispatch the request.

## Dependencies

- Uses `NETSTACK_CALL_BYFD()` from `netstack.h`.
- Depends on the selected netstack exposing a `close(int)` hook.

## Notes

- On lwIP-backed sockets, this bridge reaches `lwip_close()`.
- The wrapper itself does not clear socket bookkeeping; that happens inside the backend.
