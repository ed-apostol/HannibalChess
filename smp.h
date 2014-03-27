/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void initSearchThread(int i);
extern void setAllThreadsToStop(int thread);
extern void idleLoop(const int thread_id, SplitPoint *master_sp);
extern bool smpCutoffOccurred(SplitPoint *sp);
extern void initSmpVars();
extern bool threadIsAvailable(int thread_id, int master);
extern bool idleThreadExists(int master);
extern DWORD WINAPI winInitThread(LPVOID n);
extern void initThreads(void);
extern void stopThreads(void);
extern bool splitRemainingMoves(const position_t& pos, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master);