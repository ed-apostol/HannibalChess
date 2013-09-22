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

#ifdef EVAL_DEBUG
int showEval;
#else
#define showEval false
#endif

//attacks
/* the precomputed piece 64 bit attacks */
uint64 KnightMoves[64];
uint64 KingMoves[64];
uint64 PawnCaps[64][2];
uint64 PawnMoves[64][2];
uint64 PawnMoves2[64][2];
uint64 MagicAttacks[107648];



// eval
uint64 bewareTrapped[2];
uint64 escapeTrapped[2];


// material
/* the precomputed material values table and the flags table */
material_info_t MaterialTable[MAX_MATERIAL][MAX_MATERIAL];

/* the rank and file mask indexed by square */
uint64 RankMask[64];
uint64 FileMask[64];

/* the 64 bit attacks on 8 different ray directions + no direction */
uint64 DirBitmap[NO_DIR+1][64];

/* the 64 bit attacks between two squares of a valid ray direction */
uint64 InBetween[64][64];
/* */
#define MAX_FUT_MARGIN 10
int FutilityMarginTable[MAX_FUT_MARGIN+1][64];
int ReductionTable[2][64][64];
int needReplyReady;

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

int DrawValue[2];
/* this is used for pawn shelter and pawn storm */
uint64 PassedMask[2][64];
uint64 kingShelter[2][64]; // provides good shelter for king SAM1 added static
uint64 kingIndirectShelter[2][64]; // provides ok shelter for king SAM1 added static
/* used in debugging, etc.. */
FILE *logfile;
FILE *errfile;
FILE *dumpfile;
int origScore;





#ifndef TCEC
book_t GhannibalBook;
book_t GpolyglotBook;
learn_t Glearn;
mutex_t LearningLock[1];
mutex_t BookLock[1];
continuation_t movesSoFar;
#endif

uci_option_t Guci_options;






