# `os/net` Module Guide

## Purpose

`os/net` is the top-level networking subsystem. It contains the stack choice and protocol configuration, the backend implementations such as lwIP, Bluetooth- and BLE-related layers, and the bridge code that connects VFS/socket-facing descriptors to the active netstack.

In this worktree, the documented public slice is the bridge/control layer in `os/net/netmgr/net_vfs.c`, not the entire protocol stack.

## Public APIs Covered in This Slice

- `net_poll()`
- `net_close()`
- `net_dupsd()`
- `net_dupsd2()`
- `net_clone()`
- `net_vfcntl()`
- `net_ioctl()`

Function-level notes live beside the implementation in `os/net/netmgr/`:

- `net_poll.md`
- `net_close.md`
- `net_dupsd.md`
- `net_dupsd2.md`
- `net_clone.md`
- `net_vfcntl.md`
- `net_ioctl.md`

This guide does not claim that all public networking APIs declared in `tinyara/net/net.h` are fully documented in this worktree.

## Layout

- `os/net/netmgr/`: descriptor bridges, netstack selection helpers, and netdev control helpers
- `os/net/lwip/`: lwIP backend and related stack code
- `os/net/bluetooth/` and `os/net/blemgr/`: Bluetooth and BLE-specific layers
- `os/net/net_initialize_netmgr.c`: top-level network initialization entry for this tree

## Build and Configuration

- `CONFIG_NET` enables the networking subsystem and gates the public declarations in `tinyara/net/net.h`.
- `os/net/Kconfig` chooses the networking stack. The current top-level choice is `CONFIG_NET_LWIP`.
- `os/net/Makefile` always includes `netmgr/Make.defs` when `CONFIG_NET=y`, so the documented `net_vfs.c` bridge layer is part of the common build, not only of `CONFIG_NET_NETMGR`.
- `CONFIG_NET_NETMGR` adds manager-specific options such as zero-copy and task binding, but the bridge helpers documented here are not gated by that symbol.
- `CONFIG_NET_IGMP` and `CONFIG_NET_ROUTE` extend the `net_ioctl()` fallback chain with IGMP and route handlers.
- `CONFIG_LWNL80211` and `CONFIG_NET_LOCAL` add additional netstack backends that can participate in descriptor-based dispatch.

## Internal Model

1. Public file-descriptor wrappers in `os/fs/vfs` call into the `net_*` bridge helpers when the target descriptor is a socket-side descriptor.
2. `net_vfs.c` resolves the active backend from the descriptor with helpers such as `get_netstack_byfd()` and macros such as `NETSTACK_CALL_BYFD()`.
3. Thin bridges like `net_close()`, `net_dupsd()`, `net_dupsd2()`, and `net_poll()` forward directly to the backend hook and return the backend result unchanged.
4. `net_vfcntl()` adds local descriptor validation and local command filtering before it reaches backend hooks.
5. `net_ioctl()` adds command-family validation, socket validation, and a fallback chain through generic netdev ioctl helpers when the backend reports `-ENOTTY`.
6. `net_clone()` is currently an unsupported stub and does not dispatch to any backend.

## Behavioral Constraints

- `net_close()`, `net_dupsd()`, `net_dupsd2()`, and `net_poll()` can fall back to raw `-1` when no backend or hook is available; they do not add their own errno translation.
- `net_vfcntl()` accepts only a narrow command subset and currently has a hole where missing backend hooks can leave `ret == 0`, which looks like success for `F_DUPFD`, `F_GETFL`, and `F_SETFL`.
- `net_ioctl()` accepts only `_FIOCVALID()` / `_SIOCVALID()` command families, validates the socket first, then retries through `netdev_ifrioctl()`, `netdev_imsfioctl()`, and `netdev_rtioctl()` when the backend reports `-ENOTTY`.
- `net_ioctl()` passes `NULL` as the socket pointer to the netdev fallback helpers in the current wrapper.
- `net_clone()` currently always fails and should not be described as a real clone path.

## Dependencies

- `netstack.h` owns the `NETSTACK_CALL_*` dispatch macros used by the bridge helpers.
- Backend implementations such as `netstack_lwip.c`, `netstack_lwnl.c`, and `netstack_uds.c` define the hook behavior reached through descriptor dispatch.
- `netmgr_ioctl_netdev.c`, `netmgr_ioctl_igmp.c`, and `netmgr_ioctl_route.c` provide the `net_ioctl()` fallback handlers.
- `os/fs/vfs` wrappers such as `poll()`, `close()`, `dup()`, `dup2()`, `fcntl()`, and `ioctl()` are the public entry points that reach this slice for socket descriptors.

## Scope Boundaries

- This guide covers only the currently documented bridge/control APIs in `net_vfs.c`.
- It does not summarize every protocol, socket, or driver API under `os/net`.
- It does not replace per-backend behavior documents; backend-specific semantics still live in the selected netstack implementation.

## Maintenance Notes

- Keep `tinyara/net/net.h` comments aligned with the actual bridge limitations, especially where the wrappers return raw backend results or raw `-1`.
- If the `NETSTACK_CALL_*` macros change their default-return behavior, revisit the `net_close()`, `net_dupsd()`, `net_dupsd2()`, `net_poll()`, and `net_vfcntl()` documentation.
- If `net_clone()` grows a real backend path, update both this guide and the function-level note immediately; the current module text intentionally treats it as unsupported.
- When broader `os/net` public APIs are documented later, either extend this guide carefully or split the scope so that bridge/control coverage does not get conflated with full protocol-stack coverage.
