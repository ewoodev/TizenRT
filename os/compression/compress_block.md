# `compress_block`

## Summary

`compress_block()` compresses one memory block with the algorithm selected by `CONFIG_COMPRESSION_TYPE`.

## Behavior

- Uses LZMA when `CONFIG_COMPRESSION_TYPE == LZMA`.
- Uses Miniz when `CONFIG_COMPRESSION_TYPE == MINIZ`.
- Updates `writesize` with the compressed byte count reported by the backend.
- For LZMA, writes the encoded properties into the beginning of `out_buffer` and stores the compressed payload after that header area.
- For Miniz, writes the compressed payload directly into `out_buffer`.

## Inputs and Outputs

- `out_buffer`: destination buffer for compressed data.
- `writesize`: input capacity and output compressed size.
- `read_buffer`: source bytes to compress.
- `size`: number of source bytes.
- Return value: `0` on success or a backend-specific error code on failure.

## Error Handling

- LZMA converts `SZ_ERROR_FAIL` to a negative value but otherwise passes backend errors through.
- Miniz logs the failure and negates positive error codes before returning them.

## Notes

- The function does not allocate memory.
- Callers must size `out_buffer` for the selected backend.
