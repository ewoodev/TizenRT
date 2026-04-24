# `pwrite`

## Summary

`pwrite()` temporarily seeks, writes, and then attempts to restore the saved file position.

## Behavior

- Acts as a cancellation point.
- Uses `fs_getfilep()` to resolve the descriptor into a VFS file object.
- Saves the current position with `file_seek(filep, 0, SEEK_CUR)`.
- Seeks to the requested `offset` with `file_seek(filep, offset, SEEK_SET)`.
- Performs the data transfer with `file_write()`.
- Attempts to restore the saved position with `file_seek(filep, savepos, SEEK_SET)`.
- Propagates negative helper results through the wrapper's errno remapping path.
- Can lose detailed errno information when a seek helper returns plain `ERROR`, which can leave `errno` as `1`.

## Inputs and Outputs

- `fd`: VFS file descriptor.
- `buf`: source buffer.
- `nbytes`: requested byte count.
- `offset`: temporary write position.
- Return value: byte count on success, `0` when nothing is written, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the descriptor.
- Uses `file_seek()` to save, change, and restore file position.
- Uses `file_write()` for the actual data transfer.

## Notes

- This path does not have a socket fallback.
- The source already notes an `O_APPEND` caveat: if the file was opened with `O_APPEND`, the underlying write path may still append regardless of `offset`.
- A restore failure after a successful write is treated as an `ERROR` and can leave the file position changed.
- Because seek-helper failures can collapse to plain `ERROR`, the current wrapper may overwrite the original helper errno with a generic remapped value.
