/****************************************************************************
 *
 * Copyright 2026 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Heap use-after-free guard.
 *
 * When a chunk is freed, the pages that lie entirely inside its payload are made
 * inaccessible.  A dangling pointer that reads or writes one of them then takes a
 * data abort at the offending instruction instead of silently corrupting whoever
 * gets the memory next.  The pages are handed back the moment the allocator takes
 * the chunk off the free list again.
 *
 * Only whole pages can be protected, and the first bytes of a payload hold the
 * free list links, so the guarded range starts at the first page boundary after
 * the free node header.  A payload therefore has to be about 8KB before it is
 * guaranteed to contain a full page.  Everything smaller is invisible to this
 * mechanism.
 *
 * The chunk headers at offset 0 to 15 and the free list links at offset 16 to 31
 * are never inside a guarded page.  That is what keeps the allocator and every
 * heap walker working with no changes: mm_check_heap_corruption(),
 * heapinfo_parse_heap(), mm_mallinfo() and mm_addfreechunk() only ever touch
 * those bytes of a free chunk.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <debug.h>

#include <tinyara/mm/mm.h>
#include <tinyara/mmu_pages.h>
#include <tinyara/mmu_guard.h>

#include "mm_node.h"

#ifdef MM_GUARD_ENABLED

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MM_GUARD_NSECTIONS	CONFIG_MM_GUARD_NSECTIONS
#define MM_GUARD_NPAGES		(MM_GUARD_NSECTIONS * MMU_L2_NENTRIES)
#define MM_GUARD_NRECORDS	CONFIG_MM_GUARD_NRECORDS

/* Upper bound on the pages locked for one freed chunk.  Without it, freeing the
 * multi megabyte chunk at the top of the heap would flip thousands of page table
 * entries and invalidate a TLB entry for each of them.  Zero means no bound.
 */

#if CONFIG_MM_GUARD_CHUNK_MAX_PAGES > 0
#define MM_GUARD_CHUNK_MAX_PAGES	CONFIG_MM_GUARD_CHUNK_MAX_PAGES
#else
#define MM_GUARD_CHUNK_MAX_PAGES	MM_GUARD_NPAGES
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* What is known about one guarded free, kept so that a violation can name the
 * owner of the memory instead of just its address.
 */

