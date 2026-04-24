# `mm_pgreserve`

## Summary

`mm_pgreserve()` reserves a page-backed region in the global page allocator before normal allocation traffic begins.

## Behavior

- Forwards the request directly to `gran_reserve()`.
- Under `CONFIG_GRAN_SINGLE`, uses the single global granule allocator instance.
- Otherwise uses the stored global page-allocator handle `g_pgalloc`.
- Inherits outward rounding to page boundaries from the granule allocator.

## Inputs and Outputs

- `start`: start address of the region to reserve.
- `size`: size of the region to reserve in bytes.
- Return value: none.

## Dependencies

- Depends on `gran_reserve()` from the same folder.
- Shares the global page-allocator state created by [`mm_pginitialize.md`](mm_pginitialize.md).

## Notes

- Like `gran_reserve()`, this wrapper is intended for early boot-time carve-outs before concurrent allocation begins.
- Reserved pages can later be returned through `mm_pgfree()`, which effectively unreserves them.
