# `write`

## Summary

`write()` writes through the VFS file path or, in builds with the socket write path enabled, via `send(..., 0)`.

## Behavior

- Acts as a cancellation point.
- Routes descriptors outside the normal file range to `send(fd, buf, nbytes, 0)` only when the socket write path is built.
- Returns `EBADF` for out-of-range descriptors when that socket path is unavailable.
- Uses `fs_getfilep()` to resolve normal file descriptors.
- Uses `file_write()` for the file path; `file_write()` requires `O_WROK` and a writable inode operation.
- Converts negative `file_write()` results into `errno` and returns `ERROR`.
- On the current early `fs_getfilep()` failure path, returns the negative helper result directly instead of normalizing to `ERROR`; today that is typically `-EBADF` or `-EAGAIN`.

## Inputs and Outputs

- `fd`: file descriptor or configured socket descriptor.
- `buf`: source buffer.
- `nbytes`: requested byte count.
- Return value: byte count on success, `0` when nothing is written, `ERROR` on most failures, or a direct negative helper result on some early file-path failures.

## Dependencies

- Uses `fs_getfilep()` and `file_write()` for the file path.
- Uses `send()` for the socket path when that configuration is enabled.

## Notes

- The socket fallback is narrower than `read()` in this tree: it is compiled behind the specific socket write configuration path used by `fs_write.c`.
- The mixed return conventions are part of the current implementation and should be treated carefully by callers and future maintainers.
