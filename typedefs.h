/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

// error check shows could use minor debugging work

#pragma once
// rbp v rb opp bishop drawish
/*
TODO ED:
-implement Search/Thread/Protocol/BitBoard/ classes
-fix all Level 4 warnings in MSVC
-fix all Code Analysis warnings in MSVC
-implement one score for midgame/endgame in eval, more efficient
-add Chess 960 support
*/
/* TODO EVAL
rook vs pawn cut off on 4th rank
try transstore on ALL pruning
write situation specific transtores
can knight catch pawns code from LL
//68 at 3:29
*/
#define VERSION            "1.5beta5"
#define NUM_THREADS			    6
#define MIN_SPLIT_DEPTH			4 // best is 4
#define MAX_SPLIT_THREADS		4 // best is 4
//#define TESTING_ON true

//#define SPEED_TEST
//#define NEW_EASY true
//#define DEBUG_EASY true
//#define OPTIMIZE true
//#define DEBUG
//#define EVAL_DEBUG true
//#define DEBUG_ML true
//#define DEBUG_SEE true
//#define DEBUG_EVAL_TABLE true
//#define DEBUG_RAZOR true
//#define DEBUG_INDEPTH true

//#define SELF_TUNE2 1 //number of simultaneous training games
//#define TUNE_MAT true

#define NP2 true
#define USE_PHASH true
#define MIN_TRANS_SIZE 16

#ifdef TCEC
#define INIT_EVAL 64
#define INIT_PAWN 32
#define INIT_HASH 64
#define INIT_PVHASH 1

#else
#define INIT_EVAL 64
#define INIT_PAWN 32
#define INIT_HASH 64
#define INIT_PVHASH 1

#define DEFAULT_POLYGLOT_BOOK "HannibalPoly.bin"
#define MAX_BOOK 60 //could be MAXPLY
#endif

#define DEBUG_BOOK false
#define DEBUG_LEARN false

#define ERROR_FILE "errfile.txt"

#ifdef SPEED_TEST
#define SHOW_SEARCH false
#define RETURN_MOVE false
#else
#ifdef SELF_TUNE2
#define SHOW_SEARCH false
#define RETURN_MOVE false
#else
#define SHOW_SEARCH true
#define RETURN_MOVE true
#endif
#endif



#if defined(__x86_64) || defined(_WIN64)
#define VERSION64BIT
#endif

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <sys/timeb.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type

#include "macros.h"

#include <iostream>
using namespace std;
/* some basic definitions */
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

enum NodeType { CutNode = -1, PVNode = 0, AllNode };
enum BookType { POLYGLOT_BOOK, PUCK_BOOK };
enum HashType { HTLower, HTUpper, HTCutUpper, HTAllLower, HTExact, HTNoMoves };
enum HashMask { MLower = 1, MUpper = 2, MCutUpper = 4, MAllLower = 8, MExact = 16, MNoMoves = 32, MClear = 255 };

typedef uint32 basic_move_t;

struct continuation_t
{
    basic_move_t moves[MAXPLY + 1];
    int length;
};

///* the move structure */
struct move_t {
    basic_move_t m;
    int32 s;
};

/* the structure used in generating moves */
struct movelist_t {
    volatile uint32 phase;
    basic_move_t transmove;
    basic_move_t killer1;
    basic_move_t killer2;
    int32 evalvalue;
    int32 scout;
    int32 ply;
    int32 depth;
    int32 side;
    volatile int32 pos;
    volatile int32 size;
    volatile int32 startBad;
    uint64 pinned;
    move_t list[MAXMOVES];
};

struct book_t {
    BookType type;
    book_t() : bookFile(NULL) {}
    ~book_t() { if (bookFile) fclose(bookFile); }
    FILE *bookFile;
    int64 size;
    string name;
};

/* the undo structure */
struct pos_store_t{
    uint32 lastmove;
    int castle;
    int fifty;
    int epsq;
    int pliesFromNull;
    int open[2];
    int end[2];
    int mat_summ[2];
    uint64 phash;
    uint64 hash;
    pos_store_t* previous;
};

/* the position structure */
struct position_t{
    uint64 pawns;
    uint64 knights;
    uint64 bishops;
    uint64 rooks;
    uint64 queens;
    uint64 kings;
    uint64 color[2];
    uint64 occupied;
    int pieces[64];
    int kpos[2];
    pos_store_t posStore;

