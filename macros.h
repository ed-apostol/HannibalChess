/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
#pragma once

#ifdef DEBUG
#define ASSERT(a) { if (!(a)) \
    Print(4, "file \"%s\", line %d, assertion \"" #a "\" failed\n",__FILE__,__LINE__);}
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
    STOPPED = 0,
    THINKING,
    PONDERING,
    ANALYSING
};

#define REDUCED             4

#define LOCK(x)             (uint32)((x)>>32)
#define KEY(x)              (uint32)(x)
//#define SETHASH(l,k)        (uint64)(((l)<<32)|(k))


const int MaxNumOfThreads = 8;
const int MaxNumSplitPointsPerThread = 8;

////////#define MutexInit(x, y) InitializeCriticalSection(x)
////////#define MutexLock(x) EnterCriticalSection(x)
////////#define MutexUnlock(x) LeaveCriticalSection(x)
////////#define MutexDestroy(x) DeleteCriticalSection(x)



/*
#define MAX_DRAW 100
#define DRAWN 100
#define DRAWN1 90
#define DRAWN2 75
#define DRAWN3 60
#define DRAWN4 50
#define DRAWN5 40
#define DRAWN6 32
#define DRAWN7 24
#define DRAWN8 17
#define DRAWN9 10
#define DRAWN10 5
#define DRAWN11 3
*/
//#define DRAW_ADJUST (ONESIDE_WEIGHT/2)
//#define DRAW_ADJUST (ONESIDE_WEIGHT)
#define DRAW_ADJUST 0
//(ONESIDE_WEIGHT/2) //this is the amount contributed by other factors than endgame, such as drawish material balances and such
//#define DRAW_ADJUST ONESIDE_WEIGHT //this is the amount contributed by other factors than endgame, such as drawish material balances and such


#define MAX_DRAW 100
#define DRAWN 100
#define DRAWN1 (90 - (90 * DRAW_ADJUST) / 100) 
#define DRAWN2 (75 - (75 * DRAW_ADJUST) / 100)
#define DRAWN3 (60 - (60 * DRAW_ADJUST) / 100)
#define DRAWN4 (50 - (50 * DRAW_ADJUST) / 100)
#define DRAWN5 (40 - (40 * DRAW_ADJUST) / 100)
#define DRAWN6 (32 - (32 * DRAW_ADJUST) / 100)
#define DRAWN7 (24 - (24 * DRAW_ADJUST) / 100)
#define DRAWN8 (17 - (17 * DRAW_ADJUST) / 100)
#define DRAWN9 (10 - (10 * DRAW_ADJUST) / 100)
#define DRAWN10 (5 - (5 * DRAW_ADJUST) / 100)
#define DRAWN11 (3 - (3 * DRAW_ADJUST) / 100)

#define NP_M1 1
#define NP_M2 0
#define NP_M3 0
#define NP_M4 1
#define NP_M5 0
#define NP_M6 0
#define NP_R1 2
#define NP_R2 0
#define NP_Q 2
#define NP_QM1 0
#define NP_QM2 0
#define NP_QM3 0
#define NP_QM4 0
#define NP_QM5 0
#define NP_QR1 0
#define NP_QR2 0

//#define DRAWISH (DRAWN2 - (ONESIDE_WEIGHT/2))
//#define PRETTY_DRAWISH ((DRAWN2+DRAWN1)/2 - (ONESIDE_WEIGHT/2))
//#define VERY_DRAWISH (DRAWN1 - (ONESIDE_WEIGHT/2))
//#define SUPER_DRAWISH (((DRAWN1+DRAWN)/2)-(ONESIDE_WEIGHT/2))

#define DRAWISH (DRAWN2)
#define PRETTY_DRAWISH ((DRAWN2+DRAWN1)/2 )
#define VERY_DRAWISH (DRAWN1 )
#define SUPER_DRAWISH (((DRAWN1+DRAWN)/2))

#define DEBUG_DRAW false


#define MaxOneBit(x) (((x) & ((x)-1))==0) 
#define MinTwoBits(x) ((x) & ((x)-1)) 

#define MoveGenPhaseEvasion 0
#define MoveGenPhaseStandard (MoveGenPhaseEvasion+3)
#define MoveGenPhaseQuiescenceAndChecksPV (MoveGenPhaseStandard+7)
#define MoveGenPhaseQuiescence (MoveGenPhaseQuiescenceAndChecksPV+6)
#define MoveGenPhaseQuiescenceAndChecks (MoveGenPhaseQuiescence+4)
#define MoveGenPhaseQuiescencePV (MoveGenPhaseQuiescenceAndChecks+5)
#define MoveGenPhaseRoot (MoveGenPhaseQuiescencePV+4)


#define PV_ASSOC 8
#define EVAL_ASSOC 1
#define HASH_ASSOC 4
#define PAWN_ASSOC 1



#define FUTILITY_MOVE 64

//BEST 1160 0.516
//#define REDUCE_MIN 1.5
//#define REDUCE_SCALE 2.5 
//#define PV_REDUCE_MIN 0.50
//#define PV_REDUCE_SCALE 7.50


//#define REDUCE_MIN 1.57
//#define REDUCE_SCALE 3.87 
#define REDUCE_MIN 1.5
#define REDUCE_SCALE 2.5 

//#define PV_REDUCE_MIN 0.57 
//#define PV_REDUCE_SCALE 7.58 
#define PV_REDUCE_MIN 0.5
#define PV_REDUCE_SCALE 7.5

//      double    pvRed = log(double(hd)) * log(double(mc)) / 3.0;
//      double nonPVRed = 0.33 + log(double(hd)) * log(double(mc)) / 2.25;

#define LATE_PRUNE_MIN 3 //3 range from 0 to 10
#define FUTILITY_SCALE 18
//#define FUTILITY_SCALE 18 //072613opt1 (15, originaly 20)