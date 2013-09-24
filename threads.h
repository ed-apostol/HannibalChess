
#pragma once

#include <vector>
#include <thread>
#include "search.h"



struct ThreadStack {
    void Init () { 
        killer1 = EMPTY;
        killer2 = EMPTY;
    }
    basic_move_t killer1;
    basic_move_t killer2;
};

struct thread_t {
    SplitPoint *split_point;
    volatile bool stop;
    volatile bool running;
    volatile bool searching;
    volatile bool exit_flag;
    HANDLE idle_event;
    uint64 nodes;
    uint64 nodes_since_poll;
    uint64 nodes_between_polls;
    uint64 started; // DEBUG
    uint64 ended; // DEBUG
    int64 numsplits; // DEBUG
    int num_sp;
    ThreadStack ts[MAXPLY];
    SplitPoint sptable[MaxNumSplitPointsPerThread];
#ifdef SELF_TUNE2
    bool playingGame;
#endif
};

extern std::mutex SMPLock[1]; // ThreadsPool
extern thread_t Threads[MaxNumOfThreads]; // ThreadsPool this should be std::vector


extern std::vector<std::thread> RealThreads;

    //////for(int i = 0; i < 5; ++i){
    //////    //threads.push_back(std::thread(hello));
    //////}

    ////////for(auto& thread : threads){
    //////    //thread.join();
    ////////}

extern void setAllThreadsToStop(int thread);
extern void initSearchThread(int i);
extern bool smpCutoffOccurred(SplitPoint *sp);
extern void initSmpVars();
extern bool idleThreadExists(int master);
extern void initThreads(void);
extern void stopThreads(void);
extern bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master);

//////class Thread {
//////public:
//////    Thread ();
//////    ~Thread ();
//////private:
//////    SplitPoint *split_point;
//////    volatile bool stop;
//////    volatile bool running;
//////    volatile bool searching;
//////    volatile bool exit_flag;
//////    HANDLE idle_event;
//////    uint64 nodes;
//////    uint64 nodes_since_poll;
//////    uint64 nodes_between_polls;
//////    uint64 started; // DEBUG
//////    uint64 ended; // DEBUG
//////    int64 numsplits; // DEBUG
//////    int num_sp;
//////    ThreadStack ts[MAXPLY];
//////    SplitPoint sptable[MaxNumSplitPointsPerThread];
//////};
//////
//////class ThreadsTab {
//////public:
//////    ThreadsTab ();
//////    ~ThreadsTab ();
//////    void ResizeNumThreads(int num);
//////private:
//////    std::vector<Thread*> m_ThreadsA;
//////};
//////
//////std::thread Thraeds;