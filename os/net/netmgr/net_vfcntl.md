# `net_vfcntl`

## Summary

`net_vfcntl()` is the socket-side `fcntl()` bridge. It validates that the descriptor belongs to the current task's socket table, handles a limited command subset locally, and dispatches the remaining supported commands to the active network stack.

## Behavior

- Resolves the descriptor with `get_socket_by_pid(sd, getpid())` and fails with `EBADF` if no allocated socket is found.
- Handles `F_DUPFD` by calling the active netstack's `dup` hook through `NETSTACK_CALL_RET(st, dup, ...)`.
- Handles `F_GETFL` and `F_SETFL` by calling the active netstack's `fcntl` hook through `NETSTACK_CALL_RET(st, fcntl, ...)`.
- Rejects `F_GETFD`, `F_SETFD`, `F_GETOWN`, `F_SETOWN`, `F_GETLK`, `F_SETLK`, and `F_SETLKW` with `ENOSYS`.
- Rejects all other commands with `EINVAL`.
- Writes `errno` only for the locally rejected cases; dispatched backend calls return their own result and are responsible for any errno updates.

## Inputs and Outputs

- `sd`: socket descriptor to operate on.
- `cmd`: `fcntl` command selector.
- `ap`: command-specific argument list.
- Return value: backend result for dispatched commands, or `ERROR` for locally rejected commands.

## Dependencies

- Uses `get_socket_by_pid()` for descriptor validation.
- Uses `get_netstack_byfd()` and `NETSTACK_CALL_RET()` for backend dispatch.
- On lwIP-backed sockets, reaches `lwip_ns_vfcntl()` and then `lwip_fcntl()` for `F_GETFL` and `F_SETFL`.

## Notes

- The current `F_DUPFD` path ignores the third argument and forwards only the source descriptor to the backend `dup` hook.
- `NETSTACK_CALL_RET()` leaves `ret` unchanged when no netstack or hook is present. Because `ret` starts at `0`, `F_DUPFD`, `F_GETFL`, and `F_SETFL` can currently fall through as apparent success even when nothing handled the request.
- The lwIP backend supports only `F_GETFL` and `F_SETFL`; other commands there return `-1` with socket errno set to `ENOSYS`.
