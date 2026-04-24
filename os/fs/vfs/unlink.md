# `unlink`

## Summary

`unlink()` removes a mountpoint-owned path or a pseudo-filesystem file-like node.

## Behavior

- Resolves the pathname with `inode_find()`.
- Returns `ENOENT` when no containing inode exists.
- Mountpoint-owned paths call the mountpoint `unlink(inode, relpath)` hook.
- Pseudo-filesystem removal is limited to non-special nodes that have operations.
- Refuses to remove pseudo-filesystem nodes that still have children and reports `ENOTEMPTY`.
- Calls driver or block-driver `unlink()` hooks when those hooks exist before removing the inode from the tree.
- Returns `ENXIO` for pseudo-directories and special nodes because they are not handled by this path.

## Inputs and Outputs

- `pathname`: path to remove.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `inode_find()` to resolve the path.
- Uses the mountpoint `unlink()` hook for mounted filesystems.
- Uses driver or block-driver `unlink()` hooks for pseudo-file nodes when present.
- Uses `inode_remove()` to detach pseudo-filesystem nodes from the inode tree.

## Notes

- Pseudo-directories are intentionally handled by `rmdir()`, not `unlink()`.
- `inode_remove()` can report `-EBUSY` when the inode is still referenced; this wrapper still treats that case as a successful unlink.
