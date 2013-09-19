#pragma once

extern inline uint32 getFirstBit(uint64 b);
extern inline uint32 popFirstBit(uint64 *b);
extern inline uint32 bitCnt(uint64 b);

extern inline uint64 fillUp(uint64 b);
extern inline uint64 fillDown(uint64 b);
extern inline uint64 fillUp2(uint64 b);
extern inline uint64 fillDown2(uint64 b);
extern inline uint64 shiftLeft(uint64 b, uint32 i);
extern inline uint64 shiftRight(uint64 b, uint32 i);