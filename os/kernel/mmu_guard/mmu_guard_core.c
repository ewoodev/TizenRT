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
 * MMU guard core.
 *
 * Keeps a small set of guarded address ranges, called domains, and owns the
 * per-page protection state of each one.  It knows how to take a page away from
 * its callers and how to hand it back, and it dispatches the data aborts that
 * result to the client that registered the range.
 *
 * What a locked page means is not decided here.  The client answers that through
 * the classify hook, which runs in abort context and can check the faulting
 * address against whatever metadata it keeps.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/irq.h>
#include <tinyara/mmu_pages.h>
#include <tinyara/mmu_guard.h>

#include "sched/sched.h"

#ifdef CONFIG_MMU_GUARD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* How many times in a row the same address may be written off as a stale TLB
 * entry.  Beyond that the diagnosis is clearly wrong and the abort has to reach
 * the normal crash path instead of being retried forever.
 */

#define MMU_GUARD_MAX_STALE_RETRY	(4)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct mmu_guard_domain_s *g_domains[MMU_GUARD_MAX_DOMAINS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __mmu_guard_testbit / __mmu_guard_setbit / __mmu_guard_clrbit
 ****************************************************************************/

static inline bool __mmu_guard_testbit(FAR const uint8_t *map, uint32_t index)
{
	return ((map[index >> 3] & (1 << (index & 7))) != 0);
}

static inline void __mmu_guard_setbit(FAR uint8_t *map, uint32_t index)
{
	map[index >> 3] |= (uint8_t)(1 << (index & 7));
}

static inline void __mmu_guard_clrbit(FAR uint8_t *map, uint32_t index)
{
	map[index >> 3] &= (uint8_t)~(1 << (index & 7));
}

/****************************************************************************
 * Name: __mmu_guard_contains
 ****************************************************************************/

static inline bool __mmu_guard_contains(FAR const struct mmu_guard_domain_s *dom, uintptr_t vaddr)
{
	return (vaddr >= dom->base && vaddr < dom->base + dom->size);
}

/****************************************************************************
 * Name: __mmu_guard_page_index
 ****************************************************************************/

static inline uint32_t __mmu_guard_page_index(FAR const struct mmu_guard_domain_s *dom, uintptr_t vaddr)
{
	return (uint32_t)((MMU_PAGE_ALIGN_DOWN(vaddr) - dom->base) / MMU_PAGE_SIZE);
}

/****************************************************************************
 * Name: __mmu_guard_section_index
 *
 * Description:
 *   Index of the section containing vaddr, counted from the section that
 *   contains the domain base.  The base itself need not be section aligned.
 *
 ****************************************************************************/

static inline uint32_t __mmu_guard_section_index(FAR const struct mmu_guard_domain_s *dom, uintptr_t vaddr)
{
	return (uint32_t)((MMU_SECTION_BASE(vaddr) - MMU_SECTION_BASE(dom->base)) / MMU_SECTION_SIZE);
}

/****************************************************************************
 * Name: __mmu_guard_l2tbl
 *
 * Description:
 *   Translation table that maps the section containing vaddr.
 *
 ****************************************************************************/

static inline FAR uint32_t *__mmu_guard_l2tbl(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr)
{
	return &dom->l2tbl[__mmu_guard_section_index(dom, vaddr) * MMU_L2_NENTRIES];
}

/****************************************************************************
 * Name: __mmu_guard_locked_access
 *
 * Description:
 *   Access level a locked page of this domain gets.
 *
 ****************************************************************************/

static inline int __mmu_guard_locked_access(FAR const struct mmu_guard_domain_s *dom)
{
	return (dom->mode == MMU_GUARD_MODE_NA) ? MMU_PAGE_ACCESS_NONE : MMU_PAGE_ACCESS_RO;
}

/****************************************************************************
 * Name: __mmu_guard_apply
 *
 * Description:
 *   Bring one page into the requested state.  Returns true when the descriptor
 *   actually had to change.
 *
 ****************************************************************************/

