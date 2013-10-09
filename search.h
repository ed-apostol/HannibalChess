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
#include "threads.h"
#include "trans.h"


/* the search data structure */
struct SearchInfo{
    int thinking_status;
#ifndef TCEC
    int outOfBook;
#endif
    bool depth_is_limited;
    int depth_limit;
    bool moves_is_limited;
    bool time_is_limited;

    int64 time_limit_max;
    int64 time_limit_abs;
    bool node_is_limited;
    int64 node_limit;

    int64 start_time;
    int64 last_time;
    int64 alloc_time;

    int last_last_value;
    int last_value;
    int best_value;

    int mate_found;
    int currmovenumber;
    int change;
    int research;
    int iteration;

    // DEBUG
    uint64 cutnodes;
    uint64 allnodes;
    uint64 cutfail;
    uint64 allfail;

    int rbestscore1;
    int rbestscore2;

    int lastDepthSearched;

    int legalmoves;
    basic_move_t bestmove;
    basic_move_t pondermove;

    basic_move_t moves[MAXMOVES];
    bool mvlist_initialized;
    continuation_t rootPV;
    int32 evalgains[1024];
    int32 history[1024];
    EvalHashTable et;
    PawnHashTable pt;
    TranspositionTable tt;
};


// TODO: to be deleted
#ifndef TCEC
SearchInfo global_search_info;
SearchInfo* SearchInfoMap[MaxNumOfThreads];
#define SearchInfo(thread) (*SearchInfoMap[thread])
#else
extern SearchInfo global_search_info;
#define SearchInfo(thread) global_search_info
#endif






/* search.c */
extern inline int moveIsTactical(uint32 m);
extern inline int historyIndex(uint32 side, uint32 move);

class Search;

class SearchMgr {
public:
    SearchMgr();
    ~SearchMgr();
    static SearchMgr& Inst() { static SearchMgr inst; return inst; }
    void searchFromIdleLoop(SplitPoint* sp, const int thread_id);
    void getBestMove(position_t *pos, int thread_id);
    void checkSpeedUp(position_t* pos, char string[]);
    void benchMinSplitDepth(position_t* pos, char string[]);
    void benchThreadsperSplit(position_t* pos, char string[]);
    void benchActiveSplits(position_t* pos, char string[]);
private:
    SearchInfo info;
    Search* search;
};

extern void quit(void);