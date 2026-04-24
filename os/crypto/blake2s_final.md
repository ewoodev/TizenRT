# `blake2s_final`

## Purpose

`blake2s_final()` finalizes an initialized BLAKE2s state and copies the digest into the caller buffer.

## Behavior

- Rejects `out == NULL`.
- Rejects output buffers smaller than `S->outlen`.
- Rejects repeated finalization when the state is already marked as the last block.
- Increments the byte counter with the buffered tail length.
- Marks the state as the last block, zero-pads the remaining buffer space, and compresses the final block.
- Copies digest bytes from the internal state words into the caller buffer as full 32-bit words plus an optional trailing partial word.

## Inputs and Outputs

- `S`: initialized and not-yet-finalized hash state.
- `out`: destination digest buffer.
- `outlen`: caller-provided output buffer length.
- Return value: `0` on success or `-1` when the output buffer is invalid or the state was already finalized.

## Dependencies

- Uses `blake2s_increment_counter()`, `blake2s_set_lastblock()`, `blake2s_compress()`, `blake2_memset()`, `blake2_store32()`, and `blake2_memcpy()`.

## Notes

- The implementation uses the caller's `outlen` to decide how many state bytes to emit after only checking that `outlen >= S->outlen`. Callers should therefore pass the same digest length that was selected at initialization time; larger values can expose extra state words beyond the requested digest length.
- The function does not clear the hash state after output.
