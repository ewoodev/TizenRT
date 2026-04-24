# `net_ioctl`

## Summary

`net_ioctl()` is the socket-side ioctl bridge. It validates the command family and socket descriptor, tries the active network stack's ioctl hook first, and then falls back through generic netdev ioctl helpers when the stack reports `-ENOTTY`.

## Behavior

- Accepts only commands that satisfy `_FIOCVALID(cmd)` or `_SIOCVALID(cmd)`, otherwise fails with `ENOTTY`.
- Resolves the descriptor with `get_socket_by_pid(sd, getpid())` and fails with `EBADF` when no allocated socket is found.
- Initializes the result to `-ENOTTY`, then calls the active netstack's `ioctl` hook through `NETSTACK_CALL_RET()` when a netstack is available.
- If the current result is still `-ENOTTY`, tries `netdev_ifrioctl(NULL, cmd, ...)`.
- Under `CONFIG_NET_IGMP`, retries `netdev_imsfioctl(NULL, cmd, ...)` when the previous result is still `-ENOTTY`.
- Under `CONFIG_NET_ROUTE`, retries `netdev_rtioctl(NULL, cmd, ...)` when the previous result is still `-ENOTTY`.
- Returns non-negative results directly.
- Converts any final negative result into `errno = -ret` and returns `ERROR`.

## Inputs and Outputs

- `sd`: socket descriptor to operate on.
- `cmd`: ioctl command.
- `arg`: command argument passed through as an integer-sized pointer value.
- Return value: non-negative command-specific result on success, or `ERROR` on failure.

## Dependencies

- Uses `get_socket_by_pid()` for descriptor validation.
- Uses `get_netstack_byfd()` and `NETSTACK_CALL_RET()` for the first backend dispatch.
- Uses `netdev_ifrioctl()` for generic interface requests.
- Uses `netdev_imsfioctl()` when `CONFIG_NET_IGMP` is enabled.
- Uses `netdev_rtioctl()` when `CONFIG_NET_ROUTE` is enabled.

## Notes

- Despite the older comment, the wrapper does not verify `arg != 0` before dispatch; backend handlers decide whether the argument value is valid for the command.
- The fallback helpers are called with a `NULL` socket pointer in this wrapper.
- `lwip_ns_ioctl()` currently delegates to `netdev_lwipioctl()` and intentionally returns `-ENOTTY` for commands that are not handled by the lwIP-specific path, which is what enables the fallback chain here.
