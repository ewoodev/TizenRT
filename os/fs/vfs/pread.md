# `pread`

## Summary

`pread()` temporarily seeks, reads, and then attempts to restore the saved file position.

## Behavior

- Acts as a cancellation point.
- Uses `fs_getfilep()` to resolve the descriptor into a VFS file object.
- Saves the current position with `file_seek(filep, 0, SEEK_CUR)`.
- Seeks to the requested `offset` with `file_seek(filep, offset, SEEK_SET)`.
- Performs the data transfer with `file_read()`.
- Attempts to restore the saved position with `file_seek(filep, savepos, SEEK_SET)`.
- Returns `ERROR` immediately when the media is not seekable or a seek step fails.
- Converts negative `file_read()` results into `errno` before returning `ERROR`.
- Can lose detailed errno information because several helper failures collapse to plain `ERROR` before the public wrapper remaps them, which can leave `errno` as `1`.

## Inputs and Outputs

- `fd`: VFS file descriptor.
- `buf`: destination buffer.
- `nbytes`: requested byte count.
- `offset`: temporary read position.
- Return value: byte count on success, `0` on end-of-file, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` to resolve the descriptor.
- Uses `file_seek()` to save, change, and restore file position.
- Uses `file_read()` for the actual data transfer.

## Notes

- This path does not have a socket fallback.
- A restore failure after a successful read is treated as an `ERROR` and can leave the file position changed.
- Because several seek failures collapse to plain `ERROR`, the current wrapper may overwrite the original helper errno with a generic remapped value.
