# `mq_receive`

## Summary

`mq_receive()` removes the oldest message from the highest-priority non-empty band of a message queue and copies it into the caller's buffer.

## Behavior

- Validates `mqdes`, `msg`, read permission, and `msglen` through `mq_verifyreceive()`.
- Treats the call as a cancellation point.
- Blocks when the queue is empty and `O_NONBLOCK` is clear in the descriptor.
- Returns immediately with `EAGAIN` when the queue is empty and `O_NONBLOCK` is set.
- Uses the shared `mq_waitreceive()` helper to dequeue one queued message while the scheduler is locked and interrupts are disabled around the wait/dequeue step.
- Copies the message payload into `msg`, optionally stores the message priority in `prio`, frees the internal message object, and then wakes one sender waiting for the queue to become non-full.

## Inputs and Outputs

- `mqdes`: descriptor that must be valid and open for reading.
- `msg`: caller-owned receive buffer.
- `msglen`: size of `msg`; it must be at least the queue's `maxmsgsize`.
- `prio`: optional output for the dequeued message priority.
- Return value: positive message length on success, or `ERROR` with `errno` set when validation fails or the wait ends without a message.

## Dependencies

- Uses `mq_verifyreceive()`, `mq_waitreceive()`, and `mq_doreceive()` from `mq_rcvinternal.c`.
- Wakeup/cancellation/timeout interruption is driven through `mq_waitirq()` and the scheduler wait state `TSTATE_WAIT_MQNOTEMPTY`.
- Sender wakeup after a successful receive feeds back into the send path through the shared queue wait lists.

## Notes

- The blocking wait is descriptor-scoped because the `O_NONBLOCK` decision comes from `mqdes->oflags`, not from queue-global state.
- A successful receive always frees the internal message object before the call returns.
