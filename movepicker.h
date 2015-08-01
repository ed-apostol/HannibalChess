/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void sortInit(const position_t& pos, movelist_t& mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, Thread& sthread);
extern move_t* getMove(movelist_t& mvlist);
extern move_t* sortNext(SplitPoint* sp, SearchInfo& info, position_t& pos, movelist_t& mvlist, int& phase, Thread& sthread);
