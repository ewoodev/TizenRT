# `os/fs/vfs` Module Guide

## Purpose

`os/fs/vfs` is the descriptor-facing virtual filesystem layer that sits between public POSIX-style file APIs and the lower inode, mountpoint, pseudo-filesystem, and socket bridges.

In this worktree, the documented slice covers the common descriptor wrappers, pathname/metadata helpers, filesystem-capacity queries, readiness helpers, and the poll/select bridge. It does not claim full coverage of every helper in this directory.

## Public APIs Covered in This Slice

### Core Descriptor I/O

- `open()`
- `close()`
- `read()`
- `write()`
- `lseek()`
- `pread()`
- `pwrite()`
- `fsync()`
- `ftruncate()`

### Descriptor Control

- `dup()`
- `dup2()`
- `fcntl()`
- `ioctl()`

### Pathname and Metadata

- `mkdir()`
- `stat()`
- `fstat()`
- `rename()`
- `rmdir()`
- `unlink()`
- `statfs()`
- `fstatfs()`

### Readiness and Poll Helpers

- `poll()`
- `select()`
- `file_poll()`
- `fdesc_poll()`

Function-level notes live beside the implementation sources throughout this folder.

## Build and Configuration

- `os/Kconfig` exposes `os/fs/Kconfig` through the `File Systems` menu.
- `os/fs/vfs/Make.defs` is included from the `os/fs` build and selects which wrappers are compiled based on descriptor and mountpoint configuration.
- `CONFIG_NFILE_DESCRIPTORS` controls whether the full file-descriptor VFS path is built. When it is `0`, only the smaller socket-facing subset remains when `CONFIG_NET=y`.
- `CONFIG_NET` adds socket-descriptor fallbacks in wrappers such as `close()`, `dup()`, `dup2()`, `fcntl()`, `ioctl()`, `poll()`, and `select()`.
- `CONFIG_DISABLE_MOUNTPOINT` removes mountpoint-specific paths such as `fsync()`, `ftruncate()`, and the mountpoint branches inside many pathname wrappers.
- `CONFIG_DISABLE_PSEUDOFS_OPERATIONS` removes pseudo-filesystem mutation paths used by helpers such as `mkdir()`, `rename()`, `rmdir()`, and `unlink()`.
- `CONFIG_DISABLE_POLL` removes the VFS `poll()`/`select()` implementation path.
- `CONFIG_NFILE_STREAMS` enables stream-oriented helpers such as `fdopen()` even though that helper is outside the documented slice here.
- `CONFIG_FILE_MODE` and `CONFIG_BCH` alter selected open-path behavior.

## Internal Model

1. Descriptor wrappers first decide whether the target is a regular file descriptor, a socket descriptor, or an invalid descriptor.
2. File-descriptor paths resolve a `struct file` through helpers such as `fs_getfilep()` and then dispatch to file, driver, or mountpoint operations.
3. Socket-descriptor paths delegate to the networking bridge layer in `os/net` when networking support is built.
4. Pathname wrappers resolve inodes and split behavior between mountpoints and pseudo-filesystem objects.
5. Filesystem-capacity helpers (`statfs()`, `fstatfs()`) either call mountpoint hooks or synthesize pseudo-filesystem metadata.
6. Readiness helpers (`poll()`, `select()`, `file_poll()`, `fdesc_poll()`) bridge file descriptors and socket descriptors into one wait model, with `select()` implemented on top of `poll()`.

## Behavioral Constraints

- `open()` currently resolves only existing inodes; `O_CREAT` does not create a missing inode by itself in this path.
- Several wrappers expose raw helper behavior rather than a fully normalized POSIX contract. Early helper failures can return negated errno-style values directly, while some public wrappers can also overwrite helper-set errno with a generic value.
- Many pathname and capacity helpers treat mountpoints and pseudo-filesystem objects differently, and some mountpoint-hook absences can still fall through without a forced hard failure.
- `poll()` and `select()` share one readiness engine. Invalid positive descriptors fail with `EBADF`, while negative `pollfd` entries are ignored.
- `select()` reports the number of readiness bits set across the output sets, not the number of unique ready descriptors.

## Dependencies

- Inode and file-table helpers from `tinyara/fs/fs.h` and the neighboring `os/fs` code
- Mountpoint operation tables and pseudo-filesystem inode logic
- `os/net` bridge helpers for socket-descriptor fallback paths
- `kmm_*` allocation helpers for temporary readiness state in `select()`

## Scope Boundaries

- This guide covers only the VFS slices already documented in this worktree.
- It does not yet summarize every helper in `os/fs/vfs` such as `fdopen()` or internal lookup helpers outside the documented slice.
- It does not replace module guides for neighboring `os/fs` subdirectories such as `dirent`, `mount`, or filesystem-specific implementations.

## Maintenance Notes

- Keep the public comments aligned with the narrower implementation, especially where wrappers only cover a subset of POSIX behavior.
- Descriptor-range handling is easy to misread because many wrappers can cross from file descriptors into socket descriptors when `CONFIG_NET` is enabled.
- Readiness code should be updated carefully: `poll()`, `select()`, `file_poll()`, and `fdesc_poll()` share assumptions about error reporting, immediate readiness synthesis, and teardown behavior.
- If more VFS public APIs are documented later, extend this guide by slice rather than turning it into a file-by-file inventory.
