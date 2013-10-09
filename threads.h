
#pragma once

#include <vector>
#include <thread>
#include <condition_variable>
#include "search.h"
#include "utils.h"

extern void idleLoop(int thread_id, SplitPoint *master_sp);

struct ThreadStack {
    void Init () { 
        killer1 = EMPTY;
        killer2 = EMPTY;
    }
    basic_move_t killer1;
    basic_move_t killer2;
};

class Thread {
public:
    Thread(int thread_id)
    {
        SplitPoint* sp = NULL;
        Init();
        doSleep = true;
        //realThread = std::thread(idleLoop, thread_id, sp);
    }
    ~Thread() {
        doSleep = false;
        stop = true;
        exit_flag = true;
        searching = false;        
        Print(1, "gone here 1\n");
        triggerCondition();
        Print(1, "gone here 2\n");
        realThread.join();
        Print(1, "gone here 3\n");
    }
    void sleepAndWaitForCondition() {
        std::unique_lock<std::mutex> lk(threadLock);
        idle_event.wait(lk);
    }
    void triggerCondition() {
        doSleep = false;
        std::unique_lock<std::mutex>(threadLock);
        idle_event.notify_one();
    }
    void Init();

    SplitPoint *split_point;
    volatile bool stop;
    volatile bool doSleep;
    volatile bool searching;
    volatile bool exit_flag;

    std::thread realThread;
    uint64 nodes;
    uint64 nodes_since_poll;
    uint64 nodes_between_polls;
    uint64 started; // DEBUG
    uint64 ended; // DEBUG
    int64 numsplits; // DEBUG
    int num_sp;
    ThreadStack ts[MAXPLY];
    SplitPoint sptable[MaxNumSplitPointsPerThread];
private:
    std::condition_variable idle_event;
    std::mutex threadLock;
};

extern std::vector<Thread*> Threads; // ThreadsPool this should be std::vector

extern bool smpCutoffOccurred(SplitPoint *sp); // SplitPoint

extern void setAllThreadsToStop(int thread); // ThreadsPool
extern void initSmpVars();
extern void initThreads(void); // ThreadsPool
extern void stopThreads(void); // ThreadsPool
extern bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master);

//////class Thread {
//////public:
//////    Thread ();
//////    ~Thread ();
//////private:
//////    SplitPoint *split_point;
//////    volatile bool stop;
//////    volatile bool searching;
//////    volatile bool exit_flag; // TODO: move to threads pool
//////    std::condition_variable idle_event;
//////    std::mutex threadLock;
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
//////    std::mutex SMPLock[1];
//////private:
//////    std::vector<Thread*> m_ThreadsA;
//////};


