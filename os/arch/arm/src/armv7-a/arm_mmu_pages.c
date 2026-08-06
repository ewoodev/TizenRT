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
 * Small page re-permissioning primitives.
 *
 * The MMU maps kernel memory with 1MB section descriptors, which is too coarse
 * to take a single page away from a caller.  The primitives here replace one
 * section descriptor with a small page translation table that reproduces the
 * original mapping, and then change the access permissions of individual 4KB
 * pages inside it.
 *
 * This file knows about page table descriptors and cache maintenance and
 * nothing else.  What a guarded page means, and who is allowed to touch it, is
 * decided one layer up by the MMU guard core.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <tinyara/mmu_pages.h>

#include "cp15_cacheops.h"
#include "mmu.h"

#ifdef CONFIG_MMU_GUARD

/* No access and privileged read-only are AP[2:0] == 0b000 and AP[2:0] == 0b101,
 * which only exist while SCTLR.AFE is cleared.  PTE_AP_NONE is not even defined
 * in the access flag model, so fail early with a readable message.
 */

#ifdef CONFIG_AFE_ENABLE
#error "CONFIG_MMU_GUARD requires the legacy AP[2:0] model (SCTLR.AFE cleared)"
#endif

/* The geometry published to the callers has to match what this port actually
 * implements, otherwise a caller would compute the wrong table index.
 */

#if MMU_PAGE_SIZE != SMALL_PAGE_SIZE
#error "MMU_PAGE_SIZE does not match the small page size of this architecture"
#endif

#if MMU_SECTION_SIZE != SECTION_SIZE
#error "MMU_SECTION_SIZE does not match the section size of this architecture"
#endif

#if MMU_L2_NENTRIES != L2_PGTBL_NENTRIES
#error "MMU_L2_NENTRIES does not match the L2 translation table of this architecture"
#endif

#if MMU_L2_ALIGNMENT != L2_PGTBL_ALIGNMENT
#error "MMU_L2_ALIGNMENT does not match the L2 translation table of this architecture"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of L1 translation tables that have to be kept in sync.
 *
 * With CONFIG_APP_BINARY_SEPARATION every loadable application owns an L1 table
 * that is a plain copy of the kernel one, and up_restoretask() installs it in
 * TTBR0 while the application runs.  A section descriptor therefore has to be
 * redirected in all of them.  The permission flips afterwards only touch the
 * shared L2 table, so they are visible through every L1 table for free.
 */

#ifdef CONFIG_APP_BINARY_SEPARATION
#define MMU_PAGES_NL1TABLES	(CONFIG_NUM_APPS + 1)
#else
#define MMU_PAGES_NL1TABLES	(1)
#endif

/* The kernel L1 table always sits at the base of the page table area.  It is
 * addressed explicitly rather than through mmu_l1_pgtable(), which returns
 * whatever TTBR0 currently points at and may be an application copy.
 */

#define MMU_PAGES_L1TABLE(n)	((uint32_t *)(PGTABLE_BASE_VADDR + ((n) * L1_PGTBL_SIZE)))

/* Index of a virtual address inside an L1 and an L2 translation table */

#define MMU_PAGES_L1_INDEX(v)	((uint32_t)(v) >> SECTION_SHIFT)
#define MMU_PAGES_L2_INDEX(v)	(((uint32_t)(v) & 0x000ff000) >> 12)

/* All three access permission bits of a small page descriptor */

#define MMU_PAGES_AP_MASK	(PTE_AP_MASK | PTE_AP2)

/* Fault status of a short descriptor abort is FS == {FSR[10], FSR[3:0]} */

#define MMU_PAGES_FSR_STATUS(f)	((((f) & DFSR_FS) != 0 ? 0x10 : 0x00) | ((f) & DFSR_STATUS_MASK))

