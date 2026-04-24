# `mq_descreate`

## Summary

`mq_descreate()` allocates one message-queue descriptor and attaches it to a task group's descriptor list.

## Behavior

- Resolves `mtcb == NULL` to the currently executing task with `sched_self()`.
- Reads the owning task group from `mtcb->group`; the implementation only asserts that this group exists.
- Obtains descriptor storage from `g_desfree`, extending the pool with `mq_desblockalloc()` when necessary.
- Zeroes the descriptor, stores the backing queue pointer in `mqdes->msgq`, and stores the caller's `oflags` verbatim in `mqdes->oflags`.
- Appends the descriptor to `group->tg_msgdesq`.

## Inputs and Outputs

- `mtcb`: target task, or `NULL` to use the current task.
- `msgq`: backing queue object referenced by the descriptor.
- `oflags`: open/status flags stored on the descriptor and later reused by APIs such as `mq_setattr()`, `mq_send()`, and `mq_receive()`.
- Return value: descriptor pointer on success, or `NULL` when no descriptor can be allocated. This helper does not set `errno`.

## Dependencies

- Used by [`mq_open.md`](../../fs/mqueue/mq_open.md) after queue creation or lookup succeeds.
- The resulting descriptor is later consumed by [`mq_getattr.md`](mq_getattr.md), [`mq_setattr.md`](mq_setattr.md), [`mq_send.md`](mq_send.md), [`mq_timedsend.md`](mq_timedsend.md), [`mq_receive.md`](mq_receive.md), and [`mq_timedreceive.md`](mq_timedreceive.md).
- Descriptor teardown is handled by [`mq_desclose_group.md`](mq_desclose_group.md).

## Notes

- Descriptor ownership is task-group scoped, not thread scoped.
- The helper does not take or drop inode references; that surrounding lifetime management stays in `os/fs/mqueue`.
- Pool-allocation failure is intentionally silent here and is translated by higher-level code such as `mq_open()`.
