# `mm_pgalloc`

## Summary

`mm_pgalloc()` allocates one contiguous run of physical pages from the global page allocator.

## Behavior

- Converts `npages` into a byte count with `npages << MM_PGSHIFT`.
- Forwards that byte count to `gran_alloc()`.
- Under `CONFIG_GRAN_SINGLE`, uses the single global granule allocator instance.
- Otherwise uses the stored global page-allocator handle `g_pgalloc`.
- Returns the start address of the allocated run, or `0` when the underlying allocator fails.

## Inputs and Outputs

- `npages`: number of contiguous pages to allocate.
- Return value: start physical address of the allocated run on success, or `0` on failure.

## Dependencies

- Depends on `gran_alloc()` from the same folder.
- Shares the global page-allocator state created by [`mm_pginitialize.md`](mm_pginitialize.md).

## Notes

- The public header describes physical page allocation, but the implementation is only a page-sized view over the underlying granule allocator.
- The wrapper inherits the granule allocator's maximum contiguous-run limit.
- This tranche fixes a real bug in the wrapper: the old implementation always requested one page regardless of `npages`.
