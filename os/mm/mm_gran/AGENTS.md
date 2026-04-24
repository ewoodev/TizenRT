# `os/mm/mm_gran` Module Guide

## Purpose

`os/mm/mm_gran` implements two related public allocator families. The primary API declared in `os/include/tinyara/mm/gran.h` exposes a caller-owned granule allocator. The secondary API declared in `os/include/tinyara/pgalloc.h` layers a global physical page allocator on top of that same bitmap allocator with page-sized granules.

## Public APIs Covered in This Folder

- `gran_initialize()`
- `gran_release()`
- `gran_reserve()`
- `gran_alloc()`
- `gran_free()`
- `mm_pginitialize()`
- `mm_pgreserve()`
- `mm_pgalloc()`
- `mm_pgfree()`

Function-level behavior is documented beside the implementation sources in this folder.

## Build and Configuration

- `os/mm/Kconfig` exposes `CONFIG_GRAN`, which enables this allocator family.
- `os/mm/Kconfig` exposes `CONFIG_GRAN_SINGLE`, which collapses the API to one global allocator instance and changes `gran_initialize()` to return `int` instead of `GRAN_HANDLE`.
- `os/mm/Kconfig` exposes `CONFIG_GRAN_INTR`, which swaps semaphore-based exclusion for interrupt masking so the allocator can run from interrupt context.
- `os/mm/Kconfig` exposes `CONFIG_DEBUG_GRAN`, which controls allocator-specific debug logging.
- `os/mm/Kconfig` exposes `CONFIG_MM_PGALLOC`, which layers the page allocator in `mm_pgalloc.c` on top of this module and selects `CONFIG_GRAN`.
- `os/mm/Kconfig` exposes `CONFIG_MM_PGSIZE` and `CONFIG_DEBUG_PGALLOC` for that page-allocation wrapper.
- `os/mm/mm_gran/Make.defs` builds the core allocator files only when `CONFIG_GRAN=y`, and adds `mm_pgalloc.c` only when `CONFIG_MM_PGALLOC=y`.

## Internal Model

1. `gran_initialize()` allocates a `struct gran_s` plus the granule allocation table in kernel memory and aligns the payload heap region supplied by the caller.
2. The payload heap itself is not consumed for metadata; the allocator only tracks addresses inside the aligned caller-owned range.
3. `gran_reserve()` marks boot-time carve-outs in the same bitmap used for live allocations.
4. `gran_alloc()` performs a first-fit scan over 32-bit bitmap words and marks the first suitable contiguous run of granules as allocated.
5. `gran_free()` clears bitmap bits again by recomputing the granule count from the caller-provided size.
6. `mm_pginitialize()` configures a single global granule allocator instance whose granule size and alignment are both one page.
7. `mm_pgreserve()`, `mm_pgalloc()`, and `mm_pgfree()` reuse the same bitmap through the page-size wrapper APIs and therefore inherit the granule allocator's rounding and caller-size contracts.
8. `gran_release()` destroys the exclusion primitive and frees the metadata block.

## Behavioral Constraints

- The public API relies heavily on `DEBUGASSERT()` instead of runtime error handling. Invalid handles, oversized requests, and mismatched frees are caller bugs in release builds.
- Maximum allocation size is 32 granules.
- `gran_free()` is not `free()`-like: callers must supply the size again, and that size must round to the same number of granules as the original allocation or reservation.
- `mm_pgalloc()` inherits the same maximum contiguous run limit, so the page allocator cannot allocate more than the equivalent 32 page-sized granules in one request.
- `mm_pgfree()` is not a standalone page-table operation; it is only the page-sized wrapper over `gran_free()` and therefore depends on the caller supplying the original contiguous page count again.
- `gran_reserve()` does not take the allocator lock. It is intended for early initialization before normal concurrent allocation traffic begins.
- `mm_pgreserve()` inherits that same early-init expectation because it is only a page-sized wrapper over `gran_reserve()`.
- `gran_release()` does not verify that the heap is fully freed before tearing down metadata.
- Alignment and granularity are coupled: allocations advance in units of `1 << log2gran`, and `log2align` must not exceed `log2gran`.

## Dependencies

- Kernel heap services from `tinyara/kmalloc.h` for allocator metadata.
- Semaphore or interrupt critical-section support depending on `CONFIG_GRAN_INTR`.
- Optional page allocator integration in `mm_pgalloc.c`.

## Scope Boundaries

- This folder owns the public granule allocator APIs declared in `tinyara/mm/gran.h` and the page allocator wrapper APIs declared in `tinyara/pgalloc.h`.
- Internal helpers such as `gran_enter_critical()`, `gran_leave_critical()`, and `gran_mark_allocated()` are private implementation details.
- `mm_pgalloc.c` consumes this allocator to implement page allocation, but the wrapper still lives in this same folder and shares its bitmap model.

## Maintenance Notes

- Keep config-specific public comments synchronized with the actual prototype changes under `CONFIG_GRAN_SINGLE`.
- Preserve the explicit `size` contract in `gran_free()` whenever the interface is touched.
- Keep `tinyara/pgalloc.h` synchronized with the real wrapper semantics, especially the inherited max-run limit, page-sized outward rounding in `mm_pgreserve()`, and the fact that `mm_pgfree()` needs the original contiguous page count.
- If the bitmap layout or search strategy changes, update both the function-level `.md` files and this guide together.
