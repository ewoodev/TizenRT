# `ioctl`

## Summary

`ioctl()` performs descriptor-specific control operations by dispatching the
request to either the socket layer or the opened file driver's ioctl handler.

## Behavior

- Validates whether `fd` falls in the regular file-descriptor range.
- When `fd` is outside that range and networking is enabled, it forwards the
  request to `net_ioctl()` for socket descriptors.
- Otherwise it resolves the descriptor to a `struct file *` with
  `fs_getfilep()` and calls `file_ioctl()`.
- `file_ioctl()` rejects missing inodes and drivers without an ioctl method
  before calling the driver's `ioctl()` callback.

## Public API vs Implementation Symbol

- The public API name is always `ioctl`.
- In builds with `CONFIG_LIBC_IOCTL_VARIADIC`, the compiled implementation in
  `os/fs/vfs/fs_ioctl.c` is named `fs_ioctl` and expects the variadic wrapper
  to pass the third argument as an `unsigned long`.
- In non-variadic builds, the implementation symbol is `ioctl`.

## Inputs and Outputs

- `fd`: file or socket descriptor.
- `req`: ioctl command number.
- `arg` / `...`: command-specific argument value.
- Return value: non-negative command-specific result on success, or `ERROR`
  with `errno` set on failure.

## Error Handling

- Invalid descriptor ranges end with `EBADF`.
- Descriptor lookup failures propagate the negated error from `fs_getfilep()`.
- Unsupported driver ioctls propagate `ENOTTY`.
- Socket and file-driver failures are converted from negative internal status
  codes into `errno` before returning `ERROR`.

## Dependencies

- `fs_getfilep()`
- `file_ioctl()`
- `net_ioctl()` when `CONFIG_NET` is enabled
- Driver-specific `inode->u.i_ops->ioctl`
