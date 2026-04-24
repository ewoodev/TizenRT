# `rewinddir`

## Summary

`rewinddir()` resets a directory stream to the beginning when the active backend supports rewinding.

## Behavior

- Returns immediately for `NULL` streams or streams with no root inode.
- For mountpoint-backed streams, calls the filesystem `rewinddir()` method only when that method exists.
- For pseudo-filesystem streams, resets the next inode pointer back to the root child list and clears the logical position counter.

## Inputs and Outputs

- `dirp`: stream returned by `opendir()`.
- Return value: none.

## Dependencies

- Mountpoint behavior depends on `CONFIG_DISABLE_MOUNTPOINT` and on the filesystem method table.

## Notes

- This API has no error return, so unsupported mountpoints are silently ignored.
