/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "position.h"
#include "utils.h"
#include "bitutils.h"

/* this undos the null move done */
void unmakeNullMove(position_t& pos, pos_store_t& undo) {

    ASSERT(pos != NULL);

    --pos.ply;
    --pos.sp;
    pos.side ^= 1;

    pos.posStore = undo;
}

/* this updates the position structure from the null move being played */
void makeNullMove(position_t& pos, pos_store_t& undo) {
    ASSERT(pos != NULL);

    undo = pos.posStore;

    pos.posStore.previous = &undo;
    pos.posStore.lastmove = EMPTY;
    pos.posStore.epsq = -1;
    pos.posStore.fifty += 1;
    pos.posStore.pliesFromNull = 0;

    if (undo.epsq != -1) pos.posStore.hash ^= ZobEpsq[SQFILE(undo.epsq)];

    pos.posStore.hash ^= ZobColor;
    ++pos.ply;
    ++pos.sp;
    pos.side ^= 1;
}

/* this undos the move done */
void unmakeMove(position_t& pos, pos_store_t& undo) {
    unsigned int side, xside, m, rook_from = 0, rook_to = 0, epsq = 0, from, to;

    ASSERT(pos != NULL);

    m = pos.posStore.lastmove;

    --pos.ply;
    --pos.sp;
    xside = pos.side;
    side = xside ^ 1;
    pos.side = side;

    pos.posStore = undo;

    from = moveFrom(m);
    to = moveTo(m);
    switch (moveAction(m)) {
    case PAWN:
        pos.pieces[from] = PAWN;
        pos.pawns ^= (BitMask[from] | BitMask[to]);
        break;
    case KNIGHT:
        pos.pieces[from] = KNIGHT;
        pos.knights ^= (BitMask[from] | BitMask[to]);
        break;
    case BISHOP:
        pos.pieces[from] = BISHOP;
        pos.bishops ^= (BitMask[from] | BitMask[to]);
        break;
    case ROOK:
        pos.pieces[from] = ROOK;
        pos.rooks ^= (BitMask[from] | BitMask[to]);
        break;
    case QUEEN:
        pos.pieces[from] = QUEEN;
        pos.queens ^= (BitMask[from] | BitMask[to]);
        break;
    case KING:
        pos.pieces[from] = KING;
        pos.kpos[side] = from;
        pos.kings ^= (BitMask[from] | BitMask[to]);
        break;
    case CASTLE:
        pos.pieces[from] = KING;
        pos.kpos[side] = from;
        pos.kings ^= (BitMask[from] | BitMask[to]);
        switch (to) {
        case c1:
            rook_from = a1;
            rook_to = d1;
            break;
        case g1:
            rook_from = h1;
            rook_to = f1;
            break;
        case c8:
            rook_from = a8;
            rook_to = d8;
            break;
        case g8:
            rook_from = h8;
            rook_to = f8;
            break;
        default:
            ASSERT(false);
            break;
        }
        pos.pieces[rook_from] = ROOK;
        pos.pieces[rook_to] = EMPTY;
        pos.rooks ^= (BitMask[rook_from] | BitMask[rook_to]);
        pos.color[side] ^= (BitMask[rook_from] | BitMask[rook_to]);
        break;
    case TWOFORWARD:
        pos.pieces[from] = PAWN;
        pos.pawns ^= (BitMask[from] | BitMask[to]);
        break;
    case PROMOTE:
        pos.pieces[from] = PAWN;
        pos.pawns ^= BitMask[from];
        switch (movePromote(m)) {
        case KNIGHT:
            pos.knights ^= BitMask[to];
            break;
        case BISHOP:
            pos.bishops ^= BitMask[to];
            break;
        case ROOK:
            pos.rooks ^= BitMask[to];
            break;
        case QUEEN:
            pos.queens ^= BitMask[to];
            break;
        }
        break;
    }
    switch (moveRemoval(m)) {
    case EMPTY:
        pos.pieces[to] = EMPTY;
        break;
    case PAWN:
        pos.pieces[to] = PAWN;
        pos.pawns ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        break;
    case KNIGHT:
        pos.pieces[to] = KNIGHT;
        pos.knights ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        break;
    case BISHOP:
        pos.pieces[to] = BISHOP;
        pos.bishops ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        break;
    case ROOK:
        pos.pieces[to] = ROOK;
        pos.rooks ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        break;
    case QUEEN:
        pos.pieces[to] = QUEEN;
        pos.queens ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        break;
    case ENPASSANT:
        pos.pieces[to] = EMPTY;
        switch (side) {
        case WHITE:
            epsq = to - 8;
            break;
        case BLACK:
            epsq = to + 8;
            break;
        }
        pos.pieces[epsq] = PAWN;
        pos.pawns ^= BitMask[epsq];
        pos.color[xside] ^= BitMask[epsq];
        break;
    }
    pos.color[side] ^= (BitMask[from] | BitMask[to]);
    pos.occupied = pos.color[side] | pos.color[xside];
}

