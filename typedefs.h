/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

//#define DEBUG
//#define EVAL_DEBUG true
//#define DEBUG_INDEPTH true
#define SHOW_SEARCH true

#if defined(__x86_64) || defined(_WIN64)
#define VERSION64BIT
#endif

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4244) // Possible loss of data

#include "macros.h"

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline
#elif defined(__GNUC__)
#define INLINE __inline__ __attribute__((always_inline))
#else
#define INLINE inline
#endif
#endif

#ifndef _MSC_VER
#include <inttypes.h>
typedef int8_t			int8;
typedef uint8_t			uint8;
typedef int16_t			int16;
typedef uint16_t		uint16;
typedef int32_t			int32;
typedef uint32_t		uint32;
typedef int64_t			int64;
typedef uint64_t		uint64;
typedef unsigned int	uint;
#else
typedef __int8				int8;
typedef unsigned __int8		uint8;
typedef __int16				int16;
typedef unsigned __int16	uint16;
typedef __int32				int32;
typedef unsigned __int32	uint32;
typedef __int64				int64;
typedef unsigned __int64	uint64;
typedef unsigned int		uint;
#endif

enum HashType {
    HTLower, HTUpper, HTCutUpper, HTAllLower, HTExact, HTNoMoves
};

enum HashMask {
    MNoMoves = 1, MSingular = 2, MPV = 4
};

typedef uint32 basic_move_t;
typedef int32 EvalScore;

/* the move structure */
struct move_t {
    basic_move_t m;
    int32 s;
};

/* the structure used in generating moves */
struct movelist_t {
    basic_move_t transmove;
    basic_move_t killer1;
    basic_move_t killer2;
    int32 depth;
    int32 side;
    volatile uint32 phase;
    volatile int32 pos;
    volatile int32 size;
    volatile int32 startBad;
    uint64 pinned; // TODO: move to SS
    move_t list[MAXMOVES]; // TODO
};

/* the undo structure */
struct pos_store_t {
    pos_store_t() : previous(nullptr) {}
    uint32 lastmove;
    int castle;
    int fifty;
    int epsq;
    int pliesFromNull;
    EvalScore posScore[2];
    int16 mat_summ[2];
    uint64 phash;
    uint64 hash;
    pos_store_t* previous;
};

/* the position structure */
struct position_t {
    uint64 pawns;
    uint64 knights;
    uint64 bishops;
    uint64 rooks;
    uint64 queens;
    uint64 kings;
    uint64 color[2];
    uint64 occupied;
    int8 pieces[64];
    int8 kpos[2];
    pos_store_t posStore;
    int8 side;
};

typedef uint8 mflag_t;
/* the material info structure */
struct material_info_t {
    int16 value;
    uint8 phase;
    uint8 draw[2];
    mflag_t flags[2];
};

struct SearchStack {
    SearchStack(SearchStack *prev) :
        reducedMove(false),
        moveGivesCheck(false),
        playedMoves(0),
        hisCnt(0),
        evalvalue(-INF),
        staticEvalValue(-INF),
        bestvalue(-INF),
        bestmove(EMPTY),
        dcc(0),
        counterMove(EMPTY),
        threatMove(EMPTY),
        bannedMove(EMPTY),
        hashMove(EMPTY),
        hashDepth(-2),
        ply(prev != nullptr ? prev->ply + 1 : -1),
        ssprev(prev),
        mvlist(&movelist),
        hisMoves(&hisTable[0]) {}
    bool reducedMove;
    bool moveGivesCheck;
    int playedMoves;
    int hisCnt;
    int evalvalue;
    int staticEvalValue;
    int bestvalue;
    basic_move_t bestmove;
    uint64 dcc;
    basic_move_t counterMove;
    basic_move_t threatMove;
    basic_move_t bannedMove;
    basic_move_t hashMove;
    int hashDepth;
    int ply;
    SearchStack *ssprev;
    movelist_t* mvlist;
    basic_move_t* hisMoves;

    basic_move_t hisTable[64];
    movelist_t movelist;
};

struct GoCmdData {
    GoCmdData() {
        wtime = 0;
        btime = 0;
        winc = 0;
        binc = 0;
        movestogo = 0;
        depth = 0;
        movetime = 0;
        mate = 0;
        infinite = false;
        ponder = false;
        nodes = 0;
    }
    int wtime;
    int btime;
    int winc;
    int binc;
    int movestogo;
    int depth;
    int movetime;
    int mate;
    bool infinite;
    bool ponder;
    uint64 nodes;
};

