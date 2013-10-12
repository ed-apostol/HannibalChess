
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
    bool cutoffOccurred() {
        if (cutoff) return true;
        if (parent && parent->cutoffOccurred()) {
            cutoff = true;
            return true;
        }
        return false;
    }
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

class Thread { // TODO: extract baseclass
public:
    Thread(int _thread_id)
    {
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
    void Init() {
        searching = false;
        stop = false;
        exit_flag = false;
        nodes = 0;
        nodes_since_poll = 0;
        nodes_between_polls = 8192;
        started = 0;
        ended = 0;
        numsplits = 0;
        num_sp = 0;
        split_point = NULL;
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            for (int k = 0; k < MaxNumOfThreads; ++k) {
                sptable[j].workersBitMask = 0;
            }
        }
        for (int Idx = 0; Idx < MAXPLY; Idx++) {
            ts[Idx].Init();
        }
    }
    std::thread& RThread() { return realThread; }

    volatile bool stop;
    volatile bool doSleep;
    volatile bool searching;
    volatile bool exit_flag;

    int thread_id;
    uint64 nodes;
    uint64 nodes_since_poll;
    uint64 nodes_between_polls;
    uint64 started; // DEBUG
    uint64 ended; // DEBUG
    int64 numsplits; // DEBUG
    int num_sp;
    SplitPoint *split_point;
    SplitPoint sptable[MaxNumSplitPointsPerThread];
    ThreadStack ts[MAXPLY];
private:
    std::thread realThread;
    std::condition_variable idle_event;
    std::mutex threadLock;
};

class ThreadMgr {
public:
    void idleLoop(const int thread_id);
    void getWork(const int thread_id, SplitPoint *master_sp);
    void setAllThreadsToStop();
    void setAllThreadsToSleep();
    void wakeUpThreads();
    void initVars();
    void spawnThreads(int num);
    void killThreads(void);
    uint64 computeNodes();
    void searchSplitPoint(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread);
    Thread& threadFromIdx(int thread_id) { return *m_Threads[thread_id]; }
    size_t threadNum() const { return m_Threads.size(); }
private:
    std::vector<Thread*> m_Threads;
};

extern ThreadMgr ThreadsMgr;

