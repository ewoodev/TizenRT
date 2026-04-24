# `lseek`

## Summary

`lseek()` repositions a VFS file descriptor by delegating to `file_seek()`.

## Behavior

- Uses `fs_getfilep()` to resolve the descriptor into a VFS file object.
- Calls `file_seek()` to do the real repositioning work.
- When the inode provides a seek method, uses that method directly.
- When the inode has no seek method, synthesizes `SEEK_SET` and `SEEK_CUR` by updating `filep->f_pos`.
- Rejects negative resulting offsets with `EINVAL`.
- Rejects `SEEK_END` with `ENOSYS` when no inode seek method exists.
- Returns the resulting offset on success.
- On helper-side failures that collapse to plain `ERROR`, the current wrapper does not preserve the helper's detailed `errno` information and can end up reporting `errno = 1`.

## Inputs and Outputs

- `fd`: VFS file descriptor.
- `offset`: requested displacement.
- `whence`: `SEEK_SET`, `SEEK_CUR`, or `SEEK_END`.
- Return value: new file position on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the descriptor.
- Uses `file_seek()` for the actual position update.

## Notes

- This path is VFS-only; there is no socket fallback.
- The current wrapper relies on `file_seek()` for most errno reporting, but it does not preserve detailed errno information on every failure path.
