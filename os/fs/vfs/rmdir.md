# `rmdir`

## Summary

`rmdir()` removes a mountpoint-owned directory or an empty pseudo-filesystem directory node.

## Behavior

- Resolves the pathname with `inode_find()`.
- Returns `ENOENT` when no containing inode exists.
- Mountpoint-owned paths call the mountpoint `rmdir(inode, relpath)` hook.
- Pseudo-filesystem removal is limited to nodes with no operations.
- Refuses to remove pseudo-directories that still have children and reports `ENOTEMPTY`.
- Rejects pseudo-filesystem nodes with operations and reports `ENOTDIR`.

## Inputs and Outputs

- `pathname`: directory path to remove.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `inode_find()` to resolve the path.
- Uses the mountpoint `rmdir()` hook for mounted filesystems.
- Uses `inode_remove()` to detach pseudo-directory nodes from the inode tree.

## Notes

- This implementation does not remove file-like pseudo nodes; those stay on the `unlink()` path.
- `inode_remove()` returning `-EBUSY` is still treated as success because the node will be released when outstanding references go away.
