# `ftruncate`

## Summary

`ftruncate()` adjusts file size only for writable VFS file descriptors whose inode is a mountpoint with a `truncate` method.

## Behavior

- Rejects negative `length` values with `EINVAL` before resolving the descriptor.
- Resolves the descriptor through `fs_getfilep()`.
- Requires the file to be open for writing; otherwise `file_truncate()` reports `EBADF`.
- Requires the inode to be a mountpoint with a valid mountpoint operations table.
- Treats mountpoints without a `write` method as read-only and reports `EROFS`.
- Treats mountpoints without a `truncate` method as unsupported and reports `ENOSYS`.
- Calls the mountpoint `truncate(filep, length)` method for the actual size change.

## Inputs and Outputs

- `fd`: writable VFS file descriptor.
- `length`: requested new length.
- Return value: `0` on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the VFS file object.
- Uses `file_truncate()` for the writable mountpoint checks.
- Uses the mountpoint `truncate()` method for the actual size change.

## Notes

- This implementation does not special-case shared memory objects even though generic POSIX descriptions often mention them.
- Drivers, pseudo-files, and socket descriptors do not use this path.
- The implementation is built only when mountpoint support is enabled.
