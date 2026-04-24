# `statfs`

## Summary

`statfs()` returns filesystem-capacity metadata for a mountpoint-owned path or a pseudo-filesystem inode.

## Behavior

- Rejects `NULL` `path` or `buf` with `EFAULT`.
- Rejects an empty path with `ENOENT`.
- Resolves the path with `inode_find()`.
- Mountpoint-owned paths call the mountpoint `statfs(inode, buf)` hook.
- Non-mountpoint paths synthesize a pseudo-filesystem result with `PROC_SUPER_MAGIC` and `NAME_MAX`.
- Converts negative helper returns into `errno`.

## Inputs and Outputs

- `path`: pathname to inspect.
- `buf`: destination `struct statfs`.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `inode_find()` to resolve the path.
- Uses the mountpoint `statfs()` hook for mounted filesystems.
- Uses `statpseudofs()` for pseudo-filesystem metadata synthesis.

## Notes

- The pseudo-filesystem result does not expose storage totals; it only sets `f_type` and `f_namelen`.
- If a mountpoint is found but does not provide a `statfs()` hook, the current wrapper can still return success without forcing an error or guaranteeing that `buf` was updated.
- `fstatfs()` is the descriptor-based companion API.
