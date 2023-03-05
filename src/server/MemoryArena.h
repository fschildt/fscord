#pragma once

#include <stdint.h>
#include <stddef.h>

class MemoryArena {
public:
    void Init(uint8_t *memory, size_t size);
    void Reset();
    uint8_t* Push(size_t size);
    void Append(uint8_t *buff, size_t size);
    void Pop(size_t size);

public:
    int32_t SizeUsed;
    int32_t SizeMax;
    uint8_t *Memory;
};
