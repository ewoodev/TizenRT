# `mq_desclose_group`

## Summary

`mq_desclose_group()` removes one message-queue descriptor from a task group and returns the descriptor storage to the free pool.

## Behavior

- Expects validated `mqdes` and `group` inputs; the implementation relies on `DEBUGASSERT` instead of runtime error handling.
- Removes `mqdes` from `group->tg_msgdesq`.
- Reads the backing queue object from `mqdes->msgq`.
- Clears queue-global notification state only when the stored `ntmqdes` owner matches `mqdes`.
- Returns the descriptor storage to `g_desfree`.

## Inputs and Outputs

- `mqdes`: descriptor being torn down.
- `group`: owning task group whose descriptor list contains `mqdes`.
- Return value: none.

## Dependencies

- Called by [`mq_close_group.md`](../../fs/mqueue/mq_close_group.md) after ownership checks succeed.
- Clears the close-time notification ownership described in [`mq_notify.md`](mq_notify.md).
- Leaves inode-reference and queue-object lifetime transitions to the surrounding `os/fs/mqueue` close path.

## Notes

- The helper does not validate ownership, drop inode references, or free the queue object.
- Notification cleanup is descriptor-scoped: registration is cleared only when it was created through this exact descriptor.
- The implementation assumes the scheduler is already locked by the caller.
