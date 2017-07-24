#include "memory.h"
#include "types.h"
#include "paging.h"
#include "system_calls.h"

void init_memory() {
	virt_to_phys(PHYS_MEM_LOCATION, HEAP_LOCATION);
	page = (void*)HEAP_LOCATION;
	startptr = endptr = page;
	used = 0;
}

void* xmalloc(uint32_t size) {
	//sanity check
	if (size > MAX_MALLOC_SIZE) 
		return NULL;

	//check if available memory
	void* ptr = startptr;
	while (ptr < endptr) {
		//check size and status
		uint32_t isize = (*(uint32_t *)ptr) & 0xFFFFF;
		uint32_t isfree = (*(uint32_t *)ptr) >> 31; 

		if (isfree && isize == size) {
			*(uint32_t*)ptr = *(uint32_t*)(ptr + size + SIZEOF_UINT32_T) = (*(uint32_t *)ptr) & 0xFFFFF;
			// printf("%p\n", ptr + SIZEOF_UINT32_T);
			return ptr + SIZEOF_UINT32_T;
		}	
		else if (isfree && (isize >= size + 2 * SIZEOF_UINT32_T)) {
			//split
			*(uint32_t*)ptr = *(uint32_t*)(ptr + size + SIZEOF_UINT32_T) = size & 0xFFFFF;
			*(uint32_t*)(ptr + size + 2 * SIZEOF_UINT32_T) = *(uint32_t*)(ptr + isize + SIZEOF_UINT32_T) = (isize - size - 2 * SIZEOF_UINT32_T) & 0xFFFFF;
			// printf("%p\n", ptr + SIZEOF_UINT32_T);	
			return ptr + SIZEOF_UINT32_T;
		}
		//go next
		ptr = ptr + isize + SIZEOF_UINT32_T;
	}

	//sbrk
	void* head = sbrk(size + SIZEOF_UINT32_T);
	if (head == (void*)-1) 
		return NULL;
	*(uint32_t*)head = *(uint32_t*)(head + size + SIZEOF_UINT32_T) = size & 0xFFFFF;
	endptr += size;
	// printf("%p\n", ptr + SIZEOF_UINT32_T);
	return head + SIZEOF_UINT32_T;
}

void xfree(void* ptr) {
	//sanity check
	if (ptr == NULL 
	 || ptr > endptr
	 || ptr < startptr)
		return;
	//set free
	void* rptr = ptr - SIZEOF_UINT32_T;
	uint32_t size = (*(uint32_t*)ptr) & 0xFFFFF;
	// *(uint32_t*)rptr = *(uint32_t*)(rptr + size + SIZEOF_UINT32_T) = (size & 0xFFFFF) | 0x80000000;


	uint32_t cup = 0;
	uint32_t cdown = 0;
	void * cstart = rptr;
	uint32_t nsize = size; 
	//coalesce up
	if ((rptr - SIZEOF_UINT32_T > startptr) && (*(uint32_t*)(rptr - SIZEOF_UINT32_T) >> 31)) {
		uint32_t xsize = *(uint32_t*)(rptr - SIZEOF_UINT32_T) & 0xFFFFF;
		if (rptr - 2*SIZEOF_UINT32_T - xsize >= startptr) {
			// ensure mem is after the start
			cup = 1;
			cstart = rptr - 2*SIZEOF_UINT32_T - xsize;
			nsize += xsize + 2*SIZEOF_UINT32_T;
		}
	}

	//coalesce down
	if ((rptr + size + 2*SIZEOF_UINT32_T < endptr) && *(uint32_t*)(rptr + size + 2 * SIZEOF_UINT32_T) >> 31) {
		cdown = 1;
		uint32_t xsize = *(uint32_t*)(rptr + size + 2 * SIZEOF_UINT32_T) & 0xFFFFF;
		nsize += xsize + 2*SIZEOF_UINT32_T;
	}

	*(uint32_t*)cstart = *(uint32_t*)(cstart + nsize + SIZEOF_UINT32_T) = 0x80000000 | (nsize & 0xFFFFF);
}

void* calloc(uint32_t size, uint32_t n) {
	void* ptr = (void*) xmalloc(size * n);
	if (ptr == NULL) 
		return NULL;
	memset(ptr, 0, size * n); // TODO: check if params are correct
	return ptr;
}

void* realloc(void* ptr, uint32_t size) {
	void* rptr = ptr - SIZEOF_UINT32_T;
	uint32_t isize = *(uint32_t*)rptr & 0xFFFFF;
	if (size <= isize) 
		return ptr;
	void* nptr = xmalloc(size);
	if (nptr == NULL) return NULL;
	memcpy(nptr, ptr, isize);
	xfree(ptr);
	return nptr;
}

void* sbrk (uint32_t size) {
	used += size;
	endptr += size;

	return page + used;
}
