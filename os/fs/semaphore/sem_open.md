# `sem_open`

## Summary

`sem_open()` resolves a named semaphore under the configured VFS path, returning the existing object when present or creating a new inode-backed semaphore when `O_CREAT` is requested.

## Behavior

- Returns the existing named semaphore when `inode_find()` resolves a named-semaphore inode for the requested path.
- Rejects an existing non-semaphore inode with `ENXIO`.
- Rejects `O_CREAT | O_EXCL` when the named semaphore already exists with `EEXIST`.
- Returns `ENOENT` when the name does not exist and `O_CREAT` was not supplied.
- On creation, reads `mode_t mode` and `unsigned value` from the variadic argument list, but ignores `mode`.
- Rejects initial values above `SEM_VALUE_MAX` with `EINVAL`.
- Creates a pseudo-filesystem inode with `inode_reserve()`, allocates a `struct nsem_inode_s` with `group_malloc()`, sets `i_crefs = 1`, marks the inode as a named semaphore, and initializes the contained unnamed semaphore with `sem_init()`.
- Uses `sched_lock()` around the find-or-create sequence so existence checks and creation stay atomic relative to concurrent `sem_open()` callers.

## Inputs and Outputs

- `name`: semaphore name appended below `CONFIG_FS_NAMED_SEMPATH`.
- `oflags`: open/create flags. The implementation only acts on `O_CREAT` and `O_EXCL`.
- Optional creation arguments:
- `mode`: accepted for API compatibility but ignored.
- `value`: initial semaphore count when creating a new object.
- Return value: pointer to the named semaphore object on success, or `(sem_t *)ERROR` on failure with `errno` set for the handled failure paths.

## Dependencies

- Uses inode helpers such as `inode_find()`, `inode_reserve()`, and `inode_release()`.
- Uses the public `sem_init()` implementation from `lib/libc/semaphore/sem_init.c` to initialize newly created named semaphores.
- Paired with `sem_close.md` and `sem_unlink.md` for lifetime management after the open/create step.

## Notes

- The implementation does not set `errno` when `name == NULL`; it simply returns `(sem_t *)ERROR`.
- Reopening the same named semaphore returns the same contained `sem_t *` because the object is stored in the inode payload.
- The constructed full path lives in a fixed `MAX_SEMPATH` buffer. Distinct long names can alias after truncation because the code does not reject an oversized formatted path.
