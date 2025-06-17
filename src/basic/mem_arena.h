#ifndef MEM_ARENA_H
#define MEM_ARENA_H


#include <basic/basic.h>

typedef struct {
    size_t size_used;
    size_t size_max;
    u8 *memory;
} MemArena;

void mem_arena_init(MemArena *arena, void *memory, size_t size);
void mem_arena_reset(MemArena *arena);
void mem_arena_reset_and_zero(MemArena *arena);
void *mem_arena_push(MemArena *arena, size_t size);
size_t mem_arena_save(MemArena *arena);
void mem_arena_restore(MemArena *arena, size_t saved_size);
MemArena mem_arena_make_subarena(MemArena *arena, size_t size);

#endif // MEM_ARENA_H
