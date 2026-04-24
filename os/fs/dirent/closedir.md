# `closedir`

## Summary

`closedir()` releases a directory stream and all inode references retained by it.

## Behavior

- Rejects a `NULL` stream with `EBADF`.
- For mountpoint-backed streams, calls the filesystem `closedir()` method when it exists.
- For pseudo-filesystem streams, releases the saved `fd_next` reference when present.
- Always releases the retained root inode reference when present.
- Clears and frees the `DIR` container from kernel or user heap.

## Inputs and Outputs

- `dirp`: stream returned by `opendir()`.
- Return value: `OK` on success, or `ERROR` with `errno` set on failure.

## Dependencies

- Uses mountpoint-specific close behavior only when `CONFIG_DISABLE_MOUNTPOINT` is not set.

## Notes

- Missing mountpoint `closedir()` support is not treated as an error.
- If a mountpoint `closedir()` method fails, the wrapper maps the negated return value into `errno`.
