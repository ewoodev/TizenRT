# `os/fs/mqueue` Module Guide

## Purpose

`os/fs/mqueue` owns the named message-queue namespace and descriptor-lifetime side of the public message-queue APIs. This folder resolves queue names under the configured VFS prefix, creates pseudo-filesystem inodes for new queues, closes task-group-owned descriptors, and detaches queue names from the namespace.

## Public APIs Covered in This Folder

- `mq_open()`
- `mq_close()`
- `mq_unlink()`
- `mq_close_group()`

Function-level notes live beside the implementation sources in this folder.

## Build and Configuration

- `os/kernel/Kconfig` exposes `CONFIG_DISABLE_MQUEUE`, which disables both this folder and the kernel-side message-queue implementation.
- `os/fs/mqueue/Kconfig` exposes `CONFIG_FS_MQUEUE_MPATH`, which selects the namespace prefix used to build the full queue path.
- `os/kernel/Kconfig` exposes `CONFIG_MQ_MAXMSGSIZE`, which becomes `MQ_MAX_BYTES` and therefore limits the largest `mq_attr->mq_msgsize` that `mq_open()` can create.
- `os/fs/mqueue/Make.defs` builds `mq_open.c`, `mq_close.c`, and `mq_unlink.c` only when message queues are enabled.
- The queue-operation APIs declared in `os/include/mqueue.h` and helper APIs declared in `os/include/tinyara/mqueue.h` are split across `os/kernel/mqueue`, not this folder.

## Internal Model

1. `mq_open()` formats `CONFIG_FS_MQUEUE_MPATH "/" name`, then resolves an existing inode or reserves a new inode for first creation.
2. New queues allocate a `struct mqueue_inode_s` through `mq_msgqalloc()`, create a descriptor through `mq_descreate()`, bind the queue into the inode payload, and seed `inode->i_crefs` to `1`.
3. Reopening an existing queue skips creation and only adds another task-group-owned descriptor to the already-bound queue object.
4. `mq_close()` resolves the current task group and delegates the real descriptor teardown to `mq_close_group()`.
5. `mq_close_group()` removes one descriptor through `mq_desclose_group()`, clears descriptor-owned notification state, and drops the inode reference through `mq_inode_release()`.
6. `mq_unlink()` removes the namespace entry first, leaving the inode flagged as deleted until the final reference drop lets `mq_inode_release()` free the queue object and inode container.

## Behavioral Constraints

- Queue names are always interpreted below `CONFIG_FS_MQUEUE_MPATH`; the caller does not pass a full VFS path.
- `mq_open()` accepts `mode_t mode` for POSIX compatibility, but the current implementation ignores it.
- `mq_open()` and `mq_unlink()` use the fixed `MAX_MQUEUE_PATH` buffer from `mqueue.h`; oversized formatted paths fail with `ENAMETOOLONG`.
- Queue lifetime is inode-reference-based, not name-based. `mq_close()` only drops descriptors and references, while `mq_unlink()` only detaches the namespace entry.
- This folder does not implement queue data-path operations such as send, receive, notify, or attribute updates.

## Dependencies

- Pseudo-filesystem inode helpers under `os/fs/inode`
- Scheduler locking plus SMP-safe critical sections
- Public helper APIs in `os/kernel/mqueue`: `mq_msgqalloc()`, `mq_descreate()`, `mq_desclose_group()`, and `mq_msgqfree()`
- Kernel-side queue-operation APIs in `os/kernel/mqueue`

## Scope Boundaries

- This folder owns queue naming, inode binding, descriptor close, and unlink semantics.
- It does not own `mq_send()`, `mq_timedsend()`, `mq_receive()`, `mq_timedreceive()`, `mq_notify()`, `mq_setattr()`, `mq_getattr()`, `mq_msgqalloc()`, `mq_descreate()`, `mq_desclose_group()`, or `mq_msgqfree()`.

## Maintenance Notes

- Keep `os/include/mqueue.h` aligned with the actual namespace prefix and deferred-destruction model instead of copying generic POSIX wording.
- Keep `os/include/tinyara/mqueue.h` aligned for `mq_close_group()` because it is a public helper even though most queue helpers live in `os/kernel/mqueue`.
- Any change to path formatting or inode reference handling should update `mq_open.md`, `mq_close.md`, `mq_close_group.md`, `mq_unlink.md`, and this guide together.
