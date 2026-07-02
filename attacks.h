/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

/* Magic sliding-attack lookups are defined inline here (rather than in a .cpp)
   so they inline in every translation unit without needing whole-program
   optimization to link. They rely on the magic tables from constants.h/data.h,
   which are always included before attacks.h. */

/* the following routines return a 64 bit attacks of pieces
on the From square with the limiting Occupied bits */
inline uint64 bishopAttacksBB(const uint32 from, const uint64 occ) {
    return MagicAttacks[BOffset[from] + (((BMagicMask[from] & (occ)) * BMagic[from]) >> BShift[from])];
}

inline uint64 rookAttacksBB(const uint32 from, const uint64 occ) {
    return MagicAttacks[ROffset[from] + (((RMagicMask[from] & (occ)) * RMagic[from]) >> RShift[from])];
}

inline uint64 queenAttacksBB(const uint32 from, const uint64 occ) {
    return (MagicAttacks[BOffset[from] + (((BMagicMask[from] & (occ)) * BMagic[from]) >> BShift[from])]
        | MagicAttacks[ROffset[from] + (((RMagicMask[from] & (occ)) * RMagic[from]) >> RShift[from])]);
}

/* the following routines return a 64 bit xray attacks of pieces
on the From square with the limiting Occupied bits */
inline uint64 bishopAttacksBBX(const uint32 from, const uint64 occ) {
    return bishopAttacksBB(from, occ & ~(bishopAttacksBB(from, occ) & occ));
}

inline uint64 rookAttacksBBX(const uint32 from, const uint64 occ) {
    return rookAttacksBB(from, occ & ~(rookAttacksBB(from, occ) & occ));
}

extern bool isAtt(const position_t& pos, uint32 color, uint64 target);
extern bool isSqAtt(const position_t& pos, uint64 occ, int sq, int color);
extern uint64 pieceAttacksFromBB(const position_t& pos, const int pc, const int sq, const uint64 occ);
/* this determines if the side to move is in check */
inline bool kingIsInCheck(const position_t& pos) {
    return isSqAtt(pos, pos.occupied, pos.kpos[pos.side], pos.side ^ 1);
}
extern uint64 pinnedPieces(const position_t& pos, uint32 c);
extern uint64 discoveredCheckCandidates(const position_t& pos, uint32 c);
extern bool moveIsLegal(const position_t& pos, uint32 move, uint64 pinned, bool incheck);
extern bool moveIsCheck(const position_t& pos, basic_move_t m, uint64 dcc);
extern uint64 attackingPiecesSide(const position_t& pos, uint32 sq, uint32 side);
extern uint64 behindFigure(const position_t& pos, uint32 from, int dir);
extern int swap(const position_t& pos, uint32 m);
