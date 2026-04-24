# `mm_pginitialize`

## Summary

`mm_pginitialize()` initializes the global physical page allocator over a caller-supplied address range.

## Behavior

- Configures the page allocator as a thin wrapper over the granule allocator with both granule size and alignment fixed to one page.
- Under `CONFIG_GRAN_SINGLE`, calls `gran_initialize()` and expects an `OK` status.
- Without `CONFIG_GRAN_SINGLE`, stores the returned `GRAN_HANDLE` in the module-global `g_pgalloc`.
- Uses `DEBUGASSERT()` instead of runtime error handling for initialization failure.

## Inputs and Outputs

- `heap_start`: physical start address of the page pool.
- `heap_size`: size of the page pool in bytes.
- Return value: none.

## Dependencies

- Depends on `gran_initialize()` from the same folder.
- Uses `MM_PGSHIFT` from [`tinyara/pgalloc.h`](../../include/tinyara/pgalloc.h) to match one granule to one page.

## Notes

- The wrapper does not align the inputs itself; it relies on the underlying granule allocator initialization path.
- Initialization failure becomes a debug assertion instead of a recoverable public error.
