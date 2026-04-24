# `blake2s_update`

## Purpose

`blake2s_update()` absorbs streaming input into an initialized BLAKE2s state.

## Behavior

- Expects a live `blake2s_state` plus an input pointer.
- Returns immediately with `0` when `inlen <= 0`.
- Appends new bytes into the internal block buffer.
- When enough data is available, completes the pending block, increments the byte counter, and compresses the full block.
- Continues compressing full input blocks directly from the caller buffer while more than one full block remains.
- Leaves the trailing partial block buffered in `S->buf` for a later `blake2s_update()` or `blake2s_final()` call.

## Inputs and Outputs

- `S`: streaming hash state.
- `pin`: input bytes to absorb.
- `inlen`: number of input bytes.
- Return value: `0` in non-asserting paths.

## Dependencies

- Uses `blake2_memcpy()`, `blake2s_increment_counter()`, and `blake2s_compress()`.

## Notes

- The implementation relies on `DEBUGASSERT(pin && S)` instead of a normal runtime error path. Invalid pointers can therefore assert in debug builds instead of returning `-1`.
- Because the assertion checks `pin` unconditionally, a `NULL` input pointer with `inlen == 0` is still debug-assert sensitive even though the function would otherwise return immediately.
