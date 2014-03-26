/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern uint64 bishopAttacksBBX(uint32 from, uint64 occ);
extern uint64 rookAttacksBBX(uint32 from, uint64 occ);
extern uint64 bishopAttacksBB(uint32 from, uint64 occ);
extern uint64 rookAttacksBB(uint32 from, uint64 occ);
extern uint64 queenAttacksBB(uint32 from, uint64 occ);
extern uint32 isAtt(const position_t *pos, uint32 color, uint64 target);
extern bool isSqAtt(const position_t *pos, uint64 occ, int sq, int color);
extern uint64 pieceAttacksFromBB(const position_t* pos, const int pc, const int sq, const uint64 occ);
extern bool kingIsInCheck(const position_t *pos);
extern uint32 isMoveDefence(const position_t *pos, uint32 move, uint64 target);
extern uint64 pinnedPieces(const position_t *pos, uint32 c);
extern uint64 discoveredCheckCandidates(const position_t *pos, uint32 c);
extern uint32 moveIsLegal(const position_t *pos, uint32 move, uint64 pinned, uint32 incheck);
extern bool moveIsCheck(const position_t *pos, basic_move_t m, uint64 dcc);
extern uint64 attackingPiecesSide(const position_t *pos, uint32 sq, uint32 side);
extern uint64 behindFigure(const position_t *pos, uint32 from, int dir);
extern int swap(const position_t *pos, uint32 m);
extern int swapSquare(const position_t *pos, uint32 m);