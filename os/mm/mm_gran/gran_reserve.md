# `gran_reserve`

## Purpose

`gran_reserve()` permanently marks an address range as already occupied until the same span is later released with `gran_free()`.

## Behavior

- Treats `size == 0` as a no-op.
- Rounds `start` down to the containing granule boundary.
- Builds an inclusive end address from `start + size - 1`, rounds that end up to the next granule boundary, and reserves every granule in the span.
- Calls `gran_mark_allocated()` directly instead of taking the allocator exclusion lock.
- In `CONFIG_GRAN_SINGLE=y`, operates on the global allocator instance.
- In the multi-instance configuration, operates on the caller-supplied `GRAN_HANDLE`.

## Inputs and Outputs

- `CONFIG_GRAN_SINGLE=y`: `start` and `size`.
- `!CONFIG_GRAN_SINGLE`: `handle`, `start`, and `size`.
- Return value: none.

## Dependencies

- Uses `gran_mark_allocated()` to set the granule allocation bitmap.
- Depends on allocator state created by `gran_initialize.md`.
- Reserved spans can be released again through `gran_free.md`.

## Notes

- This API is intended for early carve-out use before normal concurrent allocations begin. The implementation does not serialize against `gran_alloc()` or `gran_free()`.
- Overlap and bounds checking rely on debug assertions in `gran_mark_allocated()`. Release builds do not report reservation conflicts.
