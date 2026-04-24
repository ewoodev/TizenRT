# `fsync`

## Summary

`fsync()` synchronizes only writable VFS file descriptors whose inode is a mountpoint with a `sync` method.

## Behavior

- Acts as a cancellation point.
- Resolves the descriptor through `fs_getfilep()`.
- Requires the file to be open for writing; otherwise `file_fsync()` reports `EBADF`.
- Requires the inode to be a mountpoint with a valid mountpoint operations table and a `sync` method.
- Rejects non-mountpoint descriptors and mountpoints without `sync()` support with `EINVAL`.
- Calls the mountpoint `sync(filep)` method and returns its result after normalization.

## Inputs and Outputs

- `fd`: writable VFS file descriptor.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the VFS file object.
- Uses `file_fsync()` for the writable mountpoint checks.
- Uses the mountpoint `sync()` method for the actual synchronization step.

## Notes

- This path does not handle drivers, pseudo-files, or socket descriptors.
- The implementation is built only when mountpoint support is enabled.
