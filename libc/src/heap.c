/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/heap.h"
#include "../h/mem.h"
#include "../h/debug.h"
#include <string.h>

#define PAGE_SIZE		4096

/* an area in memory */
typedef struct sMemArea sMemArea;
struct sMemArea {
	u32 size;
	void *address;
	sMemArea *next;
};

/* a linked list of free and usable areas. That means the areas have an address and size */
static sMemArea *usableList = NULL;
/* a linked list of free but not usable areas. That means the areas have no address and size */
static sMemArea *freeList = NULL;
/* a hashmap with occupied-lists, key is (address % OCC_MAP_SIZE) */
static sMemArea *occupiedMap[OCC_MAP_SIZE] = {NULL};
/* total number of pages we're using */
static u32 pageCount = 0;

/**
 * Allocates a new page for areas
 *
 * @return true on success
 */
static bool loadNewAreas(void);

/**
 * Allocates new space for alloation
 *
 * @param size the minimum size
 * @return true on success
 */
static bool loadNewSpace(u32 size);

void *malloc(u32 size) {
	sMemArea *area,*prev,*narea;
	sMemArea **list;

	if(size == 0)
		return NULL;

	/* find a suitable area */
	prev = NULL;
	area = usableList;
	while(area != NULL) {
		if(area->size >= size)
			break;
		prev = area;
		area = area->next;
	}

	/* no area found? */
	if(area == NULL) {
		if(!loadNewSpace(size))
			return NULL;
		/* we can assume that it fits */
		area = usableList;
		/* remove from usable-list */
		usableList = area->next;
	}
	else {
		/* remove from usable-list */
		if(prev == NULL)
			usableList = area->next;
		else
			prev->next = area->next;
	}

	/* is there space left? */
	if(size < area->size) {
		if(freeList == NULL) {
			if(!loadNewAreas()) {
				/* TODO we may have changed something... */
				return NULL;
			}
		}

		/* split the area */
		narea = freeList;
		freeList = freeList->next;
		narea->address = (void*)((u32)area->address + size);
		narea->size = area->size - size;
		area->size = size;
		/* insert in usable-list */
		narea->next = usableList;
		usableList = narea;
	}

	/* insert in occupied-map */
	list = occupiedMap + ((u32)area->address % OCC_MAP_SIZE);
	area->next = *list;
	*list = area;

	return area->address;
}

void *calloc(u32 num,u32 size) {
	void *a = malloc(num * size);
	if(a == NULL)
		return NULL;

	memset(a,0,num * size);
	return a;
}

