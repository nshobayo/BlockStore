#ifndef ALLOC_H
#define ALLOC_H

/* Sets up a new "memory allocation area".  'memarea' points to a chunk of memory
 * of 'size' bytes.  Returns 0 on success, nonzero if setup failed.
 */
int alloc_init(void * memarea, int size);

/* Allocates a chunk of memory of the given size from the memory area. Returns 
 * a pointer to the chunk of memory on success; returns 0 if the allocator ran
 * out of memory.
 */
void * alloc_get(void * memarea, int size);

/* Releases a chunk of memory previously allocated. No-op if mem == 0.
 */
void alloc_release(void * memarea, void * mem);

/* Changes the size of the given memory chunk, returning a pointer to the new
 * area, or 0 if there is not enough memory remaining. If mem == 0, same as 
 * alloc_get().
 */
void * alloc_resize(void * memarea, void * mem, int size);

#endif /*ALLOC_H*/