enum directions {
    SW, W, NW, N, NE, E, SE, S, NO_DIR
};//{-9, -1, 7, 8, 9, 1, -7, -8};

/* the squares */
enum squarenames {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

/* the phases used for lazy move generation */
enum movegen_phases {
    PH_NONE = 0,
    PH_ROOT,
    PH_EVASION,
    PH_TRANS,
    PH_ALL_CAPTURES,
    PH_ALL_CAPTURES_PURE,
    PH_GOOD_CAPTURES,
    PH_GOOD_CAPTURES_PURE,
    PH_BAD_CAPTURES,
    PH_KILLER_MOVES,
    PH_QUIET_MOVES,
    PH_NONTACTICAL_CHECKS,
    PH_NONTACTICAL_CHECKS_WIN,
    PH_NONTACTICAL_CHECKS_PURE,
    PH_END
};

INLINE uint moveFrom(basic_move_t m) {
    return (63 & (m));
}
INLINE uint moveTo(basic_move_t m) {
    return (63 & ((m) >> 6));
}
INLINE uint movePiece(basic_move_t m) {
    return (7 & ((m) >> 12));
}
INLINE uint moveAction(basic_move_t m) {
    return (63 & ((m) >> 12));
}
INLINE uint moveCapture(basic_move_t m) {
    return (7 & ((m) >> 18));
}
INLINE uint moveRemoval(basic_move_t m) {
    return (15 & ((m) >> 18));
}
INLINE uint movePromote(basic_move_t m) {
    return (7 & ((m) >> 22));
}
INLINE uint isCastle(basic_move_t m) {
    return (((m) >> 15) & 1);
}
INLINE uint isPawn2Forward(basic_move_t m) {
    return (((m) >> 16) & 1);
}
INLINE uint isPromote(basic_move_t m) {
    return (((m) >> 17) & 1);
}
INLINE uint isEnPassant(basic_move_t m) {
    return  (((m) >> 21) & 1);
}

INLINE basic_move_t GenOneForward(uint f, uint t) {
    return ((f) | ((t) << 6) | (PAWN << 12));
}
INLINE basic_move_t GenTwoForward(uint f, uint t) {
    return ((f) | ((t) << 6) | (PAWN << 12) | (1 << 16));
}
INLINE basic_move_t GenPromote(uint f, uint t, uint r, uint c) {
    return ((f) | ((t) << 6) | (PAWN << 12) | ((c) << 18) | ((r) << 22) | (1 << 17));
}
INLINE basic_move_t GenPromoteStraight(uint f, uint t, uint r) {
    return ((f) | ((t) << 6) | (PAWN << 12) | ((r) << 22) | (1 << 17));
}
INLINE basic_move_t GenEnPassant(uint f, uint t) {
    return ((f) | ((t) << 6) | (PAWN << 12) | (PAWN << 18) | (1 << 21));
}
INLINE basic_move_t GenPawnMove(uint f, uint t, uint c) {
    return ((f) | ((t) << 6) | (PAWN << 12) | ((c) << 18));
}
INLINE basic_move_t GenKnightMove(uint f, uint t, uint c) {
    return ((f) | ((t) << 6) | (KNIGHT << 12) | ((c) << 18));
}
INLINE basic_move_t GenBishopMove(uint f, uint t, uint c) {
    return ((f) | ((t) << 6) | (BISHOP << 12) | ((c) << 18));
}
INLINE basic_move_t GenRookMove(uint f, uint t, uint c) {
    return ((f) | ((t) << 6) | (ROOK << 12) | ((c) << 18));
}
INLINE basic_move_t GenQueenMove(uint f, uint t, uint c) {
    return ((f) | ((t) << 6) | (QUEEN << 12) | ((c) << 18));
}
INLINE basic_move_t GenKingMove(uint f, uint t, uint c) {
    return ((f) | ((t) << 6) | (KING << 12) | ((c) << 18));
}
INLINE basic_move_t GenWhiteOO(void) {
    return (e1 | (g1 << 6) | (KING << 12) | (1 << 15));
}
INLINE basic_move_t GenWhiteOOO(void) {
    return (e1 | (c1 << 6) | (KING << 12) | (1 << 15));
}
INLINE basic_move_t GenBlackOO(void) {
    return (e8 | (g8 << 6) | (KING << 12) | (1 << 15));
}
INLINE basic_move_t GenBlackOOO(void) {
    return (e8 | (c8 << 6) | (KING << 12) | (1 << 15));
}
INLINE basic_move_t GenBasicMove(uint f, uint t, int pieceType, uint c) {
    return ((f) | ((t) << 6) | (pieceType << 12) | ((c) << 18));
}