void free(void *addr) {
	sMemArea *area,*a,*prev,*next,*oprev,*nprev,*pprev,*tprev;

	/* addr may be null */
	if(addr == NULL)
		return;

	/* find the area with given address */
	oprev = NULL;
	area = occupiedMap[(u32)addr % OCC_MAP_SIZE];
	while(area != NULL) {
		if(area->address == addr)
			break;
		oprev = area;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return;

	/* find the previous and next free areas */
	prev = NULL;
	next = NULL;
	tprev = NULL;
	a = usableList;
	while(a != NULL) {
		if((u32)a->address + a->size == (u32)addr) {
			prev = a;
			pprev = tprev;
		}
		if((u32)a->address == (u32)addr + area->size) {
			next = a;
			nprev = tprev;
		}
		/* do we have both? */
		if(prev && next)
			break;
		tprev = a;
		a = a->next;
	}

	/* remove area from occupied-map */
	if(oprev)
		oprev->next = area->next;
	else
		occupiedMap[(u32)addr % OCC_MAP_SIZE] = area->next;

	/* see what we have to merge */
	if(prev && next) {
		/* merge prev & area & next */
		area->size += prev->size + next->size;
		area->address = prev->address;
		/* remove prev and next from the list */
		if(nprev)
			nprev->next = next->next;
		else
			usableList = next->next;
		/* special-case if next is the previous of prev */
		if(pprev == next) {
			if(nprev)
				nprev->next = prev->next;
			else
				usableList = prev->next;
		}
		else {
			if(pprev)
				pprev->next = prev->next;
			else
				usableList = prev->next;
		}
		/* put area on the usable-list */
		area->next = usableList;
		usableList = area;
		/* put prev and next on the freelist */
		prev->next = next;
		next->next = freeList;
		freeList = prev;
	}
	else if(prev) {
		/* merge preg & area */
		prev->size += area->size;
		/* put area on the freelist */
		area->next = freeList;
		freeList = area;
	}
	else if(next) {
		/* merge area & next */
		next->address = area->address;
		next->size += area->size;
		/* put area on the freelist */
		area->next = freeList;
		freeList = area;
	}
	else {
		/* insert area in usableList */
		area->next = usableList;
		usableList = area;
	}
}

void *realloc(void *addr,u32 size) {
	sMemArea *area,*a,*prev;
	/* find the area with given address */
	area = occupiedMap[(u32)addr % OCC_MAP_SIZE];
	while(area != NULL) {
		if(area->address == addr)
			break;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return NULL;

	/* ignore size-shrinks */
	if(size < area->size)
		return addr;

	a = usableList;
	prev = NULL;
	while(a != NULL) {
		/* found the area behind? */
		if(a->address == (u8*)area->address + area->size) {
			/* if the size of both is big enough we can use them */
			if(area->size + a->size >= size) {
				/* space left? */
				if(size < area->size + a->size) {
					/* so move the area forward */
					a->address = (void*)((u32)area->address + size);
					a->size = (area->size + a->size) - size;
				}
				/* otherwise we don't need a anymore */
				else {
					/* remove a from usable-list */
					if(prev == NULL)
						usableList = a->next;
					else
						prev->next = a->next;
					/* free a */
					a->next = freeList;
					freeList = a;
				}

				area->size = size;
				return addr;
			}

			/* makes no sense to continue since we've found the area behind */
			break;
		}
		prev = a;
		a = a->next;
	}

	/* the areas are not big enough, so allocate a new one */
	a = malloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size);
	free(addr);
	return a;
}

static bool loadNewSpace(u32 size) {
	s32 res;
	sMemArea *area;

	/* no free areas? */
	if(freeList == NULL) {
		if(!loadNewAreas())
			return false;
	}

	/* allocate the required pages */
	s32 count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	res = changeSize(count);
	if(res < 0) {
		/* free the above reserved page */
		changeSize(-1);
		return false;
	}

	pageCount += count;
	/* take one area from the freelist and put the memory in it */
	area = freeList;
	freeList = freeList->next;
	area->address = (void*)(res * PAGE_SIZE);
	area->size = PAGE_SIZE * count;
	/* put area in the usable-list */
	area->next = usableList;
	usableList = area;
	return true;
}

static bool loadNewAreas(void) {
	sMemArea *area,*end;
	s32 res;

	/* allocate one page for area-structs */
	res = changeSize(1);
	if(res < 0)
		return false;

	/* determine start- and end-address */
	pageCount++;
	area = (sMemArea*)(res * PAGE_SIZE);
	end = area + (PAGE_SIZE / sizeof(sMemArea));

	/* put all areas in the freelist */
	freeList = area;
	area++;
	while(area < end) {
		area->next = freeList;
		freeList = area;
		area++;
	}

	return true;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

u32 heap_getFreeSpace(void) {
	sMemArea *area;
	u32 c = 0;
	area = usableList;
	while(area != NULL) {
		c += area->size;
		area = area->next;
	}
	return c;
}

void heap_print(void) {
	sMemArea *area;
	u32 i;

	debugf("PageCount=%d\n",pageCount);
	debugf("UsableList:\n");
	area = usableList;
	while(area != NULL) {
		debugf("\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
		area = area->next;
	}

	/*debugf("FreeList:\n");
	area = freeList;
	while(area != NULL) {
		debugf("\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
		area = area->next;
	}*/

	debugf("OccupiedMap:\n");
	for(i = 0; i < OCC_MAP_SIZE; i++) {
		area = occupiedMap[i];
		if(area != NULL) {
			debugf("\t%d:\n",i);
			while(area != NULL) {
				debugf("\t\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
				area = area->next;
			}
		}
	}
}

#endif
