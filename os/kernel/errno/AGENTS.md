# `os/kernel/errno` Module Guide

## Purpose

`os/kernel/errno` implements the function-backed errno access path used when direct errno access is not available or when code needs the underlying accessor implementations.

## Public APIs

- `get_errno_ptr()`: select the active errno storage for the current context
- `set_errno()`: write an errno value through the selected storage pointer
- `get_errno()`: read the current errno value through the selected storage pointer

Function-level API notes live beside the implementation sources:

- `get_errno_ptr.md`
- `set_errno.md`
- `get_errno.md`

## Build and Header Ownership

- The owner header for these APIs is `os/include/errno.h`.
- `os/include/debug.h` and `os/include/assert.h` carry secondary declarations for `get_errno()`, but they do not own the API contract.
- `set_errno()` and `get_errno()` are declared only when `__DIRECT_ERRNO_ACCESS` is not enabled.
- There is no dedicated Kconfig entry for this folder; behavior is selected by build mode and errno-access macros in `errno.h`.

## Internal Model

- `get_errno_ptr()` returns the running task's `pterrno` field when normal task context is available.
- Interrupt context, early boot, or transient scheduler states use the fallback `g_irqerrno` slot instead.
- `set_errno()` and `get_errno()` are thin wrappers around that pointer selection logic.

## Maintenance Notes

- Any change to `get_errno_ptr()` affects all function-based errno access paths.
- The fallback interrupt errno slot is shared. If nested-interrupt-safe errno tracking is ever required, this module is where the design must change.
- Keep `errno.h` comments aligned with actual build-mode behavior, especially around `__DIRECT_ERRNO_ACCESS`.
