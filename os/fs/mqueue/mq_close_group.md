# `mq_close_group`

## Summary

`mq_close_group()` closes one message queue descriptor held by the supplied task group.

## Behavior

- Rejects `mqdes == NULL` with `EBADF`.
- Rejects `group == NULL` with `EINVAL`.
- Locks the scheduler while it scans `group->tg_msgdesq` for the descriptor.
- Rejects descriptors that are not owned by the supplied group with `EBADF`.
- On a match, reads the backing `struct mqueue_inode_s` from `mqdes->msgq`, removes the descriptor through `mq_desclose_group()`, then drops the inode reference through the private `mq_inode_release()` helper.
- Does not remove the queue name from the namespace.
- Frees the queue object only when the inode was already marked deleted and this close drops the final reference.

## Inputs and Outputs

- `mqdes`: message queue descriptor that must already belong to `group`.
- `group`: owning task group whose `tg_msgdesq` list is searched.
- Return value: `OK` when the descriptor is removed, or `ERROR` with `errno` set when validation or ownership checks fail.

## Dependencies

- Uses the public helper `mq_desclose_group()` to remove the descriptor and clear descriptor-owned notification state.
- Uses the private `mq_inode_release()` helper in `mq_close.c` to drop the inode reference and perform deferred final destruction.
- `mq_close.md` is the current-task-group wrapper over this helper.

## Notes

- This helper is the real owner-side lifetime transition in `os/fs/mqueue`; `mq_close()` only resolves the current group and delegates here.
- Notification cleanup is descriptor-scoped: only notification registered through this exact descriptor is cleared.