struct mm_guard_record_s {
	uintptr_t chunk;			/* Start of the chunk, header included */
	mmsize_t chunksize;			/* Size of the chunk */
	uintptr_t guard_start;			/* First guarded page */
	uint32_t npages;			/* Guarded pages */
	mmaddress_t alloc_call_addr;		/* Who allocated it */
	pid_t alloc_pid;
	mmaddress_t free_call_addr;		/* Who freed it */
	pid_t free_call_pid;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Translation tables for the covered sections and the per-page lock state.
 *
 * These live in .bss rather than in the reserved page table area, which has no
 * room for this many tables.  A small page translation table only has to be
 * MMU_L2_ALIGNMENT aligned, and DRAM is mapped one to one, so its virtual
 * address is also the physical address the walker needs.
 */

static uint32_t g_mm_guard_l2tbl[MMU_GUARD_L2_WORDS(MM_GUARD_NSECTIONS)]
	__attribute__((aligned(MMU_L2_ALIGNMENT)));

static uint32_t g_mm_guard_secentry[MM_GUARD_NSECTIONS];
static uint8_t g_mm_guard_lockstate[MMU_GUARD_BITMAP_BYTES(MM_GUARD_NPAGES)];

static struct mm_guard_record_s g_mm_guard_records[MM_GUARD_NRECORDS];
static uint32_t g_mm_guard_next;

static struct mmu_guard_domain_s g_mm_guard;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __mm_guard_record_find
 *
 * Description:
 *   Most recent record whose chunk contains the given address, or NULL.  Runs in
 *   data abort context, so it only reads the ring.
 *
 ****************************************************************************/

static FAR struct mm_guard_record_s *__mm_guard_record_find(uintptr_t addr)
{
	FAR struct mm_guard_record_s *rec;
	uint32_t i;
	uint32_t index;

	for (i = 0; i < MM_GUARD_NRECORDS; i++) {
		/* Walk backwards from the newest entry so that a page which has been
		 * freed more than once names the most recent owner.
		 */

		index = (g_mm_guard_next + MM_GUARD_NRECORDS - 1 - i) % MM_GUARD_NRECORDS;
		rec = &g_mm_guard_records[index];

		if (rec->chunksize != 0 && addr >= rec->chunk && addr < rec->chunk + rec->chunksize) {
			return rec;
		}
	}

	return NULL;
}

/****************************************************************************
 * Name: __mm_guard_report
 *
 * Description:
 *   Print what is known about a use-after-free.  Called from the data abort
 *   handler, so it prints with lldbg() and touches nothing that can fault.
 *
 ****************************************************************************/

static void __mm_guard_report(FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
	FAR struct mm_guard_record_s *rec;

	lldbg_noarg("\n");
	lldbg("#########################################################################\n");
	lldbg("USE AFTER FREE: %s freed heap memory at 0x%08x\n", up_mmu_fault_is_write(dfsr) ? "write to" : "read from", dfar);
	lldbg("Offending instruction : 0x%08x\n", regs[REG_PC]);

	rec = __mm_guard_record_find(dfar);
	if (rec != NULL) {
		lldbg("Chunk 0x%08x size %u, guarded 0x%08x - 0x%08x, offset %u into the chunk\n", rec->chunk, rec->chunksize, rec->guard_start, rec->guard_start + (rec->npages * MMU_PAGE_SIZE), (uint32_t)(dfar - rec->chunk));
		lldbg("Allocated by pid %d at 0x%08x\n", rec->alloc_pid, rec->alloc_call_addr);
		lldbg("Freed by pid %d at 0x%08x\n", rec->free_call_pid, rec->free_call_addr);
	} else {
		lldbg("The chunk was freed too long ago to still be on record\n");
	}

	lldbg("#########################################################################\n");
}

/****************************************************************************
 * Name: __mm_guard_classify
 *
 * Description:
 *   Every locked page of this domain sits inside a chunk that has been freed, so
 *   any access to one is a use-after-free.  The hook exists to attach the report
 *   to it.
 *
 ****************************************************************************/

static int __mm_guard_classify(FAR struct mmu_guard_domain_s *dom, FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
	__mm_guard_report(regs, dfar, dfsr);

	return MMU_GUARD_VERDICT_VIOLATION;
}

/****************************************************************************
 * Name: __mm_guard_chunk_range
 *
 * Description:
 *   Pages of a chunk that may be protected.
 *
 *   The range starts at the first page boundary at or after the end of the free
 *   node header, so that the chunk header and the free list links stay reachable,
 *   and it ends at the last page boundary inside the chunk, so that the header of
 *   the following chunk stays reachable too.
 *
 * Returned Value:
 *   Number of pages, zero when the chunk has no fully contained page.
 *
 ****************************************************************************/

static unsigned int __mm_guard_chunk_range(uintptr_t node, mmsize_t chunksize, FAR uintptr_t *start)
{
	uintptr_t first;
	uintptr_t last;

	if (chunksize <= SIZEOF_MM_FREENODE) {
		return 0;
	}

	first = MMU_PAGE_ALIGN_UP(node + SIZEOF_MM_FREENODE);
	last = MMU_PAGE_ALIGN_DOWN(node + chunksize);

	/* Clip to the guarded range.  The heap is wider than the domain: the section
	 * that holds the start of the heap and the section that holds its end are
	 * deliberately left alone.
	 */

	if (first < g_mm_guard.base) {
		first = g_mm_guard.base;
	}

	if (last > g_mm_guard.base + g_mm_guard.size) {
		last = g_mm_guard.base + g_mm_guard.size;
	}

	if (last <= first) {
		return 0;
	}

	*start = first;

	return (unsigned int)((last - first) / MMU_PAGE_SIZE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_guard_initialize
 ****************************************************************************/

void mm_guard_initialize(void)
{
	FAR struct mm_heap_s *heap;
	uintptr_t heapstart;
	uintptr_t heapend;
	uintptr_t base;
	uintptr_t end;
	int ret;

	heap = kmm_get_baseheap();
	if (heap == NULL || heap->mm_heapstart[0] == NULL || heap->mm_heapend[0] == NULL) {
		mdbg("Kernel heap is not initialized yet\n");
		return;
	}

	heapstart = (uintptr_t)heap->mm_heapstart[0];
	heapend = (uintptr_t)heap->mm_heapend[0];

	/* Guard whole sections only, and skip the first and the last section the heap
	 * touches.
	 *
	 * The section that contains the start of the heap also contains the vector
	 * table, the kernel text, data and bss, the idle stack and the heap structure
	 * itself, so it is not worth splitting for the few hundred kilobytes of heap
	 * it holds.  The section that contains the end of the heap is shared with the
	 * static RAM of the first loadable application, whose page tables would
	 * otherwise be built on top of ours.
	 */

	base = MMU_SECTION_BASE(heapstart) + MMU_SECTION_SIZE;
	end = MMU_SECTION_BASE(heapend);

	if (end <= base) {
		mdbg("Heap 0x%08x - 0x%08x spans no whole section\n", heapstart, heapend);
		return;
	}

	if ((end - base) / MMU_SECTION_SIZE > MM_GUARD_NSECTIONS) {
		/* Guard as much as the configured tables can cover and say what is left
		 * out, rather than silently protecting a part of the heap.
		 */

		mdbg("Heap needs %u sections, CONFIG_MM_GUARD_NSECTIONS is %d, 0x%08x - 0x%08x is left unguarded\n", (uint32_t)((end - base) / MMU_SECTION_SIZE), MM_GUARD_NSECTIONS, base + ((uintptr_t)MM_GUARD_NSECTIONS * MMU_SECTION_SIZE), end);
		end = base + ((uintptr_t)MM_GUARD_NSECTIONS * MMU_SECTION_SIZE);
	}

	g_mm_guard.name = "kernel heap";
	g_mm_guard.base = base;
	g_mm_guard.size = end - base;

	/* Reads of freed memory are as much of a bug as writes, so trap both */

	g_mm_guard.mode = MMU_GUARD_MODE_NA;

	g_mm_guard.classify = __mm_guard_classify;
	g_mm_guard.l2tbl = g_mm_guard_l2tbl;
	g_mm_guard.secentry = g_mm_guard_secentry;
	g_mm_guard.lockstate = g_mm_guard_lockstate;

	ret = mmu_guard_register(&g_mm_guard);
	if (ret != OK) {
		mdbg("Failed to guard the kernel heap: %d\n", ret);
		return;
	}
}

/****************************************************************************
 * Name: mm_guard_protect
 ****************************************************************************/

void mm_guard_protect(FAR void *mem, mmsize_t chunksize, mmaddress_t free_call_addr, pid_t free_call_pid)
{
	FAR struct mm_allocnode_s *node;
	FAR struct mm_guard_record_s *rec;
	uintptr_t start = 0;
	unsigned int npages;

	if (!g_mm_guard.registered || mem == NULL) {
		return;
	}

	node = (FAR struct mm_allocnode_s *)((FAR char *)mem - SIZEOF_MM_ALLOCNODE);

	npages = __mm_guard_chunk_range((uintptr_t)node, chunksize, &start);
	if (npages == 0) {
		return;
	}

	if (npages > MM_GUARD_CHUNK_MAX_PAGES) {
		npages = MM_GUARD_CHUNK_MAX_PAGES;
	}

	/* Record who owned the memory before it becomes unreadable, so the report can
	 * name them without touching the guarded pages.
	 */

	rec = &g_mm_guard_records[g_mm_guard_next];
	g_mm_guard_next = (g_mm_guard_next + 1) % MM_GUARD_NRECORDS;

	rec->chunk = (uintptr_t)node;
	rec->chunksize = chunksize;
	rec->guard_start = start;
	rec->npages = npages;
	rec->free_call_addr = free_call_addr;
	rec->free_call_pid = free_call_pid;
#ifdef CONFIG_DEBUG_MM_HEAPINFO
	rec->alloc_call_addr = node->alloc_call_addr;
	rec->alloc_pid = node->pid;
#else
	rec->alloc_call_addr = NULL;
	rec->alloc_pid = 0;
#endif

	/* Unloading a binary folds every page table entry of the kernel L1 table back
	 * into a section, which would leave this domain with no page granularity at
	 * all.  Repair the one section we are about to lock; the rest are repaired by
	 * the frees that follow.
	 */

	mmu_guard_rebind_section(&g_mm_guard, start);

	/* The chunk still holds whatever the owner last wrote, so the lines have to be
	 * cleaned before the pages lose write access.
	 */

	mmu_guard_lock(&g_mm_guard, start, npages, true);
}

/****************************************************************************
 * Name: mm_guard_unprotect
 ****************************************************************************/

void mm_guard_unprotect(FAR void *node, mmsize_t chunksize)
{
	if (!g_mm_guard.registered || g_mm_guard.nlocked == 0) {
		return;
	}

	/* Unlock by address range rather than by recomputing the range that was
	 * locked.  A chunk grows when it is merged with a free neighbour, so the range
	 * derived from it now is not the range that was protected then.
	 */

	mmu_guard_unlock_range(&g_mm_guard, (uintptr_t)node, (uintptr_t)node + chunksize);
}

/****************************************************************************
 * Name: mm_guard_unprotect_all
 ****************************************************************************/

void mm_guard_unprotect_all(void)
{
	if (!g_mm_guard.registered || g_mm_guard.nlocked == 0) {
		return;
	}

	mmu_guard_unlock_all(&g_mm_guard);
}

/****************************************************************************
 * Name: mm_guard_dump
 ****************************************************************************/

void mm_guard_dump(void)
{
	mmu_guard_domain_dump(&g_mm_guard);
}

#endif							/* MM_GUARD_ENABLED */
