# `rename`

## Summary

`rename()` renames a path within one mountpoint or, for non-mountpoint source paths, moves a pseudo-filesystem inode by recreating the destination entry.

## Behavior

- Requires both paths to be non-NULL, non-empty, and absolute.
- Resolves the source path with `inode_find()`.
- Mountpoint-owned renames resolve the destination path too and require both paths to belong to the same mountpoint inode.
- Cross-mount renames fail with `EXDEV`.
- Mounted filesystems require a mountpoint `rename()` hook.
- Only non-mountpoint source paths use the pseudo-filesystem flow. That path reserves a destination inode with `inode_reserve()`, copies the source inode state into it, removes the old source path, and then detaches the old inode's child link.
- In the pseudo-filesystem path, any `inode_reserve()` failure is currently collapsed to `EEXIST`, even if the underlying failure reason differs.

## Inputs and Outputs

- `oldpath`: source pathname.
- `newpath`: destination pathname.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `inode_find()` to resolve source and destination ownership.
- Uses the mountpoint `rename()` hook for mounted filesystems.
- Uses `inode_reserve()` and `inode_remove()` for pseudo-filesystem renames.

## Notes

- The comment claims root-like paths are filtered up front, but the actual guard only enforces non-empty absolute paths. A rename of `/` fails later because root has no backing inode.
- The pseudo-filesystem path copies inode fields directly instead of delegating to a filesystem-specific rename helper.
