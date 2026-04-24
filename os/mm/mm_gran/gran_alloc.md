# `gran_alloc`

## Purpose

`gran_alloc()` finds the first contiguous free run of granules large enough for the request and returns the corresponding heap address.

## Behavior

- Uses only `DEBUGASSERT()` to enforce a valid allocator handle and the documented maximum request size of 32 granules.
- Returns `NULL` immediately when `size == 0` or the allocator handle is missing.
- Rounds `size` up to the next whole granule.
- Enters the allocator critical section before scanning and updating the bitmap.
- Scans the granule allocation table in 32-bit chunks and uses a first-fit search for the earliest span whose bits are all clear.
- Marks the chosen span as allocated with `gran_mark_allocated()`.
- Leaves the critical section before returning either the allocation address or failure.
- Returns `NULL` when no contiguous span of the requested size is available.

## Inputs and Outputs

- `CONFIG_GRAN_SINGLE=y`: `size`.
- `!CONFIG_GRAN_SINGLE`: `handle` and `size`.
- Return value: pointer to the allocated region on success, or `NULL` on failure.

## Dependencies

- Uses `gran_enter_critical()` and `gran_leave_critical()` for mutual exclusion.
- Uses `gran_mark_allocated()` to set the allocation bitmap.
- Allocations are returned to the same allocator with `gran_free.md`.

## Notes

- The 32-granule maximum is a hard caller contract. Release builds do not reject larger requests safely.
- Returned addresses advance in granule-sized steps from the aligned heap start chosen by `gran_initialize.md`.
