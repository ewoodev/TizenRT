# `allocate_compress_buffer`

## Summary

`allocate_compress_buffer()` allocates one kernel buffer for a compressed block operation.

## Behavior

- Starts with the caller-requested `size` and `offset`.
- Adds `LZMA_PROPS_SIZE` when `CONFIG_COMPRESSION_TYPE == LZMA`, because the implementation stores LZMA properties at the start of the compressed output.
- Allocates the final size from the kernel heap with `kmm_malloc()`.
- Returns the raw pointer without clearing the buffer.

## Inputs and Outputs

- `offset`: extra bytes the caller wants to reserve before the payload.
- `size`: requested payload size.
- Return value: a writable kernel buffer, or `NULL` if allocation fails.

## Notes

- The function does not validate the inputs.
- The caller owns the returned buffer and must free it.
