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
    numsplits = 1;
    numsplits2 = 1;
    workers2 = 0;
    num_sp = 0;
    activeSplitPoint = NULL;
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
        if (master_sp == NULL && doSleep) {
            SleepAndWaitForCondition();            
        }
        if (master_sp == NULL && thread_id == 0 && !doSleep) {
            LogAndPrintInfo() << "IdleLoop: Main thread waking up to start searching!";
            CEngine.getBestMove(*this);
            doSleep = true;
        }
        if (!doSleep && stop) {
            GetWork(master_sp);
        }
        if (!doSleep && !stop) {
            SplitPoint* const sp = activeSplitPoint; // this is correctly located, don't move this, else bug
            CEngine.searchFromIdleLoop(*sp, *this);
            std::lock_guard<Spinlock> lck(sp->updatelock);
            sp->workersBitMask &= ~((uint64)1 << thread_id);
            stop = true;
        }
        if (master_sp != NULL && (!master_sp->workersBitMask || doSleep)) return;
    }
}

void Thread::GetWork(SplitPoint* const master_sp) {
    int best_depth = 0;
    Thread* thread_to_help = NULL;
    SplitPoint* best_split_point = NULL;

    for (Thread* th : *threadgroup) {
        if (th->thread_id == thread_id) continue; // no need to help self
        if (master_sp != NULL && !(master_sp->workersBitMask & ((uint64)1 << th->thread_id))) continue; // helpful master: looking to help threads still actively working for it
        for (int splitIdx = 0, num_splits = th->num_sp; splitIdx < num_splits; ++splitIdx) {
            SplitPoint* const sp = &th->sptable[splitIdx];
            if (sp->cutoff) continue; // if it already has cutoff, move on
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (sp->depth > best_depth) { // deeper is better
                best_split_point = sp;
                best_depth = sp->depth;
                thread_to_help = th;
            }
            break; // only check the first valid split in every thread, it is always the deepest, saves time
        }
    }
    if (!doSleep && best_split_point != NULL && thread_to_help != NULL) {
        std::lock_guard<Spinlock> lck(best_split_point->updatelock);
        if (!best_split_point->cutoff // check if the splitpoint has not cutoff yet
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask) // check if all helper threads are still searching this splitpoint
            && (master_sp == NULL || (master_sp->workersBitMask & ((uint64)1 << thread_to_help->thread_id)))) { // check if the helper thread is still working for master
            best_split_point->workersBitMask |= ((uint64)1 << thread_id);
            best_split_point->allWorkersBitMask |= ((uint64)1 << thread_id);
            activeSplitPoint = best_split_point;
            stop = false;
        }
    }
}

void Thread::SearchSplitPoint(const position_t& pos, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot) {
    SplitPoint *active_sp = &sptable[num_sp];

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
    active_sp->origpos = pos;
    active_sp->workersBitMask = ((uint64)1 << thread_id);
    active_sp->allWorkersBitMask = ((uint64)1 << thread_id);
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
    activeSplitPoint = active_sp->parent;
    if (!doSleep) {
        stop = false;
    }

    int cnt = bitCnt(active_sp->allWorkersBitMask);
    if (cnt > 1) {
        numsplits2++;
        workers2 += cnt;
    }
    active_sp->updatelock.unlock();
}