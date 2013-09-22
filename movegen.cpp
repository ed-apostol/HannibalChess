/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "attacks.h"
#include "movegen.h"
#include "utils.h"
#include "bitutils.h"
#include "search.h"

/*
// 00000000 00000000 00000000 00111111 = from square     = bits 1-6
// 00000000 00000000 00001111 11000000 = to square       = bits 7-12
// 00000000 00000000 01110000 00000000 = piece           = bits 13-15
// 00000000 00000000 10000000 00000000 = castle	         = bit  16
// 00000000 00000001 00000000 00000000 = pawn 2 forward  = bit  17
// 00000000 00000010 00000000 00000000 = promotion       = bit  18
// 00000000 00011100 00000000 00000000 = capture piece   = bits 19-21
// 00000000 00100000 00000000 00000000 = en passant      = bit  22
// 00000001 11000000 00000000 00000000 = promotion piece = bits 23-25
// 11111110 00000000 00000000 00000000 = reserve bits    = bits 26-32
*/

INLINE basic_move_t GenBasicMove(uint f, uint t, int pieceType, uint c) {return ((f) | ((t)<<6) | (pieceType<<12) | ((c)<<18));}
/* the move generator, this generates all legal moves*/
void genLegal(const position_t *pos, movelist_t *mvlist, int promoteAll) {
    movelist_t mlt;
    uint64 pinned;

    ASSERT(pos != NULL);
    ASSERT(mvlist != NULL);

    if (kingIsInCheck(pos)) {
        mvlist->pos = 0;
        genEvasions(pos, mvlist);
        if (promoteAll)
            for (mlt.pos = 0; mlt.pos < mvlist->size; mlt.pos++) {
                int mv = mvlist->list[mlt.pos].m;
                if (movePromote(mv)==QUEEN) { // makes up for not generating ROOK and BISHOP promotes
                    int from = moveFrom(mv);
                    int to = moveTo(mv);
                    if (moveCapture(mv)) {
                        mvlist->list[mvlist->size++].m =GenPromote(from, to, ROOK, getPiece(pos, to));
                        mvlist->list[mvlist->size++].m =GenPromote(from, to, BISHOP, getPiece(pos, to));
                    }
                    else {
                        mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, ROOK);
                        mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, BISHOP);
                    }
                }
            }
    } else {
        pinned = pinnedPieces(pos, pos->side);
        mvlist->size = 0;
        mlt.pos = 0;
        genCaptures(pos, &mlt);
        for (int i = 0; i < mlt.size; i++) {
            int mv = mlt.list[i].m;
            if (!moveIsLegal(pos, mv , pinned, false)) continue;
            mvlist->list[mvlist->size++] = mlt.list[i];
            if (promoteAll && movePromote(mv)==QUEEN) { // makes up for not generating ROOK and BISHOP promotes
                int from = moveFrom(mv);
                int to = moveTo(mv);
                if (moveCapture(mv)) {
                    mvlist->list[mvlist->size++].m =GenPromote(from, to, ROOK, getPiece(pos, to));
                    mvlist->list[mvlist->size++].m =GenPromote(from, to, BISHOP, getPiece(pos, to));
                }
                else {
                    mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, ROOK);
                    mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, BISHOP);
                }
            }
        }
        mlt.pos = mlt.size;
        genNonCaptures(pos, &mlt);
        for (int i = mlt.pos; i < mlt.size; i++) {
            if (!moveIsLegal(pos, mlt.list[i].m, pinned, false)) continue;
            mvlist->list[mvlist->size++] = mlt.list[i];
        }
    }
}

