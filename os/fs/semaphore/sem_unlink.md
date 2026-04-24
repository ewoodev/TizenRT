# `sem_unlink`

## Summary

`sem_unlink()` removes a named semaphore from the VFS namespace and then drops the caller's inode reference, leaving final destruction to the last `sem_close()` when other references still exist.

## Behavior

- Builds the full semaphore path as `CONFIG_FS_NAMED_SEMPATH "/" name`.
- Locks scheduling before resolving and removing the inode.
- Returns `ENOENT` when no inode exists for the constructed path.
- Returns `ENXIO` when the resolved inode is not a named semaphore.
- Returns `ENOTEMPTY` when the inode still has children.
- Calls `inode_remove()` while holding an inode reference, expecting the inode to remain allocated but marked deleted.
- Releases the inode tree semaphore and then calls `sem_close()` on the named semaphore object stored in the inode.
- Returns the status from `sem_close()`, which completes immediate destruction only when no other open references remain.

## Inputs and Outputs

- `name`: semaphore name appended below `CONFIG_FS_NAMED_SEMPATH`.
- Return value: `OK` on success, or `ERROR` with `errno = ENOENT`, `ENXIO`, or `ENOTEMPTY` on the handled failure paths.

## Dependencies

- Uses `inode_find()`, `inode_remove()`, `inode_release()`, `inode_semtake()`, and `inode_semgive()`.
- Uses `sem_close.md` to release the unlink call's own inode reference and trigger final destruction when possible.

## Notes

- The function does not validate `name` before formatting the full path, so callers must provide a valid string pointer.
- Unlinking does not immediately destroy an in-use named semaphore. The underlying object persists until the final close drops the last reference.
- The formatted full path also uses the fixed `MAX_SEMPATH` buffer, so long names can be truncated before lookup and removal.
