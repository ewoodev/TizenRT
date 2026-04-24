# `mq_msgqalloc`

## Summary

`mq_msgqalloc()` allocates and initializes one backing `struct mqueue_inode_s` for a named message queue.

## Behavior

- Returns `NULL` immediately when `attr != NULL` and `attr->mq_msgsize > MQ_MAX_BYTES`.
- Allocates the queue object with `kmm_zalloc()`, so fields not explicitly initialized later start at zero.
- Initializes the queued-message list with `sq_init()`.
- Copies `attr->mq_maxmsg` and `attr->mq_msgsize` into the new queue object when `attr != NULL`.
- Falls back to the internal defaults `MQ_MAX_MSGS` and `MQ_MAX_BYTES` when `attr == NULL`.
- Initializes the notification owner PID to `INVALID_PROCESS_ID` when signals are enabled.

## Inputs and Outputs

- `mode`: accepted for API compatibility but ignored by the implementation.
- `attr`: optional creation attributes. Only `mq_maxmsg` and `mq_msgsize` are consumed on input.
- Return value: queue-object pointer on success, or `NULL` on failure. This helper does not set `errno`.

## Dependencies

- Used by [`mq_open.md`](../../fs/mqueue/mq_open.md) when a named queue must be created.
- The resulting queue object is later consumed by the send, receive, notify, and attribute APIs in this folder.
- Failed opens and deferred final destruction release the queue object through [`mq_msgqfree.md`](mq_msgqfree.md).

## Notes

- `mq_flags` and `mq_curmsgs` in the caller's `struct mq_attr` are ignored on input.
- The helper does not validate `mq_maxmsg`; it copies that field exactly as supplied.
- Public callers see error mapping from higher-level code such as `mq_open()`, not from this helper itself.