/* this generates non tactical non checking passed pawn moves only */
/*
void genPassedPawnMoves(const position_t *pos, movelist_t *mvlist) {
static const int Shift[] = {9, 7};
uint64 pc_bits, mv_bits, pawns, xpawns, mask, dcc;
int from, to;
int mover = pos->side;
int nonMover = mover ^ 1;
ASSERT(pos != NULL);
ASSERT(mvlist != NULL);

mvlist->size = 0;
pawns = pos->pawns & pos->color[mover];
xpawns = pos->pawns & pos->color[nonMover];
dcc = discoveredCheckCandidates(pos, mover);
mask = ~Rank8ByColorBB[mover] & ~((*FillPtr2[nonMover])((*ShiftPtr[nonMover])(pos->pawns, 8)))
& ~(*FillPtr2[nonMover])(((*ShiftPtr[nonMover])(xpawns, Shift[nonMover]) & ~FileABB)
| ((*ShiftPtr[nonMover])(xpawns, Shift[mover]) & ~FileHBB))
& ~PawnCaps[pos->kpos[nonMover]][nonMover];
// SAM I know this is inneficient, come back and do better if it works
#ifdef RAZOR_PAWN_THREATS
{
uint64 targets = (pos->rooks | pos->queens | pos->knights);
uint64 pawnAttacks = (((*ShiftPtr[nonMover])(targets, Shift[nonMover]) & ~FileABB) | ((*ShiftPtr[nonMover])(targets, Shift[mover]) & ~FileHBB));
mask |= pawnAttacks;
}
#endif
mv_bits = mask & (*ShiftPtr[pos->side])(pawns, 8) & ~pos->occupied;
while (mv_bits) {
to = popFirstBit(&mv_bits);
pc_bits = (PawnMoves[to][nonMover] & pawns) & ~dcc;
ASSERT(bitCnt(pc_bits) <= 1);
while (pc_bits) {
from = popFirstBit(&pc_bits);
mvlist->list[mvlist->size++].m = GenOneForward(from, to);
}
}
mv_bits = mask & Rank4ByColorBB[mover] & (*ShiftPtr[mover])(pawns, 16)
& (*ShiftPtr[mover])(~pos->occupied, 8) & ~pos->occupied;
while (mv_bits) {
to = popFirstBit(&mv_bits);
pc_bits = pawns & Rank2ByColorBB[mover] & FileMask[to] & ~dcc;
ASSERT(bitCnt(pc_bits) <= 1);
while (pc_bits) {
from = popFirstBit(&pc_bits);
mvlist->list[mvlist->size++].m = GenTwoForward(from, to);
}
}

}
*/
void genGainingMoves(const position_t *pos, movelist_t *mvlist, int delta, int thread_id) {
    int from, to;
    uint64 pc_bits,pc_bits_p1,pc_bits_p2, mv_bits;
    uint64 occupied = pos->occupied;
    uint64 allies = pos->color[pos->side];
    uint64 mask = ~occupied;

    ASSERT(pos != NULL);
    ASSERT(mvlist != NULL);

    //mvlist->size = 0;
    mvlist->size = mvlist->pos;

    if (pos->side == BLACK) { //TODO consider writing so we don't need this expensive branch
        if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenBlackOO())] >= delta && (pos->posStore.castle&BCKS) && (!(occupied&(F8|G8)))) {
            if (!isAtt(pos, pos->side^1, E8|F8|G8))
                mvlist->list[mvlist->size++].m = GenBlackOO();
        }
        if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenBlackOOO())] >= delta && (pos->posStore.castle&BCQS) && (!(occupied&(B8|C8|D8)))) {
            if (!isAtt(pos, pos->side^1, E8|D8|C8))
                mvlist->list[mvlist->size++].m = GenBlackOOO();
        }
        pc_bits_p1 = (pos->pawns & allies & ~Rank2BB) & ((~occupied)<<8);
        pc_bits_p2 = (pos->pawns & allies & Rank7BB) & ((~occupied)<<8) & ((~occupied)<<16);
    } else {
        if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenWhiteOO())] >= delta && (pos->posStore.castle&WCKS) && (!(occupied&(F1|G1)))) {
            if (!isAtt(pos, pos->side^1, E1|F1|G1))
                mvlist->list[mvlist->size++].m = GenWhiteOO();
        }
        if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenWhiteOOO())] >= delta && (pos->posStore.castle&WCQS) && (!(occupied&(B1|C1|D1)))) {
            if (!isAtt(pos, pos->side^1, E1|D1|C1))
                mvlist->list[mvlist->size++].m = GenWhiteOOO();
        }
        pc_bits_p1 = (pos->pawns & allies & ~Rank7BB) & ((~occupied)>>8);
        pc_bits_p2 = (pos->pawns & allies & Rank2BB) & ((~occupied)>>8) & ((~occupied)>>16);
    }
    /* pawn moves 1 forward, no promotions */
    while (pc_bits_p1) {
        from = popFirstBit(&pc_bits_p1);
        mv_bits = PawnMoves[from][pos->side];
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenOneForward(from, to))] >= delta)
                mvlist->list[mvlist->size++].m = GenOneForward(from, to);
        }
    }
    /* pawn moves 2 forward */
    while (pc_bits_p2) {
        from = popFirstBit(&pc_bits_p2);
        mv_bits = PawnMoves2[from][pos->side];
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenTwoForward(from, to))] >= delta)
                mvlist->list[mvlist->size++].m = GenTwoForward(from, to);
        }
    }
    pc_bits = pos->knights & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KnightMoves[from] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenKnightMove(from, to, EMPTY))] >= delta)
                mvlist->list[mvlist->size++].m = GenKnightMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->bishops & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = bishopAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenBishopMove(from, to, EMPTY))] >= delta)
                mvlist->list[mvlist->size++].m = GenBishopMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->rooks & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = rookAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenRookMove(from, to, EMPTY))] >= delta)
                mvlist->list[mvlist->size++].m = GenRookMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->queens & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = queenAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenQueenMove(from, to, EMPTY))] >= delta)
                mvlist->list[mvlist->size++].m = GenQueenMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->kings & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KingMoves[from] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            if (SearchInfo(thread_id).evalgains[historyIndex(pos->side, GenKingMove(from, to, EMPTY))] >= delta)
                mvlist->list[mvlist->size++].m = GenKingMove(from, to, EMPTY);
        }
    }

}

