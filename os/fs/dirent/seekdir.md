# `seekdir`

## Summary

`seekdir()` moves a directory stream to the position used by the next `readdir()` call.

## Behavior

- Returns immediately for `NULL` streams or streams with no root inode.
- For pseudo-filesystem streams, either starts from the current position or rewinds to the root and then walks peer inodes until it reaches the requested offset or the end.
- For mountpoint-backed streams, rewinds first when seeking backward and then advances by repeatedly calling the filesystem `readdir()` method.
- Pseudo-filesystem seeks always store the furthest reachable position, which can land at end-of-directory when the requested offset is out of range.
- Mountpoint-backed seeks update the stored position only when the helper path completes successfully.

## Inputs and Outputs

- `dirp`: stream returned by `opendir()`.
- `offset`: desired logical directory position.
- Return value: none.

## Dependencies

- Best-effort mountpoint behavior depends on both `rewinddir()` and `readdir()` support in the filesystem method table.

## Notes

- This API cannot report failure. Unsupported mountpoints, failed reads, and out-of-range offsets are handled silently, and the final stream position depends on which backend path was active.
- The intended offset source is `telldir()`, but `telldir()` is implemented outside `os/` and remains pending in this workspace.
