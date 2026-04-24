# `readdir`

## Summary

`readdir()` returns the next directory entry from a stream.

## Behavior

- Rejects a `NULL` stream with `EBADF`.
- Returns `NULL` with `errno = 0` when the stream is already at end-of-directory.
- For mountpoint-backed streams, requires a filesystem `readdir()` method and returns `EACCES` when that method is missing.
- For pseudo-filesystem streams, copies the current inode name into the embedded `dirent`, derives `d_type`, advances to the peer inode, and manages references for the next read.
- Increments the internal stream position only after a successful read.

## Inputs and Outputs

- `dirp`: stream returned by `opendir()`.
- Return value: pointer to the stream-owned `struct dirent` on success, or `NULL` on error or end-of-directory.

## Dependencies

- Uses pseudo-filesystem inode traversal when the stream is not backed by a mountpoint.

## Notes

- EOF and error are both reported as `NULL`, so callers need `errno` to distinguish them.
- The returned `struct dirent` lives inside the `DIR` object and is overwritten by the next successful `readdir()` call.
