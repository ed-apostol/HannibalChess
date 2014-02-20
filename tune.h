/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

/* tune.h */
#ifdef OPTIMIZE
extern void optimize(position_t *pos, int threads);
#endif
#ifdef SELF_TUNE2
extern Spinlock SMPLock[1]; // ThreadsPool
extern void NewTuneGame(const int player1);
#endif