/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "macros.h"
#include "data.h"
#include "constants.h"
#include "attacks.h"

/* the following routines returns a 64 bit xray attacks of pieces
on the From square with the limiting Occupied bits */
inline uint64 bishopAttacksBBX(const uint32 from, const uint64 occ) {
    return bishopAttacksBB(from, occ & ~(bishopAttacksBB(from, occ) & occ));
}

inline uint64 rookAttacksBBX(const uint32 from, const uint64 occ) {
    return rookAttacksBB(from, occ & ~(rookAttacksBB(from, occ) & occ));
}

/* the following routines returns a 64 bit attacks of pieces
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

/* this is the attack routine, capable of multiple targets,
can be used to determine if the bits on the 64 bit parameter
is being attacked by the side color */
bool isAtt(const position_t& pos, const uint32 color, uint64 target) {
    int from;

    ASSERT(target);
    ASSERT(colorIsOk(color));

    while (target) {
        from = popFirstBit(&target);
        if ((pos.rooks | pos.queens) & pos.color[color]
            & rookAttacksBB(from, pos.occupied)) return true;
        if ((pos.bishops | pos.queens) & pos.color[color]
            & bishopAttacksBB(from, pos.occupied)) return true;
        if (pos.knights & pos.color[color] & KnightMoves[from]) return true;
        if (pos.kings & pos.color[color] & KingMoves[from]) return true;
        if (pos.pawns & pos.color[color] & PawnCaps[from][color ^ 1]) return true;
    }
    return false;
}
bool isSqAtt(const position_t& pos, uint64 occ, int sq, int color) {
    return ((PawnCaps[sq][color ^ 1] & pos.pawns & pos.color[color])) ||
        (KnightMoves[sq] & pos.knights & pos.color[color]) ||
        (KingMoves[sq] & pos.kings & pos.color[color]) ||
        (bishopAttacksBB(sq, occ) & ((pos.bishops | pos.queens) & pos.color[color])) ||
        (rookAttacksBB(sq, occ) & ((pos.rooks | pos.queens) & pos.color[color]));
}
uint64 pieceAttacksFromBB(const position_t& pos, const int pc, const int sq, const uint64 occ) {
    switch (pc) {
    case PAWN: return PawnCaps[sq][pos.side];
    case KNIGHT: return KnightMoves[sq];
    case BISHOP: return bishopAttacksBB(sq, occ);
    case ROOK: return rookAttacksBB(sq, occ);
    case QUEEN: return queenAttacksBB(sq, occ);
    case KING: return KingMoves[sq];
    }
    return 0;
}

/* this determines if the side to move is in check */
inline bool kingIsInCheck(const position_t& pos) {
    return isSqAtt(pos, pos.occupied, pos.kpos[pos.side], pos.side ^ 1);
}
/* this returns the pinned pieces to the King of the side Color */
uint64 pinnedPieces(const position_t& pos, uint32 c) { //SAM TODO this could be done more efficiently I think?
    uint64 b, pin, pinners;
    int t, ksq;
    pin = 0ULL;
    ksq = pos.kpos[c];
    pinners = pos.color[c ^ 1] & (pos.queens | pos.rooks);
    if (pinners) {
        b = rookAttacksBBX(ksq, pos.occupied) & pinners;
        while (b) {
            t = popFirstBit(&b);
            pin |= InBetween[t][ksq] & pos.color[c];
        }
    }
    pinners = pos.color[c ^ 1] & (pos.queens | pos.bishops);
    if (pinners) {
        b = bishopAttacksBBX(ksq, pos.occupied) & pinners;
        while (b) {
            t = popFirstBit(&b);
            pin |= InBetween[t][ksq] & pos.color[c];
        }
    }
    return pin;
}
/* this returns the discovered check candidates pieces to the King of
the opposite side of Color */
uint64 discoveredCheckCandidates(const position_t& pos, uint32 c) {
    uint64 b, pin, pinners;
    int t, ksq;
    pin = 0ULL;
    ksq = pos.kpos[c ^ 1];
    pinners = pos.color[c] & (pos.queens | pos.rooks);
    if (pinners) {
        b = rookAttacksBBX(ksq, pos.occupied) & pinners;
        while (b) {
            t = popFirstBit(&b);
            pin |= InBetween[t][ksq] & pos.color[c];
        }
    }
    pinners = pos.color[c] & (pos.queens | pos.bishops);
    if (pinners) {
        b = bishopAttacksBBX(ksq, pos.occupied) & pinners;
        while (b) {
            t = popFirstBit(&b);
            pin |= InBetween[t][ksq] & pos.color[c];
        }
    }
    return pin;
}

