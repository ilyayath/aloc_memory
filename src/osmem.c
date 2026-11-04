// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <sys/mman.h>

#include "osmem.h"
#include "block_meta.h"

#define ALIGNMENT 8
#define ALIGN(size) (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
#define META_SIZE ALIGN(sizeof(struct block_meta))
#define MMAP_THRESHOLD (128 * 1024)

/* all blocks allocated with brk, in address order */
static struct block_meta *heap_head;

static void *payload(struct block_meta *block)
{
	return (char *)block + META_SIZE;
}

static struct block_meta *get_block(void *ptr)
{
	return (struct block_meta *)((char *)ptr - META_SIZE);
}

static void *heap_extend(size_t size)
{
	void *ptr = sbrk(size);

	DIE(ptr == (void *)-1, "sbrk");
	return ptr;
}

static void *map_block(size_t size)
{
	struct block_meta *block;

	block = mmap(NULL, size + META_SIZE, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	DIE(block == MAP_FAILED, "mmap");

	block->size = size;
	block->status = STATUS_MAPPED;
	block->prev = NULL;
	block->next = NULL;

	return payload(block);
}

/* first heap use: grab 128K at once and keep it as one free block */
static void heap_init(void)
{
	heap_head = heap_extend(MMAP_THRESHOLD);
	heap_head->size = MMAP_THRESHOLD - META_SIZE;
	heap_head->status = STATUS_FREE;
	heap_head->prev = NULL;
	heap_head->next = NULL;
}

static struct block_meta *last_block(void)
{
	struct block_meta *block = heap_head;

	while (block->next)
		block = block->next;
	return block;
}

static void *heap_alloc(size_t size)
{
	struct block_meta *block, *last;

	if (!heap_head)
		heap_init();

	for (block = heap_head; block; block = block->next) {
		if (block->status == STATUS_FREE && block->size >= size) {
			block->status = STATUS_ALLOC;
			return payload(block);
		}
	}

	last = last_block();
	block = heap_extend(META_SIZE + size);
	block->size = size;
	block->status = STATUS_ALLOC;
	block->prev = last;
	block->next = NULL;
	last->next = block;

	return payload(block);
}

void *os_malloc(size_t size)
{
	if (!size)
		return NULL;

	size = ALIGN(size);
	/* big blocks get their own mapping, small ones go on the heap */
	if (size + META_SIZE >= MMAP_THRESHOLD)
		return map_block(size);
	return heap_alloc(size);
}

void os_free(void *ptr)
{
	struct block_meta *block;

	if (!ptr)
		return;

	block = get_block(ptr);
	if (block->status == STATUS_MAPPED)
		DIE(munmap(block, block->size + META_SIZE) == -1, "munmap");
	else if (block->status == STATUS_ALLOC)
		block->status = STATUS_FREE; /* heap memory is kept for reuse */
}

void *os_calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement os_calloc */
	return NULL;
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
	return NULL;
}
