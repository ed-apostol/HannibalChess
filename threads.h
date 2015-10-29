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
        parent = nullptr;
        sscurr = nullptr;
        ssprev = nullptr;
        depth = 0;
        inCheck = false;
        inRoot = false;
        nodeType = PVNode;
        alpha = 0;
        beta = 0;
        bestvalue = 0;
        played = 0;
        hisCount = 0;
        bestmove = EMPTY;
        workersBitMask = 0;
        workAvailable = false;
        cutoff = false;
        joinedthreads = 0;
    }
    bool cutoffOccurred() {
        if (cutoff) return true;
        if (parent && parent->cutoffOccurred()) {
            cutoff = true;
            return true;
        }
        return false;
    }
    position_t* origpos;
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
    volatile int hisCount;
    volatile basic_move_t bestmove;
    volatile uint64 workersBitMask;
    volatile bool workAvailable;
    volatile bool cutoff;
    volatile uint64 joinedthreads;
    Spinlock movelistlock;
    Spinlock updatelock;
    Spinlock movesplayedlock;
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
protected:
    std::thread nativeThread;
    std::condition_variable sleepCondition;
    std::mutex threadLock;
};

class Engine; // HACK: forward declare

class Thread : public ThreadBase {
public:
    static const int MaxNumSplitPointsPerThread = 8;
    typedef std::function<void(Thread&)> CBFuncThink;
    typedef std::function<void(SplitPoint&, Thread&)> CBFuncSearch;

    Thread(int _thread_id, std::vector<Thread*>& _thread_group, CBFuncThink _getbest, CBFuncSearch _searchfromidle) :
        ThreadBase(_thread_id),
        mThreadGroup(_thread_group),
        CBGetBestMove(_getbest),
        CBSearchFromIdleLoop(_searchfromidle) {
        Init();
        NativeThread() = std::thread(&Thread::IdleLoop, this);
    }

    void Init();
    void IdleLoop();
    void GetWork(SplitPoint* const master_sp);
    void SearchSplitPoint(position_t& pos, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot);

    uint64 numsplits;
    uint64 numsplitsjoined;
    uint64 numworkers;
    uint64 nodes;

    volatile int num_sp;
    SplitPoint *activeSplitPoint;
    ThreadStack ts[MAXPLY];
    int32 evalgains[1024];
    int32 history[1024];
    PawnHashTable pt;
private:
    SplitPoint sptable[MaxNumSplitPointsPerThread];
    std::vector<Thread*>& mThreadGroup;
    CBFuncThink CBGetBestMove;
    CBFuncSearch CBSearchFromIdleLoop;
};

class TimerThread : public ThreadBase {
public:
    TimerThread(std::function<int64()> _cbfunc) : ThreadBase(0), CBFuncCheckTimer(_cbfunc) {
        Init();
        NativeThread() = std::thread(&TimerThread::IdleLoop, this);
    }
    void IdleLoop();
private:
    std::function<int64()> CBFuncCheckTimer;
};
