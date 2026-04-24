# `close`

## Summary

`close()` closes either a VFS file descriptor or a configured socket descriptor.

## Behavior

- Acts as a cancellation point.
- Treats descriptors in the normal file range as VFS file descriptors and closes them through `files_close()`.
- When networking is enabled, treats out-of-range descriptors that still fall inside the socket range as socket descriptors and closes them through `net_close()`.
- Rejects descriptors that fit neither range with `EBADF`.

## Inputs and Outputs

- `fd`: file descriptor or configured socket descriptor.
- Return value: `OK` on success, or `ERROR` with `errno` set from the failing close path.

## Dependencies

- Uses `files_close()` for the file path.
- Uses `net_close()` for the socket path when networking support is built.

## Notes

- The file path relies on the driver or mountpoint close method to tolerate repeated opens and closes from different descriptors.
