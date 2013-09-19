/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "typedefs.h"

/* search.c */
extern inline int moveIsTactical(uint32 m);
extern inline int historyIndex(uint32 side, uint32 move);

extern void searchFromIdleLoop(SplitPoint* sp, const int thread_id);

extern void getBestMove(position_t *pos, int thread_id);

extern void checkSpeedUp(position_t* pos, char string[]);
extern void benchSplitDepth(position_t* pos, char string[]);
extern void benchSplitThreads(position_t* pos, char string[]);

extern void quit(void);