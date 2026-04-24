# `os/fs/semaphore` Module Guide

## Purpose

`os/fs/semaphore` is the named-semaphore owner folder. It maps public `sem_open()`, `sem_close()`, and `sem_unlink()` calls onto pseudo-filesystem inodes that hold a `struct nsem_inode_s` wrapper around a normal unnamed semaphore.

## Public APIs Covered in This Folder

- `sem_open()`
- `sem_close()`
- `sem_unlink()`

Function-level notes live beside the implementation sources in this folder.

## Build and Configuration

- `os/fs/semaphore/Kconfig` exposes `CONFIG_FS_NAMED_SEMAPHORES`.
- `os/fs/semaphore/Kconfig` exposes `CONFIG_FS_NAMED_SEMPATH`, which selects the namespace prefix used to build the full semaphore path.
- `os/fs/semaphore/Make.defs` builds `sem_open.c`, `sem_close.c`, and `sem_unlink.c` only when `CONFIG_FS_NAMED_SEMAPHORES=y`.

## Internal Model

1. `sem_open()` formats `CONFIG_FS_NAMED_SEMPATH "/" name`, then resolves or creates a pseudo-filesystem inode for that path.
2. New named semaphores allocate a `struct nsem_inode_s`, store it in the inode payload, mark the inode as a named semaphore, set `i_crefs = 1`, and initialize the contained unnamed semaphore with `sem_init()`.
3. Reopening an existing named semaphore returns the same contained `sem_t *` and retains the inode reference associated with the successful lookup/open.
4. `sem_unlink()` removes the inode from the namespace, leaving the inode flagged as deleted while references still exist, then hands its own reference to `sem_close()`.
5. `sem_close()` decrements the reference count and performs final destruction only when the inode has already been unlinked and no references remain.

## Behavioral Constraints

- `mode_t mode` is parsed by `sem_open()` when creating a semaphore, but the current implementation ignores it.
- `sem_open()` does not set `errno` for a `NULL` `name`; it simply returns `(sem_t *)ERROR`.
- `sem_close()` relies on debug assertions instead of runtime pointer validation.
- `sem_unlink()` does not validate `name` before formatting the full path.
- Name resolution uses the fixed `MAX_SEMPATH` buffer from `semaphore.h`, so oversized names can be truncated before lookup or creation.
- Named semaphore lifetime is reference-counted through inode references, not only through the contained `sem_t`.

## Dependencies

- Pseudo-filesystem inode helpers under `os/fs/inode`
- Group/kernel allocators for the `struct nsem_inode_s` container and inode teardown
- The public unnamed-semaphore primitives documented in `os/kernel/semaphore` and `lib/libc/semaphore`

## Scope Boundaries

- This folder owns only the named semaphore APIs.
- It does not own unnamed semaphore operations such as `sem_wait()`, `sem_post()`, `sem_destroy()`, or `sem_getvalue()`.

## Maintenance Notes

- Keep header comments aligned with the actual lifetime model: unlink detaches the name first, and destruction is delayed until the last close.
- Any future change to the inode reference model should update both `sem_open.md`/`sem_close.md`/`sem_unlink.md` and this guide together.
- `CONFIG_FS_NAMED_SEMPATH` is part of the public behavior because the implementation literally prefixes every semaphore name with that configured namespace path.
