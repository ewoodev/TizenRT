# `net_dupsd2`

## Summary

`net_dupsd2()` is the socket-side descriptor replacement bridge. It forwards duplication from one socket descriptor into another descriptor slot through the active network stack selected from the source descriptor.

## Behavior

- Uses `NETSTACK_CALL_BYFD(sd1, dup2, ...)` to select the active netstack with `get_netstack_byfd(sd1)`.
- Passes both descriptors straight to the backend `dup2` hook.
- Returns the backend dup2 result directly when a stack and `dup2` hook are present.
- Falls back to raw `-1` when no stack or no `dup2` hook is available for the source descriptor.
- Does not add its own descriptor validation or errno translation.

## Inputs and Outputs

- `sd1`: source socket descriptor used for netstack selection and duplication.
- `sd2`: destination descriptor slot passed to the backend.
- Return value: backend dup2 result directly, or `-1` when the bridge cannot dispatch the request.

## Dependencies

- Uses `NETSTACK_CALL_BYFD()` from `netstack.h`.
- Depends on the selected netstack exposing a `dup2(int, int)` hook.

## Notes

- The current lwIP backend implements `dup2` logic and returns `0` on success or `-1` with errno on failure.
- `dup2.md` in `os/fs/vfs` describes the public file-descriptor wrapper that reaches this helper for socket descriptors.
