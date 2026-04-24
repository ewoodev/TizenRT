# `getrandom`

## Purpose

`getrandom` returns cryptographically strong random bytes from the global random pool without going through a device file interface.

## Behavior

- The function is available only when `CONFIG_CRYPTO_RANDOM_POOL` is enabled.
- It takes the random-pool semaphore before touching generator state.
- It fills the caller-provided buffer by calling `rng_buf_internal(bytes, nbytes)`.
- It releases the semaphore before returning.

## Inputs and Outputs

- `bytes`: destination buffer that receives random output.
- `nbytes`: number of bytes to generate.
- Return value: none.

## Concurrency and Failure Model

- Calls are serialized with `g_rng.rd_sem`.
- Interrupted semaphore waits are retried until they succeed.
- The implementation does not expose a recoverable error path; failures are handled with assertions inside the random-pool code.

## Dependencies

- `up_randompool_initialize()` must have initialized the random-pool state before callers rely on this API.
- Output quality depends on entropy collection and reseeding handled by the rest of `random_pool.c`.
