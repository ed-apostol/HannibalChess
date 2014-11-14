/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "threads.h"

extern void genLegal(const position_t& pos, movelist_t& mvlist, int promoteAll);
extern void genGainingMoves(const position_t& pos, movelist_t& mvlist, int delta, Thread& sthread);
extern void genNonCaptures(const position_t& pos, movelist_t& mvlist);
extern void genCaptures(const position_t& pos, movelist_t& mvlist);
extern void genEvasions(const position_t& pos, movelist_t& mvlist);
extern void genQChecks(const position_t& pos, movelist_t& mvlist);
extern bool genMoveIfLegal(const position_t& pos, uint32 move, uint64 pinned);
