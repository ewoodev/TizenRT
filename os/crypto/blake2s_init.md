# `blake2s_init`

## Purpose

`blake2s_init()` prepares a `blake2s_state` for an unkeyed sequential hash with the caller-selected digest length.

## Behavior

- Rejects `outlen == 0` or `outlen > BLAKE2S_OUTBYTES`.
- Builds a default parameter block for the standard sequential BLAKE2s mode:
  - requested digest length
  - no key
  - `fanout = 1`
  - `depth = 1`
  - zero `leaf_length`, `node_offset`, `xof_length`, `salt`, and `personal`
- Delegates actual state initialization to `blake2s_init_param()`.

## Inputs and Outputs

- `S`: hash state to initialize.
- `outlen`: requested digest length in bytes.
- Return value: `0` on success or `-1` when the digest length is invalid or delegated initialization fails.

## Dependencies

- Depends on `blake2s_init_param()`.
- Uses `blake2_store32()`, `blake2_store16()`, and `blake2_memset()` while building the parameter block.

## Notes

- When `CONFIG_BLAKE2_SELFTEST` is enabled, the first delegated `blake2s_init_param()` call also triggers the one-time self-test path.
- The function does not validate `S` at runtime before delegating; `blake2s_init_param()` contains the relevant debug assertion.
