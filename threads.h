/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
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
    Spinlock() : m_Lock(false) { }
    void lock() { while (m_Lock.exchange(true)); }
    void unlock() { m_Lock.store(false); }
private:
    std::atomic<bool> m_Lock;
};

struct SplitPoint {
    SplitPoint() :
    parent(NULL),
    sscurr(NULL),
    ssprev(NULL),
    depth(0),
    inCheck(false),
    inRoot(false),
    nodeType(PVNode),
    alpha(0),
    beta(0),
    bestvalue(0),
    played(0),
    bestmove(EMPTY),
    workersBitMask(0),
    allWorkersBitMask(0),
    cutoff(false)
    { }
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
        numsplits = 1;
        numsplits2 = 1;
        workers2 = 0;
        num_sp = 0;
        activeSplitPoint = NULL;
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            sptable[j].workersBitMask = 0;
        }
        for (int Idx = 0; Idx < MAXPLY; Idx++) {
            ts[Idx].Init();
        }
        memset(history, 0, sizeof(history));
        memset(evalgains, 0, sizeof(evalgains));
    }
    uint64 nodes;
    uint64 nodes_since_poll;
    uint64 nodes_between_polls;
    uint64 numsplits; // DEBUG
    uint64 numsplits2; // DEBUG
    uint64 workers2; // DEBUG
    int num_sp;
    SplitPoint *activeSplitPoint;
    SplitPoint sptable[MaxNumSplitPointsPerThread];
    ThreadStack ts[MAXPLY];
    int32 evalgains[1024];
    int32 history[1024];
    EvalHashTable et;
    PawnHashTable pt;
};

class ThreadsManager {
public:
    ThreadsManager() : m_StartThinking(false) { }
    void StartThinking();
    void IdleLoop(const int thread_id);
    void GetWork(const int thread_id, SplitPoint* master_sp);
    void SetAllThreadsToStop();
    void SetAllThreadsToSleep();
    void SetAllThreadsToWork();
    void InitVars();
    void SetNumThreads(int num); // SetNumThreads(0) must be called for program to exit
    uint64 ComputeNodes();
    void SearchSplitPoint(const position_t& pos, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread);

    Thread& ThreadFromIdx(int thread_id) { return *m_Threads[thread_id]; }
    size_t ThreadNum() const { return m_Threads.size(); }

    void InitPawnHash(int size, int bucket) { for (Thread* th : m_Threads) th->pt.Init(size, bucket); }
    void InitEvalHash(int size, int bucket) { for (Thread* th : m_Threads) th->et.Init(size, bucket); }
    void ClearPawnHash() { for (Thread* th : m_Threads) th->pt.Clear(); }
    void ClearEvalHash() { for (Thread* th : m_Threads) th->et.Clear(); }

    void PrintDebugData() {
        Print(2, "================================================================\n");
        for (Thread* th : m_Threads) {
            Print(2, "%s: thread_id: %d, nodes: %d joined_split: %0.2f%% threads_per_split: %0.2f\n",
                __FUNCTION__, th->thread_id, th->nodes,
                double(th->numsplits2 * 100.0) / double(th->numsplits), double(th->workers2) / double(th->numsplits2));
        }
        Print(2, "================================================================\n");
    }
    bool StillThinking() { return m_StartThinking; }

    int m_MinSplitDepth;
    int m_MaxThreadsPerSplit;
    int m_MaxActiveSplitsPerThread;
private:
    std::vector<Thread*> m_Threads;
    volatile bool m_StartThinking;
    volatile bool m_StopThreads;
};

extern ThreadsManager ThreadsMgr;

