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

///@file tinyara/mmu_guard.h
///@brief Take pages of kernel memory away from their callers and report the
///       accesses that come anyway.

#ifndef __INCLUDE_TINYARA_MMU_GUARD_H
#define __INCLUDE_TINYARA_MMU_GUARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <tinyara/mmu_pages.h>

#ifdef CONFIG_MMU_GUARD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The guard re-permissions memory with small page granularity, so a guarded
 * range must be page aligned and a multiple of the page size.
 */

#define MMU_GUARD_PAGE_SIZE       MMU_PAGE_SIZE

#define MMU_GUARD_MAX_DOMAINS     CONFIG_MMU_GUARD_MAX_DOMAINS
#define MMU_GUARD_MAX_RECORDS     CONFIG_MMU_GUARD_MAX_RECORDS

/* Size of the task name copied into a violation record */

#define MMU_GUARD_NAME_SIZE       (32)

/* Protection applied to a page while it is locked.
 *
 * MMU_GUARD_MODE_RO - The page becomes read-only for privileged code, so only
 *   writes are trapped.  Diagnostic code that only reads the memory keeps
 *   working, which makes this the least intrusive choice.
 * MMU_GUARD_MODE_NA - The page becomes inaccessible at every privilege level,
 *   so reads are trapped as well.  Use it where a read is as much of a bug as a
 *   write, but be aware that any dump or print of the memory also faults.
 */

#define MMU_GUARD_MODE_RO         (0)
#define MMU_GUARD_MODE_NA         (1)

/* What a classifier makes of an abort that landed on one of its pages.
 *
 * MMU_GUARD_VERDICT_FOREIGN   - Not the guard's business after all.  The abort
 *   goes to the normal crash path untouched.
 * MMU_GUARD_VERDICT_VIOLATION - A genuine violation.  The core records it and
 *   then lets the abort reach the crash path, so that the full register and
 *   stack dump identifies the offender.
 * MMU_GUARD_VERDICT_ALLOW     - The access is legitimate even though the page is
 *   locked.  The core opens the page and re-executes the instruction.  Note that
 *   the page stays open afterwards, so a classifier that returns this has to
 *   arrange for it to be locked again.
 */

#define MMU_GUARD_VERDICT_FOREIGN   (0)
#define MMU_GUARD_VERDICT_VIOLATION (1)
#define MMU_GUARD_VERDICT_ALLOW     (2)

/* Storage a client has to provide for a domain, given a number of sections and
 * a number of pages.
 */

#define MMU_GUARD_L2_WORDS(nsect)   ((nsect) * MMU_L2_NENTRIES)
#define MMU_GUARD_BITMAP_BYTES(np)  (((np) + 7) / 8)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* One recorded access to a locked page */

struct mmu_guard_record_s {
	uint32_t dfar;				/* Address that was accessed */
	uint32_t dfsr;				/* Raw data fault status */
	uint32_t pc;				/* Instruction that made the access */
	uint32_t lr;				/* Return address of the offending function */
	pid_t pid;				/* Task that made the access */
	uint8_t cpu;				/* CPU that took the abort */
	bool write;				/* true: store, false: load */
	char name[MMU_GUARD_NAME_SIZE];		/* Name of the offending task */
};

/* The counters are updated from the data abort handler, which the compiler
 * cannot see, so they have to be volatile for a caller to observe an update that
 * happened across an access it believes to be harmless.
 */

struct mmu_guard_stats_s {
	volatile uint32_t nviolations;		/* Accesses trapped on a locked page */
	volatile uint32_t nstale;		/* Aborts healed as stale TLB entries */
	volatile uint32_t nrecords;		/* Valid entries in record[] */
	struct mmu_guard_record_s record[MMU_GUARD_MAX_RECORDS];
};

struct mmu_guard_domain_s;

/****************************************************************************
 * Name: mmu_guard_classify_t
 *
 * Description:
 *   Client hook that decides what an abort on one of its locked pages means.
 *   It runs in data abort context, so it must not take a lock, must not call
 *   anything that can fault and should print with lldbg() only.
 *
 *   A domain without a classifier treats every abort on a locked page as a
 *   violation.
 *
 * Returned Value:
 *   One of the MMU_GUARD_VERDICT_* values.
 *
 ****************************************************************************/

typedef int (*mmu_guard_classify_t)(FAR struct mmu_guard_domain_s *dom, FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr);

/* A guarded address range.
 *
 * The client owns the structure and the buffers it points at, fills in the
 * configuration fields and hands it to mmu_guard_register().  Everything after
 * the marker belongs to the core.
 *
 * A range starts out fully accessible.  The client decides which pages to take
 * away and when, which is what makes this usable from an allocator: the pages
 * of a chunk are locked when it is freed and handed back when it is reused.
 */

struct mmu_guard_domain_s {
	/* Configuration, set by the client before registering */

	FAR const char *name;			/* Shown in dumps */
	uintptr_t base;				/* Start of the range, page aligned */
	size_t size;				/* Size of the range, multiple of a page */
	uint8_t mode;				/* MMU_GUARD_MODE_* */
	mmu_guard_classify_t classify;		/* May be NULL */
	FAR void *priv;				/* Client context for the classifier */

	/* Storage, sized by the client with the macros above.
	 *
	 * l2tbl     - One small page translation table per covered section.  Must be
	 *   MMU_L2_ALIGNMENT aligned and must not live inside the range itself.
	 * secentry  - One saved L1 descriptor per covered section.
	 * lockstate - One bit per page, set while the page is locked.
	 */

	FAR uint32_t *l2tbl;
	FAR uint32_t *secentry;
	FAR uint8_t *lockstate;

	/* Owned by the core from here on */

