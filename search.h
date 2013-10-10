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
#include "utils.h"


/* the search data structure */
struct SearchInfo{
    void Init() {
        /* initialization */
        depth_is_limited = false;
        depth_limit = MAXPLY;
        moves_is_limited = false;
        time_is_limited = false;
        time_limit_max = 0;
        time_limit_abs = 0;
        node_is_limited = false;
        node_limit = 0;
        start_time = last_time = getTime();
        alloc_time = 0;
        best_value = -INF;
        last_value = -INF;
        last_last_value = -INF;
        change = 0;
        research = 0;
        bestmove = 0;
        pondermove = 0;
        mate_found = 0;
        memset(history, 0, sizeof(history)); //TODO this is bad to share with learning
        memset(evalgains, 0, sizeof(evalgains)); //TODO this is bad to share with learning

        // DEBUG
        cutnodes = 1;
        allnodes = 1;
        cutfail = 1;
        allfail = 1;

        memset(moves, 0, sizeof(moves));
    }
    int thinking_status;
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
    SearchInfo& Info() { return info; }
private:
    SearchInfo info;
    Search* search;
};

extern void quit(void);