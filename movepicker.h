/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

/* movepicker.c */
extern void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, Thread& sthread);
extern move_t* sortNext(SplitPoint* sp, position_t *pos, SearchStack& ss, int thread_id);
extern bool moveIsPassedPawn(const position_t * pos, uint32 move);