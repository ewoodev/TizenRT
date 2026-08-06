/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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

#ifndef __MM_MM_HEAP_MM_NODE_H
#define __MM_MM_HEAP_MM_NODE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <debug.h>

#include <tinyara/mm/mm.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEBUGASSERT_MM_FREE_NODE(heap, node)		\
	do {		\
		DEBUGASSERT(node);		\
		if (!((node)->blink)) {		\
			mfdbg("Corrupted free node %08x\n", node);		\
			mm_dump_heap_free_node_list(heap);		\
			mm_dump_node((struct mm_allocnode_s *)node, "CORRUPT NODE");		\
			mm_dump_node((struct mm_allocnode_s *)((char *)node - (node->preceding & ~MM_ALLOC_BIT)), "PREV NODE");		\
			mm_dump_node((struct mm_allocnode_s *)((char *)node + node->size), "NEXT NODE");		\
			DEBUGPANIC();		\
		}		\
	} while (0)

/* Hand the protected pages of a chunk back before it is taken off the free list.
 *
 * Every path that writes into the interior of a free chunk goes through here
 * first: the splits in mm_malloc() and mm_memalign(), the neighbour extension and
 * bulk copy in mm_realloc(), the trailing split in mm_shrinkchunk() and the
 * merges in mm_free().  Unprotecting here therefore covers all of them.
 */

#ifdef MM_GUARD_ENABLED
#define MM_GUARD_UNPROTECT_NODE(node)		\
	mm_guard_unprotect((node), ((FAR struct mm_allocnode_s *)(node))->size)
#else
#define MM_GUARD_UNPROTECT_NODE(node)		\
	do {		\
	} while (0)
#endif

#define REMOVE_NODE_FROM_LIST(node)				\
	do {							\
		MM_GUARD_UNPROTECT_NODE(node);			\
		(node)->blink->flink = (node)->flink;		\
		if ((node)->flink) {				\
			(node)->flink->blink = (node)->blink;	\
		}						\
	} while (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#endif /* __MM_MM_HEAP_MM_NODE_H */
