
#pragma once

#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
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
    volatile bool searching;
    volatile bool exit_flag; // TODO: move to threads pool
    std::condition_variable idle_event;
    std::mutex threadLock;
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

extern void idleLoop(int thread_id, SplitPoint *master_sp);
extern void setAllThreadsToStop(int thread); // ThreadsPool
extern void initSearchThread(int i);
extern bool smpCutoffOccurred(SplitPoint *sp); // static
extern void initSmpVars(); // ThreadsPool
extern bool idleThreadExists(int master); // ThreadsPool
extern void initThreads(void); // ThreadsPool
extern void stopThreads(void); // ThreadsPool
extern bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master);


//////class ThreadBase {
//////public:
//////    ThreadBase() : exit_flag(false) {}
//////    virtual ~ThreadBase() {}
//////    virtual void idleLoop() = 0;
//////    void notifyOne() {
//////        std::unique_lock<std::mutex>(m_ThreadLock);
//////        m_IdleEvent.notify_one();
//////    }
//////    void waitFor(volatile const bool& b) {
//////        std::unique_lock<std::mutex> lk(m_ThreadLock);
//////        m_IdleEvent.wait(lk, [&]{ return b; });
//////    }
//////private:
//////    std::condition_variable m_IdleEvent;
//////    std::mutex m_ThreadLock;
//////    std::vector<std::thread> realThread;
//////    volatile bool exit_flag;
//////};
//////
//////class SearchThread : public ThreadBase {
//////public:
//////    SearchThread ();
//////    ~SearchThread ();
//////private:
//////    SplitPoint *split_point;
//////    volatile bool stop;
//////    volatile bool searching;
//////
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
//////class ThreadPool {
//////public:
//////    ThreadPool () {
//////
//////    }
//////    ~ThreadPool () {
//////
//////    }
//////    void ResizeNumThreads(int num);
//////    std::mutex SMPLock[1];
//////private:
//////    std::vector<SearchThread*> m_ThreadsA;
//////};






class Spinlock {
public:
    Spinlock() { lock.clear(std::memory_order_release); }
    ~Spinlock() { }
    void acquire() { while (lock.test_and_set(std::memory_order_acquire)); }
    void release() { lock.clear(std::memory_order_release); }
private:
    std::atomic_flag lock;
};

