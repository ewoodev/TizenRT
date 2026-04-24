# `up_rngaddint`

## Purpose

`up_rngaddint()` is the one-word entropy submission wrapper for the global random pool.

## Behavior

- Packages the caller's `int` value into a one-element `uint32_t` buffer.
- Forwards the source type and that single sample to `up_rngaddentropy(type, buf, 1)`.
- Does not add locking, throttling, or reseed logic of its own.

## Inputs and Outputs

- `type`: entropy source classification.
- `val`: one integer sample to mix into the pool.
- Return value: none.

## Dependencies

- Depends entirely on `up_rngaddentropy()` for mixing policy, IRQ de-duplication, time mixing, and entropy-counter updates.

## Notes

- Negative `int` values are passed through the implicit integer-to-`uint32_t` conversion used when the temporary buffer is populated.
