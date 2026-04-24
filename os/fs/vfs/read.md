# `read`

## Summary

`read()` reads from a VFS file descriptor or, when configured, from a socket-like descriptor via `recv(..., 0)`.

## Behavior

- Acts as a cancellation point.
- Routes descriptors outside the normal file range to `recv(fd, buf, nbytes, 0)` when the socket path is enabled.
- Returns `EBADF` for out-of-range descriptors when the socket path is not available.
- Uses `fs_getfilep()` to resolve normal file descriptors.
- Uses `file_read()` for the file path; `file_read()` requires `O_RDOK` and a readable inode operation.
- Converts negative `file_read()` results into `errno` and returns `ERROR`.
- On the current early `fs_getfilep()` failure path, returns the negative helper result directly instead of normalizing to `ERROR`; today that is typically `-EBADF` or `-EAGAIN`.

## Inputs and Outputs

- `fd`: file descriptor or configured socket descriptor.
- `buf`: destination buffer.
- `nbytes`: requested byte count.
- Return value: byte count on success, `0` on end-of-file, `ERROR` on most failures, or a direct negative helper result on some early file-path failures.

## Dependencies

- Uses `fs_getfilep()` and `file_read()` for the file path.
- Uses `recv()` for the socket path when networking support is built.

## Notes

- The socket fallback means this API is not limited to regular files in the current tree.
- The mixed return conventions are part of the current implementation and should be treated carefully by callers and future maintainers.