/* this determines if a pseudo-legal move is legal without executing
makeMove */
bool moveIsLegal(const position_t& pos, uint32 move, uint64 pinned, bool incheck) {
    int us, them, ksq, from, to, capsq;
    uint64 b;

    ASSERT(moveIsOk(move));

    if (incheck) return true;
    if (isCastle(move)) return true;

    us = pos.side;
    them = us ^ 1;
    from = moveFrom(move);
    to = moveTo(move);
    ksq = pos.kpos[us];

    if (isEnPassant(move)) {
        capsq = (SQRANK(from) << 3) + SQFILE(to);
        b = pos.occupied ^ BitMask[from] ^ BitMask[capsq] ^ BitMask[to];
        return
            (!(rookAttacksBB(ksq, b) & (pos.queens | pos.rooks) & pos.color[them]) &&
                !(bishopAttacksBB(ksq, b) & (pos.queens | pos.bishops) & pos.color[them]));
    }
    if (from == ksq) return !(isSqAtt(pos, pos.occupied ^ (pos.kings&pos.color[us]), to, them));
    if (!(pinned & BitMask[from])) return true;
    if (DirFromTo[from][ksq] == DirFromTo[to][ksq]) return true;
    return false;
}

/* this determines if a move gives check */
bool moveIsCheck(const position_t& pos, basic_move_t m, uint64 dcc) {
    int us, them, ksq, from, to;
    uint64 temp;
    us = pos.side;
    them = us ^ 1;

    ASSERT(moveIsOk(m));

    from = moveFrom(m);
    to = moveTo(m);
    ksq = pos.kpos[them];

    switch (movePiece(m)) {
    case PAWN:
        if (PawnCaps[ksq][them] & BitMask[to]) return true;
        else if ((dcc & BitMask[from]) && DirFromTo[from][ksq] != DirFromTo[to][ksq]) return true;
        temp = pos.occupied ^ BitMask[from] ^ BitMask[to];
        switch (movePromote(m)) {
        case EMPTY:
            break;
        case KNIGHT:
            if (KnightMoves[ksq] & BitMask[to]) return true;
            return false;
        case BISHOP:
            if (bishopAttacksBB(ksq, temp) & BitMask[to]) return true;
            return false;
        case ROOK:
            if (rookAttacksBB(ksq, temp) & BitMask[to]) return true;
            return false;
        case QUEEN:
            if (queenAttacksBB(ksq, temp) & BitMask[to]) return true;
            return false;
        }
        if (isEnPassant(m)) {
            temp ^= BitMask[(SQRANK(from) << 3) + SQFILE(to)];
            if ((rookAttacksBB(ksq, temp) & (pos.queens | pos.rooks)
                & pos.color[us]) || (bishopAttacksBB(ksq, temp) & (pos.queens | pos.bishops) & pos.color[us]))
                return true;
        }
        return false;

    case KNIGHT:
        if ((dcc & BitMask[from])) return true;
        else if (KnightMoves[ksq] & BitMask[to]) return true;
        return false;

    case BISHOP:
        if ((dcc & BitMask[from])) return true;
        else if (bishopAttacksBB(ksq, pos.occupied) & BitMask[to]) return true;
        return false;

    case ROOK:
        if ((dcc & BitMask[from])) return true;
        else if (rookAttacksBB(ksq, pos.occupied) & BitMask[to]) return true;
        return false;

    case QUEEN:
        ASSERT(!(dcc & BitMask[from]));
        if (queenAttacksBB(ksq, pos.occupied) & BitMask[to]) return true;
        return false;

    case KING:
        if ((dcc & BitMask[from]) &&
            DirFromTo[from][ksq] != DirFromTo[to][ksq]) return true;
        temp = pos.occupied ^ BitMask[from] ^ BitMask[to];
        if (from - to == 2) {
            if (rookAttacksBB(ksq, temp) & BitMask[from - 1]) return true;
        }
        if (from - to == -2) {
            if (rookAttacksBB(ksq, temp) & BitMask[from + 1]) return true;
        }
        return false;

    default:
        ASSERT(false);
        return false;
    }
}

