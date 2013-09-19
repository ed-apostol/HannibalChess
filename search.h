/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "typedefs.h"

/* search.c */
inline int moveIsTactical(uint32 m);
inline int historyIndex(uint32 side, uint32 move);

extern void searchFromIdleLoop(SplitPoint* sp, const int thread_id);

template <bool inRoot, bool inSplitPoint, bool inSingular>
int searchNode(position_t *pos, int alpha, int beta, const int depth, SearchStack& ssprev, const int thread_id, NodeType nt);
extern void getBestMove(position_t *pos, int thread_id);