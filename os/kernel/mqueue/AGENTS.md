# `os/kernel/mqueue` Module Guide

## Purpose

`os/kernel/mqueue` owns the kernel-side state and behavior of POSIX message queues after the named namespace layer has resolved or created a queue. This folder manages queue objects, descriptor attachment, pooled message storage, send/receive waits, timed waits, one-shot notification, and task-exit recovery.

## Public APIs Covered in This Folder

- `mq_send()`
- `mq_timedsend()`
- `mq_receive()`
- `mq_timedreceive()`
- `mq_notify()`
- `mq_setattr()`
- `mq_getattr()`
- `mq_msgqalloc()`
- `mq_descreate()`
- `mq_desclose_group()`
- `mq_msgqfree()`

Function-level notes live beside the implementation sources in this folder.

## Build and Configuration

- `os/kernel/Kconfig` exposes `CONFIG_DISABLE_MQUEUE`. When it is enabled, this folder and the companion named-queue code under `os/fs/mqueue` are both excluded.
- `os/kernel/Kconfig` exposes `CONFIG_PREALLOC_MQ_MSGS`, which sizes the general preallocated message pool used by `mq_msgalloc()` and `mq_msgfree()`.
- `os/kernel/Kconfig` exposes `CONFIG_MQ_MAXMSGSIZE`, which becomes `MQ_MAX_BYTES`. It fixes the payload buffer size inside every internal `struct mqueue_msg_s` and caps `mq_msgqalloc()` requests through `attr->mq_msgsize`.
- `CONFIG_DISABLE_SIGNALS` changes the module surface. `os/kernel/mqueue/Make.defs` omits both `mq_notify.c` and `mq_waitirq.c` when signals are disabled, so one-shot notification support and the shared wait-interruption helper are both gated by signal support.
- `os/kernel/mqueue/Make.defs` always builds the core send, receive, helper, initialization, release, and recovery sources when message queues are enabled.
- Some pool sizes are code constants, not Kconfig values: descriptor blocks grow in chunks of `NUM_MSG_DESCRIPTORS` and the interrupt-reserved message pool uses `NUM_INTERRUPT_MSGS`.

## Internal Model

1. `mq_initialize()` seeds the general and interrupt-reserved message pools and allocates the first descriptor block.
2. `mq_msgqalloc()` creates zeroed `struct mqueue_inode_s` queue objects. `os/fs/mqueue` binds those objects to pseudo-filesystem inodes and names.
3. `mq_descreate()` attaches caller-owned descriptors to `task_group_s::tg_msgdesq`; `mq_desclose_group()` removes them again without touching inode references.
4. The send path (`mq_send()`, `mq_timedsend()`, `mq_sndinternal.c`) validates caller state, optionally waits for the queue to become non-full, allocates one internal message, inserts it by descending priority with FIFO ordering inside the same priority, clears one-shot notification state, and wakes one not-empty waiter.
5. The receive path (`mq_receive()`, `mq_timedreceive()`, `mq_rcvinternal.c`) optionally waits for the queue to become non-empty, copies one message out, frees the internal message object, and wakes one not-full waiter.
6. `mq_setattr()` and `mq_getattr()` expose descriptor-local flags together with queue limits and current occupancy.
7. `mq_notify()` stores queue-global one-shot notification ownership on the queue object. Successful send or descriptor teardown clears that stored registration.
8. `mq_msgqfree()` performs the final queue-object drain and free after the higher-level lifetime logic in `os/fs/mqueue` determines that the queue is unreachable.
9. `mq_release()` and `mq_recover()` integrate the queue subsystem with task-group exit and blocked-task cleanup.

## Behavioral Constraints

- Descriptor ownership is task-group scoped. The same queue object can be referenced by multiple descriptors across one or more task groups.
- `O_NONBLOCK` is stored on the descriptor, not on the queue object. `mq_setattr()` only changes that descriptor-local bit.
- `mq_send()` has an interrupt-context fast path that skips the blocking wait and uses the interrupt-reserved message pool when necessary. `mq_timedsend()` is written for task context and asserts that it is not called from interrupt context.
- Timed send and receive APIs interpret `abstime` as an absolute `CLOCK_REALTIME` deadline.
- `mq_notify()` ignores `sigev_notify`; the current implementation always behaves like signal-based one-shot notification using the stored signal number and value.
- `mq_msgqalloc()` and `mq_descreate()` can fail without setting `errno`; higher-level callers decide how to map those failures into public errors.
- Wait/wakeup behavior is one-at-a-time. The send and receive helpers wake one blocked peer rather than broadcasting to every waiter.

## Dependencies

- `os/fs/mqueue` for queue naming, inode binding, current-group close, and unlink-driven deferred destruction
- Scheduler locking, critical sections, wait-state transitions, and watchdog timers
- Signal support for `mq_notify()` and wait interruption paths
- Public contracts in `os/include/mqueue.h` and `os/include/tinyara/mqueue.h`

## Scope Boundaries

- This folder owns queue-object allocation and free, descriptor allocation and free, message buffering, blocking waits, timeout handling, notification bookkeeping, and task-exit recovery.
- It does not own namespace path construction, pseudo-filesystem inode creation, current-task-group close wrappers, or unlink semantics. Those stay in `os/fs/mqueue`.

## Maintenance Notes

- Keep `os/include/mqueue.h` aligned with the actual implementation instead of generic POSIX wording. The important implementation details are descriptor-scoped `O_NONBLOCK`, absolute `CLOCK_REALTIME` timeouts, one-shot notification ownership, and the up-front watchdog reservation in timed waits.
- Keep `os/include/tinyara/mqueue.h` aligned with helper responsibilities and caller obligations, especially the no-`errno` NULL failure paths in `mq_msgqalloc()` and `mq_descreate()` and the caller-held scheduler lock requirement for `mq_desclose_group()`.
- When changing priority insertion, waiter accounting, or pool sizing, update the helper docs (`mq_msgqalloc.md`, `mq_descreate.md`, `mq_desclose_group.md`, `mq_msgqfree.md`), the send/receive docs, and this guide together.
- If queue lifetime or notification ownership changes, update both `os/kernel/mqueue` and `os/fs/mqueue` guides because the public behavior is split across those two folders.