/* this updates the position structure from the move being played */
void makeMove(position_t& pos, pos_store_t& undo, basic_move_t m) {
    unsigned int rook_from = 0, rook_to = 0, epsq = 0, prom, from, to, side, xside;

    ASSERT(pos != NULL);
    ASSERT(moveIsOk(m));

    from = moveFrom(m);
    to = moveTo(m);
    side = pos.side;
    xside = side ^ 1;

    undo = pos.posStore;

#ifdef LAZY
    pos.posStore.sinceLazy++;
#endif

    pos.posStore.previous = &undo;
    pos.posStore.lastmove = m;
    pos.posStore.epsq = -1;
    pos.posStore.castle = undo.castle & CastleMask[from] & CastleMask[to];
    pos.posStore.fifty += 1;
    pos.posStore.pliesFromNull += 1;

    if (undo.epsq != -1) pos.posStore.hash ^= ZobEpsq[SQFILE(undo.epsq)];
    pos.posStore.hash ^= ZobCastle[pos.posStore.castle ^ undo.castle];
    pos.posStore.phash ^= ZobCastle[pos.posStore.castle ^ undo.castle];

    pos.posStore.hash ^= ZobColor;

    switch (moveAction(m)) {
    case PAWN:
        pos.pieces[to] = PAWN;
        pos.pawns ^= (BitMask[from] | BitMask[to]);
        pos.posStore.hash ^= ZobPiece[side][PAWN][from];
        pos.posStore.hash ^= ZobPiece[side][PAWN][to];
        pos.posStore.phash ^= ZobPiece[side][PAWN][from];
        pos.posStore.phash ^= ZobPiece[side][PAWN][to];
        pos.posStore.open[side] += PST(side, PAWN, to, MIDGAME) - PST(side, PAWN, from, MIDGAME);
        pos.posStore.end[side] += PST(side, PAWN, to, ENDGAME) - PST(side, PAWN, from, ENDGAME);
        pos.posStore.fifty = 0;
        break;
    case KNIGHT:
        pos.pieces[to] = KNIGHT;
        pos.knights ^= (BitMask[from] | BitMask[to]);
        pos.posStore.hash ^= ZobPiece[side][KNIGHT][from];
        pos.posStore.hash ^= ZobPiece[side][KNIGHT][to];
        pos.posStore.open[side] += PST(side, KNIGHT, to, MIDGAME) - PST(side, KNIGHT, from, MIDGAME);
        pos.posStore.end[side] += PST(side, KNIGHT, to, ENDGAME) - PST(side, KNIGHT, from, ENDGAME);
        break;
    case BISHOP:
        pos.pieces[to] = BISHOP;
        pos.bishops ^= (BitMask[from] | BitMask[to]);
        pos.posStore.hash ^= ZobPiece[side][BISHOP][from];
        pos.posStore.hash ^= ZobPiece[side][BISHOP][to];
        pos.posStore.open[side] += PST(side, BISHOP, to, MIDGAME) - PST(side, BISHOP, from, MIDGAME);
        pos.posStore.end[side] += PST(side, BISHOP, to, ENDGAME) - PST(side, BISHOP, from, ENDGAME);
        break;
    case ROOK:
        pos.pieces[to] = ROOK;
        pos.rooks ^= (BitMask[from] | BitMask[to]);
        pos.posStore.hash ^= ZobPiece[side][ROOK][from];
        pos.posStore.hash ^= ZobPiece[side][ROOK][to];
        pos.posStore.open[side] += PST(side, ROOK, to, MIDGAME) - PST(side, ROOK, from, MIDGAME);
        pos.posStore.end[side] += PST(side, ROOK, to, ENDGAME) - PST(side, ROOK, from, ENDGAME);
        break;
    case QUEEN:
        pos.pieces[to] = QUEEN;
        pos.queens ^= (BitMask[from] | BitMask[to]);
        pos.posStore.hash ^= ZobPiece[side][QUEEN][from];
        pos.posStore.hash ^= ZobPiece[side][QUEEN][to];
        pos.posStore.open[side] += PST(side, QUEEN, to, MIDGAME) - PST(side, QUEEN, from, MIDGAME);
        pos.posStore.end[side] += PST(side, QUEEN, to, ENDGAME) - PST(side, QUEEN, from, ENDGAME);
        break;
    case KING:
        pos.pieces[to] = KING;
        pos.kpos[side] = to;
        pos.kings ^= (BitMask[from] | BitMask[to]);
        pos.posStore.hash ^= ZobPiece[side][KING][from];
        pos.posStore.hash ^= ZobPiece[side][KING][to];
        pos.posStore.open[side] += PST(side, KING, to, MIDGAME) - PST(side, KING, from, MIDGAME);
        pos.posStore.end[side] += PST(side, KING, to, ENDGAME) - PST(side, KING, from, ENDGAME);
        pos.posStore.phash ^= ZobPiece[side][KING][from];
        pos.posStore.phash ^= ZobPiece[side][KING][to];
        break;
    case TWOFORWARD:
        pos.pieces[to] = PAWN;
        pos.pawns ^= (BitMask[from] | BitMask[to]);
        pos.posStore.epsq = (from + to) / 2;
        if (pos.pawns & pos.color[xside] & PawnCaps[pos.posStore.epsq][side]) {
            pos.posStore.hash ^= ZobEpsq[SQFILE(pos.posStore.epsq)];
        } else pos.posStore.epsq = -1;
        pos.posStore.hash ^= ZobPiece[side][PAWN][from];
        pos.posStore.hash ^= ZobPiece[side][PAWN][to];
        pos.posStore.phash ^= ZobPiece[side][PAWN][from];
        pos.posStore.phash ^= ZobPiece[side][PAWN][to];
        pos.posStore.open[side] += PST(side, PAWN, to, MIDGAME) - PST(side, PAWN, from, MIDGAME);
        pos.posStore.end[side] += PST(side, PAWN, to, ENDGAME) - PST(side, PAWN, from, ENDGAME);
        pos.posStore.fifty = 0;
        break;
    case CASTLE:
        pos.pieces[to] = KING;
        pos.kpos[side] = to;
        pos.kings ^= (BitMask[from] | BitMask[to]);
        switch (to) {
        case c1:
            rook_from = a1;
            rook_to = d1;
            break;
        case g1:
            rook_from = h1;
            rook_to = f1;
            break;
        case c8:
            rook_from = a8;
            rook_to = d8;
            break;
        case g8:
            rook_from = h8;
            rook_to = f8;
            break;
        default:
            ASSERT(false);
            break;
        }
        pos.pieces[rook_to] = ROOK;
        pos.pieces[rook_from] = EMPTY;
        pos.rooks ^= (BitMask[rook_from] | BitMask[rook_to]);
        pos.color[side] ^= (BitMask[rook_from] | BitMask[rook_to]);
        pos.posStore.hash ^= ZobPiece[side][KING][from];
        pos.posStore.hash ^= ZobPiece[side][KING][to];
        pos.posStore.hash ^= ZobPiece[side][ROOK][rook_from];
        pos.posStore.hash ^= ZobPiece[side][ROOK][rook_to];
        pos.posStore.phash ^= ZobPiece[side][KING][from];
        pos.posStore.phash ^= ZobPiece[side][KING][to];
        pos.posStore.open[side] += PST(side, KING, to, MIDGAME) - PST(side, KING, from, MIDGAME);
        pos.posStore.end[side] += PST(side, KING, to, ENDGAME) - PST(side, KING, from, ENDGAME);
        pos.posStore.open[side] += PST(side, ROOK, rook_to, MIDGAME) - PST(side, ROOK, rook_from, MIDGAME);
        pos.posStore.end[side] += PST(side, ROOK, rook_to, ENDGAME) - PST(side, ROOK, rook_from, ENDGAME);
        break;
    case PROMOTE:
        prom = movePromote(m);
        pos.pawns ^= BitMask[from];
        pos.posStore.fifty = 0;
        pos.posStore.open[side] += PST(side, prom, to, MIDGAME) - PST(side, PAWN, from, MIDGAME);
        pos.posStore.end[side] += PST(side, prom, to, ENDGAME) - PST(side, PAWN, from, ENDGAME);
        pos.posStore.mat_summ[side] -= MatSummValue[PAWN];
        pos.posStore.hash ^= ZobPiece[side][PAWN][from];
        pos.posStore.phash ^= ZobPiece[side][PAWN][from];
        switch (prom) {
        case KNIGHT:
            pos.pieces[to] = KNIGHT;
            pos.knights ^= BitMask[to];
            pos.posStore.mat_summ[side] += MatSummValue[KNIGHT];
            pos.posStore.hash ^= ZobPiece[side][KNIGHT][to];
            break;
        case BISHOP:
            pos.pieces[to] = BISHOP;
            pos.bishops ^= BitMask[to];
            pos.posStore.mat_summ[side] += MatSummValue[BISHOP];
            pos.posStore.hash ^= ZobPiece[side][BISHOP][to];
            break;
        case ROOK:
            pos.pieces[to] = ROOK;
            pos.rooks ^= BitMask[to];
            pos.posStore.mat_summ[side] += MatSummValue[ROOK];
            pos.posStore.hash ^= ZobPiece[side][ROOK][to];
            break;
        case QUEEN:
            pos.pieces[to] = QUEEN;
            pos.queens ^= BitMask[to];
            pos.posStore.mat_summ[side] += MatSummValue[QUEEN];
            pos.posStore.hash ^= ZobPiece[side][QUEEN][to];
            break;
        }
        break;
    }

    switch (moveRemoval(m)) {
    case EMPTY:
        break;
    case PAWN:
        pos.pawns ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        pos.posStore.fifty = 0;
        pos.posStore.open[xside] -= PST(xside, PAWN, to, MIDGAME);
        pos.posStore.end[xside] -= PST(xside, PAWN, to, ENDGAME);
        pos.posStore.mat_summ[xside] -= MatSummValue[PAWN];
        pos.posStore.hash ^= ZobPiece[xside][PAWN][to];
        pos.posStore.phash ^= ZobPiece[xside][PAWN][to];
        break;
    case KNIGHT:
        pos.knights ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        pos.posStore.fifty = 0;
        pos.posStore.open[xside] -= PST(xside, KNIGHT, to, MIDGAME);
        pos.posStore.end[xside] -= PST(xside, KNIGHT, to, ENDGAME);
        pos.posStore.mat_summ[xside] -= MatSummValue[KNIGHT];
        pos.posStore.hash ^= ZobPiece[xside][KNIGHT][to];
        break;
    case BISHOP:
        pos.bishops ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        pos.posStore.fifty = 0;
        pos.posStore.open[xside] -= PST(xside, BISHOP, to, MIDGAME);
        pos.posStore.end[xside] -= PST(xside, BISHOP, to, ENDGAME);
        pos.posStore.mat_summ[xside] -= MatSummValue[BISHOP];
        pos.posStore.hash ^= ZobPiece[xside][BISHOP][to];
        break;
    case ROOK:
        pos.rooks ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        pos.posStore.fifty = 0;
        pos.posStore.open[xside] -= PST(xside, ROOK, to, MIDGAME);
        pos.posStore.end[xside] -= PST(xside, ROOK, to, ENDGAME);
        pos.posStore.mat_summ[xside] -= MatSummValue[ROOK];
        pos.posStore.hash ^= ZobPiece[xside][ROOK][to];
        break;
    case QUEEN:
        pos.queens ^= BitMask[to];
        pos.color[xside] ^= BitMask[to];
        pos.posStore.fifty = 0;
        pos.posStore.open[xside] -= PST(xside, QUEEN, to, MIDGAME);
        pos.posStore.end[xside] -= PST(xside, QUEEN, to, ENDGAME);
        pos.posStore.mat_summ[xside] -= MatSummValue[QUEEN];
        pos.posStore.hash ^= ZobPiece[xside][QUEEN][to];
        break;
    case ENPASSANT:
        switch (side) {
        case WHITE:
            epsq = to - 8;
            break;
        default:
            epsq = to + 8;
            break;
        }
        pos.pieces[epsq] = EMPTY;
        pos.pawns ^= BitMask[epsq];
        pos.color[xside] ^= BitMask[epsq];
        pos.posStore.fifty = 0;
        pos.posStore.open[xside] -= PST(xside, PAWN, epsq, MIDGAME);
        pos.posStore.end[xside] -= PST(xside, PAWN, epsq, ENDGAME);
        pos.posStore.mat_summ[xside] -= MatSummValue[PAWN];
        pos.posStore.hash ^= ZobPiece[xside][PAWN][epsq];
        pos.posStore.phash ^= ZobPiece[xside][PAWN][epsq];
        break;
    case KING:
        ASSERT(false);
        Print(8, "Move capturing the King: %s\n", move2Str(m));
        displayBoard(pos, 8);
        break;
    }
    pos.pieces[from] = EMPTY;
    pos.color[side] ^= (BitMask[from] | BitMask[to]);
    pos.occupied = pos.color[side] | pos.color[xside];
    ++pos.ply;
    ASSERT(pos.sp + 1 < MAX_HASH_STORE);
    ++pos.sp;
    pos.side = xside;
#ifdef DEBUG
    positionIsOk(pos);
#endif
#ifdef DEBUG_INDEPTH
    ASSERT(evalSymmetryIsOk(pos));
#endif
}

