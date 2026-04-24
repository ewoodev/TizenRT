# `open`

## Summary

`open()` resolves an existing driver or mountpoint inode, allocates a file descriptor, and calls the underlying open path.

## Behavior

- Acts as a cancellation point and delegates most work to the internal `vopen()` helper.
- Rejects a `NULL` `path` with `EINVAL`.
- Looks up an existing inode with `inode_find()`.
- Returns `ENOENT` when the path does not resolve to an inode, even if `O_CREAT` is present.
- Can route block-driver paths through `block_proxy()` when BCH proxy support is enabled.
- Rejects non-driver, non-mountpoint, or special inodes with `ENXIO`.
- Verifies requested read/write access through `inode_checkflags()`.
- Allocates a descriptor with `files_allocate()` and then calls the driver or mountpoint open method.

## Inputs and Outputs

- `path`: pathname to resolve.
- `oflags`: access and mode flags.
- Variadic `mode`: currently consumed when `CONFIG_FILE_MODE` is enabled and either `O_WRONLY` or `O_CREAT` is set.
- Return value: a non-negative file descriptor on success, or `ERROR` with `errno` set from the negated internal failure.

## Dependencies

- Uses `inode_find()`, `inode_checkflags()`, `files_allocate()`, and `fs_getfilep()`.
- Uses mountpoint `open(filep, relpath, oflags, mode)` when the inode is a mountpoint.
- Uses `block_proxy()` for eligible block drivers when the BCH proxy path is built.

## Notes

- The current implementation consumes the variadic `mode` argument before it knows whether inode creation is needed.
- Even when `O_CREAT` is present, this path does not create a missing inode on its own.
- `open()` is file-system oriented; socket descriptors are not created through this path.
