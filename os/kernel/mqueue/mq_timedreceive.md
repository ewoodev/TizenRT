# `mq_timedreceive`

## Summary

`mq_timedreceive()` behaves like `mq_receive()`, but limits the blocking wait with an absolute `CLOCK_REALTIME` timeout.

## Behavior

- Validates `mqdes`, `msg`, read permission, and `msglen` through `mq_verifyreceive()`.
- Rejects `abstime == NULL` and `tv_nsec` outside `[0, 1000000000)` with `EINVAL`.
- Treats the call as a cancellation point.
- Reserves one watchdog up front with `wd_create()` before it knows whether the queue is currently empty.
- If the queue is empty, converts `abstime` to ticks with `clock_abstime2ticks(CLOCK_REALTIME, ...)`, returns `ETIMEDOUT` immediately when the deadline is already past, then arms the watchdog and waits through `mq_waitreceive()`.
- If the queue is already non-empty, skips the tick conversion and watchdog start, but still pays the up-front watchdog allocation/deallocation cost.
- Cancels and deletes the watchdog after the wait path, whether the wait completed with a message, interruption, or timeout.
- On success, uses `mq_doreceive()` to copy the payload, return the optional priority, free the internal message object, and wake one sender waiting for queue space.

## Inputs and Outputs

- `mqdes`: descriptor that must be valid and open for reading.
- `msg`: caller-owned receive buffer.
- `msglen`: size of `msg`; it must be at least the queue's `maxmsgsize`.
- `prio`: optional output for the dequeued message priority.
- `abstime`: absolute `CLOCK_REALTIME` deadline used only when a blocking wait is needed.
- Return value: positive message length on success, or `ERROR` with `errno` set when validation fails, no message arrives before the deadline, or the wait ends without a message.

## Dependencies

- Uses `mq_verifyreceive()`, `mq_waitreceive()`, and `mq_doreceive()` from `mq_rcvinternal.c`.
- Uses `clock_abstime2ticks()` to translate the absolute deadline and `wd_create()` / `wd_start()` / `wd_cancel()` / `wd_delete()` for timeout handling.
- Timeout wakeup is delivered by the local watchdog callback `mq_rcvtimeout()`, which in turn uses `mq_waitirq()` to unblock the waiting task with `ETIMEDOUT`.

## Notes

- The timeout is relevant only when the queue is empty and the descriptor is blocking. If `O_NONBLOCK` is set, the shared receive helper returns `EAGAIN` without using the timeout.
- Because the watchdog is reserved before queue inspection, `mq_timedreceive()` can fail with `ENOMEM` even when a message is already available and no timed wait would be needed.
