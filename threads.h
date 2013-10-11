
#pragma once

#include <vector>
#include <thread>
#include <condition_variable>
#include "utils.h"

class Spinlock {
public:
    Spinlock() { m_Lock.clear(std::memory_order_release); }
    ~Spinlock() { }
    void lock() { while (m_Lock.test_and_set(std::memory_order_acquire)); }
    void unlock() { m_Lock.clear(std::memory_order_release); }
private:
    std::atomic_flag m_Lock;
};

struct SplitPoint {
    position_t pos[MaxNumOfThreads];
    position_t origpos;
    SplitPoint* parent;
    SearchStack* sscurr;
    SearchStack* ssprev;
    int depth;
    bool inCheck;
    bool inRoot;
    NodeType nodeType;
    volatile int alpha;
    volatile int beta;
    volatile int bestvalue;
    volatile int played;
    volatile basic_move_t bestmove;
    volatile uint64 workersBitMask;
    volatile uint64 allWorkersBitMask;
    volatile bool cutoff;
    Spinlock movelistlock[1];
    Spinlock updatelock[1];
    //std::mutex movelistlock[1];
    //std::mutex updatelock[1];
};

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
    }
    ~Thread() {
        doSleep = false;
        stop = true;
        exit_flag = true;
        searching = false;
        triggerCondition();
        realThread.join();
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
    void idleLoop(const int thread_id);
    void checkForWork(const int thread_id);
    void helpfulMaster(const int thread_id, SplitPoint *master_sp);
    void setAllThreadsToStop();
    void setAllThreadsToSleep();
    void wakeUpThreads();
    void initSmpVars();
    void initThreads(int num);
    void stopThreads(void);
    uint64 computeNodes();
    bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread);
    Thread& ThreadFromIdx(int thread_id) { return *m_Threads[thread_id]; }
    size_t Size() const { return m_Threads.size(); }
private:
    std::vector<Thread*> m_Threads;
};

extern ThreadMgr ThreadsMgr;

