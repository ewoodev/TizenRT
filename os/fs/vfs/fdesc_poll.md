# `fdesc_poll`

## Summary

`fdesc_poll()` resolves a file-table descriptor slot through `fs_getfilep()` and then forwards the live poll registration to `file_poll()`.

## Behavior

- Rejects descriptors only when `fs_getfilep()` fails range or file-list lookup and returns that negated errno directly.
- Requires a live `struct pollfd`; the helper passes the same `fds` pointer through to `file_poll()`.
- Does not verify that the resolved slot represents an open file; if the slot's inode is `NULL`, the downstream `file_poll()` path reports `POLLERR | POLLHUP` and returns `OK`.
- Performs no extra readiness classification of its own.

## Inputs and Outputs

- `fd`: file-table descriptor slot to resolve through the VFS file table.
- `fds`: live `struct pollfd` entry to update.
- `setup`: `true` to register, `false` to tear down.
- Return value: the negated errno from `fs_getfilep()` on lookup failure, or the result of `file_poll()` on success.

## Dependencies

- Uses `fs_getfilep()` for descriptor resolution.
- Uses `file_poll()` for the actual poll registration and teardown.

## Notes

- This helper is the descriptor-facing wrapper around the file-based poll path described in [`file_poll.md`](file_poll.md) and [`poll.md`](poll.md).
- It does not add socket-specific behavior; socket dispatch happens higher up in [`poll.md`](poll.md).
- `fs_getfilep()` can also return `-EAGAIN` when the current task has no VFS file list.
