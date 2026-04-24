# `net_clone`

## Summary

`net_clone()` is the socket-structure clone bridge, but the current implementation is only an unsupported stub.

## Behavior

- Logs `"Not supported yet"` through `NET_LOGKV`.
- Returns `-1` immediately.
- Does not inspect or copy either socket structure.
- Does not dispatch to the active network stack.

## Inputs and Outputs

- `psock1`: source socket structure pointer.
- `psock2`: destination socket structure pointer.
- Return value: always `-1` in the current implementation.

## Dependencies

- Uses the `NET_LOGKV` logging path in `net_vfs.c`.

## Notes

- Unlike `net_dupsd()` and `net_dupsd2()`, this helper does not use `NETSTACK_CALL_BYFD()`.
- The current lwIP netstack file contains its own clone stub, but this bridge never calls it.
