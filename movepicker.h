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
extern void sortInit (const position_t *pos, SearchStack& ss, uint64 pinned, int scout, int depth, int type, Thread& sthread);
extern move_t* sortNext (SplitPoint* sp, position_t *pos, SearchStack& ss, Thread& sthread);
extern bool moveIsPassedPawn (const position_t * pos, uint32 move);
extern move_t* getMove (movelist_t *mvlist);