# `mq_getattr`

## Summary

`mq_getattr()` copies a snapshot of the queue limits, the current queued-message count, and the descriptor flag word into the caller's `struct mq_attr`.

## Behavior

- Rejects `mqdes == NULL` with `EBADF`.
- Rejects `mq_stat == NULL` with `EINVAL`.
- Reads `mq_maxmsg`, `mq_msgsize`, and `mq_curmsgs` from the backing `struct mqueue_inode_s`.
- Reads `mq_flags` from the descriptor's `oflags` field rather than from shared queue state.
- Returns immediately without blocking or allocating memory.

## Inputs and Outputs

- `mqdes`: message queue descriptor whose backing queue and descriptor flags are sampled.
- `mq_stat`: caller-provided output buffer that receives the snapshot.
- Return value: `OK` on success, or `ERROR` with `errno` set when validation fails.

## Dependencies

- Depends on the descriptor and queue objects created by `mq_open.md`, `mq_msgqalloc.md`, and `mq_descreate.md`.
- `mq_setattr.md` reuses this API to populate `oldstat` before changing the descriptor flags.

## Notes

- The snapshot is not scheduler-locked, so `mq_curmsgs` can change immediately after it is read.
- `mq_flags` reflects the descriptor's stored open flags. In the current implementation, `mq_setattr()` only updates the `O_NONBLOCK` bit of that field.
