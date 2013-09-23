/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#ifdef EVAL_DEBUG
static int showEval;
#else
#define showEval FALSE
#endif

/* the precomputed static piece 64 bit attacks */
static uint64 KnightMoves[64];
static uint64 KingMoves[64];
static uint64 PawnCaps[64][2];
static uint64 PawnMoves[64][2];
static uint64 PawnMoves2[64][2];

uint64 MagicAttacks[107648];

static uint64 bewareTrapped[2];
static uint64 escapeTrapped[2];

/* the precomputed material values table and the flags table */
static material_info_t MaterialTable[MAX_MATERIAL][MAX_MATERIAL];

/* the rank and file mask indexed by square */
static uint64 RankMask[64];
static uint64 FileMask[64];

/* the 64 bit attacks on 8 different ray directions + no direction */
static uint64 DirBitmap[NO_DIR+1][64];

/* the 64 bit attacks between two squares of a valid ray direction */
static uint64 InBetween[64][64];
/* */
#define MAX_FUT_MARGIN 10
static int FutilityMarginTable[MAX_FUT_MARGIN+1][64];
static int ReductionTable[2][64][64];
static int needReplyReady;

#define MoveGenPhaseEvasion 0
#define MoveGenPhaseStandard (MoveGenPhaseEvasion+3)
#define MoveGenPhaseQuiescenceAndChecksPV (MoveGenPhaseStandard+7)
#define MoveGenPhaseQuiescence (MoveGenPhaseQuiescenceAndChecksPV+6)
#define MoveGenPhaseQuiescenceAndChecks (MoveGenPhaseQuiescence+4)
#define MoveGenPhaseQuiescencePV (MoveGenPhaseQuiescenceAndChecks+5)
#define MoveGenPhaseRoot (MoveGenPhaseQuiescencePV+4)

static const int MoveGenPhase[] = {
    PH_NONE, PH_EVASION, PH_END, //MoveGenPhaseEvasion
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES, PH_KILLER_MOVES, PH_QUIET_MOVES, PH_BAD_CAPTURES, PH_END, //MoveGenPhaseStandard
    PH_NONE, PH_TRANS, PH_ALL_CAPTURES, PH_NONTACTICAL_CHECKS_PURE, PH_GAINING, PH_END, //MoveGenPhaseQuiescenceAndChecksPV
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES_PURE, PH_END, //MoveGenPhaseQuiescence
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES_PURE, PH_NONTACTICAL_CHECKS_WIN, PH_END, //MoveGenPhaseQuiescenceAndChecks
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES_PURE, PH_END, //MoveGenPhaseQuiescencePV
    PH_NONE, PH_ROOT, PH_END, //MoveGenPhaseRoot
};

/* contains the delta of possible piece moves between two squares,
zero otherwise */
static int DirFromTo[64][64];

/* used for pre-computed piece-square table */
static int PcSqTb[2048];
static int OutpostValue[2][64];
/* used in updating the castle status of the position */
static int CastleMask[64];
/* used as initial king penalty */
static int KingPosPenalty[2][64];

static int DrawValue[2];
/* this is used for pawn shelter and pawn storm */
static uint64 PassedMask[2][64];
static uint64 kingShelter[2][64]; // provides good shelter for king SAM1 added static
static uint64 kingIndirectShelter[2][64]; // provides ok shelter for king SAM1 added static
/* used in debugging, etc.. */
FILE *logfile;
FILE *errfile;
FILE *dumpfile;
static int origScore;


static transtable_t global_trans_table;
//#define TransTable(thread) (SearchInfo(thread).tt)
#define TransTable(thread) global_trans_table
pvhashtable_t PVHashTable;


#ifndef TCEC
static book_t GpolyglotBook;
#ifdef LEARNING
static book_t GhannibalBook;
static learn_t Glearn;
mutex_t LearningLock[1];
mutex_t BookLock[1];
continuation_t movesSoFar;
static search_info_t global_search_info;
static search_info_t* SearchInfoMap[MaxNumOfThreads];
#define SearchInfo(thread) (*SearchInfoMap[thread])
#else
static search_info_t global_search_info;
#define SearchInfo(thread) global_search_info
#endif
#endif
static uci_option_t Guci_options;

mutex_t SMPLock[1];
thread_t Threads[MaxNumOfThreads];



