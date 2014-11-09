/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "typedefs.h"
#include "threads.h"
#include "trans.h"
#include "utils.h"
#include "uci.h"

/* the search data structure */
struct SearchInfo {
    void Init() {
        thinking_status = THINKING;
        stop_search = false;
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
        iteration = 0;
        bestmove = 0;
        pondermove = 0;
        mate_found = 0;
        currmovenumber = 0;
        multipvIdx = 0;
        legalmoves = 0;
        mvlist_initialized = false;
        memset(moves, 0, sizeof(moves));
        time_buffer = 0;
        contempt = 0;
        multipv = 0;
    }
    volatile int thinking_status;
    volatile bool stop_search; // TODO: replace with sthread.stop?

    int time_buffer;
    int contempt;
    int multipv;

    bool depth_is_limited;
    int depth_limit;
    bool moves_is_limited;
    bool time_is_limited;

    int64 time_limit_max;
    int64 time_limit_abs;
    bool node_is_limited;
    uint64 node_limit;

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

    int multipvIdx;

    int legalmoves;
    basic_move_t bestmove;
    basic_move_t pondermove;

    basic_move_t moves[MAXMOVES];
    bool mvlist_initialized;
    continuation_t rootPV;
};


class Search;

class Engine {
public:
    Engine();
    ~Engine();
    void PonderHit();
    void SendBestMove();
    void SearchFromIdleLoop(SplitPoint& sp, Thread& sthread);
    void GetBestMove(Thread& sthread);
    void StopSearch();
    void StartThinking(GoCmdData& data, position_t& pos);

    void InitTTHash(int size) {
        transtable.Init(size, transtable.BUCKET);
    }
    void InitPVTTHash(int size) {
        pvhashtable.Init(size, pvhashtable.BUCKET);
    }
    void ClearTTHash() {
        transtable.Clear();
    }
    void ClearPVTTHash() {
        pvhashtable.Clear();
    }
    void InitPawnHash(int size) {
        for (Thread* th : mThreads) th->pt.Init(size, th->pt.BUCKET);
    }
    void InitEvalHash(int size) {
        for (Thread* th : mThreads) th->et.Init(size, th->et.BUCKET);
    }
    void ClearPawnHash() {
        for (Thread* th : mThreads) th->pt.Clear();
    }
    void ClearEvalHash() {
        for (Thread* th : mThreads) th->et.Clear();
    }
    
    uint64 ComputeNodes() {
        uint64 nodes = 0;
        for (Thread* th : mThreads) nodes += th->nodes;
        return nodes;
    }
    void InitVars() {
        mMinSplitDepth = UCIOptionsMap["Min Split Depth"].GetInt();
        mMaxActiveSplitsPerThread = UCIOptionsMap["Max Active Splits/Thread"].GetInt();
        for (Thread* th : mThreads) {
            th->Init();
        }
        mThreads[0]->stop = false;
    }
    void SetNumThreads(int num) {
        while (mThreads.size() < num) {
            int id = (int)mThreads.size();
            mThreads.push_back(new Thread(id, &mThreads));
        }
        while (mThreads.size() > num) {
            delete mThreads.back();
            mThreads.pop_back();
        }
        InitPawnHash(UCIOptionsMap["Pawn Hash"].GetInt());
        InitEvalHash(UCIOptionsMap["Eval Cache"].GetInt());
    }
    void PrintThreadStats() {
        LogInfo() << "================================================================";
        for (Thread* th : mThreads) {
            LogInfo() << "thread_id: " << th->thread_id
                << " nodes: " << th->nodes
                << " joined_split: " << double(th->numsplits2 * 100.0) / double(th->numsplits)
                << " threads_per_split: " << double(th->workers2) / double(th->numsplits2);
        }
        LogInfo() << "================================================================";
    }
    volatile bool StillThinking() {
        return mThinking;
    }
    void WaitForThreadsToSleep() {
        for (Thread* th : mThreads) {
            while (!th->doSleep);
        }
    }
    void SetAllThreadsToStop() {
        for (Thread* th : mThreads) {
            th->stop = true;
            th->doSleep = true;
        }
    }
    void SetAllThreadsToWork() {
        for (Thread* th : mThreads) {
            if (th->thread_id != 0) th->TriggerCondition(); // thread_id == 0 is triggered separately
        }
    }
    Thread& ThreadFromIdx(int thread_id) {
        return *mThreads[thread_id];
    }
    size_t ThreadNum() const {
        return mThreads.size();
    }

    int mMinSplitDepth;
    int mMaxActiveSplitsPerThread;

    uint64 nodes_since_poll;
    uint64 nodes_between_polls;

    position_t rootpos;
    SearchInfo info;
private:
    volatile bool mThinking;
    Search* search;
    TranspositionTable transtable;
    PvHashTable pvhashtable;
    std::vector<Thread*> mThreads;
};

extern Engine CEngine; // TODO: move this inside Interface class

extern inline bool moveIsTactical(uint32 m);
extern inline int historyIndex(uint32 side, uint32 move);
