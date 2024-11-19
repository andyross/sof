#ifndef _SOF_PLATFORM_MTK_LIB_MEMORY_H
#define _SOF_PLATFORM_MTK_LIB_MEMORY_H

#define PLATFORM_DCACHE_ALIGN 128

#define uncache_to_cache(addr) (addr)
#define cache_to_uncache(addr) (addr)

static inline void *platform_shared_get(void *ptr, int bytes)
{
	return ptr;
}

#define host_to_local(addr) (addr)

#define PLATFORM_HEAP_SYSTEM 1
#define PLATFORM_HEAP_SYSTEM_RUNTIME 1
#define PLATFORM_HEAP_RUNTIME 1
#define PLATFORM_HEAP_BUFFER 1

#define SHARED_DATA /* Legacy section attribute */

#endif /* _SOF_PLATFORM_MTK_LIB_MEMORY_H */
