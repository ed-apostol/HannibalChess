/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, int thread_id);
extern move_t* getMove(movelist_t *mvlist);
extern BOOL moveIsPassedPawn(const position_t * pos, uint32 move);
extern move_t* sortNext(SplitPoint* sp, position_t *pos, movelist_t *mvlist, int& phase, int thread_id);