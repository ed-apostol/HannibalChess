/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "bitutils.h"

/* NOTE: getFirstBit/popFirstBit/bitCnt and the eval-score packing helpers are
   defined inline in bitutils.h so they inline in every translation unit. */

/* returns a bitboard with all bits above b filled up (excluding b) */
uint64 fillUp(uint64 b) {
    b |= b << 8;
    b |= b << 16;
    return (b | (b << 32)) << 8;
}

/* returns a bitboard with all bits below b filled down (excluding b) */
uint64 fillDown(uint64 b) {
    b |= b >> 8;
    b |= b >> 16;
    return (b | (b >> 32)) >> 8;
}

/* returns a bitboard with all bits above b filled up (including b) */
uint64 fillUp2(uint64 b) {
    b |= b << 8;
    b |= b << 16;
    return (b | (b << 32));
}

/* returns a bitboard with all bits below b filled down (including b) */
uint64 fillDown2(uint64 b) {
    b |= b >> 8;
    b |= b >> 16;
    return (b | (b >> 32));
}

// shift the parameter b with i places to the left
uint64 shiftLeft(uint64 b, uint32 i) {
    return (b << i);
}

// shift the parameter b with i places to the right
uint64 shiftRight(uint64 b, uint32 i) {
    return (b >> i);
}
uint64 adjacent(const uint64 BB) {
    return (BB & (shiftLeft(BB & ~FileHBB, 1) | shiftRight(BB & ~FileABB, 1)));
}
uint64 pawnAttackBB(const uint64 pawns, const int color) {
    static const int Shift[] = { 9, 7 };
    const uint64 pawnAttackLeft = (*ShiftPtr[color])(pawns, Shift[color ^ 1]) & ~FileHBB;
    const uint64 pawnAttackright = (*ShiftPtr[color])(pawns, Shift[color]) & ~FileABB;
    return (pawnAttackLeft | pawnAttackright);
}
uint64 doublePawnAttackBB(const uint64 pawns, const int color) {
    static const int Shift[] = { 9, 7 };
    const uint64 pawnAttackLeft = (*ShiftPtr[color])(pawns, Shift[color ^ 1]) & ~FileHBB;
    const uint64 pawnAttackright = (*ShiftPtr[color])(pawns, Shift[color]) & ~FileABB;
    return (pawnAttackLeft & pawnAttackright);
}
