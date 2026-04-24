# `os/crypto` Module Guide

## Purpose

`os/crypto` currently serves two distinct roles:

- a kernel-owned entropy pool and strong random-byte generator built around `random_pool.c`
- a reusable BLAKE2s hash implementation in `blake2s.c`

The random-pool path uses BLAKE2s/BLAKE2Xs internally, but the hash implementation is also a standalone public API for callers that need direct hashing.

## Public APIs

### Random-pool APIs

- `getrandom()`: fill a caller buffer with bytes from the global random generator
- `up_rngaddint()`: add one entropy sample to the pool
- `up_rngaddentropy()`: add a buffer of entropy samples to the pool
- `up_rngreseed()`: attempt a reseed after taking the RNG lock, but only when enough new entropy has accumulated
- `up_randompool_initialize()`: initialize the RNG state and semaphore
- `add_irq_randomness()`, `add_sensor_randomness()`, `add_time_randomness()`, `add_hw_randomness()`, `add_sw_randomness()`, `add_ui_randomness()`: convenience macros over `up_rngaddint()`

Function-level notes currently exist for:

- `getrandom.md`
- `up_rngaddint.md`
- `up_rngaddentropy.md`
- `up_rngreseed.md`
- `up_randompool_initialize.md`

### BLAKE2s APIs

- `blake2s_init()`
- `blake2s_init_key()`
- `blake2s_init_param()`
- `blake2s_update()`
- `blake2s_final()`
- `blake2s()`

Function-level notes live beside `blake2s.c` for all six public entry points.

## Build and Configuration

- `os/Kconfig` exposes this folder through the `Crypto Module` menu.
- `CONFIG_CRYPTO` enables the module menu.
- `CONFIG_CRYPTO_RANDOM_POOL` builds `random_pool.c` and also selects `CONFIG_CRYPTO_BLAKE2S`.
- `CONFIG_CRYPTO_BLAKE2S` builds `blake2s.c` when the random-pool path is not already pulling it in.
- `CONFIG_SCHED_CPULOAD` enables extra CPU-load mixing in `getentropy()`, and that path iterates per CPU with `CONFIG_SMP_NCPUS`.
- `CONFIG_BOARD_ENTROPY_POOL` only exposes a board-level `board_entropy_pool` declaration in the public header. The current `random_pool.c` implementation still uses its own static `entropy_pool`.

## Internal Flow

1. `up_randompool_initialize()` zeroes the RNG state and initializes the semaphore.
2. `up_rngaddint()` and `up_rngaddentropy()` mix external samples into `entropy_pool`.
3. IRQ and time-based samples are rate-limited before they increase the "new entropy" counter.
4. `getrandom()` serializes access with `g_rng.rd_sem` and calls `rng_buf_internal()`.
5. `rng_buf_internal()` reseeds through `rng_reseed()` when output has not been initialized yet, when enough new entropy has accumulated, or when the BLAKE2Xs output counter wraps.
6. `rng_reseed()` hashes the entropy pool through `getentropy()`, folds in the previous root, and prepares a new BLAKE2Xs output state.
7. `blake2s.c` provides the low-level hash primitives used by the RNG and by direct public hash callers.

## Behavioral Constraints

- `getrandom()` has no normal error return. Interrupted semaphore waits are retried and unexpected failures are handled with assertions.
- `up_rngreseed()` only reseeds when `rd_newentr` has reached the minimum threshold; otherwise it simply returns after taking and releasing the semaphore.
- The random-pool code does not preserve source-specific accounting beyond the limited IRQ/time throttling logic.
- The `add_*_randomness()` macros remain available even when `CONFIG_CRYPTO_RANDOM_POOL` is disabled, but they collapse to no-op macros in that configuration.
- `blake2s_init()` and `blake2s_init_key()` reject invalid digest lengths, and `blake2s_init_key()` also rejects missing or oversized keys.
- `blake2s_final()` rejects a too-small output buffer and rejects repeated finalization on the same state.

## Dependencies

- `sem_wait()` / `sem_post()` serialize random-byte generation and explicit reseeds.
- `clock_gettime()` contributes time-derived mixing data in `up_rngaddentropy()`.
- `clock_cpuload()` contributes extra extraction noise when CPU-load accounting is enabled.
- `explicit_bzero()` and the BLAKE2s helpers are used to clear temporary key material and internal state.
- `kmm_malloc()` / `kmm_free()` are used by the optional BLAKE2 self-test path.

## Maintenance Notes

- Keep the RNG and hash responsibilities separate in documentation and code review. `random_pool.c` owns entropy policy and reseeding; `blake2s.c` owns the generic hash implementation.
- Any change to `rd_newentr`, IRQ de-duplication, or time-based throttling changes reseed frequency and should be reflected in both header comments and Markdown.
- `getrandom()` depends on prior initialization. If initialization order changes, update both the function-level note and this module guide.
- `CONFIG_CRYPTO_RANDOM_POOL` indirectly depends on BLAKE2s behavior because the RNG output path is built on top of `blake2s.c`.
- `blake2s.c` also contains a source-only `CONFIG_BLAKE2_SELFTEST` guard around a one-time self-test path in `blake2s_init_param()`. If that guard or its call site changes, document the new startup cost and failure behavior.
