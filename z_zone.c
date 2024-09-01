// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2023-2024 by Frenkel Smeijers
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdint.h>
#include "compiler.h"
#include "z_zone.h"
#include "doomdef.h"
#include "i_system.h"


//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC		1	// static entire execution time
#define PU_LEVEL		2	// static until level exited
#define PU_LEVSPEC		3	// a special thinker in a level
#define PU_CACHE		4

#define PU_PURGELEVEL PU_CACHE


//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
//

#if defined INSTRUMENTED
    static int32_t running_count = 0;
#endif


#define	ZONEID	0x1dea

typedef struct
{
#if SIZE_OF_SEGMENT_T == 2
    uint32_t  size;			// including the header and possibly tiny fragments
    uint16_t  tag;			// purgelevel
#else
    uint32_t  size:24;		// including the header and possibly tiny fragments
    uint32_t  tag:4;		// purgelevel
#endif
    void __far*__far*    user;	// NULL if a free block
    segment_t prev;
    segment_t next;
#if defined ZONEIDCHECK
    uint16_t id;			// should be ZONEID
#endif
} memblock_t;


#define PARAGRAPH_SIZE 16

typedef char assertMemblockSize[sizeof(memblock_t) <= PARAGRAPH_SIZE ? 1 : -1];


static memblock_t __far*	mainzone_sentinal;
static segment_t			mainzone_sentinal_segment;
static segment_t			mainzone_rover_segment;


static segment_t pointerToSegment(const memblock_t __far* ptr)
{
#if defined RANGECHECK
	if ((((uint32_t) ptr) & (PARAGRAPH_SIZE - 1)) != 0)
		I_Error("pointerToSegment: pointer is not aligned: 0x%lx", ptr);
#endif

	return D_FP_SEG(ptr);
}

static memblock_t __far* segmentToPointer(segment_t seg)
{
	return D_MK_FP(seg, 0);
}


static segment_t I_ZoneBase(uint32_t initialsizetotry, uint32_t *size)
{
	uint32_t paragraphs = initialsizetotry / PARAGRAPH_SIZE;

	uint8_t __far* ptr = fmemalloc(paragraphs * PARAGRAPH_SIZE);
	while (paragraphs != 0 && ptr == NULL)
	{
		paragraphs--;
		ptr = fmemalloc(paragraphs * PARAGRAPH_SIZE);
	}

	if (paragraphs == 0)
	{
		*size = 0;
		return 0;
	}
	else
	{
		*size = paragraphs * PARAGRAPH_SIZE;
		segment_t segment = D_FP_SEG(ptr);
		printf("%6ld bytes allocated at 0x%4x\n", *size, segment);
		return segment;
	}
}


typedef struct
{
	uint32_t  size;
	segment_t segment;
} block_t;


static int compare(const void* l, const void* r)
{
	const block_t* bl = *(const block_t**)l;
	const block_t* br = *(const block_t**)r;

	return bl->segment - br->segment;
}


//
// Z_Init
//

#define MINFRAGMENT		64
#define MAXBLOCKS		10

