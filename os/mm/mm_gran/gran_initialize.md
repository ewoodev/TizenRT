# `gran_initialize`

## Purpose

`gran_initialize()` creates one granule allocator instance that tracks a caller-supplied heap with a bitmap stored in separately allocated kernel memory.

## Behavior

- Uses only `DEBUGASSERT()` to validate `heapstart`, `heapsize`, `log2gran`, and `log2align`.
- Rounds `heapstart` up to the requested alignment boundary.
- Rounds the usable heap size down to a whole number of granules.
- Stores allocator metadata outside the managed heap by allocating `struct gran_s` plus the granule allocation table with `kmm_zalloc()`.
- Records the aligned heap start and the computed granule count in the new allocator state.
- Initializes a semaphore for allocator exclusion unless `CONFIG_GRAN_INTR` switches the implementation to interrupt masking.
- In `CONFIG_GRAN_SINGLE=y`, installs the allocator in the global `g_graninfo` slot and returns `OK` or `-ENOMEM`.
- In the multi-instance configuration, returns the newly allocated `GRAN_HANDLE` or `NULL`.

## Inputs and Outputs

- `heapstart`: first byte of the caller-managed heap.
- `heapsize`: size of that heap in bytes.
- `log2gran`: granule size exponent.
- `log2align`: minimum alignment exponent, which must not exceed `log2gran`.
- Return value:
- `CONFIG_GRAN_SINGLE=y`: `OK` on success or `-ENOMEM` on metadata-allocation failure.
- `!CONFIG_GRAN_SINGLE`: non-NULL handle on success or `NULL` on failure.

## Dependencies

- Uses `kmm_zalloc()` for metadata allocation.
- Uses `sem_init()` when `CONFIG_GRAN_INTR` is disabled.
- Feeds `gran_reserve()`, `gran_alloc()`, `gran_free()`, and `gran_release()`.

## Notes

- The function does not reject a heap that rounds down to zero usable granules. That case can still initialize successfully, but later allocations will fail.
- The managed heap is not consumed for metadata. Only the payload region beginning at the aligned heap start is tracked.
- Release builds rely on the caller to satisfy the parameter constraints that are only checked by `DEBUGASSERT()`.
