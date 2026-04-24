# `up_randompool_initialize`

## Purpose

`up_randompool_initialize()` initializes the RNG state used with the global entropy pool.

## Behavior

- Calls the private `rng_init()` helper.
- `rng_init()` clears the `g_rng` structure and initializes `g_rng.rd_sem` as a binary semaphore.
- Does not perform a reseed.
- Does not emit random output.
- Does not explicitly overwrite `entropy_pool`.

## Inputs and Outputs

- No inputs.
- Return value: none.

## Dependencies

- Depends on `rng_init()`.
- `rng_init()` depends on `memset()` and `sem_init()`.

## Notes

- The public header currently exposes this as the initialization entry for the random-pool subsystem, but the implementation only resets RNG-side state and synchronization.
- Pool contents still depend on static initialization and later entropy submissions through `up_rngaddint()` / `up_rngaddentropy()`.
