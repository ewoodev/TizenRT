# `mq_notify`

## Summary

`mq_notify()` registers or removes one queue-global one-shot notification for a message queue.

## Behavior

- Rejects `mqdes == NULL` with `EBADF`.
- Rejects descriptors whose `msgq` pointer is missing with `EBADF`.
- When no registration exists and `notification != NULL`, validates only `notification->sigev_signo` with `GOOD_SIGNO()`, then stores the current task PID, the descriptor pointer, the signal number, and the signal value on the queue.
- Ignores `notification->sigev_notify`; the current implementation always behaves like a signal-based registration.
- When no registration exists and `notification == NULL`, returns `OK` without changing anything.
- When another task already owns the registration, returns `EBUSY`.
- When the current task already owns the registration, `notification == NULL` detaches it, but `notification != NULL` still returns `EBUSY` instead of replacing it in place.
- Registration is one-shot. The sender path clears the stored notification before queueing the signal after a successful transition to not-empty.
- Closing the owning descriptor also clears the stored registration through `mq_desclose_group()`.

## Inputs and Outputs

- `mqdes`: descriptor that will own the registration while it is active.
- `notification`: optional `struct sigevent`. Only `sigev_signo` and `sigev_value` are consumed by this implementation.
- Return value: `OK` on success, or `ERROR` with `errno` set when validation or ownership checks fail.

## Dependencies

- Uses the per-queue notification fields in `struct mqueue_inode_s` declared in `tinyara/mqueue.h`.
- One-shot delivery and auto-detach are completed by the send path in `mq_sndinternal.c`.
- Descriptor teardown in `mq_desclose_group()` also clears any registration owned by the closing descriptor.

## Notes

- The current behavior is intentionally documented as non-POSIX in one case: notification is still sent even if another task is already blocked in `mq_receive()`.
- Detach permission is checked against the stored task PID. The stored descriptor pointer is used later for close-time cleanup through `mq_desclose_group()`.
