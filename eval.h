/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#include "trans.h"

/* the eval info structure */
struct eval_info_t {
    uint64 atkall[2];
    uint64 atkpawns[2];
    uint64 atkknights[2];
    uint64 atkbishops[2];
    uint64 kingatkbishops[2];
    uint64 atkrooks[2];
    uint64 kingatkrooks[2];
    uint64 atkqueens[2];
    uint64 atkkings[2];
    uint64 kingzone[2];
    uint64 kingadj[2];
    uint64 potentialPawnAttack[2];
    uint64 pawns[2];
    int mid_score[2];
    int end_score[2];
    int draw[2];
    int atkcntpcs[2];
    int phase;
    int MLindex[2];
    int flags;
    int queening;
    uint8 endFlags[2];
    PawnEntry* pawn_entry;
};

extern int eval(const position_t& pos, int thread_id, int *pessimism);
extern void evalEndgame(int attacker, const position_t& pos, eval_info_t *ei, int *score, int *draw, int mover);