static bool __mmu_guard_apply(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr, bool lock)
{
	uint32_t index = __mmu_guard_page_index(dom, vaddr);

	if (__mmu_guard_testbit(dom->lockstate, index) == lock) {
		return false;
	}

	up_mmu_page_set_access(__mmu_guard_l2tbl(dom, vaddr), vaddr, lock ? __mmu_guard_locked_access(dom) : MMU_PAGE_ACCESS_RW);

	if (lock) {
		__mmu_guard_setbit(dom->lockstate, index);
		dom->nlocked++;
	} else {
		__mmu_guard_clrbit(dom->lockstate, index);
		dom->nlocked--;
	}

	return true;
}

/****************************************************************************
 * Name: __mmu_guard_find
 *
 * Description:
 *   Domain that owns the given address, or NULL.
 *
 ****************************************************************************/

static FAR struct mmu_guard_domain_s *__mmu_guard_find(uintptr_t vaddr)
{
	FAR struct mmu_guard_domain_s *dom;
	int i;

	for (i = 0; i < MMU_GUARD_MAX_DOMAINS; i++) {
		dom = g_domains[i];

		if (dom != NULL && dom->registered && __mmu_guard_contains(dom, vaddr)) {
			return dom;
		}
	}

	return NULL;
}

/****************************************************************************
 * Name: __mmu_guard_range_check
 ****************************************************************************/

