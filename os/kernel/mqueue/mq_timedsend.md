# `mq_timedsend`

## Summary

`mq_timedsend()` is the absolute-timeout variant of `mq_send()` for kernel-managed POSIX message queues reached through caller-owned descriptors.

## Behavior

- Validates the same send arguments as `mq_send()` through `mq_verifysend()`, then rejects `abstime == NULL` and out-of-range `tv_nsec`.
- Treats the call as a cancellation point.
- Reserves a watchdog with `wd_create()` before checking whether the queue is already full.
- If the queue is full and the descriptor is blocking, converts `abstime` from absolute `CLOCK_REALTIME` time to ticks with `clock_abstime2ticks()`, starts the watchdog, and waits through `mq_waitsend()`.
- Returns `ETIMEDOUT` immediately when the absolute deadline has already expired before the timed wait starts.
- If the queue is already non-full, skips the timed wait and reuses the same allocation and enqueue path as `mq_send()`.
- Deletes the reserved watchdog before returning, regardless of whether the send succeeded.

## Inputs and Outputs

- `mqdes`: descriptor that must be valid and open for writing.
- `msg`: caller-owned payload buffer.
- `msglen`: payload size; it must not exceed the queue's configured `mq_msgsize`.
- `prio`: message priority; the implementation accepts values up to `MQ_PRIO_MAX`.
- `abstime`: absolute `CLOCK_REALTIME` deadline used only when the queue is full and the descriptor is blocking.
- Return value: `OK` on success, or `ERROR` with `errno` set when validation, timeout setup, waiting, or message allocation fails.

## Dependencies

- Reuses `mq_verifysend()`, `mq_waitsend()`, `mq_msgalloc()`, and `mq_dosend()` from `mq_sndinternal.c`.
- Shares the enqueue side effects documented in [`mq_send.md`](mq_send.md) and [`mq_notify.md`](mq_notify.md).
- Uses `clock_abstime2ticks()` to interpret `abstime` as an absolute `CLOCK_REALTIME` deadline.

## Notes

- `O_NONBLOCK` still wins over the timeout: a full non-blocking descriptor returns `EAGAIN` without waiting.
- Because the watchdog is reserved up front, `ENOMEM` can be reported even when the queue already has room for the message.
- The function is written for task context; the implementation asserts that it is not entered from interrupt context.