/* this returns the bitboard of all pieces of a given side attacking a certain square */

uint64 attackingPiecesSide(const position_t& pos, uint32 sq, uint32 side) {
    ASSERT(squareIsOk(sq));
    ASSERT(colorIsOk(side));

    uint64 attackers = PawnCaps[sq][side ^ 1] & pos.pawns;
    attackers |= KnightMoves[sq] & pos.knights;
    attackers |= KingMoves[sq] & pos.kings;
    attackers |= bishopAttacksBB(sq, pos.occupied) & (pos.bishops | pos.queens);
    attackers |= rookAttacksBB(sq, pos.occupied) & (pos.rooks | pos.queens);
    return (attackers & pos.color[side]);
}

/* this is the Static Exchange Evaluator */
/* this returns the bitboard of sliding pieces attacking in a direction
behind the piece attacker */
uint64 behindFigure(const position_t& pos, uint32 from, int dir) {
    ASSERT(squareIsOk(from));
    switch (dir) {
    case SW: return bishopAttacksBB(from, pos.occupied) & (pos.queens | pos.bishops) & DirBitmap[SW][from];
    case W: return rookAttacksBB(from, pos.occupied) & (pos.queens | pos.rooks) & DirBitmap[W][from];
    case NW: return bishopAttacksBB(from, pos.occupied) & (pos.queens | pos.bishops) & DirBitmap[NW][from];
    case N: return rookAttacksBB(from, pos.occupied) & (pos.queens | pos.rooks) & DirBitmap[N][from];
    case NE: return bishopAttacksBB(from, pos.occupied) & (pos.queens | pos.bishops) & DirBitmap[NE][from];
    case E: return rookAttacksBB(from, pos.occupied) & (pos.queens | pos.rooks) & DirBitmap[E][from];
    case SE: return bishopAttacksBB(from, pos.occupied) & (pos.queens | pos.bishops) & DirBitmap[SE][from];
    case S: return rookAttacksBB(from, pos.occupied) & (pos.queens | pos.rooks) & DirBitmap[S][from];
    }
    return 0;
}
int swap(const position_t& pos, uint32 m) {
    int to = moveTo(m);
    int from = moveFrom(m);
    int pc = movePiece(m);
    int promoValue = (movePromote(m) != 0) * (PcValSEE[QUEEN] - PcValSEE[PAWN]); // always Q or not enter
    int lastvalue = PcValSEE[pc] + promoValue;
    int n = 1;
    int lsb;
    int c = pos.side ^ 1;
    int dir;// = Direction[to][from];
    int slist[32];
    int promBonus = (to >= a8 || to <= h1)*(PcValSEE[QUEEN] - PcValSEE[PAWN]);
    uint64 attack;

    uint64 occ;

    ASSERT(moveIsOk(m));

    slist[0] = PcValSEE[moveCapture(m)] + promoValue;
    occ = pos.occupied ^  BitMask[from];

    if (isEnPassant(m)) occ ^= BitMask[to + ((pos.side == WHITE) ? -8 : 8)];
    //	attack = attackingPiecesAll(pos, occ, to) & occ;
    attack = PawnCaps[to][BLACK] & pos.pawns & pos.color[WHITE];
    attack |= PawnCaps[to][WHITE] & pos.pawns & pos.color[BLACK];
    attack |= KnightMoves[to] & pos.knights;
    attack |= KingMoves[to] & pos.kings;
    attack |= bishopAttacksBB(to, occ) & (pos.bishops | pos.queens);
    attack |= rookAttacksBB(to, occ) & (pos.rooks | pos.queens);
    attack &= occ;

    while (attack) {
        if (attack & pos.pawns & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue + promBonus;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[PAWN] + promBonus;
            lsb = getFirstBit(attack & pos.pawns & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.knights & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[KNIGHT];

            lsb = getFirstBit(attack & pos.knights & pos.color[c]);
        }
        else if (attack & pos.bishops & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[BISHOP];

            lsb = getFirstBit(attack & pos.bishops & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.rooks & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[ROOK];

            lsb = getFirstBit(attack & pos.rooks & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.queens & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[QUEEN];
            lsb = getFirstBit(attack & pos.queens & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.kings & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[KING];
            lsb = getFirstBit(attack & pos.kings & pos.color[c]);
        }
        else break;

        attack ^= BitMask[lsb];
        n++;
        c ^= 1;
    }
    while (--n) {
        if (slist[n] > -slist[n - 1])
            slist[n - 1] = -slist[n];
    }
    return slist[0];
}
int swapSquare(const position_t& pos, uint32 m) {
    int to = moveFrom(m);
    int pc = movePiece(m);
    int lastvalue = PcValSEE[pc]; // consider PcValSEE[QUEEN]
    int n = 1;
    int lsb;
    int c = pos.side ^ 1;
    int dir;
    int slist[32];
    int promBonus = (to >= a8 || to <= h1)*(PcValSEE[QUEEN] - PcValSEE[PAWN]);
    uint64 attack;

    ASSERT(moveIsOk(m));

    //attack = attackingPiecesAll(pos, pos.occupied,to);
    attack = PawnCaps[to][BLACK] & pos.pawns & pos.color[WHITE];
    attack |= PawnCaps[to][WHITE] & pos.pawns & pos.color[BLACK];
    attack |= KnightMoves[to] & pos.knights;
    attack |= KingMoves[to] & pos.kings;
    attack |= bishopAttacksBB(to, pos.occupied) & (pos.bishops | pos.queens);
    attack |= rookAttacksBB(to, pos.occupied) & (pos.rooks | pos.queens);

    // promotion is important for first move, not sure about rest
    slist[0] = 0;

    while (attack) {
        if (attack & pos.pawns & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue + promBonus;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[PAWN] + promBonus;
            lsb = getFirstBit(attack & pos.pawns & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.knights & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[KNIGHT];

            lsb = getFirstBit(attack & pos.knights & pos.color[c]);
        }
        else if (attack & pos.bishops & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[BISHOP];

            lsb = getFirstBit(attack & pos.bishops & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.rooks & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[ROOK];

            lsb = getFirstBit(attack & pos.rooks & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.queens & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[QUEEN];
            lsb = getFirstBit(attack & pos.queens & pos.color[c]);
            dir = DirFromTo[to][lsb];
            attack |= behindFigure(pos, lsb, dir);
        }
        else if (attack & pos.kings & pos.color[c]) {
            slist[n] = -slist[n - 1] + lastvalue;
            if (lastvalue == PcValSEE[KING]) break;
            lastvalue = PcValSEE[KING];
            lsb = getFirstBit(attack & pos.kings & pos.color[c]);
        }
        else break;

        attack ^= BitMask[lsb];
        n++;
        c ^= 1;
    }
    while (--n) {
        if (slist[n] > -slist[n - 1])
            slist[n - 1] = -slist[n];
    }
    return slist[0];
}