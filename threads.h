
#pragma once

#include <vector>
#include <thread>
#include <condition_variable>
#include "utils.h"
#include "trans.h"

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
};

struct ThreadStack {
    void Init () { 
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
        searching = false;
        stop = false;
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
    std::thread& NativeThread() { return nativeThread; }

    int thread_id;
    volatile bool stop;
    volatile bool doSleep;
    volatile bool searching;
    volatile bool exit_flag;
private:
    std::thread nativeThread;
    std::condition_variable sleepCondition;
    std::mutex threadLock;
};

class Thread : public ThreadBase {
public:
    Thread(int _thread_id) : ThreadBase(_thread_id) {
        Init();
    }
    void Init() {
        ThreadBase::Init();
        nodes = 0;
        nodes_since_poll = 0;
        nodes_between_polls = 8192;
        started = 0;
        ended = 0;
        numsplits = 0;
        joined = 0;
        num_sp = 0;
        activeSplitPoint = NULL;
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            sptable[j].workersBitMask = 0;
        }
        for (int Idx = 0; Idx < MAXPLY; Idx++) {
            ts[Idx].Init();
        }
        pt.Init(INIT_PAWN, PAWN_ASSOC);
        et.Init(INIT_EVAL, EVAL_ASSOC);
    }
    uint64 nodes;
    uint64 nodes_since_poll;
    uint64 nodes_between_polls;
    uint64 started; // DEBUG
    uint64 ended; // DEBUG
    uint64 numsplits; // DEBUG
    uint64 joined; // DEBUG
    int num_sp;
    SplitPoint *activeSplitPoint;
    SplitPoint sptable[MaxNumSplitPointsPerThread]; // TODO: convert to vector?
    ThreadStack ts[MAXPLY];
    EvalHashTable et;
    PawnHashTable pt;
};

class ThreadMgr {
public:
    void IdleLoop(const int thread_id);
    void GetWork(const int thread_id, SplitPoint *master_sp);
    void SetAllThreadsToStop();
    void SetAllThreadsToSleep();
    void SetAllThreadsToWork();
    void InitVars();
    void SetNumThreads(int num);
    uint64 ComputeNodes();
    void SearchSplitPoint(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread);
    Thread& ThreadFromIdx(int thread_id) { return *m_Threads[thread_id]; }
    size_t ThreadNum() const { return m_Threads.size(); }
    void InitPawnHash(int size) { for (Thread* th: m_Threads) th->pt.Init(size, PAWN_ASSOC); }
    void InitEvalHash(int size) { for (Thread* th: m_Threads) th->et.Init(size, EVAL_ASSOC); }
    void ClearPawnHash() { for (Thread* th: m_Threads) th->pt.Clear(); }
    void ClearEvalHash() { for (Thread* th: m_Threads) th->et.Clear(); }
    void PrintDebugData() {
        Print(2, "================================================================\n");
        for (Thread* th: m_Threads) {
            Print(2, "%s: thread_id:%d, num_sp:%d searching:%d stop:%d started:%d ended:%d nodes:%d numsplits:%d joined:%d\n", 
                __FUNCTION__, th->thread_id, 
                th->num_sp, th->searching, th->stop, 
                th->started, th->ended, th->nodes, th->numsplits, th->joined);
        }
    }
private:
    std::vector<Thread*> m_Threads;
};

extern ThreadMgr ThreadsMgr;

