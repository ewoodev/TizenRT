apps/examples/mm_guard
======================

Front end for the heap use-after-free guard, CONFIG_MM_GUARD_FREED_PAGES.

What the guard does
-------------------

When a kernel heap chunk is freed, the pages that lie entirely inside its payload
are made inaccessible in the MMU.  A read or write through a dangling pointer
then takes a data abort at the offending instruction instead of silently
corrupting whichever chunk gets the memory next.  The pages come back the moment
the allocator takes the chunk off the free list again.

Only whole 4KB pages can be protected, and the first bytes of a payload hold the
free list links, so the guarded range starts at the first page boundary after the
free node header.  A payload therefore has to be about 8KB before it is
guaranteed to contain a full page.  Smaller allocations are not covered.

Usage
-----

  TASH> mm_guard [uaf|dump]

  uaf   Allocate 64KB from the kernel heap, free it, then read 8KB into the
        payload.  Those numbers are chosen so that the payload contains whole
        pages whatever its alignment and the offset lands inside the first of
        them.  This is the default.

  dump  Print the guard state and the violations recorded so far.

Expected result of "uaf"
------------------------

It crashes the board.  That is the pass condition: the guard is configured to
panic on a violation, so the run should end with a report like

  #####################################################################
  USE AFTER FREE: read from freed heap memory at 0x60xxxxxx
  Offending instruction : 0x0exxxxxx
  Chunk 0x60xxxxxx size 65552, guarded 0x60xxx000 - 0x60xxx000, offset 8208 ...
  Allocated by pid 3 at 0x0exxxxxx
  Freed by pid 3 at 0x0exxxxxx
  #####################################################################

followed by the normal data abort dump.  After the reset,
READ_REBOOT_REASON() should report REBOOT_SYSTEM_DATAABORT (51).

If the task returns and prints "The use-after-free was NOT trapped", the access
went through unnoticed and the guard is not doing its job.

Configuration
-------------

  CONFIG_MMU_GUARD=y                 shared page re-permissioning machinery
  CONFIG_MM_GUARD_FREED_PAGES=y      the heap client
  CONFIG_MM_GUARD_NSECTIONS          1MB sections of the heap to cover
  CONFIG_MM_GUARD_CHUNK_MAX_PAGES    pages protected per freed chunk
  CONFIG_MM_GUARD_NRECORDS           freed chunks kept on record for the report
  CONFIG_EXAMPLES_MM_GUARD=y         this front end

The guard needs CONFIG_DEBUG_MM_FREEINFO for the free call site and pid, and it
is a debug facility: it cleans the data cache and rewrites page table entries on
every free of a large chunk.  Do not enable it in production builds.
