# `net_poll`

## Summary

`net_poll()` dispatches poll setup and teardown for a network descriptor to the active netstack selected by `get_netstack_byfd(sd)`.

## Behavior

- Does not implement readiness waiting itself.
- Passes `sd`, `fds`, and `setup` straight to the active stack's `poll` hook through `NETSTACK_CALL_BYFD()`.
- Lets the backend own the wait state, semaphore handling, and readiness classification.
- Returns the backend's poll result directly when a stack and hook are present.
- Falls back to `-1` if no stack or no `poll` hook is available for the descriptor.

## Inputs and Outputs

- `sd`: network descriptor routed through the active netstack.
- `fds`: live `struct pollfd` entry supplied by the caller.
- `setup`: `true` to register, `false` to tear down.
- Return value: backend result directly, including backend-specific failures such as lwIP's negated errno values, or `-1` when no stack or hook is available.

## Dependencies

- Uses `get_netstack_byfd()` to select the active stack.
- Uses the active stack's `poll(int, struct pollfd *, bool)` implementation.

## Notes

- On lwIP-backed sockets, the actual wait state is implemented by `lwip_poll()` and its internal setup/teardown helpers.
- `net_poll()` is the bridge layer beneath the VFS `poll()` wrapper, not a second readiness engine.
