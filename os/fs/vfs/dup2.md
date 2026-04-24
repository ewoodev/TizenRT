# `dup2`

## Summary

`dup2()` duplicates a source descriptor into a requested target slot by dispatching on the source descriptor type.

## Behavior

- Uses the source descriptor range to decide whether the operation is a file-path duplication or a socket-path duplication.
- For file descriptors, delegates to `fs_dupfd2(fd1, fd2)`.
- For configured socket descriptors, delegates to `net_dupsd2(fd1, fd2)`.
- Rejects a source descriptor outside both configured ranges with `EBADF`.
- The file-path helper closes the target file slot first, clears its private data, then clones the source file state into that slot.
- When `fd1 == fd2`, the file-path helper returns the original descriptor without changing it.

## Inputs and Outputs

- `fd1`: source descriptor.
- `fd2`: requested destination descriptor.
- Return value: `fd1` on the `fd1 == fd2` fast path, `OK` on other successful file-path replacements, or the socket-helper result on the socket path. Failures return `ERROR`.

## Dependencies

- Uses `fs_dupfd2()` and the `file_dup2()` helper for file descriptors.
- Uses `net_dupsd2()` for socket descriptors when networking support is built.

## Notes

- File-path duplication cannot target a descriptor outside the file-descriptor table because `fs_dupfd2()` resolves both descriptors through `fs_getfilep()`.
- Like `dup()`, the file-path wrapper assumes negated errno-style helper failures, but `file_dup2()` reports failures through `errno` plus `ERROR`. In those paths the public wrapper can overwrite the original errno with a generic value.
