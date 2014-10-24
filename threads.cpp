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

ThreadsManager ThreadsMgr;

void ThreadsManager::StartThinking() {
    mThinking = true;
    nodes_since_poll = 0;
    nodes_between_polls = 8192;
    ThreadFromIdx(0).TriggerCondition();
}

void ThreadsManager::IdleLoop(const int thread_id) {
    Thread& sthread = *mThreads[thread_id];
    SplitPoint* master_sp = sthread.activeSplitPoint;
    while (!sthread.exit_flag) {
        if (master_sp == NULL && sthread.doSleep) {
            sthread.SleepAndWaitForCondition();
            if (mThinking && thread_id == 0) {
                CEngine.getBestMove(sthread);
                mThinking = false;
                sthread.searching = false;
            }
        }
        if (!mStopThreads && !sthread.searching) {
            GetWork(thread_id, master_sp);
        }
        if (!mStopThreads && sthread.searching) {
            SplitPoint* sp = sthread.activeSplitPoint; // this is correctly located, don't move this, else bug
            CEngine.searchFromIdleLoop(*sp, sthread);
            std::lock_guard<Spinlock> lck(sp->updatelock);
            sp->workersBitMask &= ~((uint64)1 << thread_id);
            sthread.searching = false;
        }
        if (master_sp != NULL && (!master_sp->workersBitMask || mStopThreads)) return;
    }
}

void ThreadsManager::GetWork(const int thread_id, SplitPoint* master_sp) {
    int best_depth = 0;
    Thread* thread_to_help = NULL;
    SplitPoint* best_split_point = NULL;

    for (Thread* th : mThreads) {
        if (th->thread_id == thread_id) continue; // no need to help self
        if (master_sp != NULL && !(master_sp->workersBitMask & ((uint64)1 << th->thread_id))) continue; // helpful master: looking to help threads still actively working for it
        for (int splitIdx = 0, num_splits = th->num_sp; splitIdx < num_splits; ++splitIdx) {
            SplitPoint* sp = &th->sptable[splitIdx];
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
    if (!mStopThreads && best_split_point != NULL && thread_to_help != NULL) {
        std::lock_guard<Spinlock> lck(best_split_point->updatelock);
        if (!best_split_point->cutoff // check if the splitpoint has not cutoff yet
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask) // check if all helper threads are still searching this splitpoint
            && (master_sp == NULL || (master_sp->workersBitMask & ((uint64)1 << thread_to_help->thread_id)))) { // check if the helper thread is still working for master
            best_split_point->workersBitMask |= ((uint64)1 << thread_id);
            best_split_point->allWorkersBitMask |= ((uint64)1 << thread_id);
            mThreads[thread_id]->activeSplitPoint = best_split_point;
            mThreads[thread_id]->searching = true;
            mThreads[thread_id]->stop = false;
        }
    }
}

void ThreadsManager::SetAllThreadsToStop() {
    mStopThreads = true;
    for (Thread* th : mThreads) {
        th->stop = true;
    }
}

void ThreadsManager::SetAllThreadsToSleep() {
    for (Thread* th : mThreads) {
        th->doSleep = true;
    }
}

void ThreadsManager::SetAllThreadsToWork() {
    for (Thread* th : mThreads) {
        if (th->thread_id != 0) th->TriggerCondition(); // thread_id == 0 is triggered separately
    }
}

uint64 ThreadsManager::ComputeNodes() {
    uint64 nodes = 0;
    for (Thread* th : mThreads) nodes += th->nodes;
    return nodes;
}

void ThreadsManager::InitVars() {
    mStopThreads = false;
    mMinSplitDepth = UCIOptionsMap["Min Split Depth"].GetInt();
    mMaxActiveSplitsPerThread = UCIOptionsMap["Max Active Splits/Thread"].GetInt();
    for (Thread* th : mThreads) {
        th->Init();
    }
}

void ThreadsManager::SetNumThreads(int num) {
    while (mThreads.size() < num) {
        int id = (int)mThreads.size();
        mThreads.push_back(new Thread(id));
        mThreads[id]->NativeThread() = std::thread(&ThreadsManager::IdleLoop, this, id);
    }
    while (mThreads.size() > num) {
        delete mThreads.back();
        mThreads.pop_back();
    }
    InitPawnHash(UCIOptionsMap["Pawn Hash"].GetInt());
    InitEvalHash(UCIOptionsMap["Eval Cache"].GetInt());
}

void ThreadsManager::SearchSplitPoint(const position_t& pos, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread) {
    SplitPoint *active_sp = &sthread.sptable[sthread.num_sp];

    active_sp->updatelock.lock();
    active_sp->parent = sthread.activeSplitPoint;
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
    active_sp->workersBitMask = ((uint64)1 << sthread.thread_id);
    active_sp->allWorkersBitMask = ((uint64)1 << sthread.thread_id);
    sthread.activeSplitPoint = active_sp;
    sthread.searching = true;
    sthread.stop = false;
    ++sthread.num_sp;
    ++sthread.numsplits;
    active_sp->updatelock.unlock();

    IdleLoop(sthread.thread_id);

    active_sp->updatelock.lock();
    --sthread.num_sp;
    ss->bestvalue = active_sp->bestvalue;
    ss->bestmove = active_sp->bestmove;
    ss->playedMoves = active_sp->played;
    sthread.activeSplitPoint = active_sp->parent;
    if (!mStopThreads) {
        sthread.stop = false;
        sthread.searching = true;
    }

    int cnt = bitCnt(active_sp->allWorkersBitMask);
    if (cnt > 1) {
        sthread.numsplits2++;
        sthread.workers2 += cnt;
    }
    active_sp->updatelock.unlock();
}