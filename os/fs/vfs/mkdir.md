# `mkdir`

## Summary

`mkdir()` creates a directory through a writable mountpoint or by reserving a pseudo-filesystem inode.

## Behavior

- Resolves the pathname with `inode_find()`.
- When a containing mountpoint is found, requires a mountpoint operations table and a `mkdir()` hook.
- For nested mountpoint-relative paths, probes the parent path with the mountpoint `stat()` hook and rejects non-writable parents with `EACCES`.
- For root-level mountpoint-relative paths, skips that parent-permission probe.
- When no containing inode exists and pseudo-filesystem operations are enabled, reserves a new inode directly with `inode_reserve()`.
- When mountpoint support is absent for a containing inode, reports `EEXIST` in the current wrapper.

## Inputs and Outputs

- `pathname`: directory path to create.
- `mode`: requested mode bits passed to the mountpoint `mkdir()` hook.
- Return value: `OK` on success, or `ERROR` on failure.

## Dependencies

- Uses `inode_find()` to detect mountpoint ownership.
- Uses the mountpoint `stat()` hook for parent write checks on nested relative paths.
- Uses the mountpoint `mkdir()` hook for mounted filesystems.
- Uses `inode_reserve()` for pseudo-filesystem creation.

## Notes

- The pseudo-filesystem path does not preserve the caller's `mode`; it just reserves an inode node in the tree.
- This implementation is built only when writable mountpoints or pseudo-filesystem operations are enabled.
