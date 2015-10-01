/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "search.h"
#include "threads.h"
#include "utils.h"
#include "bitutils.h"
#include "uci.h"

void Thread::Init() {
    ThreadBase::Init();
    nodes = 0;
    num_sp = 0;
    numsplits = 0;
    numsplitsjoined = 0;
    numworkers = 0;
    activeSplitPoint = nullptr;
    for (int Idx = 0; Idx < MaxNumSplitPointsPerThread; ++Idx) {
        sptable[Idx].Init();
    }
    for (int Idx = 0; Idx < MAXPLY; ++Idx) {
        ts[Idx].Init();
    }
    memset(history, 0, sizeof(history));
    memset(evalgains, 0, sizeof(evalgains));
}

void Thread::IdleLoop() {
    SplitPoint* const master_sp = activeSplitPoint;
    while (!exit_flag) {
        if (!exit_flag && master_sp == nullptr && doSleep) {
            SleepAndWaitForCondition();
        }
        if (!exit_flag && !doSleep && master_sp == nullptr && thread_id == 0) {
            LogInfo() << "IdleLoop: Main thread waking up to start searching!";
            CBGetBestMove(*this);
        }
        if (!exit_flag && !doSleep && stop) {
            GetWork(master_sp);
        }
        if (!exit_flag && !doSleep && !stop) {
            SplitPoint* const sp = activeSplitPoint;
            CBSearchFromIdleLoop(*sp, *this);
            std::lock_guard<Spinlock> lck(sp->updatelock);
            sp->workersBitMask &= ~((uint64)1 << thread_id);
            sp->workAvailable = false;
            activeSplitPoint = nullptr;
            stop = true;
        }
        if (master_sp != nullptr && (!master_sp->workersBitMask || doSleep)) return;
    }
}

void Thread::GetWork(SplitPoint* const master_sp) {
    int best_weight = 0;
    Thread* thread_to_help = nullptr;
    SplitPoint* best_split_point = nullptr;

    for (Thread* th : mThreadGroup) {
        if (th->thread_id == thread_id)
        {
            // there are only two possible scenario to be here
            // either this is an idle thread or a splitpoint master thread waiting for helper threads to finish
            // if idle thread: it has no splitpoint. so don't waste time checking
            // if helpful master: it shouldn't help it's current splitpoint (no more work)
            // or it's parent splitpoints (inefficient and will hang sometimes)
            // therefore, there is no need to check on it's own splitpoints
            continue;
        }
        // if this is a helpful master, only help splitpoints under helper threads still actively working for it
        if (master_sp != nullptr && !(master_sp->workersBitMask & ((uint64)1 << th->thread_id))) continue;
        for (int splitIdx = 0, num_splits = th->num_sp; splitIdx < num_splits; ++splitIdx) {
            SplitPoint* const sp = &th->sptable[splitIdx];
            // if it already has cutoff, no need to also check child splitpoints as they will return ASAP too
            // so break here and stop checking next splitpoints which are just child splitpoints of this current splitpoint
            if (sp->cutoff) break;
            // only search those with all threads still searching,
            // otherwise there is no more work in that splitpoint that needs help
            if (!sp->workAvailable) continue;
            // deeper is better; the higher the depth, the bigger the workload,
            // bigger workload means better efficiency as we avoid joining splitpoints too much
            // which is inefficient as we copy splitpoint data like position structure
            // and joining a splitpoint in general takes computation time
            int weight = (sp->depth << 10) - sp->played;
            if (weight > best_weight) {
                best_split_point = sp;
                best_weight = weight;
                thread_to_help = th;
            }
            // only check the first valid splitpoint in every thread, it is always the deepest, saves time
            break;
        }
    }
    // do a redundant check under lock protection
    // this is to make sure that the previous conditions still applies before joining the splitpoint
    if (!doSleep && best_split_point != nullptr && thread_to_help != nullptr) {
        std::lock_guard<Spinlock> lck(best_split_point->updatelock);
        // check if the splitpoint has not cutoff yet
        if (!best_split_point->cutoff
            // check if all helper threads are still searching this splitpoint
            && best_split_point->workAvailable
            // check if this is a helpful master and if the helper thread is still working for it
            && (master_sp == nullptr || (master_sp->workersBitMask & ((uint64)1 << thread_to_help->thread_id)))) {
            best_split_point->workersBitMask |= ((uint64)1 << thread_id);
            activeSplitPoint = best_split_point;
            activeSplitPoint->joinedthreads += 1;
            stop = false;
        }
    }
}

void Thread::SearchSplitPoint(position_t& pos, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot) {
    SplitPoint* const active_sp = &sptable[num_sp];

    active_sp->updatelock.lock();
    active_sp->parent = activeSplitPoint;
    active_sp->depth = depth;
    active_sp->alpha = alpha;
    active_sp->beta = beta;
    active_sp->nodeType = nt;
    active_sp->bestvalue = ss->bestvalue;
    active_sp->bestmove = ss->bestmove;
    active_sp->played = ss->playedMoves;
    active_sp->inCheck = inCheck;
    active_sp->inRoot = inRoot;
    active_sp->cutoff = false;
    active_sp->sscurr = ss;
    active_sp->ssprev = ssprev;
    active_sp->origpos = &pos;
    active_sp->workersBitMask = ((uint64)1 << thread_id);
    active_sp->workAvailable = true;
    active_sp->joinedthreads = 1;
    activeSplitPoint = active_sp;
    stop = false;
    ++num_sp;
    ++numsplits;
    active_sp->updatelock.unlock();

    IdleLoop();

    active_sp->updatelock.lock();
    --num_sp;
    ss->bestvalue = active_sp->bestvalue;
    ss->bestmove = active_sp->bestmove;
    ss->playedMoves = active_sp->played;
    ss->hisCnt = active_sp->hisCount;
    activeSplitPoint = active_sp->parent;
    if (!doSleep) stop = false;

    if (active_sp->joinedthreads > 1) {
        ++numsplitsjoined;
        numworkers += active_sp->joinedthreads;
    }
    active_sp->updatelock.unlock();
}

void TimerThread::IdleLoop() {
    while (!exit_flag) {
        std::unique_lock<std::mutex> lk(threadLock);
        auto now = std::chrono::system_clock::now();
        sleepCondition.wait_until(lk, stop ? now + std::chrono::hours(INT_MAX) : now + std::chrono::milliseconds(50));
        if (!exit_flag && !stop) CBFuncCheckTimer();
    }
}