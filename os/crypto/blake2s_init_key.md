# `blake2s_init_key`

## Purpose

`blake2s_init_key()` prepares a keyed sequential BLAKE2s state and preloads the key as the padded first input block.

## Behavior

- Rejects `outlen == 0` or `outlen > BLAKE2S_OUTBYTES`.
- Rejects `key == NULL`, `keylen == 0`, or `keylen > BLAKE2S_KEYBYTES`.
- Builds the default sequential parameter block used by `blake2s_init()`, but sets `key_length` to the requested key size.
- Delegates state initialization to `blake2s_init_param()`.
- Propagates a delegated initialization failure, including the optional one-time self-test failure path behind `CONFIG_BLAKE2_SELFTEST`.
- Clears a 64-byte local block, copies the caller key into the front of that block, and feeds the full padded block through `blake2s_update()`.
- Burns the temporary key block from the stack with `secure_zero_memory()` before returning.

## Inputs and Outputs

- `S`: hash state to initialize.
- `outlen`: requested digest length in bytes.
- `key`: caller-provided key buffer.
- `keylen`: key length in bytes.
- Return value: `0` on success or `-1` when the digest length, key arguments, or delegated initialization are invalid.

## Dependencies

- Depends on `blake2s_init_param.md` for the underlying state initialization.
- Depends on `blake2s_update.md` to absorb the padded key block.
- Uses `secure_zero_memory()` to scrub the temporary stack copy of the key material.

## Notes

- The keyed initialization path always absorbs a full `BLAKE2S_BLOCKBYTES` block with zero padding after the key bytes. It does not hash only `keylen` bytes.
- The function itself does not add runtime validation for `S`; the underlying initialization path relies on debug assertions for pointer checks.
