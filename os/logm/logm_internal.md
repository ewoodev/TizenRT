# `logm_internal`

## Purpose

Formats a log message and sends it either to the buffered logger path or to the low-level output path.

## Buffered Path

- Uses the buffered path only when the logger is marked ready.
- Refuses the buffered path while a resize request is pending.
- Requires `flag == LOGM_NORMAL`.
- Requires normal thread context; interrupt context falls back to the low-level path.

## Buffered Behavior

- Enters a critical section before touching shared buffer state.
- Drops new messages once buffer overflow has been marked and increments the dropped-message counter.
- Initializes a `lib_outstream_s` backed by the circular buffer.
- Optionally prepends a timestamp when `CONFIG_LOGM_TIMESTAMP` is enabled.
- Formats the message with `lib_vsprintf()` and advances the tail index.
- Marks overflow when the new tail would collide with the head.

## Fallback Behavior

- Uses `lib_lowoutstream()` when low-level output support is compiled in.
- Flushes any queued buffered bytes before printing the fallback message.
- Formats directly into the low-level stream.

## Return Value

- Returns the character count reported by `lib_vsprintf()` on the active backend.
- Returns `0` when a buffered message is rejected because the overflow flag is already set.

## Notes

The function ignores `indx` and `priority` in the current implementation. They are part of the API shape but do not affect routing or formatting yet.
