# `opendir`

## Summary

`opendir()` opens a directory stream from an absolute filesystem path.

## Behavior

- Rejects `NULL` or empty paths with `ENOENT`.
- Rejects relative paths with `ENOTDIR`.
- Treats `/` as a special pseudo-filesystem root case.
- Uses `inode_search()` to resolve non-root paths.
- Allocates the `DIR` container from kernel or user heap depending on the calling task context.
- For mountpoints, requires `stat()` and `opendir()` methods and rejects non-directory targets.
- For pseudo-filesystem nodes, opens the child list of a directory node or an empty dangling directory node.

## Inputs and Outputs

- `path`: absolute path to open.
- Return value: a `DIR *` on success, or `NULL` with `errno` set on failure.

## Dependencies

- Built only when `CONFIG_NFILE_DESCRIPTORS != 0`.
- Behavior changes when `CONFIG_DISABLE_MOUNTPOINT` is enabled because only pseudo-filesystem paths remain.
- Uses inode search/stat helpers and the internal `struct fs_dirent_s` container.

## Notes

- Missing paths currently map to `ENOTDIR`, not `ENOENT`, after inode lookup fails.
- Root is always treated as a pseudo-filesystem directory even if the root inode also carries mountpoint state.
- `readdir_r()` and `telldir()` are declared in the same public header but implemented outside `os/`; this tranche does not cover them.
