/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "utils.h"
#include "trans.h"

class Spinlock {
public:
    Spinlock() {
        mLock.clear(std::memory_order_release);
    }
    void lock() {
        while (mLock.test_and_set(std::memory_order_acquire));
    }
    void unlock() {
        mLock.clear(std::memory_order_release);
    }
private:
    std::atomic_flag mLock;
};

struct SplitPoint {
    SplitPoint() {
        Init();
    }
    void Init() {
        parent = NULL;
        sscurr = NULL;
        ssprev = NULL;
        depth = 0;
        inCheck = false;
        inRoot = false;
        nodeType = PVNode;
        alpha = 0;
        beta = 0;
        bestvalue = 0;
        played = 0;
        bestmove = EMPTY;
        workersBitMask = 0;
        allWorkersBitMask = 0;
        cutoff = false;
    }
    bool cutoffOccurred() {
        if (cutoff) return true;
        if (parent && parent->cutoffOccurred()) {
            cutoff = true;
            return true;
        }
        return false;
    }
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
    Spinlock movelistlock;
    Spinlock updatelock;
};

struct ThreadStack {
    void Init() {
        killer1 = EMPTY;
        killer2 = EMPTY;
    }
    basic_move_t killer1;
    basic_move_t killer2;
};

class ThreadBase {
public:
    ThreadBase(int _thread_id) : thread_id(_thread_id) {
        Init();
        doSleep = true;
    }
    ~ThreadBase() {
        stop = true;
        exit_flag = true;
        TriggerCondition();
        nativeThread.join();
    }
    virtual void Init() {
        stop = true;
        exit_flag = false;
    }
    void SleepAndWaitForCondition() {
        std::unique_lock<std::mutex> lk(threadLock);
        sleepCondition.wait(lk);
    }
    void TriggerCondition() {
        doSleep = false;
        std::unique_lock<std::mutex>(threadLock);
        sleepCondition.notify_one();
    }
    std::thread& NativeThread() {
        return nativeThread;
    }

    int thread_id;
    volatile bool stop;
    volatile bool doSleep;
    volatile bool exit_flag;
private:
    std::thread nativeThread;
    std::condition_variable sleepCondition;
    std::mutex threadLock;
};

class Thread : public ThreadBase {
public:
    static const int MaxNumSplitPointsPerThread = 8;

    Thread(int _thread_id, std::vector<Thread*>* const _thread_group) : ThreadBase(_thread_id) {
        Init();
        mThreadGroup = _thread_group;
        NativeThread() = std::thread(&Thread::IdleLoop, this);
    }
    ~Thread() {
        //threadgroup = NULL;
    }
    void Init();
    void IdleLoop();
    void GetWork(SplitPoint* const master_sp);
    void SearchSplitPoint(const position_t& pos, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot);

    uint64 nodes;
    uint64 numsplits; // DEBUG
    uint64 numsplits2; // DEBUG
    uint64 workers2; // DEBUG
    volatile int num_sp;
    SplitPoint *activeSplitPoint;
    SplitPoint sptable[MaxNumSplitPointsPerThread];
    ThreadStack ts[MAXPLY];
    int32 evalgains[1024];
    int32 history[1024];
    EvalHashTable et;
    PawnHashTable pt;
    std::vector<Thread*>* mThreadGroup;
};


