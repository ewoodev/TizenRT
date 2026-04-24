# `mq_send`

## Summary

`mq_send()` enqueues one message on a kernel-managed POSIX message queue through a caller-owned descriptor.

## Behavior

- Validates `mqdes`, `msg`, write permission, `msglen`, and `prio` through `mq_verifysend()`.
- Treats the call as a cancellation point.
- In task context, returns immediately with `EAGAIN` when the queue is full and the descriptor has `O_NONBLOCK` set.
- Otherwise waits for the queue to become non-full through `mq_waitsend()`, then allocates one internal message object through `mq_msgalloc()`.
- Copies the caller's payload into the internal message object and inserts it through `mq_dosend()`.
- Inserts higher priorities ahead of lower priorities, while equal-priority messages stay FIFO.
- On success, consumes any queue-global one-shot `mq_notify()` registration and wakes one task waiting for the queue to become non-empty.

## Inputs and Outputs

- `mqdes`: descriptor that must be valid and open for writing.
- `msg`: caller-owned payload buffer.
- `msglen`: payload size; it must not exceed the queue's configured `mq_msgsize`.
- `prio`: message priority; the implementation accepts values up to `MQ_PRIO_MAX`.
- Return value: `OK` on success, or `ERROR` with `errno` set when validation, waiting, or message allocation fails.

## Dependencies

- Uses `mq_verifysend()`, `mq_waitsend()`, `mq_msgalloc()`, and `mq_dosend()` from `mq_sndinternal.c`.
- The descriptor-local `O_NONBLOCK` bit consumed by this path can be updated through [`mq_setattr.md`](mq_setattr.md).
- One-shot notification side effects reuse the state described in [`mq_notify.md`](mq_notify.md).
- Space for blocked senders is created by successful receives, described in [`mq_receive.md`](mq_receive.md) and [`mq_timedreceive.md`](mq_timedreceive.md).

## Notes

- Task-context sends fall back to dynamic allocation when the shared free-list pool is empty, so `ENOMEM` is possible even after the queue becomes writable.
- Interrupt-context sends do not use the queue-space wait path; message allocation falls back to the interrupt-reserved pool after the shared free list.
- A successful send wakes at most one not-empty waiter.
