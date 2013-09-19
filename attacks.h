#pragma once

/* attacks.c */
extern uint64 rankAttacks(uint64 occ, int sq);
extern uint64 fileAttacks(uint64 occ, int sq);
extern uint64 diagonalAttacks(uint64 occ, int sq);
extern uint64 antiDiagAttacks(uint64 occ, int sq);
extern uint64 rankAttacksX(uint64 occ, int sq);
extern uint64 fileAttacksX(uint64 occ, int sq);
extern uint64 diagonalAttacksX(uint64 occ, int sq);
extern uint64 antiDiagAttacksX(uint64 occ, int sq);
extern uint64 bishopAttacksBBX(uint32 from, uint64 occ);
extern uint64 rookAttacksBBX(uint32 from, uint64 occ);
extern uint64 bishopAttacksBB(uint32 from, uint64 occ);
extern uint64 rookAttacksBB(uint32 from, uint64 occ);
extern uint64 queenAttacksBB(uint32 from, uint64 occ);

extern uint32 isAtt(const position_t *pos, uint32 color, uint64 target);
extern bool kingIsInCheck(const position_t *pos);
extern uint64 pinnedPieces(const position_t *pos, uint32 c);
extern uint64 discoveredCheckCandidates(const position_t *pos, uint32 c);
extern uint32 moveIsLegal(const position_t *pos, uint32 move, uint64 pinned, uint32 incheck);
extern bool moveIsCheck(const position_t *pos, uint32 m, uint64 dcc);
extern uint64 attackingPiecesAll(const position_t *pos, uint64 occ, uint32 sq);
extern uint64 attackingPiecesSide(const position_t *pos, uint32 sq, uint32 side);
extern int moveAttacksSquare(const position_t *pos, uint32 move, uint32 sq);
extern int swap(const position_t *pos, uint32 m);
extern bool isSqAtt(const position_t *pos, uint64 occ, int sq,int color);
extern uint64 pieceAttacksFromBB(const position_t* pos, const int pc, const int sq, const uint64 occ);