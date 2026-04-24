# `select`

## Summary

`select()` monitors read, write, and exception readiness by translating the caller's `fd_set` inputs into a temporary `pollfd` array and delegating the wait to `poll()`.

## Behavior

- Scans the descriptor range `0..nfds-1` twice.
- Builds one `pollfd` entry per requested descriptor and sets `POLLIN`, `POLLOUT`, and `POLLERR` according to the input sets.
- Allocates the temporary `pollfd` array with `kmm_zalloc()` only when at least one descriptor is monitored.
- Clears the caller-provided `fd_set` objects before copying readiness bits back from `poll()` results.
- Maps `POLLIN | POLLHUP` back to `readfds`, `POLLOUT` back to `writefds`, and `POLLERR` back to `exceptfds`.
- Returns the number of readiness bits set across the output sets, not the number of unique ready descriptors.
- Uses `_set_timeout()` to convert `struct timeval` to milliseconds; microseconds are truncated and negative or out-of-range timeval fields are not validated.
- Treats negative timeout values as an infinite wait once the conversion reaches `poll()`.

## Inputs and Outputs

- `nfds`: highest descriptor number plus one.
- `readfds`: descriptors to test for readability, or `NULL`.
- `writefds`: descriptors to test for writability, or `NULL`.
- `exceptfds`: descriptors to test for exception readiness, or `NULL`.
- `timeout`: relative wait interval in `struct timeval` units, or `NULL` for no timeout.
- Return value: number of readiness bits set on success, `0` on timeout, or `ERROR` on failure.

## Dependencies

- Uses `poll()` as the underlying wait primitive.
- Uses `kmm_zalloc()` and `kmm_free()` for the temporary descriptor list.
- Uses `FD_ISSET()`, `FD_SET()`, and `FD_ZERO()` for descriptor set handling.

## Notes

- There is no bounds check that ties `nfds` back to the current `fd_set` capacity, so callers must supply a compatible range.
- `select()` is a cancellation point.
- The current `ndx != npfds` error path returns early and bypasses the cleanup that would otherwise run after `poll()`.