void Z_Init (void)
{
	// allocate all available conventional memory.
	uint32_t heapSize = 0;

	int16_t  num_blocks = 0;
	block_t  blocks[MAXBLOCKS];
	block_t* block_ptrs[MAXBLOCKS];

	uint32_t  blocksize;
	segment_t blocksegment = I_ZoneBase(640 * 1024L, &blocksize);

	while (blocksize >= MINFRAGMENT)
	{
		blocks[num_blocks].size    = blocksize;
		blocks[num_blocks].segment = blocksegment;
		block_ptrs[num_blocks] = &blocks[num_blocks];
		heapSize += blocksize;
		num_blocks++;

		blocksegment = I_ZoneBase(blocksize, &blocksize);
	}

	heapSize -= (PARAGRAPH_SIZE * (num_blocks - 1));
	printf("%6ld bytes allocated for zone\n", heapSize);

	qsort(block_ptrs, num_blocks, sizeof(block_t*), compare);

	mainzone_rover_segment = block_ptrs[0]->segment;
	memblock_t __far* mainzone = segmentToPointer(mainzone_rover_segment);

	// align blocklist
	uint_fast8_t i = 0;
	static uint8_t __far mainzone_sentinal_buffer[PARAGRAPH_SIZE * 2];
	uint32_t b = (uint32_t) &mainzone_sentinal_buffer[i++];
	while ((b & (PARAGRAPH_SIZE - 1)) != 0)
		b = (uint32_t) &mainzone_sentinal_buffer[i++];
	mainzone_sentinal = (memblock_t __far*)b;

#if defined __WATCOMC__ && defined _M_I86
	// normalize pointer
	mainzone_sentinal = D_MK_FP(D_FP_SEG(mainzone_sentinal) + D_FP_OFF(mainzone_sentinal) / PARAGRAPH_SIZE, 0);
#endif

	mainzone_sentinal_segment = pointerToSegment(mainzone_sentinal);

	mainzone_sentinal->tag  = PU_STATIC;
	mainzone_sentinal->user = (void __far*)mainzone;
	mainzone_sentinal->prev = block_ptrs[num_blocks - 1]->segment;
	mainzone_sentinal->next = mainzone_rover_segment;

	// set the entire zone to a couple of free blocks
	mainzone->size = block_ptrs[0]->size;
	mainzone->tag  = 0;
	mainzone->user = NULL; // NULL indicates a free block.
	mainzone->prev = mainzone_sentinal_segment;
	mainzone->next = mainzone_sentinal_segment;
#if defined ZONEIDCHECK
	mainzone->id   = ZONEID;
#endif

	for (int16_t i = 1; i < num_blocks; i++)
	{
		segment_t romblock_segment = block_ptrs[i - 1]->segment + block_ptrs[i - 1]->size / PARAGRAPH_SIZE - 1;
		memblock_t __far* romblock = segmentToPointer(romblock_segment);
		romblock->size = (uint32_t)(block_ptrs[i]->segment - romblock_segment) * PARAGRAPH_SIZE;
		romblock->tag  = PU_STATIC;
		romblock->user = (void __far*)mainzone;
		romblock->prev = block_ptrs[i - 1]->segment;
		romblock->next = block_ptrs[i]->segment;
#if defined ZONEIDCHECK
		romblock->id   = ZONEID;
#endif

		memblock_t __far* umbblock = segmentToPointer(block_ptrs[i]->segment);
		umbblock->size = block_ptrs[i]->size;
		umbblock->tag  = 0;
		umbblock->user = NULL; // NULL indicates a free block.
		umbblock->prev = romblock_segment;
		umbblock->next = mainzone_sentinal_segment;
#if defined ZONEIDCHECK
		umbblock->id   = ZONEID;
#endif

		memblock_t __far* prevblock = segmentToPointer(block_ptrs[i - 1]->segment);
		prevblock->size -= PARAGRAPH_SIZE;
		prevblock->next = romblock_segment;
	}
}


static void Z_ChangeTag(const void __far* ptr, uint_fast8_t tag)
{
#if defined RANGECHECK
	if ((((uint32_t) ptr) & (PARAGRAPH_SIZE - 1)) != 0)
		I_Error("Z_ChangeTag: pointer is not aligned: 0x%lx %s %i", ptr, f, l);
#endif

#if defined _M_I86
	memblock_t __far* block = (memblock_t __far*)(((uint32_t)ptr) - 0x00010000);
#else
	memblock_t __far* block = (memblock_t __far*)(((uint32_t)ptr) - 0x00010);
#endif

#if defined ZONEIDCHECK
	if (block->id != ZONEID)
		I_Error("Z_ChangeTag: block has id %x instead of ZONEID", block->id);
#endif
	block->tag = tag;
}


void Z_ChangeTagToStatic(const void __far* ptr)
{
	Z_ChangeTag(ptr, PU_STATIC);
}


void Z_ChangeTagToCache(const void __far* ptr)
{
	Z_ChangeTag(ptr, PU_CACHE);
}


