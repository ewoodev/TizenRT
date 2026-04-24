# `blake2s_init_param`

## Purpose

`blake2s_init_param()` initializes a BLAKE2s state directly from a caller-supplied parameter block by XORing that block into the IV-derived state.

## Behavior

- When `CONFIG_BLAKE2_SELFTEST` is enabled, runs the built-in self-test only on the first call in the process and returns `-1` if that self-test fails.
- Uses only `DEBUGASSERT(S && P)` for pointer validation.
- Calls `blake2s_init0()` to zero the working state fields and reload the fixed IV words.
- Treats the parameter block as eight little-endian 32-bit words and XORs those words into `S->h[0..7]`.
- Copies `P->digest_length` into `S->outlen`.
- Returns `0` whenever the optional self-test path succeeds.

## Inputs and Outputs

- `S`: hash state to initialize.
- `P`: caller-supplied BLAKE2s parameter block.
- Return value: `0` on success or `-1` only when the optional self-test fails.

## Dependencies

- Uses `blake2s_init0()` to reset the state.
- Uses `blake2_load32()` to consume the parameter block words.
- May call `blake2s_selftest()` on the first invocation when `CONFIG_BLAKE2_SELFTEST` is enabled.

## Notes

- The function does not validate the contents of the parameter block at runtime. Digest length, key length, tree parameters, salt, and personalization bytes are trusted as provided.
- When `CONFIG_BLAKE2_SELFTEST` is enabled, the one-time self-test latch is set before the test runs. A first-call self-test failure therefore returns `-1` once and is not retried by later calls.
- Because the parameter block is consumed as raw bytes, this entry point is narrower and lower-level than `blake2s_init.md` and `blake2s_init_key.md`.
