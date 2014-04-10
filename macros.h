/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#ifdef DEBUG
#define ASSERT(a) { if (!(a)) \
    Print(4, "file \"%s\", line %d, assertion \"" #a "\" failed\n", __FILE__, __LINE__); }
#else
#define ASSERT(a)
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#  define CACHE_LINE_ALIGNMENT __declspec(align(64))
#else
#  define CACHE_LINE_ALIGNMENT  __attribute__ ((aligned(64)))
#endif

#define MAXPLY              128
#define MAX_HASH_STORE      (MAXPLY+102)
#define MAXMOVES            256
#define INF                 32500
#define MAXEVAL             32000
#define MAXHIST             32500
//#define INF                 16000
//#define MAXEVAL             14000
//#define MAXHIST             16384

#define WHITE               0
#define BLACK               1
#define LEFT                0
#define RIGHT               1

enum PieceTypes {
    EMPTY = 0,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
};

#define CASTLE              14
#define TWOFORWARD          17
#define PROMOTE             33
#define ENPASSANT           9

#define WCKS                1
#define WCQS                2
#define BCKS                4
#define BCQS                8

#define NOMASK              0

#define STARTPOS  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define GetOnlyBit				getFirstBit //SAM122909
#define NOT_BEHIND(k,p,c) ((k)*sign[c] >= (p)*sign[c])
#define IN_FRONT(k,p,c) ((k)*sign[c] > (p)*sign[c])

#define SQRANK(x)                ((x) >> 3)
#define SQFILE(x)                ((x) & 7)
#define MAX(a,b)                 (((a)>(b))?(a):(b))
#define MIN(a,b)                 (((a)<(b))?(a):(b))
#define ABS(a)                   (((a)<0)?-(a):(a))
#ifdef ARRAY_DIST
#define DISTANCE(a,b)			(distance[(a)][(b)]) //SAM122909
#else
#define DISTANCE(a,b)             MAX((ABS((SQRANK(a))-(SQRANK(b)))),(ABS((SQFILE(a))-(SQFILE(b)))))
#endif
#define PAWN_RANK(f,c)           (((c)==BLACK)?(7-SQRANK(f)):(SQRANK(f)))
#define Q_DIST(f,c)              (((c)==WHITE)?(7-SQRANK(f)):(SQRANK(f)))
#define PAWN_MOVE_INC(c)         ((c)?-8:8)
//#define PAWN_PROMOTE(sq,c)       (SQFILE(sq) + ((c==BLACK)?0:56))

#define PAWN_PROMOTE(sq,c)       ((c) ? ((sq) & 007) : ((sq) | 070))

#define SQUARE_COLOUR(square)    (((square)^((square)>>3))&1)
#define FILE_MIRROR(square)      ((square)^07)
#define RANK_MIRROR(square)      ((square)^070)
#define COLOUR_IS_WHITE(a)       ((a) == WHITE)
#define COLOUR_IS_BLACK(a)       ((a) == BLACK)
#define MAX_MATERIAL             (2*3*3*3*9)
#define U64(u)					 (u##ULL)

#define PST(c,p,s,l)    (PcSqTb[(((c)<<10)|((p)<<7)|((s)<<1)|(l))])
#define MIDGAME 0
#define ENDGAME 1

enum ThinkingStatus {
    THINKING = 0,
    PONDERING,
    ANALYSING
};

#define REDUCED             4

#define LOCK(x)             (uint32)((x)>>32)
#define KEY(x)              (uint32)(x)
//#define SETHASH(l,k)        (uint64)(((l)<<32)|(k))

const int MaxNumOfThreads = 32;
const int MaxNumSplitPointsPerThread = 8;
