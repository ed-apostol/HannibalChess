
#pragma once

extern mutex_t SMPLock[1];
extern thread_t Threads[MaxNumOfThreads];

extern void setAllThreadsToStop(int thread);
extern void initSearchThread(int i);
extern bool smpCutoffOccurred(SplitPoint *sp);
extern void initSmpVars();
extern bool idleThreadExists(int master);
extern void initThreads(void);
extern void stopThreads(void);
extern bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master);
