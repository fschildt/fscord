#include <MemoryArena.h>

#include <assert.h>
#include <string.h>

void MemoryArena::Init(uint8_t *memory, size_t size) {
    SizeUsed = 0;
    SizeMax = size;
    Memory = memory;
}

void MemoryArena::Reset() {
    SizeUsed = 0;
}

uint8_t* MemoryArena::Push(size_t size) {
    assert(SizeUsed + size <= SizeMax);

    uint8_t *mem = Memory + SizeUsed;
    SizeUsed += size;
    return mem;
}

void MemoryArena::Append(uint8_t *buff, size_t size) {
    assert(SizeUsed + size <= SizeMax);

    memcpy(Memory + SizeUsed, buff, size);
    SizeUsed += size;
}

void MemoryArena::Pop(size_t size) {
    assert(size <= SizeUsed);
    SizeUsed -= size;
}
