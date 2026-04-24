# `compress_read`

## Summary

`compress_read()` reads a byte range from a compressed file as if the file were already decompressed.

## Behavior

- Uses the module-global header created by `compress_init()`.
- Computes the first and last compressed blocks that cover the requested uncompressed range.
- Reads and decompresses only those blocks.
- Copies the requested span into `buffer`, handling partial first and last blocks and full intermediate blocks.

## Inputs and Outputs

- `filfd`: open file descriptor for the compressed file.
- `binary_header_size`: bytes that precede the compression header in the file.
- `buffer`: destination for uncompressed bytes.
- `readsize`: requested number of uncompressed bytes.
- `offset`: starting byte offset in the uncompressed data range.
- Return value: the number of bytes copied into `buffer`, or a negative
  failure result when a seek, read, or decode step fails. Some helpers return
  specific negative errno-style codes, but several local failure paths return
  plain `ERROR` (`-1`).

## Dependencies

- Requires a valid `compression_header` and temporary buffers from `compress_init()`.
- Uses `compress_read_block()` to fetch compressed blocks and `decompress_block()` to expand them.

## Notes

- The function does not re-check that `filfd` matches `active_filefd`.
- The copy logic assumes the header and block metadata were already validated during `compress_init()`.