    int side;
    int ply;
    int sp;
};

typedef uint8 mflag_t;

/* the material info structure */
struct material_info_t{
    int16 value;
    uint8 phase;
    uint8 draw[2];
    mflag_t flags[2];
};

struct SearchStack {
    SearchStack() :
    firstExtend(false),
    reducedMove(false),
    moveGivesCheck(false),
    playedMoves(0),
    hisCnt(0),
    evalvalue(-INF),
    bestvalue(-INF),
    bestmove(EMPTY),
    dcc(0),
    counterMove(EMPTY),
    threatMove(EMPTY),
    bannedMove(EMPTY),
    hashMove(EMPTY),
    hashDepth(0),
    mvlist(&movelist),
    mvlist_phase(0)
    { }
    int playedMoves;
    int hisCnt;
    basic_move_t hisMoves[64];
    int bestvalue;
    basic_move_t bestmove;

    bool firstExtend;
    bool reducedMove;
    bool moveGivesCheck;
    int evalvalue;
    uint64 dcc;
    basic_move_t counterMove;
    basic_move_t threatMove;
    basic_move_t bannedMove;
    basic_move_t hashMove;
    int hashDepth;

    movelist_t movelist;
    movelist_t* mvlist;
    int mvlist_phase;
};

enum directions { SW, W, NW, N, NE, E, SE, S, NO_DIR };//{-9, -1, 7, 8, 9, 1, -7, -8};

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
    PH_GAINING,
    PH_END
};

INLINE uint moveFrom(basic_move_t m) { return (63 & (m)); }
INLINE uint moveTo(basic_move_t m) { return (63 & ((m) >> 6)); }
INLINE uint movePiece(basic_move_t m) { return (7 & ((m) >> 12)); }
INLINE uint moveAction(basic_move_t m) { return (63 & ((m) >> 12)); }
INLINE uint moveCapture(basic_move_t m) { return (7 & ((m) >> 18)); }
INLINE uint moveRemoval(basic_move_t m) { return (15 & ((m) >> 18)); }
INLINE uint movePromote(basic_move_t m) { return (7 & ((m) >> 22)); }
INLINE uint isCastle(basic_move_t m) { return (((m) >> 15) & 1); }
INLINE uint isPawn2Forward(basic_move_t m) { return (((m) >> 16) & 1); }
INLINE uint isPromote(basic_move_t m) { return (((m) >> 17) & 1); }
INLINE uint isEnPassant(basic_move_t m) { return  (((m) >> 21) & 1); }

INLINE basic_move_t GenOneForward(uint f, uint t) { return ((f) | ((t) << 6) | (PAWN << 12)); }
INLINE basic_move_t GenTwoForward(uint f, uint t) { return ((f) | ((t) << 6) | (PAWN << 12) | (1 << 16)); }
INLINE basic_move_t GenPromote(uint f, uint t, uint r, uint c) { return ((f) | ((t) << 6) | (PAWN << 12) | ((c) << 18) | ((r) << 22) | (1 << 17)); }
INLINE basic_move_t GenPromoteStraight(uint f, uint t, uint r) { return ((f) | ((t) << 6) | (PAWN << 12) | ((r) << 22) | (1 << 17)); }
INLINE basic_move_t GenEnPassant(uint f, uint t) { return ((f) | ((t) << 6) | (PAWN << 12) | (PAWN << 18) | (1 << 21)); }
INLINE basic_move_t GenPawnMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (PAWN << 12) | ((c) << 18)); }
INLINE basic_move_t GenKnightMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (KNIGHT << 12) | ((c) << 18)); }
INLINE basic_move_t GenBishopMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (BISHOP << 12) | ((c) << 18)); }
INLINE basic_move_t GenRookMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (ROOK << 12) | ((c) << 18)); }
INLINE basic_move_t GenQueenMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (QUEEN << 12) | ((c) << 18)); }
INLINE basic_move_t GenKingMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (KING << 12) | ((c) << 18)); }
INLINE basic_move_t GenWhiteOO(void) { return (e1 | (g1 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenWhiteOOO(void) { return (e1 | (c1 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenBlackOO(void) { return (e8 | (g8 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenBlackOOO(void) { return (e8 | (c8 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenBasicMove(uint f, uint t, int pieceType, uint c) { return ((f) | ((t) << 6) | (pieceType << 12) | ((c) << 18)); }


