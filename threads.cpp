/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
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

ThreadMgr ThreadsMgr;

void ThreadMgr::StartThinking(position_t* pos) {
    m_StartThinking = true;
    m_pPos= pos;
    ThreadFromIdx(0).TriggerCondition();
}

void ThreadMgr::IdleLoop(const int thread_id) {
    SplitPoint *master_sp = m_Threads[thread_id]->activeSplitPoint;
    while (!m_Threads[thread_id]->exit_flag) {
        if (master_sp == NULL && m_Threads[thread_id]->doSleep) {
            m_Threads[thread_id]->SleepAndWaitForCondition();
            if (m_StartThinking && thread_id == 0) { // this is located here since it's only checked at thread's waking up
                SearchManager.getBestMove(m_pPos, ThreadFromIdx(thread_id));
                m_Threads[thread_id]->searching = false;
                m_StartThinking = false;
                continue; // to avoid getting into GetWork after the search has terminated
            }
        }
        if (!m_Threads[thread_id]->searching) {
            GetWork(thread_id, master_sp);
        }
        if (m_Threads[thread_id]->searching) {
            ++m_Threads[thread_id]->started;
            SplitPoint* sp = m_Threads[thread_id]->activeSplitPoint; // this is correctly located, don't move this, else bug
            SearchManager.searchFromIdleLoop(sp, *m_Threads[thread_id]);
            sp->updatelock->lock();
            sp->workersBitMask &= ~(1 << thread_id);
            m_Threads[thread_id]->searching = false;
            sp->updatelock->unlock();
            ++m_Threads[thread_id]->ended;
        }
        if (master_sp != NULL && !master_sp->workersBitMask) return;
    }
}

void ThreadMgr::GetWork(const int thread_id, SplitPoint *master_sp) {
    int best_depth = 0;
    Thread* master_thread = NULL;
    SplitPoint *best_split_point = NULL;

    for (Thread* th: m_Threads) {
        if (th->thread_id == thread_id) continue;
        if (!th->searching) continue; // idle thread or master waiting for other threads, no need to help
        if (master_sp && !(master_sp->allWorkersBitMask & ((uint64)1<<th->thread_id))) continue; // helpful master: looking to help threads working for it
        for (int splitIdx = 0; splitIdx < th->num_sp; splitIdx++) {
            SplitPoint* sp = &th->sptable[splitIdx];
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (bitCnt(sp->allWorkersBitMask) >= Guci_options.max_threads_per_split) continue; // enough threads working, no need to help
            if (sp->depth > best_depth) {
                best_split_point = sp;
                best_depth = sp->depth;
                master_thread = th;
                break; // best split found on this thread, break
            }
        }
    }
    if (best_split_point != NULL) {
        best_split_point->updatelock->lock();
        if (master_thread->searching && master_thread->num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && (!master_sp || (best_split_point->allWorkersBitMask & ((uint64)1<<master_thread->thread_id)))
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                m_Threads[thread_id]->activeSplitPoint = best_split_point;
                m_Threads[thread_id]->searching = true;
                m_Threads[thread_id]->stop = false;
                ++master_thread->joined;
        }
        best_split_point->updatelock->unlock();
    }
}

void ThreadMgr::SetAllThreadsToStop() {
    SearchManager.info.thinking_status = STOPPED;
    for (Thread* th: m_Threads) {
        th->stop = true;
    }
}

void ThreadMgr::SetAllThreadsToSleep() {
    for (Thread* th: m_Threads) {
        th->doSleep = true;
    }
}

void ThreadMgr::SetAllThreadsToWork() {
    for (Thread* th: m_Threads) {
        if (th->thread_id != 0) th->TriggerCondition(); // thread_id == 0 is triggered separately
    }
}

uint64 ThreadMgr::ComputeNodes() {
    uint64 nodes = 0;
    for (Thread* th: m_Threads) nodes += th->nodes;
    return nodes;
}

void ThreadMgr::InitVars() {
    for (Thread* th: m_Threads) {
        th->Init();
    }
}

void ThreadMgr::SetNumThreads(int num) {
    while (m_Threads.size() < num) {
        int id = m_Threads.size();
        m_Threads.push_back(new Thread(id));
        m_Threads[id]->NativeThread() = std::thread(&ThreadMgr::IdleLoop, this, id);
    }
    while (m_Threads.size() > num) {
        delete m_Threads.back();
        m_Threads.pop_back();
    }
    InitPawnHash(Guci_options.pawnhashsize);
    InitEvalHash(Guci_options.evalcachesize);
}

void ThreadMgr::SearchSplitPoint(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread) {
    SplitPoint *active_sp = &sthread.sptable[sthread.num_sp];    

    active_sp->updatelock->lock();
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
    active_sp->pos[sthread.thread_id] = *p;
    active_sp->origpos = *p;
    active_sp->workersBitMask = ((uint64)1<<sthread.thread_id);
    active_sp->allWorkersBitMask = ((uint64)1<<sthread.thread_id);
    sthread.activeSplitPoint = active_sp;
    sthread.searching = true;
    sthread.stop = false;
    sthread.num_sp++;
    active_sp->updatelock->unlock();

    IdleLoop(sthread.thread_id);

    active_sp->updatelock->lock();
    sthread.num_sp--;
    ss->bestvalue = active_sp->bestvalue;
    ss->bestmove = active_sp->bestmove;
    ss->playedMoves = active_sp->played;
    sthread.activeSplitPoint = active_sp->parent;
    sthread.numsplits++;
    if (SearchManager.info.thinking_status != STOPPED) {
        sthread.stop = false; 
        sthread.searching = true;
    }
    active_sp->updatelock->unlock();
}