static void Z_FreeBlock(memblock_t __far* block)
{
#if defined ZONEIDCHECK
    if (block->id != ZONEID)
        I_Error("Z_FreeBlock: block has id %x instead of ZONEID", block->id);
#endif

    if (D_FP_SEG(block->user) != 0)
    {
        // far pointers with segment 0 are not user pointers
        // Note: OS-dependend

        // clear the user's mark
        *block->user = NULL;
    }

    // mark as free
    block->user = NULL;
    block->tag  = 0;


#if defined INSTRUMENTED
    running_count -= block->size;
    printf("Free: %ld\n", running_count);
#endif

    memblock_t __far* other = segmentToPointer(block->prev);

    if (!other->user)
    {
        // merge with previous free block
        other->size += block->size;
        other->next  = block->next;
        segmentToPointer(other->next)->prev = block->prev; // == pointerToSegment(other);

        if (pointerToSegment(block) == mainzone_rover_segment)
            mainzone_rover_segment = block->prev; // == pointerToSegment(other);

        block = other;
    }

    other = segmentToPointer(block->next);
    if (!other->user)
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next  = other->next;
        segmentToPointer(block->next)->prev = pointerToSegment(block);

        if (pointerToSegment(other) == mainzone_rover_segment)
            mainzone_rover_segment = pointerToSegment(block);
    }
}


//
// Z_Free
//
void Z_Free (const void __far* ptr)
{
#if defined RANGECHECK
	if ((((uint32_t) ptr) & (PARAGRAPH_SIZE - 1)) != 0)
		I_Error("Z_Free: pointer is not aligned: 0x%lx", ptr);
#endif

#if defined _M_I86
	memblock_t __far* block = (memblock_t __far*)(((uint32_t)ptr) - 0x00010000);
#else
	memblock_t __far* block = (memblock_t __far*)(((uint32_t)ptr) - 0x00010);
#endif

	Z_FreeBlock(block);
}


static uint32_t Z_GetLargestFreeBlockSize(void)
{
	uint32_t largestFreeBlockSize = 0;

	for (memblock_t __far* block = segmentToPointer(mainzone_sentinal->next); pointerToSegment(block) != mainzone_sentinal_segment; block = segmentToPointer(block->next))
		if (!block->user && block->size > largestFreeBlockSize)
			largestFreeBlockSize = block->size;

	return largestFreeBlockSize;
}

static uint32_t Z_GetTotalFreeMemory(void)
{
	uint32_t totalFreeMemory = 0;

	for (memblock_t __far* block = segmentToPointer(mainzone_sentinal->next); pointerToSegment(block) != mainzone_sentinal_segment; block = segmentToPointer(block->next))
		if (!block->user)
			totalFreeMemory += block->size;

	return totalFreeMemory;
}


//
// Z_TryMalloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
// Because Z_TryMalloc is static, we can control the input and we can make sure tag is always < PU_PURGELEVEL.
//

static void __far* Z_TryMalloc(uint16_t size, int8_t tag, void __far*__far* user)
{
    size = (size + (PARAGRAPH_SIZE - 1)) & ~(PARAGRAPH_SIZE - 1);

    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.

    // account for size of block header
    size += PARAGRAPH_SIZE;

    // if there is a free block behind the rover,
    //  back up over them
    memblock_t __far* base = segmentToPointer(mainzone_rover_segment);

    memblock_t __far* previous_block = segmentToPointer(base->prev);
    if (!previous_block->user)
        base = previous_block;

    memblock_t __far* rover   = base;
    segment_t   start_segment = base->prev;

    do
    {
        if (pointerToSegment(rover) == start_segment)
        {
            // scanned all the way around the list
            return NULL;
        }

        if (rover->user)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                //  so move base past it
                base = rover = segmentToPointer(rover->next);
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base  = segmentToPointer(base->prev);
                Z_FreeBlock(rover);
                base  = segmentToPointer(base->next);
                rover = segmentToPointer(base->next);
            }
        }
        else
            rover = segmentToPointer(rover->next);

    } while (base->user || base->size < size);
    // found a block big enough

    int32_t newblock_size = base->size - size;
    if (newblock_size > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        segment_t base_segment     = pointerToSegment(base);
        segment_t newblock_segment = base_segment + (size / PARAGRAPH_SIZE);

        memblock_t __far* newblock = segmentToPointer(newblock_segment);
        newblock->size = newblock_size;
        newblock->tag  = 0;
        newblock->user = NULL; // NULL indicates free block.
        newblock->next = base->next;
        newblock->prev = base_segment;
#if defined ZONEIDCHECK
        newblock->id   = ZONEID;
#endif

        segmentToPointer(base->next)->prev = newblock_segment;
        base->size = size;
        base->next = newblock_segment;
    }

    base->tag  = tag;
    if (user)
        base->user = user;
    else
        base->user = (void __far*__far*) D_MK_FP(0,2); // unowned