/* the move generator, this generates all pseudo-legal non tactical moves,
castling is generated legally */
void genNonCaptures(const position_t *pos, movelist_t *mvlist) {
    int from, to;
    uint64 pc_bits,pc_bits_1,pc_bits_2, mv_bits;
    uint64 occupied = pos->occupied;
    uint64 allies = pos->color[pos->side];
    uint64 mask = ~occupied;

    ASSERT(pos != NULL);
    ASSERT(mvlist != NULL);

    //mvlist->size = 0;
    mvlist->size = mvlist->pos;

    if (pos->side == BLACK) { //TODO consider writing so we don't need this expensive branch
        if ((pos->posStore.castle&BCKS) && (!(occupied&(F8|G8)))) {
            if (!isAtt(pos, pos->side^1, E8|F8|G8))
                mvlist->list[mvlist->size++].m = GenBlackOO();
        }
        if ((pos->posStore.castle&BCQS) && (!(occupied&(B8|C8|D8)))) {
            if (!isAtt(pos, pos->side^1, E8|D8|C8))
                mvlist->list[mvlist->size++].m = GenBlackOOO();
        }
        pc_bits_1 = (pos->pawns & allies & ~Rank2BB) & ((~occupied)<<8);
        pc_bits_2 = (pos->pawns & allies & Rank7BB) & ((~occupied)<<8) & ((~occupied)<<16);
    } else {
        if ((pos->posStore.castle&WCKS) && (!(occupied&(F1|G1)))) {
            if (!isAtt(pos, pos->side^1, E1|F1|G1))
                mvlist->list[mvlist->size++].m = GenWhiteOO();
        }
        if ((pos->posStore.castle&WCQS) && (!(occupied&(B1|C1|D1)))) {
            if (!isAtt(pos, pos->side^1, E1|D1|C1))
                mvlist->list[mvlist->size++].m = GenWhiteOOO();
        }
        pc_bits_1 = (pos->pawns & allies & ~Rank7BB) & ((~occupied)>>8);
        pc_bits_2 = (pos->pawns & allies & Rank2BB) & ((~occupied)>>8) & ((~occupied)>>16);
    }
    /* pawn moves 1 forward, no promotions */
    while (pc_bits_1) {
        from = popFirstBit(&pc_bits_1);
        mv_bits = PawnMoves[from][pos->side];
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenOneForward(from, to);
        }
    }
    /* pawn moves 2 forward */
    while (pc_bits_2) {
        from = popFirstBit(&pc_bits_2);
        mv_bits = PawnMoves2[from][pos->side];
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenTwoForward(from, to);
        }
    }
    pc_bits = pos->knights & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KnightMoves[from] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenKnightMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->bishops & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = bishopAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenBishopMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->rooks & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = rookAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenRookMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->queens & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = queenAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenQueenMove(from, to, EMPTY);
        }
    }
    pc_bits = pos->kings & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KingMoves[from] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenKingMove(from, to, EMPTY);
        }
    }

}

