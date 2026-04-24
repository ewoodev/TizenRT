# `up_rngreseed`

## Purpose

`up_rngreseed()` is the public helper that asks the random generator to reseed itself from the current entropy pool state.

## Behavior

- Takes `g_rng.rd_sem` before touching generator state.
- Retries interrupted `sem_wait()` calls until the wait succeeds, asserting that intermediate failures are `EINTR`.
- Calls `rng_reseed()` only when `g_rng.rd_newentr` has reached `MIN_SEED_NEW_ENTROPY_WORDS`.
- Releases the semaphore and returns with no status value.

## Inputs and Outputs

- No inputs.
- Return value: none.

## Dependencies

- Uses `sem_wait()` / `sem_post()` for serialization.
- Uses `rng_reseed()` for the actual reseed operation.

## Notes

- Despite its name, this helper does not force an unconditional reseed. It is a threshold-checked reseed request.
- When the entropy threshold has not been met yet, the function becomes a synchronized no-op.
