# `mq_unlink`

## Summary

`mq_unlink()` removes a named message queue entry from the `CONFIG_FS_MQUEUE_MPATH` namespace.

## Behavior

- Rejects `mq_name == NULL` with `EINVAL`.
- Rejects names whose formatted `CONFIG_FS_MQUEUE_MPATH "/" mq_name` path does not fit `MAX_MQUEUE_PATH` with `ENAMETOOLONG`.
- Locks the scheduler while it resolves and detaches the inode.
- Returns `ENOENT` when the queue name does not exist.
- Rejects non-message-queue inodes with `ENXIO`.
- Rejects inodes that still have children with `ENOTEMPTY`.
- Calls `inode_remove()` to detach the name from the inode tree. The expected result is usually `-EBUSY`, which means the inode is now marked deleted but cannot be freed yet because references remain.
- Propagates unexpected negative `inode_remove()` failures instead of silently ignoring them.
- Releases its own inode reference through `mq_inode_release()`, which performs final queue destruction only when no references remain.

## Inputs and Outputs

- `mq_name`: queue name appended below `CONFIG_FS_MQUEUE_MPATH`.
- Return value: `OK` when the namespace entry is removed, or `ERROR` with `errno` set when lookup, validation, or inode removal fails.

## Dependencies

- Uses inode helpers such as `inode_find()`, `inode_remove()`, and `inode_release()`.
- Uses the private `mq_inode_release()` helper in `mq_close.c` for deferred final destruction after unlink.
- Existing users of the queue continue through descriptors documented in `mq_close.md` until the final close drops the last reference.

## Notes

- `mq_unlink()` removes only the name immediately. Open descriptors continue to refer to the queue object after unlink.
- The fixed `MAX_MQUEUE_PATH` buffer makes the namespace prefix part of the public API contract for long names.
