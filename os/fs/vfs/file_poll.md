# `file_poll`

## Summary

`file_poll()` applies poll registration to a `struct file` slot and is the common helper used by descriptor-backed and detached-file polling paths.

## Behavior

- Requires a live `struct pollfd`; the helper dereferences `fds` and posts `fds->sem` directly.
- Returns `OK` immediately when `filep->f_inode` is `NULL`, after setting `POLLERR | POLLHUP`, clearing `POLLOUT`, and posting the waiter semaphore.
- Delegates driver-backed inodes with a `poll()` method to that driver hook.
- Treats mountpoints and block devices as immediately ready for requested `POLLIN` and `POLLOUT` events during setup.
- Treats mountpoint and block-device teardown as a successful no-op after the descriptor has already been registered.
- Leaves the default return value as `-ENOSYS` when no driver hook or mountpoint/block special case applies.

## Inputs and Outputs

- `filep`: `struct file` slot supplied by the caller.
- `fds`: live `struct pollfd` entry to update.
- `setup`: `true` to register, `false` to tear down.
- Return value: `OK` on the `NULL` inode path or mountpoint/block setup path, a negated errno from a driver hook, or `-ENOSYS` when nothing handles the inode.

## Dependencies

- Uses the inode `poll()` hook when the inode is a driver with poll support.
- Uses the mountpoint and block-device readiness behavior documented in [`poll.md`](poll.md).

## Notes

- The helper does not accept `NULL` for `fds`.
- The helper synthesizes readiness for the missing-inode path and for mountpoint/block descriptors instead of delegating every case to a driver hook.