/* this generate captures including en-passant captures, and promotions*/
void genCaptures(const position_t *pos, movelist_t *mvlist) {
    int from, to;
    uint64 pc_bits, mv_bits;
    uint64 occupied = pos->occupied;
    uint64 allies = pos->color[pos->side];
    const uint64 mask = pos->color[pos->side^1] & ~pos->kings;
    ASSERT(pos != NULL);
    ASSERT(mvlist != NULL);

    //mvlist->size = 0;
    mvlist->size = mvlist->pos;

    /* promotions only */
    pc_bits = pos->pawns & allies & Rank7ByColorBB[pos->side];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = PawnMoves[from][pos->side] & ~occupied;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, QUEEN);
            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, KNIGHT);
            //            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, ROOK);
            //            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, BISHOP);
        }
        mv_bits = PawnCaps[from][pos->side] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenPromote(from, to, QUEEN, getPiece(pos, to));
            mvlist->list[mvlist->size++].m = GenPromote(from, to, KNIGHT, getPiece(pos, to));
            //          mvlist->list[mvlist->size++].m = GenPromote(from, to, ROOK, getPiece(pos, to));
            //          mvlist->list[mvlist->size++].m = GenPromote(from, to, BISHOP, getPiece(pos, to));
        }
    }
    /* pawn captures only */
    pc_bits = pos->pawns & allies & ~Rank7ByColorBB[pos->side];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = PawnCaps[from][pos->side] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenPawnMove(from, to, getPiece(pos, to));
        }
    }
    if ((pos->posStore.epsq != -1)) {
        mv_bits = pos->pawns & pos->color[pos->side]
        & PawnCaps[pos->posStore.epsq][pos->side^1];
        while (mv_bits) {
            from = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenEnPassant(from, pos->posStore.epsq);
        }
    }
    pc_bits = pos->knights & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KnightMoves[from] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenKnightMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->bishops & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = bishopAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenBishopMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->rooks & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = rookAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenRookMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->queens & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = queenAttacksBB(from, occupied) & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenQueenMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->kings & allies;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KingMoves[from] & mask;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenKingMove(from, to, getPiece(pos, to));
        }
    }
}

