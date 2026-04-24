# `dup`

## Summary

`dup()` duplicates either a VFS file descriptor or, when networking is enabled, a configured socket descriptor into the lowest available slot in the matching descriptor table.

## Behavior

- Treats descriptors in the file range as VFS file descriptors and delegates to `fs_dupfd(fd, 0)`.
- When networking is enabled, treats out-of-file-range descriptors in the socket range as socket descriptors and delegates to `net_dupsd(fd)`.
- Rejects descriptor values that fit neither configured range with `EBADF`.
- File-path duplication eventually clones the underlying file structure through `file_dup()`, carrying over the original file flags and file position.
- File-path duplication reopens the underlying inode through either the mountpoint `dup()` method or the driver `open()` method.

## Inputs and Outputs

- `fd`: file descriptor or configured socket descriptor.
- Return value: new descriptor on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_dupfd()` and the `file_dup()` helper for file descriptors.
- Uses `net_dupsd()` for socket descriptors when networking support is built.

## Notes

- The file-path wrapper expects negated errno-style helper failures, but `file_dup()` reports failures through `errno` plus `ERROR`. In those paths the public wrapper can overwrite the original errno with a generic value.
- The selected descriptor comes from the lowest available slot in the chosen file or socket descriptor space.