	bool registered;
	uint16_t nsections;			/* Sections the range spans */
	uint32_t npages;			/* Pages in the range */
	uint32_t nlocked;			/* Pages locked right now */
	uintptr_t stale_addr;			/* Address of the last stale TLB abort */
	uint8_t stale_retry;			/* Consecutive stale aborts on it */
	struct mmu_guard_stats_s stats;
};

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
 * Name: mmu_guard_register
 *
 * Description:
 *   Take over the sections that cover the domain's range, replacing each one
 *   with a small page translation table that reproduces the original mapping.
 *   From this point on the core can lock and unlock individual pages of the
 *   range and claims the data aborts they raise.
 *
 *   The range must be page aligned, must not contain the domain's own
 *   translation tables, and every section it touches must still be mapped as a
 *   plain section.  A domain therefore has to be registered before the
 *   application loader splits sections of its own.
 *
 *   Registration is permanent.  This is a boot time debug facility, and giving
 *   the sections back while another core might hold a stale descriptor would be
 *   a hazard with no use case behind it.
 *
 * Returned Value:
 *   OK on success, a negated errno value on failure.
 *
 ****************************************************************************/

int mmu_guard_register(FAR struct mmu_guard_domain_s *dom);

/****************************************************************************
 * Name: mmu_guard_lock
 *
 * Description:
 *   Take a page run away from its callers.  Idempotent per page, and it accepts
 *   a run that is partly locked already.
 *
 * Input Parameters:
 *   dom    - A registered domain.
 *   vaddr  - Start of the run, page aligned, inside the domain.
 *   npages - Length of the run in pages.
 *   flush  - Clean the run out of the data cache before it loses write access.
 *
 *     A line that is dirty when the page becomes inaccessible would be written
 *     back later without going through a permission check, so any client that
 *     locks memory whose contents may still be dirty has to pass true.  Cache
 *     maintenance by MVA is itself translated and permission checked, which is
 *     why the core has to do it here rather than afterwards.
 *
 * Returned Value:
 *   OK on success, a negated errno value on failure.
 *
 ****************************************************************************/

int mmu_guard_lock(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr, unsigned int npages, bool flush);

/****************************************************************************
 * Name: mmu_guard_unlock
 *
 * Description:
 *   Hand a page run back.  Idempotent per page.
 *
 ****************************************************************************/

int mmu_guard_unlock(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr, unsigned int npages);

/****************************************************************************
 * Name: mmu_guard_unlock_range
 *
 * Description:
 *   Unlock whichever pages of an address range happen to be locked, clipping the
 *   range to the domain.  Neither end has to be page aligned.
 *
 *   Use this instead of mmu_guard_unlock() when the caller knows the memory it is
 *   about to reuse but not which pages of it were protected.  A heap chunk grows
 *   when it is merged with a free neighbour, so the range derived from it later is
 *   not the range that was locked earlier, and recomputing it would leave pages
 *   protected while the memory is handed out again.
 *
 *   The bitmap walk skips whole bytes at a time and returns immediately when the
 *   domain has nothing locked, so it is cheap enough for an allocation path.
 *
 ****************************************************************************/

int mmu_guard_unlock_range(FAR struct mmu_guard_domain_s *dom, uintptr_t start, uintptr_t end);

/****************************************************************************
 * Name: mmu_guard_unlock_all
 *
 * Description:
 *   Hand the whole range back, for diagnostics that read the memory raw.
 *
 ****************************************************************************/

int mmu_guard_unlock_all(FAR struct mmu_guard_domain_s *dom);

/****************************************************************************
 * Name: mmu_guard_rebind_section
 *
 * Description:
 *   Re-install the translation table of the one section that contains vaddr.
 *
 *   Unloading a binary folds every page table entry of the kernel L1 table back
 *   into a section, which would silently drop the protection.  The operation is a
 *   no-op when the table is already in place, so it can simply be re-run whenever
 *   that is suspected.
 *
 *   A domain that spans many sections cannot afford to check all of them on a hot
 *   path.  Repairing just the section that is about to be used costs two L1 entry
 *   comparisons, and the remaining sections are repaired by the calls that follow.
 *   Until then their locked pages read as accessible, so the effect of a fold is a
 *   missed detection and never a false one.
 *
 * Returned Value:
 *   true if the descriptor had to be repaired.
 *
 ****************************************************************************/

bool mmu_guard_rebind_section(FAR struct mmu_guard_domain_s *dom, uintptr_t vaddr);

/****************************************************************************
 * Name: mmu_guard_record
 *
 * Description:
 *   Store one violation in the domain's report buffer.  Exposed so that a
 *   classifier can also record an access it decides to allow.
 *
 ****************************************************************************/

void mmu_guard_record(FAR struct mmu_guard_domain_s *dom, FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr);

/****************************************************************************
 * Name: mmu_guard_domain_dump
 *
 * Description:
 *   Print the state of one domain and every violation recorded for it.
 *
 ****************************************************************************/

void mmu_guard_domain_dump(FAR struct mmu_guard_domain_s *dom);

/****************************************************************************
 * Name: mmu_guard_dataabort
 *
 * Description:
 *   Data abort hook, called from the architecture's data abort handler before
 *   the normal crash handling.  It claims aborts that belong to a registered
 *   domain.
 *
 * Input Parameters:
 *   regs - The register save array of the aborted context.
 *   dfar - Data fault address register.
 *   dfsr - Data fault status register.
 *
 * Returned Value:
 *   true if the abort was handled and the faulting instruction may be
 *   re-executed, false if the abort has to run into the normal crash path.
 *
 ****************************************************************************/

bool mmu_guard_dataabort(FAR uint32_t *regs, uint32_t dfar, uint32_t dfsr);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* CONFIG_MMU_GUARD */
#endif							/* __INCLUDE_TINYARA_MMU_GUARD_H */