#if defined ZONEIDCHECK
    base->id  = ZONEID;
#endif

    // next allocation will start looking here
    mainzone_rover_segment = base->next;

#if defined INSTRUMENTED
    running_count += base->size;
    printf("Alloc: %ld (%ld)\n", base->size, running_count);
#endif

#if defined _M_I86
    memblock_t __far* block = (memblock_t __far*)(((uint32_t)base) + 0x00010000);
#else
    memblock_t __far* block = (memblock_t __far*)(((uint32_t)base) + 0x00010);
#endif

    return block;
}


static void __far* Z_Malloc(uint16_t size, int8_t tag, void __far*__far* user) {
	void __far* ptr = Z_TryMalloc(size, tag, user);
	if (!ptr)
		I_Error ("Z_Malloc: failed to allocate %u B, max free block %li B, total free %li", size, Z_GetLargestFreeBlockSize(), Z_GetTotalFreeMemory());
	return ptr;
}


void __far* Z_TryMallocStatic(uint16_t size)
{
	return Z_TryMalloc(size, PU_STATIC, NULL);
}


void __far* Z_MallocStatic(uint16_t size)
{
	return Z_Malloc(size, PU_STATIC, NULL);
}


void __far* Z_MallocStaticWithUser(uint16_t size, void __far*__far* user)
{
	return Z_Malloc(size, PU_STATIC, user);
}


void __far* Z_MallocLevel(uint16_t size, void __far*__far* user)
{
	return Z_Malloc(size, PU_LEVEL, user);
}


void __far* Z_CallocLevel(uint16_t size)
{
    void __far* ptr = Z_Malloc(size, PU_LEVEL, NULL);
    _fmemset(ptr, 0, size);
    return ptr;
}


void __far* Z_CallocLevSpec(uint16_t size)
{
	void __far* ptr = Z_Malloc(size, PU_LEVSPEC, NULL);
	_fmemset(ptr, 0, size);
	return ptr;
}


boolean Z_IsEnoughFreeMemory(uint16_t size)
{
	const uint8_t __far* ptr = Z_TryMallocStatic(size);
	if (ptr)
	{
		Z_Free(ptr);
		return true;
	} else
		return false;
}


//
// Z_FreeTags
//
void Z_FreeTags(void)
{
    memblock_t __far* next;

    for (memblock_t __far* block = segmentToPointer(mainzone_sentinal->next); pointerToSegment(block) != mainzone_sentinal_segment; block = next)
    {
        // get link before freeing
        next = segmentToPointer(block->next);

        // already a free block?
        if (!block->user)
            continue;

        if (PU_LEVEL <= block->tag && block->tag <= (PU_PURGELEVEL - 1))
            Z_FreeBlock(block);
    }
}

//
// Z_CheckHeap
//
void Z_CheckHeap (void)
{
    for (memblock_t __far* block = segmentToPointer(mainzone_sentinal->next); ; block = segmentToPointer(block->next))
    {
        if (block->next == mainzone_sentinal_segment)
        {
            // all blocks have been hit
            break;
        }

#if defined ZONEIDCHECK
        if (block->id != ZONEID)
            I_Error("Z_CheckHeap: block has id %x instead of ZONEID", block->id);
#endif

        if (pointerToSegment(block) + (block->size / PARAGRAPH_SIZE) != block->next)
            I_Error ("Z_CheckHeap: block size does not touch the next block\n");

        if (segmentToPointer(block->next)->prev != pointerToSegment(block))
            I_Error ("Z_CheckHeap: next block doesn't have proper back link\n");

        if (!block->user && !segmentToPointer(block->next)->user)
            I_Error ("Z_CheckHeap: two consecutive free blocks\n");
    }
}
