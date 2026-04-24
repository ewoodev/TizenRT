# `timer_getoverrun`

## Summary

`timer_getoverrun()` is declared in the public header, but the current implementation is only a stub.

## Behavior

- Does not inspect `timerid`.
- Sets `errno` to `ENOSYS`.
- Returns `ERROR` for every call.

## Inputs and Outputs

- `timerid`: ignored by the current implementation.
- Return value: always `ERROR` with `errno` set to `ENOSYS`.

## Notes

- No overrun count is tracked anywhere in `os/kernel/timer`.
- Callers cannot use this API to distinguish valid and invalid timer handles in the current build.
