/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>

static size_t pages = 0;

uintptr_t kheap_allocAreas(void) {
	if(pmem_getFreeFrames(MM_DEF) < 1 || (pages + 1) * PAGE_SIZE > KERNEL_HEAP_SIZE)
		return 0;

	/* allocate one page for area-structs */
	paging_map(KERNEL_HEAP_START + pages * PAGE_SIZE,NULL,1,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_GLOBAL);
	return KERNEL_HEAP_START + pages++ * PAGE_SIZE;
}

uintptr_t kheap_allocSpace(size_t count) {
	if(pmem_getFreeFrames(MM_DEF) < count || (pages + count) * PAGE_SIZE > KERNEL_HEAP_SIZE)
		return 0;
	paging_map(KERNEL_HEAP_START + pages * PAGE_SIZE,NULL,count,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_GLOBAL);
	pages += count;
	return KERNEL_HEAP_START + (pages - count) * PAGE_SIZE;
}