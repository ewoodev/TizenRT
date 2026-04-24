# `mq_msgqfree`

## Summary

`mq_msgqfree()` drains every queued message from a message-queue object and then frees the queue object itself.

## Behavior

- Walks the queue's `msglist` from head to tail.
- Releases each queued message through the private `mq_msgfree()` helper, which returns fixed/interrupt-reserved messages to their pools and frees dynamically allocated messages.
- Frees the queue object itself with `sched_kfree()` after the queued messages are drained.

## Inputs and Outputs

- `msgq`: queue object to destroy.
- Return value: none.

## Dependencies

- Used by [`mq_open.md`](../../fs/mqueue/mq_open.md) to clean up partially created queues.
- Used by the close/unlink lifetime path after the final reference is dropped, as described in [`mq_close_group.md`](../../fs/mqueue/mq_close_group.md) and [`mq_unlink.md`](../../fs/mqueue/mq_unlink.md).
- Depends on the private `mq_msgfree()` helper for per-message cleanup.

## Notes

- The helper assumes no descriptor, sender, receiver, or namespace path can still reach `msgq`.
- It does not drop inode references or clear descriptor state; higher-level lifetime logic must do that first.
