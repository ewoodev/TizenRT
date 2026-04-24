# `decompress_block`

## Summary

`decompress_block()` expands one compressed block into an output buffer.

## Behavior

- Uses LZMA-specific decoding when `CONFIG_COMPRESSION_TYPE == LZMA`.
- Uses Miniz-specific decoding when `CONFIG_COMPRESSION_TYPE == MINIZ`.
- Treats `writesize` as the output capacity and leaves the decoded byte count there on success.
- For LZMA, consumes the property header stored at the front of `read_buffer`.

## Inputs and Outputs

- `out_buffer`: destination for the decoded bytes.
- `writesize`: output buffer capacity on entry and decoded byte count on success.
- `read_buffer`: compressed input block.
- `size`: compressed input length on entry; the function may adjust it while decoding.
- Return value: `OK` on success, `-ENOMEM` when the destination is too small, or `-EIO` on backend decode failure.

## Notes

- The LZMA path treats `SZ_ERROR_INPUT_EOF` as success.
- The function does not verify that the caller selected the right compression type for the data stream.
