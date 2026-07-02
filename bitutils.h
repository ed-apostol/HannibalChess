/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#include <intrin.h>
#include "typedefs.h"

/* Hot bit primitives are defined inline in this header so every translation
   unit can inline them. Keeping the definitions in a .cpp required whole-program
   optimization (MSVC /GL) to link; header definitions link on any toolchain. */

#ifdef VERSION64BIT
#ifdef _MSC_VER
#if defined(_WIN64) || defined(__LP64__)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#define USING_INTRINSICS
#endif
#elif defined(__GNUC__) && defined(__LP64__)
static INLINE unsigned char _BitScanForward64(unsigned int* const Index, const U64 Mask) {
    U64 Ret;
    __asm__
    (
        "bsfq %[Mask], %[Ret]"
        :[Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned int)Ret;
    return Mask ? 1 : 0;
}
static INLINE unsigned char _BitScanReverse64(unsigned int* const Index, const U64 Mask) {
    U64 Ret;
    __asm__
    (
        "bsrq %[Mask], %[Ret]"
        :[Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned int)Ret;
    return Mask ? 1 : 0;
}
#define USING_INTRINSICS
#endif
/* returns the least significant square from the 64 bitfield */
INLINE uint32 getFirstBit(uint64 bb) {
    unsigned long index = 0;
    _BitScanForward64(&index, bb);
    return index;
}

/* returns the least significant square and clears it from the 64 bitfield */
INLINE uint32 popFirstBit(uint64 *b) {
    unsigned long index = 0;
    _BitScanForward64(&index, *b);
    *b &= (*b - 1);
    return index;
}
/* this count the number of bits in a given bitfield,
it is using a SWAR algorithm I think */
INLINE uint32 bitCnt(uint64 x) {
    x -= (x >> 1) & 0x5555555555555555ULL;
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return (uint32)((x * 0x0101010101010101ULL) >> 56);
}
#else
static const int FoldedTable[64] = {
    63, 30, 3, 32, 59, 14, 11, 33,
    60, 24, 50, 9, 55, 19, 21, 34,
    61, 29, 2, 53, 51, 23, 41, 18,
    56, 28, 1, 43, 46, 27, 0, 35,
    62, 31, 58, 4, 5, 49, 54, 6,
    15, 52, 12, 40, 7, 42, 45, 16,
    25, 57, 48, 13, 10, 39, 8, 44,
    20, 47, 38, 22, 17, 37, 36, 26,
};
/* this count the number of bits in a given bitfield,
it is using a SWAR algorithm I think */
INLINE uint32 bitCnt(uint64 bb) {
    uint32 w = (uint32)(bb >> 32);
    uint32 v = (uint32)bb;

    v = v - ((v >> 1) & 0x55555555);
    w = w - ((w >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F;
    w = (w + (w >> 4)) & 0x0F0F0F0F;
    v = ((v + w) * 0x01010101) >> 24;
    return v;
}
/* returns the least significant square from the 64 bitfield */
INLINE uint32 getFirstBit(uint64 b) {
    uint32 folded;
    b ^= (b - 1);
    folded = ((uint32)b) ^ ((uint32)(b >> 32));
    return FoldedTable[(folded * 0x78291ACF) >> 26];
}

/* returns the least significant square and clears it from the 64 bitfield */
INLINE uint32 popFirstBit(uint64 *b) {
    uint64 bb = *b ^ (*b - 1);
    uint32 folded = ((uint32)bb ^ (uint32)(bb >> 32));
    *b &= (*b - 1);
    return FoldedTable[(folded * 0x78291ACF) >> 26];
}
#endif

extern uint64 fillUp(uint64 b);
extern uint64 fillDown(uint64 b);
extern uint64 fillUp2(uint64 b);
extern uint64 fillDown2(uint64 b);
extern uint64 shiftLeft(uint64 b, uint32 i);
extern uint64 shiftRight(uint64 b, uint32 i);
extern uint64 adjacent(const uint64 BB);
extern uint64 pawnAttackBB(const uint64 pawns, const int color);
extern uint64 doublePawnAttackBB(const uint64 pawns, const int color);

#define MaxOneBit(x) (((x) & ((x)-1))==0)
#define MinTwoBits(x) ((x) & ((x)-1))

inline EvalScore ComposeEvalScore(int16 s1, int16 s2) { return COMP(s1, s2); }
inline int16 GetOpening(EvalScore s1) { return ((int16)(s1 & 0xFFFF)); }
inline int16 GetEndgame(EvalScore s1) { return (((s1 >> 15) & 1) + (int16)(s1 >> 16)); }
