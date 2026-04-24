# `blake2s`

## Purpose

`blake2s()` is the one-shot helper that hashes one input buffer into one output buffer, optionally with a key.

## Behavior

- Validates `out`, `outlen`, and `key`/`keylen` before touching hash state.
- Rejects `in == NULL` only when `inlen > 0`.
- Rejects `out == NULL`.
- Rejects `outlen == 0` or `outlen > BLAKE2S_OUTBYTES`.
- Rejects `keylen > BLAKE2S_KEYBYTES`.
- Uses `blake2s_init_key()` when `keylen > 0`; otherwise uses `blake2s_init()`.
- Calls `blake2s_update()` with the caller input and then `blake2s_final()` into the caller output buffer.

## Inputs and Outputs

- `out`: destination digest buffer.
- `outlen`: requested digest length in bytes.
- `in`: input buffer.
- `inlen`: input length in bytes.
- `key`: optional key buffer.
- `keylen`: key length in bytes.
- Return value: `0` on success or `-1` on rejected parameters or initialization failure.

## Dependencies

- Depends on `blake2s_init()` or `blake2s_init_key()`.
- Depends on `blake2s_update()` and `blake2s_final()` for the streaming steps.

## Notes

- The wrapper does not set `errno`.
- In debug builds, `in == NULL` with `inlen == 0` still reaches `blake2s_update()` and can trip its `DEBUGASSERT(pin && S)` even though the top-level parameter checks allow that combination.
