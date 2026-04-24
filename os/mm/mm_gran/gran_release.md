# `gran_release`

## Purpose

`gran_release()` destroys one granule allocator instance and frees only the allocator metadata allocated during `gran_initialize()`.

## Behavior

- Destroys the internal exclusion semaphore when `CONFIG_GRAN_INTR` is disabled.
- Frees the allocator state structure with `kmm_free()`.
- In `CONFIG_GRAN_SINGLE=y`, clears the global `g_graninfo` pointer after releasing the allocator.
- Does not inspect or reclaim outstanding allocations in the managed heap.
- Does not free, clear, or otherwise return ownership of the caller-managed heap buffer itself.

## Inputs and Outputs

- `CONFIG_GRAN_SINGLE=y`: no input parameter.
- `!CONFIG_GRAN_SINGLE`: `handle` is the `GRAN_HANDLE` returned by `gran_initialize()`.
- Return value: none.

## Dependencies

- Uses `sem_destroy()` when semaphore-based exclusion is enabled.
- Uses `kmm_free()` to release the metadata block.
- Completes the lifecycle started by `gran_initialize.md`.

## Notes

- Pointer validation is debug-only through `DEBUGASSERT()`.
- The function is a metadata teardown primitive, not a safe leak detector or heap cleanup pass.
