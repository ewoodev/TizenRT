# `stat`

## Summary

`stat()` attempts to return metadata for an exact root path, a mountpoint-owned path, or a pseudo-filesystem inode.

## Behavior

- Rejects `NULL` `path` or `buf` with `EFAULT`.
- Rejects an empty path with `ENOENT`.
- Treats the exact `/` path as a fake root directory and synthesizes metadata without resolving an inode.
- Resolves other paths with `inode_find()`.
- Mountpoint-owned paths call the mountpoint `stat(inode, relpath, buf)` hook when that hook exists.
- Non-mountpoint paths use `inode_stat()` to synthesize pseudo-filesystem metadata.
- If a mountpoint is found without a `stat()` hook, the wrapper can still return success without forcing an error or guaranteeing that `buf` was updated.
- Converts negative helper returns into `errno`.

## Inputs and Outputs

- `path`: pathname to inspect.
- `buf`: destination `struct stat`.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `inode_find()` to resolve the path.
- Uses `statroot()` for the exact root directory.
- Uses the mountpoint `stat()` hook when the resolved inode is a mountpoint.
- Uses `inode_stat()` for pseudo-filesystem metadata.

## Notes

- The synthesized root metadata marks `/` as a read-only directory and does not populate execute bits.
- `inode_stat()` fills only a limited subset of `struct stat`, mostly type and permission bits inferred from inode shape.
- `fstat()` is the descriptor-based companion API.