/* this generate legal moves when in check */
void genEvasions(const position_t *pos, movelist_t *mvlist) {
    int sqchecker, from, to, ksq, side, xside;
    uint64  pc_bits, mv_bits, enemies, friends, temp, checkers, pinned;

    ASSERT(pos != NULL);
    ASSERT(mvlist != NULL);

    //mvlist->size = 0;
    mvlist->size = mvlist->pos;

    side = pos->side;
    xside = side ^ 1;
    friends = pos->color[side];
    enemies = pos->color[xside];
    ksq = pos->kpos[side];
    checkers = attackingPiecesSide(pos, ksq, xside);
    mv_bits = KingMoves[ksq] & ~pos->color[side];

    while (mv_bits) {
        to = popFirstBit(&mv_bits);
        temp = pos->occupied ^ BitMask[ksq] ^ BitMask[to];
        if (((PawnCaps[to][side] & pos->pawns & enemies) == EmptyBoardBB) &&
            ((KnightMoves[to] & pos->knights & enemies) == EmptyBoardBB) &&
            ((KingMoves[to] & pos->kings & enemies) == EmptyBoardBB) &&
            ((bishopAttacksBB(to, temp) & pos->bishops & enemies) == EmptyBoardBB) &&
            ((rookAttacksBB(to, temp) & pos->rooks & enemies) == EmptyBoardBB) &&
            ((queenAttacksBB(to, temp) & pos->queens & enemies) == EmptyBoardBB))
            mvlist->list[mvlist->size++].m = GenKingMove(ksq, to, getPiece(pos, to));
    }

    if (checkers & (checkers - 1)) return;

    pinned = pinnedPieces(pos, side);
    sqchecker = getFirstBit(checkers);

    mv_bits = PawnCaps[sqchecker][xside] & pos->pawns & friends & ~pinned;
    while (mv_bits) {
        from = popFirstBit(&mv_bits);
        if ((Rank7ByColorBB[side] & BitMask[from])) {
            mvlist->list[mvlist->size++].m = GenPromote(from, sqchecker, QUEEN, getPiece(pos, sqchecker));
            mvlist->list[mvlist->size++].m = GenPromote(from, sqchecker, KNIGHT, getPiece(pos, sqchecker));
            mvlist->list[mvlist->size++].m = GenPromote(from, sqchecker, ROOK, getPiece(pos, sqchecker));
            mvlist->list[mvlist->size++].m = GenPromote(from, sqchecker, BISHOP, getPiece(pos, sqchecker));
        } else
            mvlist->list[mvlist->size++].m = GenPawnMove(from, sqchecker, getPiece(pos, sqchecker));
    }
    if ((BitMask[sqchecker] & pos->pawns & enemies) &&
        ((sqchecker + ((side == WHITE) ? (8) : (-8))) == pos->posStore.epsq)) {
            mv_bits = PawnCaps[pos->posStore.epsq][xside] & pos->pawns & friends & ~pinned;
            while (mv_bits) {
                from = popFirstBit(&mv_bits);
                mvlist->list[mvlist->size++].m = GenEnPassant(from, pos->posStore.epsq);
            }
    }
    mv_bits = KnightMoves[sqchecker] & pos->knights & friends & ~pinned;
    while (mv_bits) {
        from = popFirstBit(&mv_bits);
        mvlist->list[mvlist->size++].m = GenKnightMove(from, sqchecker, getPiece(pos, sqchecker));
    }
    mv_bits = bishopAttacksBB(sqchecker, pos->occupied) & pos->bishops & friends & ~pinned;
    while (mv_bits) {
        from = popFirstBit(&mv_bits);
        mvlist->list[mvlist->size++].m = GenBishopMove(from, sqchecker, getPiece(pos, sqchecker));
    }
    mv_bits = rookAttacksBB(sqchecker, pos->occupied) & pos->rooks & friends & ~pinned;
    while (mv_bits) {
        from = popFirstBit(&mv_bits);
        mvlist->list[mvlist->size++].m = GenRookMove(from, sqchecker, getPiece(pos, sqchecker));
    }
    mv_bits = queenAttacksBB(sqchecker, pos->occupied) & pos->queens & friends & ~pinned;
    while (mv_bits) {
        from = popFirstBit(&mv_bits);
        mvlist->list[mvlist->size++].m = GenQueenMove(from, sqchecker, getPiece(pos, sqchecker));
    }

    if (!(checkers & (pos->queens|pos->rooks|pos->bishops) & pos->color[xside])) return;

    temp = InBetween[sqchecker][ksq];

    pc_bits = pos->pawns & pos->color[side] & ~pinned;
    if (side == WHITE) mv_bits = (pc_bits << 8) & temp;
    else mv_bits = (pc_bits >> 8) & temp;
    while (mv_bits) {
        to = popFirstBit(&mv_bits);
        if (side == WHITE) from = to - 8;
        else from = to + 8;
        if ((Rank7ByColorBB[side] & BitMask[from])) {
            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, QUEEN);
            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, KNIGHT);
            //            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, ROOK);
            //            mvlist->list[mvlist->size++].m = GenPromoteStraight(from, to, BISHOP);
        } else
            mvlist->list[mvlist->size++].m = GenOneForward(from, to);
    }
    if (side == WHITE) mv_bits = (((pc_bits << 8) & ~pos->occupied & Rank3BB) << 8) & temp;
    else mv_bits = (((pc_bits >> 8) & ~pos->occupied & Rank6BB) >> 8) & temp;
    while (mv_bits) {
        to = popFirstBit(&mv_bits);
        if (side == WHITE) from = to - 16;
        else from = to + 16;
        mvlist->list[mvlist->size++].m = GenTwoForward(from, to);
    }
    pc_bits = pos->knights & friends & ~pinned;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = KnightMoves[from] & temp;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenKnightMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->bishops & friends & ~pinned;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = bishopAttacksBB(from, pos->occupied) & temp;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenBishopMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->rooks & friends & ~pinned;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = rookAttacksBB(from, pos->occupied) & temp;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenRookMove(from, to, getPiece(pos, to));
        }
    }
    pc_bits = pos->queens & friends & ~pinned;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        mv_bits = queenAttacksBB(from, pos->occupied) & temp;
        while (mv_bits) {
            to = popFirstBit(&mv_bits);
            mvlist->list[mvlist->size++].m = GenQueenMove(from, to, getPiece(pos, to));
        }
    }
    if ((pos->posStore.epsq != -1) && (checkers & pos->pawns & pos->color[xside])) {
        to = pos->posStore.epsq;
        mv_bits = pos->pawns & pos->color[side] & PawnCaps[to][xside] & ~pinned;
        while (mv_bits) {
            from = popFirstBit(&mv_bits);
            temp = pos->occupied ^ BitMask[sqchecker] ^ BitMask[from];
            if (((bishopAttacksBB(ksq, temp) & (pos->queens|pos->bishops)
                & pos->color[xside]) == EmptyBoardBB) &&
                ((rookAttacksBB(ksq, temp) & (pos->queens|pos->rooks)
                & pos->color[xside]) == EmptyBoardBB))
                mvlist->list[mvlist->size++].m = GenEnPassant(from, to);
        }
    }
}

