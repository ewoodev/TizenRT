# `fcntl`

## Summary

`fcntl()` implements a limited descriptor-control subset for VFS file descriptors and, when enabled, socket descriptors.

## Behavior

- Acts as a cancellation point.
- Routes file-range descriptors to `file_vfcntl()` after resolving the VFS file object with `fs_getfilep()`.
- Routes configured socket descriptors to `net_vfcntl()` when networking support is built.
- Rejects descriptor values outside the configured file and socket ranges with `EBADF`.
- Supports `F_DUPFD`, `F_GETFL`, and `F_SETFL` on the file path.
- `F_SETFL` masks updates with `FFCNTL`, so only the nonblocking, append, and sync-style status bits in that mask are rewritten.
- Returns `ENOSYS` for `F_GETFD`, `F_SETFD`, `F_GETLK`, `F_SETLK`, and `F_SETLKW` on the file path.
- Returns `EBADF` for `F_GETOWN` and `F_SETOWN` on the file path because those commands are treated as socket-only.

## Inputs and Outputs

- `fd`: file descriptor or configured socket descriptor.
- `cmd`: descriptor-control command.
- Variadic argument: command-specific value or pointer.
- Return value: a command-specific non-negative result on success, or `ERROR` on failure.

## Dependencies

- Uses `fs_getfilep()` and `file_vfcntl()` for file descriptors.
- Uses `net_vfcntl()` for configured socket descriptors when networking support is built.
- Uses `file_dup()` internally for `F_DUPFD` on the file path; see `dup.md` for the shared duplication mechanics.

## Notes

- The public wrapper expects negated errno-style helper failures, but both `file_vfcntl()` and `net_vfcntl()` can return `ERROR` after already setting `errno`. In those paths the public wrapper can overwrite the original errno with a generic value.
- The socket `F_DUPFD` path ignores the requested minimum descriptor because `net_vfcntl()` delegates to `dup(sd)` without consuming the variadic `minfd`.
- This API does not implement the full POSIX `fcntl()` command set in the current tree.
