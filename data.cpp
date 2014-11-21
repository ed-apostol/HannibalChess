/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include <cstdlib>
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"

/* the precomputed piece 64 bit attacks */
uint64 KnightMoves[64];
uint64 KingMoves[64];
uint64 PawnCaps[64][2];
uint64 PawnMoves[64][2];
uint64 PawnMoves2[64][2];

uint64 MagicAttacks[107648];

uint64 bewareTrapped[2];

/* the precomputed material values table and the flags table */
material_info_t MaterialTable[MAX_MATERIAL][MAX_MATERIAL];

/* the rank and file mask indexed by square */
uint64 RankMask[64];
uint64 FileMask[64];

/* the 64 bit attacks on 8 different ray directions + no direction */
uint64 DirBitmap[NO_DIR + 1][64];

/* the 64 bit attacks between two squares of a valid ray direction */
uint64 InBetween[64][64];
/* */
#define MAX_FUT_MARGIN 10
int FutilityMarginTable[MAX_FUT_MARGIN + 1][64];
int ReductionTable[2][64][64];

const int MoveGenPhase[] = {
    PH_NONE, PH_EVASION, PH_END, //MoveGenPhaseEvasion
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES, PH_KILLER_MOVES, PH_QUIET_MOVES, PH_BAD_CAPTURES, PH_END, //MoveGenPhaseStandard
    PH_NONE, PH_TRANS, PH_ALL_CAPTURES, PH_NONTACTICAL_CHECKS_PURE, PH_END, //MoveGenPhaseQuiescenceAndChecksPV
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES_PURE, PH_END, //MoveGenPhaseQuiescence
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES_PURE, PH_NONTACTICAL_CHECKS_WIN, PH_END, //MoveGenPhaseQuiescenceAndChecks
    PH_NONE, PH_TRANS, PH_GOOD_CAPTURES_PURE, PH_END, //MoveGenPhaseQuiescencePV
    PH_NONE, PH_ROOT, PH_END, //MoveGenPhaseRoot
};

/* contains the delta of possible piece moves between two squares,
zero otherwise */
int DirFromTo[64][64];

/* used for pre-computed piece-square table */
int PcSqTb[2048];
int OutpostValue[2][64];
/* used in updating the castle status of the position */
int CastleMask[64];
/* used as initial king penalty */
int KingPosPenalty[2][64];

pos_store_t UndoStack[MAX_HASH_STORE];

int DrawValue[2];
/* this is used for pawn shelter and pawn storm */
uint64 PassedMask[2][64];
uint64 kingShelter[2][64]; // provides good shelter for king SAM1 added static
uint64 kingIndirectShelter[2][64]; // provides ok shelter for king SAM1 added static

// used in setting up the position and eval symmetry
const char *FenString[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    "r3k2r/3q4/2n1b3/7n/1bB5/2N2N2/1B2Q3/R3K2R w KQkq - 0 1",
    "rnbq1bnr/1pppkp1p/4p3/2P1P3/p5p1/8/PP1PKPPP/RNBQ1BNR w - - 0 1",
    "rn1q1bnr/1bP1kp1P/1p2p3/p7/8/8/PP1pKPpP/RNBQ1BNR w - - 0 1",
    "r6r/3qk3/2n1b3/7n/1bB5/2N2N2/1B2QK2/R6R w - - 0 1",
    nullptr
};

/* used in fill algorithms */
uint64(*FillPtr[])(uint64) = { &fillUp, &fillDown };
uint64(*FillPtr2[])(uint64) = { &fillUp2, &fillDown2 };
uint64(*ShiftPtr[])(uint64, uint32) = { &shiftLeft, &shiftRight };