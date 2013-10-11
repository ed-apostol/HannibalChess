
#pragma once

#include <vector>
#include <thread>
#include <condition_variable>
#include "utils.h"


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
    Thread(int _thread_id)
    {
        SplitPoint* sp = NULL;
        Init();
        thread_id = _thread_id;
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
    int thread_id;
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

extern bool smpCutoffOccurred(SplitPoint *sp); // SplitPoint

class ThreadMgr {
public:
    void idleLoop(int thread_id);
    void checkForWork(int thread_id);
    void helpfulMaster(int thread_id, SplitPoint *master_sp);
    void setAllThreadsToStop();
    void setAllThreadsToSleep();
    void wakeUpThreads();
    void initSmpVars();
    void initThreads(void);
    void stopThreads(void);
    uint64 computeNodes();
    bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread);
    Thread& ThreadFromIdx(int thread_id) { return *m_Threads[thread_id]; }
private:
    std::vector<Thread*> m_Threads;
};

extern ThreadMgr ThreadsMgr;