/* sets position from a FEN string*/
void setPosition(position_t& pos, const char *fen) {
    int rank = 7, file = 0, pc = 0, color = 0, count = 0, i, sq;

    ASSERT(pos != NULL);
    ASSERT(fen != NULL);

    pos.pawns = EmptyBoardBB;
    pos.knights = EmptyBoardBB;
    pos.bishops = EmptyBoardBB;
    pos.rooks = EmptyBoardBB;
    pos.queens = EmptyBoardBB;
    pos.kings = EmptyBoardBB;
    pos.occupied = EmptyBoardBB;
    pos.color[WHITE] = EmptyBoardBB;
    pos.color[BLACK] = EmptyBoardBB;
    pos.side = WHITE;
    pos.ply = 0;
    pos.sp = 0;
    pos.kpos[WHITE] = 0;
    pos.kpos[BLACK] = 0;
    for (sq = a1; sq <= h8; sq++) pos.pieces[sq] = EMPTY;
    pos.posStore.previous = NULL;
    pos.posStore.lastmove = EMPTY;
    pos.posStore.epsq = -1;
    pos.posStore.castle = 0;
    pos.posStore.fifty = 0;
    pos.posStore.pliesFromNull = 0;
    pos.posStore.open[WHITE] = 0;
    pos.posStore.open[BLACK] = 0;
    pos.posStore.end[WHITE] = 0;
    pos.posStore.end[BLACK] = 0;
    pos.posStore.hash = 0;
    pos.posStore.phash = 0;
    while ((rank >= 0) && *fen) {
        count = 1;
        pc = EMPTY;
        switch (*fen) {
        case 'K':
            pc = KING;
            color = WHITE;
            break;
        case 'k':
            pc = KING;
            color = BLACK;
            break;
        case 'Q':
            pc = QUEEN;
            color = WHITE;
            break;
        case 'q':
            pc = QUEEN;
            color = BLACK;
            break;
        case 'R':
            pc = ROOK;
            color = WHITE;
            break;
        case 'r':
            pc = ROOK;
            color = BLACK;
            break;
        case 'B':
            pc = BISHOP;
            color = WHITE;
            break;
        case 'b':
            pc = BISHOP;
            color = BLACK;
            break;
        case 'N':
            pc = KNIGHT;
            color = WHITE;
            break;
        case 'n':
            pc = KNIGHT;
            color = BLACK;
            break;
        case 'P':
            pc = PAWN;
            color = WHITE;
            break;
        case 'p':
            pc = PAWN;
            color = BLACK;
            break;
        case '/':
        case ' ':
            rank--;
            file = 0;
            fen++;
            continue;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            count = *fen - '0';
            break;
        default:
            Print(3, "info string FEN Error 1!");
            return;
        }
        for (i = 0; i < count; i++, file++) {
            sq = (rank * 8) + file;
            ASSERT(pc >= EMPTY && pc <= KING);
            ASSERT(sq >= a1 && sq <= h8);
            ASSERT(color == BLACK || color == WHITE);
            if (pc != EMPTY) {
                switch (pc) {
                case PAWN:
                    pos.pawns ^= BitMask[sq];
                    break;
                case KNIGHT:
                    pos.knights ^= BitMask[sq];
                    break;
                case BISHOP:
                    pos.bishops ^= BitMask[sq];
                    break;
                case ROOK:
                    pos.rooks ^= BitMask[sq];
                    break;
                case QUEEN:
                    pos.queens ^= BitMask[sq];
                    break;
                case KING:
                    pos.kings ^= BitMask[sq];
                    pos.kpos[color] = sq;
                    break;
                }
                pos.pieces[sq] = pc;
                pos.posStore.open[color] += PST(color, pc, sq, MIDGAME);
                pos.posStore.end[color] += PST(color, pc, sq, ENDGAME);
                pos.color[color] ^= BitMask[sq];
                pos.occupied ^= BitMask[sq];
                pos.posStore.hash ^= ZobPiece[color][pc][sq];
                if (pc == PAWN || pc == KING) pos.posStore.phash ^= ZobPiece[color][pc][sq];

            }
        }
        fen++;
    }
    while (isspace(*fen)) fen++;
    switch (tolower(*fen)) {
    case 'w':
        pos.side = WHITE;
        break;
    case 'b':
        pos.side = BLACK;
        break;
    default:
        Print(3, "info string FEN Error: %c\n", *fen);
        return;
    }
    do {
        fen++;
    } while (isspace(*fen));
    while (*fen != '\0' && !isspace(*fen)) {
        if (*fen == 'K') {
            pos.posStore.castle |= WCKS;
        } else if (*fen == 'Q') {
            pos.posStore.castle |= WCQS;
        } else if (*fen == 'k') {
            pos.posStore.castle |= BCKS;
        } else if (*fen == 'q') {
            pos.posStore.castle |= BCQS;
        }
        fen++;
    }
    while (isspace(*fen)) fen++;
    if (*fen != '\0') {
        if (*fen != '-') {
            if (fen[0] >= 'a' && fen[0] <= 'h' && fen[1] >= '1' && fen[1] <= '8') {
                pos.posStore.epsq = fen[0] - 'a' + (fen[1] - '1') * 8;
            }
            do {
                fen++;
            } while (!isspace(*fen));
        }
        do {
            fen++;
        } while (isspace(*fen));
        if (isdigit(*fen)) { //Hannibal 0.1a
            int fiftyInt;
            sscanf_s(fen, "%d", &fiftyInt);
            if (fiftyInt >= 0 && fiftyInt <= 100) {
                pos.posStore.fifty = (uint32)fiftyInt;
                pos.sp = pos.posStore.fifty;
            }
        }
    }
    pos.posStore.mat_summ[WHITE] =
        bitCnt(pos.pawns & pos.color[WHITE]) * MatSummValue[PAWN] +
        bitCnt(pos.knights & pos.color[WHITE]) * MatSummValue[KNIGHT] +
        bitCnt(pos.bishops & pos.color[WHITE]) * MatSummValue[BISHOP] +
        bitCnt(pos.rooks & pos.color[WHITE]) * MatSummValue[ROOK] +
        bitCnt(pos.queens & pos.color[WHITE]) * MatSummValue[QUEEN];
    pos.posStore.mat_summ[BLACK] =
        bitCnt(pos.pawns & pos.color[BLACK]) * MatSummValue[PAWN] +
        bitCnt(pos.knights & pos.color[BLACK]) * MatSummValue[KNIGHT] +
        bitCnt(pos.bishops & pos.color[BLACK]) * MatSummValue[BISHOP] +
        bitCnt(pos.rooks & pos.color[BLACK]) * MatSummValue[ROOK] +
        bitCnt(pos.queens & pos.color[BLACK]) * MatSummValue[QUEEN];
    if (pos.posStore.epsq != -1) pos.posStore.hash ^= ZobEpsq[SQFILE(pos.posStore.epsq)];
    if (pos.side == WHITE) pos.posStore.hash ^= ZobColor;
    pos.posStore.hash ^= ZobCastle[pos.posStore.castle];
    pos.posStore.phash ^= ZobCastle[pos.posStore.castle];

#ifdef DEBUG
    positionIsOk(pos);
#endif
#ifdef EVAL_DEBUG
    ASSERT(evalSymmetryIsOk(pos));
#endif
}