static int __mmu_guard_range_check(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr, unsigned int npages)
{
	if (dom == NULL || !dom->registered) {
		return -ENOENT;
	}

	if (!MMU_PAGE_IS_ALIGNED(vaddr)) {
		return -EINVAL;
	}

	if (npages == 0) {
		return OK;
	}

	if (!__mmu_guard_contains(dom, vaddr) || !__mmu_guard_contains(dom, vaddr + ((uintptr_t)npages * MMU_PAGE_SIZE) - 1)) {
		return -EFAULT;
	}

	return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mmu_guard_register
 ****************************************************************************/

int mmu_guard_register(FAR struct mmu_guard_domain_s *dom)
{
	irqstate_t flags;
	uintptr_t first;
	uintptr_t sectbase;
	uint32_t entry;
	int slot = -1;
	int i;

	if (dom == NULL || dom->l2tbl == NULL || dom->secentry == NULL || dom->lockstate == NULL) {
		return -EINVAL;
	}

	if (dom->size == 0 || !MMU_PAGE_IS_ALIGNED(dom->base) || !MMU_PAGE_IS_ALIGNED(dom->size)) {
		dbg("Range 0x%08x/%u is not page aligned\n", dom->base, dom->size);
		return -EINVAL;
	}

	if (dom->mode != MMU_GUARD_MODE_RO && dom->mode != MMU_GUARD_MODE_NA) {
		return -EINVAL;
	}

	/* The page table walker would keep working, but privileged code has to stay
	 * able to update the descriptors while pages of the range are locked.
	 */

	if ((uintptr_t)dom->l2tbl >= dom->base && (uintptr_t)dom->l2tbl < dom->base + dom->size) {
		dbg("Translation tables 0x%08x lie inside the range\n", (uint32_t)dom->l2tbl);
		return -EFAULT;
	}

	if (((uintptr_t)dom->l2tbl & (MMU_L2_ALIGNMENT - 1)) != 0) {
		dbg("Translation tables 0x%08x are not %u byte aligned\n", (uint32_t)dom->l2tbl, MMU_L2_ALIGNMENT);
		return -EINVAL;
	}

	first = MMU_SECTION_BASE(dom->base);

	dom->npages = (uint32_t)(dom->size / MMU_PAGE_SIZE);
	dom->nsections = (uint16_t)(((MMU_SECTION_BASE(dom->base + dom->size - 1) - first) / MMU_SECTION_SIZE) + 1);

	flags = enter_critical_section();

	if (dom->registered) {
		leave_critical_section(flags);
		return -EBUSY;
	}

	for (i = 0; i < MMU_GUARD_MAX_DOMAINS; i++) {
		if (g_domains[i] == NULL && slot < 0) {
			slot = i;
		}
	}

	if (slot < 0) {
		leave_critical_section(flags);
		dbg("No free domain slot, CONFIG_MMU_GUARD_MAX_DOMAINS is %d\n", MMU_GUARD_MAX_DOMAINS);
		return -ENOSPC;
	}

	/* Snapshot the descriptors before anything is rewritten, so that a rejected
	 * registration leaves no trace.
	 */

	for (i = 0; i < dom->nsections; i++) {
		sectbase = first + ((uintptr_t)i * MMU_SECTION_SIZE);
		entry = up_mmu_section_entry(sectbase);

		if (!up_mmu_section_is_block(entry)) {
			/* Somebody else already split this section, most likely the
			 * application loader.  Hijacking that table would corrupt their
			 * mapping.
			 */

			leave_critical_section(flags);
			dbg("Section at 0x%08x is not a plain section (L1 entry 0x%08x)\n", sectbase, entry);
			return -EEXIST;
		}

		dom->secentry[i] = entry;
	}

	dom->nlocked = 0;
	dom->stale_addr = 0;
	dom->stale_retry = 0;

	memset(dom->lockstate, 0, MMU_GUARD_BITMAP_BYTES(dom->npages));

	/* Mirror each section into small pages that reproduce it exactly, then
	 * publish the tables.  The range stays fully accessible: the client decides
	 * which pages to take away and when.
	 */

	for (i = 0; i < dom->nsections; i++) {
		up_mmu_l2_init(&dom->l2tbl[i * MMU_L2_NENTRIES], first + ((uintptr_t)i * MMU_SECTION_SIZE));
		up_mmu_l2_flush(&dom->l2tbl[i * MMU_L2_NENTRIES]);
	}

	for (i = 0; i < dom->nsections; i++) {
		sectbase = first + ((uintptr_t)i * MMU_SECTION_SIZE);
		up_mmu_section_split(sectbase, &dom->l2tbl[i * MMU_L2_NENTRIES], dom->secentry[i]);
	}

	g_domains[slot] = dom;
	dom->registered = true;

	leave_critical_section(flags);

	lldbg("Guarding %s 0x%08x - 0x%08x (%u page%s in %u section%s, %s)\n", dom->name != NULL ? dom->name : "range", dom->base, dom->base + dom->size, dom->npages, dom->npages == 1 ? "" : "s", dom->nsections, dom->nsections == 1 ? "" : "s", dom->mode == MMU_GUARD_MODE_NA ? "no access" : "read-only");

	return OK;
}

/****************************************************************************
 * Name: mmu_guard_lock
 ****************************************************************************/

int mmu_guard_lock(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr, unsigned int npages, bool flush)
{
	irqstate_t flags;
	unsigned int i;
	int ret;

	ret = __mmu_guard_range_check(dom, vaddr, npages);
	if (ret != OK || npages == 0) {
		return ret;
	}

	flags = enter_critical_section();

	/* Clean the whole run out in one pass before any page loses write access.
	 * Cache maintenance by MVA is translated and permission checked, so it has to
	 * happen while the pages are still accessible.
	 */

	if (flush) {
		up_mmu_clean_dcache_range(vaddr, vaddr + ((uintptr_t)npages * MMU_PAGE_SIZE));
	}

	for (i = 0; i < npages; i++) {
		__mmu_guard_apply(dom, vaddr + ((uintptr_t)i * MMU_PAGE_SIZE), true);
	}

	leave_critical_section(flags);

	return OK;
}

/****************************************************************************
 * Name: mmu_guard_unlock
 ****************************************************************************/

int mmu_guard_unlock(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr, unsigned int npages)
{
	irqstate_t flags;
	unsigned int i;
	int ret;

	ret = __mmu_guard_range_check(dom, vaddr, npages);
	if (ret != OK || npages == 0) {
		return ret;
	}

	flags = enter_critical_section();

	for (i = 0; i < npages; i++) {
		__mmu_guard_apply(dom, vaddr + ((uintptr_t)i * MMU_PAGE_SIZE), false);
	}

	leave_critical_section(flags);

	return OK;
}

/****************************************************************************
 * Name: mmu_guard_unlock_range
 ****************************************************************************/

int mmu_guard_unlock_range(FAR struct mmu_guard_domain_s *dom, uintptr_t start, uintptr_t end)
{
	irqstate_t flags;
	uintptr_t limit;
	uint32_t first;
	uint32_t last;
	uint32_t index;

	if (dom == NULL || !dom->registered) {
		return -ENOENT;
	}

	/* Nothing is locked, so there is nothing to look for.  This is the common case
	 * on the allocation path and it has to stay free of a bitmap walk.
	 */

	if (dom->nlocked == 0) {
		return OK;
	}

	limit = dom->base + dom->size;

	if (start < dom->base) {
		start = dom->base;
	}

	if (end > limit) {
		end = limit;
	}

	if (end <= start) {
		return OK;
	}

	first = __mmu_guard_page_index(dom, start);
	last = __mmu_guard_page_index(dom, end - 1);

	flags = enter_critical_section();

	for (index = first; index <= last && dom->nlocked > 0; index++) {
		/* Skip whole bytes of the bitmap at a time.  A caller that hands over a
		 * multi megabyte chunk must not pay for a test per page.
		 */

		if ((index & 7) == 0) {
			while (dom->lockstate[index >> 3] == 0 && index + 7 <= last) {
				index += 8;
			}
		}

		if (__mmu_guard_testbit(dom->lockstate, index)) {
			__mmu_guard_apply(dom, dom->base + ((uintptr_t)index * MMU_PAGE_SIZE), false);
		}
	}

	leave_critical_section(flags);

	return OK;
}

/****************************************************************************
 * Name: mmu_guard_unlock_all
 ****************************************************************************/

int mmu_guard_unlock_all(FAR struct mmu_guard_domain_s *dom)
{
	if (dom == NULL || !dom->registered) {
		return -ENOENT;
	}

	return mmu_guard_unlock(dom, dom->base, dom->npages);
}

/****************************************************************************
 * Name: mmu_guard_rebind_section
 ****************************************************************************/

bool mmu_guard_rebind_section(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr)
{
	uint32_t index;

	if (dom == NULL || !dom->registered || !__mmu_guard_contains(dom, vaddr)) {
		return false;
	}

	index = __mmu_guard_section_index(dom, vaddr);

	if (!up_mmu_section_split(MMU_SECTION_BASE(vaddr), &dom->l2tbl[index * MMU_L2_NENTRIES], dom->secentry[index])) {
		return false;
	}

	lldbg("Guard binding for %s section 0x%08x was repaired\n", dom->name != NULL ? dom->name : "range", MMU_SECTION_BASE(vaddr));

	return true;
}

/****************************************************************************
 * Name: mmu_guard_record
 ****************************************************************************/

void mmu_guard_record(FAR struct mmu_guard_domain_s *dom, FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
	FAR struct mmu_guard_record_s *rec;
	FAR struct tcb_s *tcb;

	dom->stats.nviolations++;

	if (dom->stats.nrecords >= MMU_GUARD_MAX_RECORDS) {
		return;
	}

	rec = &dom->stats.record[dom->stats.nrecords++];

	rec->dfar = dfar;
	rec->dfsr = dfsr;
	rec->pc = regs[REG_PC];
	rec->lr = regs[REG_LR];
	rec->write = up_mmu_fault_is_write(dfsr);
	rec->cpu = (uint8_t)this_cpu();

	tcb = this_task();
	if (tcb != NULL) {
		rec->pid = tcb->pid;
#if CONFIG_TASK_NAME_SIZE > 0
		strncpy(rec->name, tcb->name, MMU_GUARD_NAME_SIZE - 1);
		rec->name[MMU_GUARD_NAME_SIZE - 1] = '\0';
#endif
	}
}

/****************************************************************************
 * Name: mmu_guard_domain_dump
 ****************************************************************************/

void mmu_guard_domain_dump(FAR struct mmu_guard_domain_s *dom)
{
	FAR struct mmu_guard_record_s *rec;
	uint32_t i;

	if (dom == NULL) {
		return;
	}

	lldbg("MMU guard: %s %s 0x%08x/%u, %u of %u page(s) locked, mode %s\n", dom->name != NULL ? dom->name : "range", dom->registered ? "guarded" : "idle", dom->base, dom->size, dom->nlocked, dom->npages, dom->mode == MMU_GUARD_MODE_NA ? "NA" : "RO");
	lldbg("MMU guard: %u violation(s), %u stale TLB hit(s)\n", dom->stats.nviolations, dom->stats.nstale);

	for (i = 0; i < dom->stats.nrecords; i++) {
		rec = &dom->stats.record[i];
		lldbg("  [%u] %s addr 0x%08x from pc 0x%08x lr 0x%08x dfsr 0x%08x cpu %u pid %d %s\n", i, rec->write ? "WRITE" : "READ ", rec->dfar, rec->pc, rec->lr, rec->dfsr, rec->cpu, rec->pid, rec->name);
	}

	if (dom->stats.nviolations > dom->stats.nrecords) {
		lldbg("  ... %u more violation(s) not recorded\n", dom->stats.nviolations - dom->stats.nrecords);
	}
}

/****************************************************************************
 * Name: mmu_guard_dataabort
 ****************************************************************************/

bool mmu_guard_dataabort(FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
	FAR struct mmu_guard_domain_s *dom;
	uintptr_t page_base;
	uint32_t index;
	int verdict;

	dom = __mmu_guard_find(dfar);
	if (dom == NULL) {
		return false;
	}

	/* Only permission faults belong to the guard.  Anything else that happens to
	 * land in the range, an external abort for instance, has to reach the normal
	 * crash path: retrying it would abort again forever.
	 */

	if (!up_mmu_fault_is_permission(dfsr)) {
		return false;
	}

	index = __mmu_guard_page_index(dom, dfar);
	page_base = MMU_PAGE_ALIGN_DOWN(dfar);

	if (!__mmu_guard_testbit(dom->lockstate, index)) {
		/* The descriptor grants the access, so this core faulted on a TLB entry
		 * that predates the last permission change.  Drop it and retry.
		 */

		if (page_base == dom->stale_addr && dom->stale_retry >= MMU_GUARD_MAX_STALE_RETRY) {
			lldbg("MMU guard: 0x%08x keeps faulting on an open page, giving up\n", dfar);
			return false;
		}

		if (page_base == dom->stale_addr) {
			dom->stale_retry++;
		} else {
			dom->stale_addr = page_base;
			dom->stale_retry = 1;
		}

		dom->stats.nstale++;
		up_mmu_tlb_invalidate_page(page_base);

		return true;
	}

	dom->stale_addr = 0;
	dom->stale_retry = 0;

	/* Ask the client what this access means.  Without a classifier every access
	 * to a locked page is a violation.
	 */

	verdict = MMU_GUARD_VERDICT_VIOLATION;
	if (dom->classify != NULL) {
		verdict = dom->classify(dom, regs, dfar, dfsr);
	}

	if (verdict == MMU_GUARD_VERDICT_VIOLATION) {
		mmu_guard_record(dom, regs, dfar, dfsr);

		lldbg("MMU GUARD VIOLATION: %s 0x%08x from pc 0x%08x (dfsr 0x%08x)\n", up_mmu_fault_is_write(dfsr) ? "write to" : "read from", dfar, regs[REG_PC], dfsr);
	}

	if (verdict != MMU_GUARD_VERDICT_ALLOW) {
		/* Let the architecture handler run its normal course so that the full
		 * register and stack dump identifies the offender.
		 */

		return false;
	}

	/* The client vouched for this access.  Open the page and return so that the
	 * faulting instruction succeeds on retry.
	 */

	__mmu_guard_apply(dom, page_base, false);

	return true;
}

#endif							/* CONFIG_MMU_GUARD */