#define MMU_PAGES_FS_PERM_SECT	(0x0d)
#define MMU_PAGES_FS_PERM_PAGE	(0x0f)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __mmu_pages_ap
 *
 * Description:
 *   Translate a generic access level into the AP bits of a small page
 *   descriptor.
 *
 ****************************************************************************/

static uint32_t __mmu_pages_ap(int access)
{
	switch (access) {
	case MMU_PAGE_ACCESS_NONE:
		return PTE_AP_NONE;

	case MMU_PAGE_ACCESS_RO:
		return PTE_AP_R1;

	default:
		return PTE_AP_RW1;
	}
}

/****************************************************************************
 * Name: __mmu_pages_l1entry
 *
 * Description:
 *   Build the L1 descriptor that points at the given small page translation
 *   table.  The domain and the non-secure attribute are taken over from the
 *   section descriptor being replaced so that the memory keeps its original
 *   identity.
 *
 ****************************************************************************/

static uint32_t __mmu_pages_l1entry(const uint32_t *l2tbl, uint32_t secentry)
{
	uint32_t entry;

	entry = ((uint32_t)l2tbl & PMD_PTE_PADDR_MASK) | PMD_TYPE_PTE;

	/* Section and page table descriptors carry the domain in the same bits */

	entry |= (secentry & PMD_SECT_DOM_MASK);

	if ((secentry & PMD_SECT_NS) != 0) {
		entry |= PMD_PTE_NS;
	}

	return entry;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_mmu_tlb_invalidate_page
 ****************************************************************************/

void up_mmu_tlb_invalidate_page(uintptr_t vaddr)
{
	/* The inner shareable variant is used on SMP because
	 * cp15_invalidate_tlb_bymva() only broadcasts when CONFIG_ARM_HAVE_MPCORE is
	 * set, and CONFIG_ARCH_CORTEXA32 does not select it.  Without a broadcast the
	 * second core would keep a stale descriptor and either miss a violation or
	 * fault on a page that is already accessible.  An inner shareable operation
	 * degrades to a local one when the core is outside the shareability domain,
	 * so it is never worse than the plain variant.
	 */

	__asm__ __volatile__(
		"\tdsb\n"
#ifdef CONFIG_SMP
		"\tmcr p15, 0, %0, c8, c3, 3\n"		/* TLBIMVAAIS */
#else
		"\tmcr p15, 0, %0, c8, c7, 3\n"		/* TLBIMVAA */
#endif
		"\tdsb\n"
		"\tisb\n"
		:
		: "r" (vaddr)
		: "memory"
	);
}

/****************************************************************************
 * Name: up_mmu_tlb_invalidate_section
 ****************************************************************************/

void up_mmu_tlb_invalidate_section(uintptr_t vaddr)
{
	uintptr_t sectbase = vaddr & ~(uintptr_t)(SECTION_SIZE - 1);
	int i;

	/* A section leaves the TLB as one 1MB entry and comes back as up to 256
	 * small page entries, so the whole range has to be invalidated.
	 */

	for (i = 0; i < L2_PGTBL_NENTRIES; i++) {
		up_mmu_tlb_invalidate_page(sectbase + ((uintptr_t)i * SMALL_PAGE_SIZE));
	}
}

/****************************************************************************
 * Name: up_mmu_section_entry
 ****************************************************************************/

uint32_t up_mmu_section_entry(uintptr_t vaddr)
{
	return MMU_PAGES_L1TABLE(0)[MMU_PAGES_L1_INDEX(vaddr)];
}

/****************************************************************************
 * Name: up_mmu_section_is_block
 ****************************************************************************/

bool up_mmu_section_is_block(uint32_t secentry)
{
	return ((secentry & PMD_TYPE_MASK) == PMD_TYPE_SECT);
}

/****************************************************************************
 * Name: up_mmu_l2_init
 ****************************************************************************/

void up_mmu_l2_init(uint32_t *l2tbl, uintptr_t vaddr)
{
	uintptr_t sectbase = vaddr & PMD_SECT_PADDR_MASK;
	int i;

	/* MMU_L2_MEMFLAGS reproduces the attributes of MMU_MEMFLAGS exactly:
	 * write-back cacheable, shareable when SMP is enabled, AP[2:0] == 0b001 and
	 * executable.  DRAM is mapped one to one, so the virtual address of the
	 * section is also its physical base.
	 */

	for (i = 0; i < L2_PGTBL_NENTRIES; i++) {
		l2tbl[i] = (sectbase + ((uintptr_t)i * SMALL_PAGE_SIZE)) | MMU_L2_MEMFLAGS;
	}
}

/****************************************************************************
 * Name: up_mmu_l2_set_access
 ****************************************************************************/

void up_mmu_l2_set_access(uint32_t *l2tbl, uintptr_t vaddr, int access)
{
	uint32_t index = MMU_PAGES_L2_INDEX(vaddr);
	uint32_t entry = l2tbl[index];

	entry &= ~MMU_PAGES_AP_MASK;
	entry |= __mmu_pages_ap(access);

	l2tbl[index] = entry;
}

/****************************************************************************
 * Name: up_mmu_l2_flush
 ****************************************************************************/

void up_mmu_l2_flush(const uint32_t *l2tbl)
{
	cp15_clean_dcache((uintptr_t)l2tbl, (uintptr_t)l2tbl + (L2_PGTBL_NENTRIES * sizeof(uint32_t)));
}

/****************************************************************************
 * Name: up_mmu_page_set_access
 ****************************************************************************/

void up_mmu_page_set_access(uint32_t *l2tbl, uintptr_t vaddr, int access)
{
	uint32_t index = MMU_PAGES_L2_INDEX(vaddr);

	up_mmu_l2_set_access(l2tbl, vaddr, access);

	/* Make the new descriptor visible to the page table walker and to every core
	 * before the caller assumes the permission has changed.
	 */

	cp15_clean_dcache_bymva((uint32_t)&l2tbl[index]);
	up_mmu_tlb_invalidate_page(vaddr);
}

/****************************************************************************
 * Name: up_mmu_section_split
 ****************************************************************************/

bool up_mmu_section_split(uintptr_t vaddr, uint32_t *l2tbl, uint32_t secentry)
{
	uint32_t index = MMU_PAGES_L1_INDEX(vaddr);
	uint32_t *l1table;
	uint32_t entry;
	bool changed = false;
	int i;

	entry = __mmu_pages_l1entry(l2tbl, secentry);

	for (i = 0; i < MMU_PAGES_NL1TABLES; i++) {
		l1table = MMU_PAGES_L1TABLE(i);

		if (l1table[index] == entry) {
			continue;
		}

		l1table[index] = entry;
		cp15_clean_dcache_bymva((uint32_t)&l1table[index]);
		changed = true;
	}

	if (!changed) {
		return false;
	}

	up_mmu_tlb_invalidate_section(vaddr);

	return true;
}

/****************************************************************************
 * Name: up_mmu_clean_dcache_range
 ****************************************************************************/

void up_mmu_clean_dcache_range(uintptr_t start, uintptr_t end)
{
	cp15_clean_dcache(start, end);
}

/****************************************************************************
 * Name: up_mmu_fault_is_permission
 ****************************************************************************/

bool up_mmu_fault_is_permission(uint32_t fsr)
{
	uint32_t status = MMU_PAGES_FSR_STATUS(fsr);

	return (status == MMU_PAGES_FS_PERM_PAGE || status == MMU_PAGES_FS_PERM_SECT);
}

/****************************************************************************
 * Name: up_mmu_fault_is_write
 ****************************************************************************/

bool up_mmu_fault_is_write(uint32_t fsr)
{
	return ((fsr & DFSR_WNR) != 0);
}

#endif							/* CONFIG_MMU_GUARD */
