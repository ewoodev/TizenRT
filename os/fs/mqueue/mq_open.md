# `mq_open`

## Summary

`mq_open()` resolves a named message queue below `CONFIG_FS_MQUEUE_MPATH` and returns a descriptor owned by the caller's current task group.

## Behavior

- Rejects `mq_name == NULL` with `EINVAL`.
- Rejects names whose formatted `CONFIG_FS_MQUEUE_MPATH "/" mq_name` path does not fit `MAX_MQUEUE_PATH` with `ENAMETOOLONG`.
- Wraps the find-or-create sequence in a critical section so concurrent `mq_open()` callers cannot race the existence check against queue creation.
- Reuses an existing inode only when `inode_find()` resolves a message-queue inode; existing non-mqueue inodes fail with `ENXIO`.
- Rejects `O_CREAT | O_EXCL` against an already existing queue with `EEXIST`.
- Returns `ENOENT` when the queue does not exist and `O_CREAT` was not supplied.
- On creation, consumes `mode_t mode` and `struct mq_attr *attr` from the variadic arguments, but currently ignores `mode`.
- Allocates the backing queue with `mq_msgqalloc()`, creates a descriptor with `mq_descreate()`, binds the queue into the reserved inode, and seeds `inode->i_crefs` to `1`.
- Cleans up partially created queue state by releasing the inode and freeing the queue object when descriptor creation or queue allocation fails.

## Inputs and Outputs

- `mq_name`: queue name appended below `CONFIG_FS_MQUEUE_MPATH`.
- `oflags`: open flags. The implementation acts on `O_CREAT` and `O_EXCL`, and stores the full flag set in the returned descriptor.
- Optional creation arguments:
- `mode`: accepted for API compatibility but ignored by the current implementation.
- `attr`: used only when the queue is first created. `mq_msgqalloc()` copies `mq_maxmsg` and `mq_msgsize` into the new queue object, or falls back to `MQ_MAX_MSGS` and `MQ_MAX_BYTES` when `attr == NULL`.
- Return value: `mqd_t` descriptor on success, or `(mqd_t)ERROR` on failure with `errno` set.

## Dependencies

- Uses inode helpers such as `inode_find()`, `inode_reserve()`, and `inode_release()`.
- Uses the public helper `mq_msgqalloc()` to allocate the backing queue object.
- Uses the public helper `mq_descreate()` to attach a descriptor to the caller's task group.
- Pairs with `mq_close.md`, `mq_close_group.md`, and `mq_unlink.md` for later lifetime transitions.

## Notes

- The return value is a descriptor object, not the queue object address.
- `mq_msgqalloc()` rejects `attr->mq_msgsize > MQ_MAX_BYTES`, but `mq_open()` currently maps every queue-allocation failure to `ENOSPC`, so invalid size requests and memory exhaustion share the same public error path.
- Existing descriptors keep the queue alive even after `mq_unlink()` removes the name from the namespace.
