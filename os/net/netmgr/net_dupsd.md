# `net_dupsd`

## Summary

`net_dupsd()` is the socket-side descriptor duplication bridge. It forwards duplication of one socket descriptor to the active network stack selected from that descriptor.

## Behavior

- Uses `NETSTACK_CALL_BYFD(sockfd, dup, ...)` to select the active netstack with `get_netstack_byfd(sockfd)`.
- Returns the backend dup result directly when a stack and `dup` hook are present.
- Falls back to raw `-1` when no stack or no `dup` hook is available for the descriptor.
- Does not add validation or errno translation of its own.

## Inputs and Outputs

- `sockfd`: socket descriptor to duplicate.
- Return value: backend dup result directly, or `-1` when the bridge cannot dispatch the request.

## Dependencies

- Uses `NETSTACK_CALL_BYFD()` from `netstack.h`.
- Depends on the selected netstack exposing a `dup(int)` hook.

## Notes

- The current lwIP netstack hook is still a TODO stub that returns `0`, so successful-looking results do not imply that descriptor cloning is actually implemented there.
- `dup.md` in `os/fs/vfs` describes the file-descriptor wrapper that calls this helper for socket descriptors.