char *positionToFEN(const position_t& pos) {
    static char pcstr[] = ".PNBRQK.pnbrqk";
    static char fen[64];
    int rank, file, sq, empty, pc, color, c = 0;

    for (rank = 56; rank >= 0; rank -= 8) {
        empty = 0;
        for (file = FileA; file <= FileH; file++) {
            sq = rank + file;
            pc = getPiece(pos, sq);
            if (pc != EMPTY) {
                color = getColor(pos, sq);
                if (empty > 0) fen[c++] = (char)empty + '0';
                fen[c++] = pcstr[pc + (color == WHITE ? 0 : 7)];
                empty = 0;
            } else empty++;
        }
        if (empty > 0) fen[c++] = (char)empty + '0';
        fen[c++] = (rank > Rank1) ? '/' : ' ';
    }

    fen[c++] = (pos.side == WHITE) ? 'w' : 'b';
    fen[c++] = ' ';

    if (pos.posStore.castle == 0) fen[c++] = '-';
    else {
        if (pos.posStore.castle & WCKS) fen[c++] = 'K';
        if (pos.posStore.castle & WCQS) fen[c++] = 'Q';
        if (pos.posStore.castle & BCKS) fen[c++] = 'k';
        if (pos.posStore.castle & BCQS) fen[c++] = 'q';
    }

    fen[c++] = ' ';

    if (pos.posStore.epsq == -1) fen[c++] = '-';
    else {
        fen[c++] = SQFILE(pos.posStore.epsq) + 'a';
        fen[c++] = '1' + SQRANK(pos.posStore.epsq);
    }
    fen[c] = '\0';
    return fen;
}