/* this generates all pseudo-legal non-capturing, non-promoting checks */
void genQChecks(const position_t *pos, movelist_t *mvlist) {
    int us, them, ksq, from, to;
    uint64 dc, empty, checkSqs, bit1, bit2, bit3;

    //mvlist->size = 0;
    mvlist->size = mvlist->pos;

    us = pos->side;
    them = us^1;

    ksq = pos->kpos[them];
    dc = discoveredCheckCandidates(pos, us);
    empty = ~pos->occupied;
    if (us == WHITE) {
        bit1 = pos->pawns & pos->color[us] & ~FileBB[SQFILE(ksq)];
        bit2 = bit3 = ((bit1 & dc) << 8) & ~Rank8BB & empty;
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenOneForward(to-8, to);
        }
        bit3 = ((bit2 & Rank3BB) << 8) & empty;
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenTwoForward(to-16, to);
        }
        bit1 &= ~dc;
        if (SQFILE(ksq) > FileA) bit1 &= FileBB[SQFILE(ksq)-1];
        if (SQFILE(ksq) < FileH) bit1 &= FileBB[SQFILE(ksq)+1];
        bit2 = (bit1 << 8) & empty;
        bit3 = bit2 & PawnCaps[ksq][BLACK];
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenOneForward(to-8, to);
        }
        bit3 = ((bit2 & Rank3BB) << 8) & empty & PawnCaps[ksq][BLACK];
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenTwoForward(to-16, to);
        }
    } else {
        bit1 = pos->pawns & pos->color[us] & ~FileBB[SQFILE(ksq)];
        bit2 = bit3 = ((bit1 & dc) >> 8) & ~Rank1BB & empty;
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenOneForward(to+8, to);
        }
        bit3 = ((bit2 & Rank6BB) >> 8) & empty;
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenTwoForward(to+16, to);
        }
        bit1 &= ~dc;
        if (SQFILE(ksq) > FileA) bit1 &= FileBB[SQFILE(ksq)-1];
        if (SQFILE(ksq) < FileH) bit1 &= FileBB[SQFILE(ksq)+1];
        bit2 = (bit1 >> 8) & empty;
        bit3 = bit2 & PawnCaps[ksq][WHITE];
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenOneForward(to+8, to);
        }
        bit3 = ((bit2 & Rank6BB) >> 8) & empty & PawnCaps[ksq][WHITE];
        while (bit3) {
            to = popFirstBit(&bit3);
            mvlist->list[mvlist->size++].m = GenTwoForward(to+16, to);
        }
    }
    bit1 = pos->knights & pos->color[us];
    if (bit1) {
        bit2 = bit1 & dc;
        while (bit2) {
            from = popFirstBit(&bit2);
            bit3 = KnightMoves[from] & empty;
            while (bit3) {
                to = popFirstBit(&bit3);
                mvlist->list[mvlist->size++].m = GenKnightMove(from, to, getPiece(pos, to));
            }
        }
        bit2 = bit1 & ~dc;
        checkSqs = KnightMoves[ksq] & empty;
        while (bit2) {
            from = popFirstBit(&bit2);
            bit3 = KnightMoves[from] & checkSqs;
            while (bit3) {
                to = popFirstBit(&bit3);
                mvlist->list[mvlist->size++].m = GenKnightMove(from, to, getPiece(pos, to));
            }
        }
    }
    bit1 = pos->bishops & pos->color[us];
    if (bit1) {
        bit2 = bit1 & dc;
        while (bit2) {
            from = popFirstBit(&bit2);
            bit3 = bishopAttacksBB(from, pos->occupied) & empty;
            while (bit3) {
                to = popFirstBit(&bit3);
                mvlist->list[mvlist->size++].m = GenBishopMove(from, to, getPiece(pos, to));
            }
        }
        bit2 = bit1 & ~dc;
        checkSqs = bishopAttacksBB(ksq, pos->occupied) & empty;
        while (bit2) {
            from = popFirstBit(&bit2);
            bit3 = bishopAttacksBB(from, pos->occupied) & checkSqs;
            while (bit3) {
                to = popFirstBit(&bit3);
                mvlist->list[mvlist->size++].m = GenBishopMove(from, to, getPiece(pos, to));
            }
        }
    }
    bit1 = pos->rooks & pos->color[us];
    if (bit1) {
        bit2 = bit1 & dc;
        while (bit2) {
            from = popFirstBit(&bit2);
            bit3 = rookAttacksBB(from, pos->occupied) & empty;
            while (bit3) {
                to = popFirstBit(&bit3);
                mvlist->list[mvlist->size++].m = GenRookMove(from, to, getPiece(pos, to));
            }
        }

        bit2 = bit1 & ~dc;
        checkSqs = rookAttacksBB(ksq, pos->occupied) & empty;
        while (bit2) {
            from = popFirstBit(&bit2);
            bit3 = rookAttacksBB(from, pos->occupied) & checkSqs;
            while (bit3) {
                to = popFirstBit(&bit3);
                mvlist->list[mvlist->size++].m = GenRookMove(from, to, getPiece(pos, to));
            }
        }
    }
    bit1 = pos->queens & pos->color[us];
    if (bit1) {
        checkSqs = queenAttacksBB(ksq, pos->occupied) & empty;
        while (bit1) {
            from = popFirstBit(&bit1);
            bit2 = queenAttacksBB(from, pos->occupied) & checkSqs;
            while (bit2) {
                to = popFirstBit(&bit2);
                mvlist->list[mvlist->size++].m = GenQueenMove(from, to, getPiece(pos, to));
            }
        }
    }
    if (BitMask[pos->kpos[us]] & dc) {
        bit2 = KingMoves[pos->kpos[us]] & empty;
        bit2 &= ~DirBitmap[/*getDirIndex(*/DirFromTo[ksq][pos->kpos[us]]/*)*/][ksq];
        while (bit2) {
            to = popFirstBit(&bit2);
            mvlist->list[mvlist->size++].m = GenKingMove(pos->kpos[us], to, getPiece(pos, to));
        }
    }
    if (((us == WHITE) ? (pos->posStore.castle & WCKS) : (pos->posStore.castle & BCKS))
        && (!(pos->occupied & ((us == WHITE) ? (F1|G1) : (F8|G8))))
        && !isAtt(pos, pos->side^1, (us == WHITE) ? (E1|F1|G1) : (E8|F8|G8))) {
            bit1 = pos->occupied ^ BitMask[(us == WHITE) ? e1 : e8] ^
                BitMask[(us == WHITE) ? g1 : g8];
            if (rookAttacksBB(ksq, bit1) & BitMask[(us == WHITE) ? f1 : f8])
                mvlist->list[mvlist->size++].m = (us == WHITE) ? GenWhiteOO() : GenBlackOO();
    }
    if (((us == WHITE) ? (pos->posStore.castle & WCQS) : (pos->posStore.castle & BCQS))
        && (!(pos->occupied & ((us == WHITE) ? (B1|C1|D1) : (B8|C8|D8))))
        && !isAtt(pos, pos->side^1, (us == WHITE) ? (E1|D1|C1) : (E8|D8|C8))) {
            bit1 = pos->occupied ^ BitMask[(us == WHITE) ? e1 : e8] ^
                BitMask[(us == WHITE) ? c1 : c8];
            if (rookAttacksBB(ksq, bit1) & BitMask[(us == WHITE) ? d1 : d8])
                mvlist->list[mvlist->size++].m = (us == WHITE) ? GenWhiteOOO() : GenBlackOOO();
    }
}

