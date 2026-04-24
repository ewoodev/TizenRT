# `fstat`

## Summary

`fstat()` collects metadata through a file-table descriptor slot.

## Behavior

- Accepts only descriptors in the file-descriptor table range.
- Rejects out-of-range descriptors with `EBADF`.
- Resolves the descriptor with `fs_getfilep()`.
- Assumes the resolved slot already contains an initialized inode; there is no explicit open-state validation beyond the internal debug assertion.
- Mountpoint-backed descriptors call the mountpoint `fstat(filep, buf)` hook.
- Non-mountpoint descriptors use `inode_stat()` to synthesize pseudo-filesystem metadata.
- Mountpoints without `fstat()` support fail with `ENOSYS`.

## Inputs and Outputs

- `fd`: file-table descriptor slot.
- `buf`: destination `struct stat`.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the descriptor.
- Uses the mountpoint `fstat()` hook when the inode is a mountpoint.
- Uses `inode_stat()` for pseudo-filesystem metadata.

## Notes

- There is no socket fallback.
- `fs_getfilep()` does not prove that the slot references an open file; this wrapper relies on the returned `struct file` and then dereferences `f_inode`.
- Like `stat()`, the pseudo-filesystem path reports only synthesized inode type and permission bits, not rich filesystem metadata.
- `stat.md` describes the pathname-based companion API.
