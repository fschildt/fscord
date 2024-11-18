#include <basic/mem_arena.h>
#include <assert.h>
#include <string.h>

void
mem_arena_init(MemArena *arena, void *memory, size_t size)
{
    arena->size_used = 0;
    arena->size_max = size;
    arena->memory = memory;
}

void
mem_arena_reset_and_zero(MemArena *arena)
{
    memset(arena->memory, 0, arena->size_max);
    arena->size_used = 0;
}

void
mem_arena_reset(MemArena *arena)
{
    arena->size_used = 0;
}

MemArena
mem_arena_make_subarena(MemArena *arena, size_t size)
{
    MemArena sub_arena;
    void *mem = mem_arena_push(arena, size);
    mem_arena_init(&sub_arena, mem, size);
    return sub_arena;
}

void *
mem_arena_push(MemArena *arena, size_t size)
{
    assert(arena->size_used + size <= arena->size_max);

    void *result = arena->memory + arena->size_used;
    arena->size_used += size;

    return result;
}

size_t
mem_arena_save(MemArena *arena)
{
    return arena->size_used;
}

void
mem_arena_restore(MemArena *arena, size_t saved_size)
{
    arena->size_used = saved_size;
}

