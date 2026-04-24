# `fstatfs`

## Summary

`fstatfs()` returns filesystem-capacity metadata for an open VFS file descriptor.

## Behavior

- Resolves the descriptor with `fs_getfilep()`.
- Rejects invalid file descriptors through the `fs_getfilep()` error path.
- Can fail with `EAGAIN` when the current thread has no allocated VFS file table.
- Rejects unopened file slots with `EBADF`.
- Mountpoint-backed descriptors call the mountpoint `statfs(inode, buf)` hook.
- Non-mountpoint descriptors synthesize a pseudo-filesystem result with `PROC_SUPER_MAGIC` and `NAME_MAX`.
- Mountpoints without `statfs()` support fail with `ENOSYS`.

## Inputs and Outputs

- `fd`: open VFS file descriptor.
- `buf`: destination `struct statfs`.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the descriptor.
- Uses the mountpoint `statfs()` hook when the inode is a mountpoint.
- Uses inline pseudo-filesystem synthesis for non-mountpoint inodes.

## Notes

- There is no socket fallback.
- The current implementation only `DEBUGASSERT`s that `buf` is non-NULL; it does not perform a normal runtime `NULL` check before writing through the pointer.
- Like `statfs()`, the pseudo-filesystem result sets only `f_type` and `f_namelen`.
- Unlike path-based `statfs()`, a mountpoint without `statfs()` support fails with `ENOSYS` here instead of silently falling through as success.