/* this determines if a not necessarily pseudo move is legal or not,
this must not be called when the position is in check,  */
//SAM consider rewriting to ensure no crash, but not really check legality well
uint32 genMoveIfLegal(const position_t *pos, uint32 move, uint64 pinned) {
    int from, to, pc, capt, prom, me, opp;
    int inc, delta;
    uint64 occupied;

    from = moveFrom(move);
    to = moveTo(move);
    pc = movePiece(move);
    capt = moveCapture(move);
    prom = movePromote(move);
    me = pos->side;
    opp = me ^ 1;
    occupied = pos->occupied;

    if (from == to) return false;
    if (from < a1 || from > h8) return false;
    if (to < a1 || to > h8) return false;
    if (pc < PAWN || pc > KING) return false;
    if (capt < EMPTY || capt > QUEEN) return false;
    if (pc != PAWN && prom != EMPTY) return false;

    if (move == EMPTY || getPiece(pos, from) != pc || DiffColor(pos, from, me) ||
        ((pinned & BitMask[from]) && (DirFromTo[from][pos->kpos[me]] != DirFromTo[to][pos->kpos[me]]))) return false;
    switch (pc) {
    case PAWN:
        if (isEnPassant(move)) {
            inc = ((me == WHITE) ? -8 : 8);
            if (to != pos->posStore.epsq || getPiece(pos, pos->posStore.epsq + inc) != PAWN || DiffColor(pos, pos->posStore.epsq + inc,opp)) return false;
            {
                uint64 b = pos->occupied ^ BitMask[from] ^ BitMask[pos->posStore.epsq + inc] ^ BitMask[to];
                int ksq =  pos->kpos[me];
                return
                    (!(rookAttacksBB(ksq, b) & (pos->queens | pos->rooks) & pos->color[opp]) &&
                    !(bishopAttacksBB(ksq, b) & (pos->queens | pos->bishops) & pos->color[opp]));
            }
        }
        else {
            if (prom > QUEEN || (getPiece(pos, to) != capt) || capt && DiffColor(pos, to,opp)) return false;
            inc = ((me == WHITE) ? 8 : -8);
            delta = to - from;
            if (capt == EMPTY) {
                if (!(delta == inc) && !(delta == (2*inc)
                    && PAWN_RANK(from, me) == Rank2
                    && getPiece(pos, from+inc) == EMPTY)) return false;
            } else {
                if (!(delta == (inc-1)) && !(delta == (inc+1))) return false;
            }
            return ((prom!=0) == (PAWN_RANK(to, me) == Rank8));
        }
        break;

    case KNIGHT:
        if (moveAction(move) != KNIGHT || (getPiece(pos, to) != capt) || capt && DiffColor(pos, to, opp) || !(KnightMoves[from] & BitMask[to])) return false;
        return true;
        break;
    case BISHOP:
        if (moveAction(move) != BISHOP || (getPiece(pos, to) != capt) || capt && DiffColor(pos, to, opp) || !(bishopAttacksBB(from, occupied) & BitMask[to])) return false;
        return true;
        break;

    case ROOK:
        if (moveAction(move) != ROOK || (getPiece(pos, to) != capt) || capt && DiffColor(pos, to, opp) || !(rookAttacksBB(from, occupied) & BitMask[to])) return false;
        return true;
        break;

    case QUEEN:
        if (moveAction(move) != QUEEN || (getPiece(pos, to) != capt) || capt && DiffColor(pos, to, opp) || !(queenAttacksBB(from, occupied) & BitMask[to])) return false;
        return true;
        break;

    case KING:
        if ((getPiece(pos, to) != capt) || capt && DiffColor(pos, to, opp) ) return false;
        if (isCastle(move)) {
            if (me == WHITE) {
                if (from!=e1) return false;
                if (to == g1) {
                    if (!(pos->posStore.castle & WCKS) || (occupied&(F1|G1)) || isAtt(pos, opp, E1|F1|G1)) return false;
                }
                else if (to == c1) {
                    if (!(pos->posStore.castle & WCQS) || (occupied&(B1|C1|D1)) || isAtt(pos, opp, C1|D1|E1)) return false;
                }
                else return false;
            } else {
                if (from!=e8) return false;
                if (to == g8) {
                    if (!(pos->posStore.castle & BCKS) || (occupied&(F8|G8)) || isAtt(pos, opp, E8|F8|G8)) return false;
                }
                else if (to==c8) {
                    if (!(pos->posStore.castle & BCQS) || (occupied&(B8|C8|D8)) || isAtt(pos, opp, C8|D8|E8)) return false;
                }
                else return false;
            }
            return true;
        }//isSqAtt(const position_t *pos, uint64 occ, int sq,int color) {
        else if (!(KingMoves[from] & BitMask[to]) || isSqAtt(pos,pos->occupied ^ (pos->kings & pos->color[me]),to,opp)) return false;
        return true;
        break;
    }
    return false;
}
