# `gran_free`

## Purpose

`gran_free()` clears bitmap entries for one previously allocated or reserved granule span.

## Behavior

- Uses only `DEBUGASSERT()` to validate the allocator pointer, `memory`, and the documented 32-granule size limit.
- Enters the allocator critical section before touching the bitmap.
- Computes the starting granule index from `memory - heapstart`.
- Recomputes the granule count by rounding `size` up the same way `gran_alloc()` does.
- Clears one or two granule-allocation-table words depending on whether the released span crosses a 32-bit bitmap-word boundary.
- Leaves the critical section after updating the bitmap.
- Performs no runtime check that the supplied `memory` and `size` pair exactly matches a live allocation in release builds.

## Inputs and Outputs

- `CONFIG_GRAN_SINGLE=y`: `memory` and `size`.
- `!CONFIG_GRAN_SINGLE`: `handle`, `memory`, and `size`.
- Return value: none.

## Dependencies

- Uses `gran_enter_critical()` and `gran_leave_critical()` for mutual exclusion.
- Frees spans originally created by `gran_alloc.md`.
- Also acts as the inverse of `gran_reserve.md` because reserved granules are tracked in the same bitmap.

## Notes

- This API depends on the caller supplying the original start address and a size that rounds to the same granule count as the allocation or reservation being released.
- A mismatched pointer or size can corrupt allocator state outside debug builds because there is no runtime recovery path.
