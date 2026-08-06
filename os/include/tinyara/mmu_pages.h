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

///@file tinyara/mmu_pages.h
///@brief Architecture interface for re-permissioning individual MMU pages.

#ifndef __INCLUDE_TINYARA_MMU_PAGES_H
#define __INCLUDE_TINYARA_MMU_PAGES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef CONFIG_MMU_GUARD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Geometry of the translation tables this interface manipulates.  The values
 * are checked against the architecture headers where the interface is
 * implemented, so a port that disagrees fails to build instead of computing
 * wrong table indices.
 */

#define MMU_PAGE_SIZE			(4096)
#define MMU_PAGE_MASK			(MMU_PAGE_SIZE - 1)

#define MMU_PAGE_ALIGN_DOWN(a)	((uintptr_t)(a) & ~(uintptr_t)MMU_PAGE_MASK)
#define MMU_PAGE_ALIGN_UP(a)	MMU_PAGE_ALIGN_DOWN((uintptr_t)(a) + MMU_PAGE_MASK)
#define MMU_PAGE_IS_ALIGNED(a)	(((uintptr_t)(a) & (uintptr_t)MMU_PAGE_MASK) == 0)

/* One section descriptor covers this many pages, and one small page translation
 * table has one entry per page of the section it replaces.
 */

#define MMU_L2_NENTRIES			(256)
#define MMU_SECTION_SIZE		(MMU_L2_NENTRIES * MMU_PAGE_SIZE)
#define MMU_SECTION_MASK		(MMU_SECTION_SIZE - 1)
#define MMU_SECTION_BASE(a)		((uintptr_t)(a) & ~(uintptr_t)MMU_SECTION_MASK)

/* Alignment that a small page translation table has to satisfy */

#define MMU_L2_ALIGNMENT		(1024)

/* Access levels a page can be given.
 *
 * MMU_PAGE_ACCESS_RW   - Normal privileged read/write, the state a page is in
 *   before it is ever guarded.
 * MMU_PAGE_ACCESS_RO   - Privileged read-only, so only writes are trapped.
 *   Diagnostic code that only reads the memory keeps working.
 * MMU_PAGE_ACCESS_NONE - Inaccessible at every privilege level, so reads are
 *   trapped as well.  Any dump or print of the page then faults too.
 */

#define MMU_PAGE_ACCESS_RW		(0)
#define MMU_PAGE_ACCESS_RO		(1)
#define MMU_PAGE_ACCESS_NONE	(2)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: up_mmu_section_entry
 *
 * Description:
 *   Read the kernel L1 descriptor that currently maps the given address.  The
 *   kernel table is addressed explicitly, so the result does not depend on
 *   which translation table the running task happens to use.
 *
 ****************************************************************************/

uint32_t up_mmu_section_entry(uintptr_t vaddr);

/****************************************************************************
 * Name: up_mmu_section_is_block
 *
 * Description:
 *   Report whether a descriptor returned by up_mmu_section_entry() is a plain
 *   section mapping.  A section that is already split belongs to somebody else
 *   and must not be taken over.
 *
 ****************************************************************************/

bool up_mmu_section_is_block(uint32_t secentry);

/****************************************************************************
 * Name: up_mmu_l2_init
 *
 * Description:
 *   Fill a small page translation table so that it reproduces the mapping of
 *   the section containing vaddr, one 4KB page at a time, with the normal
 *   privileged read/write attributes of kernel memory.
 *
 * Input Parameters:
 *   l2tbl - Table of MMU_L2_NENTRIES entries, MMU_L2_ALIGNMENT aligned.
 *   vaddr - Any address inside the section to be reproduced.
 *
 ****************************************************************************/

void up_mmu_l2_init(uint32_t *l2tbl, uintptr_t vaddr);

/****************************************************************************
 * Name: up_mmu_l2_set_access
 *
 * Description:
 *   Change the access level of one page in a translation table without any
 *   cache or TLB maintenance.  Use it to prepare a table that is not live yet,
 *   so that a range of pages can be published in one go by up_mmu_l2_flush()
 *   and up_mmu_section_split().
 *
 ****************************************************************************/

void up_mmu_l2_set_access(uint32_t *l2tbl, uintptr_t vaddr, int access);

/****************************************************************************
 * Name: up_mmu_l2_flush
 *
 * Description:
 *   Push a whole translation table out to memory so the page table walker sees
 *   it.
 *
 ****************************************************************************/

void up_mmu_l2_flush(const uint32_t *l2tbl);

/****************************************************************************
 * Name: up_mmu_page_set_access
 *
 * Description:
 *   Change the access level of one live page and make the change effective on
 *   every core before returning.
 *
 ****************************************************************************/

void up_mmu_page_set_access(uint32_t *l2tbl, uintptr_t vaddr, int access);

/****************************************************************************
 * Name: up_mmu_section_split
 *
 * Description:
 *   Redirect the section descriptor that maps vaddr to the given small page
 *   translation table, in every L1 translation table of the system.
 *
 *   The operation is idempotent, so it is also the repair path for code that
 *   folds page table entries back into sections behind our back.
 *
 * Input Parameters:
 *   vaddr    - Any address inside the section to split.
 *   l2tbl    - The translation table to install.
 *   secentry - The original descriptor, as returned by up_mmu_section_entry().
 *     Its domain and non-secure attribute are carried over.
 *
 * Returned Value:
 *   true if a descriptor had to be rewritten.
 *
 ****************************************************************************/

bool up_mmu_section_split(uintptr_t vaddr, uint32_t *l2tbl, uint32_t secentry);

/****************************************************************************
 * Name: up_mmu_tlb_invalidate_page
 *
 * Description:
 *   Drop the TLB entry of one page on every core of the shareability domain.
 *
 ****************************************************************************/

void up_mmu_tlb_invalidate_page(uintptr_t vaddr);

/****************************************************************************
 * Name: up_mmu_tlb_invalidate_section
 *
 * Description:
 *   Drop the TLB entries of a whole section, both the section entry itself and
 *   every small page entry that may have replaced it.
 *
 ****************************************************************************/

void up_mmu_tlb_invalidate_section(uintptr_t vaddr);

/****************************************************************************
 * Name: up_mmu_clean_dcache_range
 *
 * Description:
 *   Clean a range of memory out of the data cache.  A line that is dirty when a
 *   page becomes inaccessible would be written back later without going through
 *   a permission check, so the range has to be pushed out before it is locked.
 *
 ****************************************************************************/

void up_mmu_clean_dcache_range(uintptr_t start, uintptr_t end);

/****************************************************************************
 * Name: up_mmu_fault_is_permission
 *
 * Description:
 *   Report whether a fault status register value describes a permission fault.
 *   Only permission faults can belong to a guarded page; retrying anything else
 *   would abort again forever.
 *
 ****************************************************************************/

bool up_mmu_fault_is_permission(uint32_t fsr);

/****************************************************************************
 * Name: up_mmu_fault_is_write
 *
 * Description:
 *   Report whether the access that faulted was a store.
 *
 ****************************************************************************/

bool up_mmu_fault_is_write(uint32_t fsr);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* CONFIG_MMU_GUARD */
#endif							/* __INCLUDE_TINYARA_MMU_PAGES_H */
