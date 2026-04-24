# `compress_init`

## Summary

`compress_init()` opens one active decompression session for a compressed file descriptor.

## Behavior

- Rejects the request with `-EBUSY` when another file descriptor is already active.
- Parses and validates the compression header at `offset`.
- Stores the parsed header in the module-global `compression_header`.
- Writes the uncompressed file size to `filelen`.
- Allocates temporary `read_buffer` and `out_buffer` storage sized for the selected backend and the parsed block size.

## Inputs and Outputs

- `filfd`: open file descriptor for the compressed file.
- `offset`: header location inside the file.
- `filelen`: output pointer for the uncompressed binary size.
- Return value: `OK`, `-EBUSY`, or a negative errno-style error from header parsing or memory allocation.

## Important Constraints

- The implementation keeps only one active session at a time through the global `active_filefd`.
- The parsed header must match `CONFIG_COMPRESSION_TYPE` and `CONFIG_COMPRESSION_BLOCK_SIZE`.

## Notes

- If allocation fails after the header has been parsed, the module-global session state remains initialized until `compress_uninit()` is called.
