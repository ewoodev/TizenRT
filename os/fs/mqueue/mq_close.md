# `mq_close`

## Summary

`mq_close()` closes one message queue descriptor owned by the calling task's current group.

## Behavior

- Rejects `mqdes == NULL` with `EBADF`.
- Resolves the current TCB with `sched_self()` and requires a valid task group.
- Delegates the real close, including scheduler-locked ownership checks, to `mq_close_group()`.
- Removes any queue notification that was registered through this descriptor.
- Drops one inode reference on the backing queue.
- Does not unlink the queue name and does not free the queue immediately.

## Inputs and Outputs

- `mqdes`: descriptor previously returned by `mq_open()` for the current task group.
- Return value: `OK` when the descriptor is closed, or `ERROR` with `errno` propagated from `mq_close_group()` when the descriptor is invalid or belongs to another group.

## Dependencies

- Uses `mq_close_group.md` for the actual ownership and lifetime transition.
- Final queue destruction depends on `mq_unlink.md` having already removed the namespace entry before the last reference is dropped.

## Notes

- Closing a descriptor only affects the caller's task-group-owned descriptor state.
- The queue object persists until both conditions are true: the queue has been unlinked and every descriptor reference has been closed.
