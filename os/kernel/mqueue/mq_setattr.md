# `mq_setattr`

## Summary

`mq_setattr()` updates only the `O_NONBLOCK` bit in one message queue descriptor's flag word.

## Behavior

- Rejects `mqdes == NULL` with `EBADF`.
- Rejects `mq_stat == NULL` with `EINVAL`.
- Optionally snapshots the previous descriptor state into `oldstat` by calling `mq_getattr()`.
- Replaces only the `O_NONBLOCK` bit in `mqdes->oflags`.
- Ignores `mq_maxmsg`, `mq_msgsize`, and `mq_curmsgs` in the supplied `mq_stat`.

## Inputs and Outputs

- `mqdes`: descriptor whose flag word will be updated.
- `mq_stat`: input attributes. Only `mq_stat->mq_flags & O_NONBLOCK` affects the result.
- `oldstat`: optional output buffer for the pre-update snapshot.
- Return value: `OK` on success, or `ERROR` with `errno` set when `mqdes` or `mq_stat` is `NULL`.

## Dependencies

- Uses `mq_getattr.md` to fill `oldstat` before changing the descriptor flags.
- The resulting `O_NONBLOCK` bit is consumed later by send/receive paths such as `mq_receive()` and `mq_timedsend()`.

## Notes

- The change is descriptor-scoped, not queue-scoped. Updating one descriptor does not change the flags stored in other descriptors for the same queue.
- This API does not validate or apply any queue sizing fields from `mq_stat`.
