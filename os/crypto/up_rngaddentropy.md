# `up_rngaddentropy`

## Purpose

`up_rngaddentropy()` mixes externally supplied entropy samples into the global pool and updates the "new entropy" counter used by later reseeds.

## Behavior

- Accepts a source type plus a caller buffer of `uint32_t` samples.
- For IRQ sources, drops the call immediately when the first sample matches the previous IRQ sample.
- Always mixes in additional local noise derived from `clock_gettime(CLOCK_REALTIME)`, the source type, and a stack address.
- Limits "new entropy" counting for `RND_SRC_TIME` and `RND_SRC_IRQ` to at most 8 Hz by comparing the current time bucket against `g_rng.rd_prev_time`.
- XORs the first caller sample into the locally generated seed word when the caller supplied at least one sample.
- Mixes the combined first word through `addentropy(tbuf, 1, new_inc)`.
- Mixes the remaining caller samples, if any, through `addentropy(buf, n, new_inc)`.

## Inputs and Outputs

- `type`: entropy source classification.
- `buf`: caller-provided samples to mix.
- `n`: number of `uint32_t` elements in `buf`.
- Return value: none.

## Dependencies

- Uses `clock_gettime()` for time-derived mixing.
- Uses `addentropy()` for the actual pool update and entropy-counter increment.

## Notes

- The function does not track separate entropy budgets per source type. The only source-specific policy is the IRQ duplicate filter and the 8 Hz rate limit for IRQ/time sources.
- The implementation does not take the RNG read semaphore. Entropy producers can call this path without serializing against `getrandom()`, and the function relies on the shared global state layout already used in `random_pool.c`.
