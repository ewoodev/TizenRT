# `mm_pgfree`

## Summary

`mm_pgfree()` returns one contiguous run of physical pages to the global page allocator.

## Behavior

- Converts `npages` into a byte count with `npages << MM_PGSHIFT`.
- Forwards that byte count to `gran_free()`.
- Under `CONFIG_GRAN_SINGLE`, uses the single global granule allocator instance.
- Otherwise uses the stored global page-allocator handle `g_pgalloc`.

## Inputs and Outputs

- `paddr`: start physical address of the run to free.
- `npages`: number of contiguous pages in that run.
- Return value: none.

## Dependencies

- Depends on `gran_free()` from the same folder.
- Shares the global page-allocator state created by [`mm_pginitialize.md`](mm_pginitialize.md).

## Notes

- The caller must supply the same contiguous page count that was originally allocated or reserved.
- This wrapper does not validate that `paddr` and `npages` describe a currently allocated run except through the underlying granule allocator's debug checks.
