/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#ifdef EVAL_DEBUG
extern bool SHOW_EVAL;
#else
#define SHOW_EVAL false
#endif

/* the precomputed extern piece 64 bit attacks */
extern uint64 KnightMoves[64];
extern uint64 KingMoves[64];
extern uint64 PawnCaps[64][2];
extern uint64 PawnMoves[64][2];
extern uint64 PawnMoves2[64][2];

extern uint64 MagicAttacks[107648];

/* the precomputed material values table and the flags table */
extern material_info_t MaterialTable[MAX_MATERIAL][MAX_MATERIAL];

/* the rank and file mask indexed by square */
extern uint64 RankMask[64];
extern uint64 FileMask[64];

/* the 64 bit attacks on 8 different ray directions + no direction */
extern uint64 DirBitmap[NO_DIR + 1][64];

/* the 64 bit attacks between two squares of a valid ray direction */
extern uint64 InBetween[64][64];
/* */
#define MAX_FUT_MARGIN 10
extern int FutilityMarginTable[MAX_FUT_MARGIN + 1][64];
extern int ReductionTable[2][64][64];

#define MoveGenPhaseEvasion 0
#define MoveGenPhaseStandard (MoveGenPhaseEvasion+3)
#define MoveGenPhaseQuiescenceAndChecksPV (MoveGenPhaseStandard+7)
#define MoveGenPhaseQuiescence (MoveGenPhaseQuiescenceAndChecksPV+5)
#define MoveGenPhaseQuiescenceAndChecks (MoveGenPhaseQuiescence+4)
#define MoveGenPhaseRoot (MoveGenPhaseQuiescenceAndChecks+5)

extern const int MoveGenPhase[];

/* contains the delta of possible piece moves between two squares,
zero otherwise */
extern int DirFromTo[64][64];

/* used for pre-computed piece-square table */
extern EvalScore PieceSquareTable[2][KING][64];
/* used in updating the castle status of the position */
extern int CastleMask[64];
/* used as initial king penalty */
extern int KingPosPenalty[2][64];

extern int DrawValue[2];
extern uint64 kingShelter[2][64]; // provides good shelter for king 
extern uint64 kingIndirectShelter[2][64]; // provides ok shelter for king 

// used in setting up the position and eval symmetry
extern const char *FenString[];

extern uint64(*FillPtr[])(uint64);
extern uint64(*FillPtr2[])(uint64);
extern uint64(*ShiftPtr[])(uint64, uint32);
