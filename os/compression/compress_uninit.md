# `compress_uninit`

## Summary

`compress_uninit()` tears down the current decompression session.

## Behavior

- Frees `buffers.read_buffer` and `buffers.out_buffer` when the cached header indicates an LZMA or Miniz stream.
- Frees the cached `compression_header`.
- Clears the module-global pointers.
- Resets `active_filefd` back to `-1`.

## Inputs and Outputs

- No parameters.
- No return value.

## Notes

- The function assumes `compression_header` is valid. Callers should not invoke it before `compress_init()` has created a session.
- This is the only normal path that releases the module-global session state.
