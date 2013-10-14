/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
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

void ThreadMgr::IdleLoop(const int thread_id) {
    SplitPoint *master_sp = m_Threads[thread_id]->activeSplitPoint;
    while(!m_Threads[thread_id]->exit_flag) {
        if (master_sp == NULL && m_Threads[thread_id]->doSleep) {
            m_Threads[thread_id]->SleepAndWaitForCondition();
        }
        if(m_Threads[thread_id]->searching) {
            ++m_Threads[thread_id]->started;
            SplitPoint* sp = m_Threads[thread_id]->activeSplitPoint; // this is correctly located, don't move this, else bug
            SearchMgr::Inst().searchFromIdleLoop(sp, *m_Threads[thread_id]);
            sp->updatelock->lock();
            sp->workersBitMask &= ~(1 << thread_id);
            m_Threads[thread_id]->searching = false;
            sp->updatelock->unlock();
            ++m_Threads[thread_id]->ended;
        }
        if(master_sp != NULL && !master_sp->workersBitMask) return;
        GetWork(thread_id, master_sp);
    }
}

void ThreadMgr::GetWork(const int thread_id, SplitPoint *master_sp) {
    int best_depth = 0;
    Thread* master_thread = NULL;
    SplitPoint *best_split_point = NULL;

    for (Thread* th: m_Threads) {
        if (th->thread_id == thread_id) continue;
        if(!th->searching) continue; // idle thread or master waiting for other threads, no need to help
        if (master_sp && !(master_sp->allWorkersBitMask & ((uint64)1<<th->thread_id))) continue;
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
        }
        best_split_point->updatelock->unlock();
    }
}

void ThreadMgr::SetAllThreadsToStop() {
    SearchMgr::Inst().Info().thinking_status = STOPPED;
    for (Thread* th: m_Threads) {
        th->stop = true;
    }
}

void ThreadMgr::SetAllThreadsToSleep() {
    for (Thread* th: m_Threads) {
        th->doSleep = true;
    }
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
}

void ThreadMgr::SetAllThreadsToWork() {
    for (int i = 1; i < m_Threads.size(); ++i) { // TODO: implement GetBestMove to use Thread(0), move blocking input to main thread
        m_Threads[i]->TriggerCondition();
    }
}

uint64 ThreadMgr::ComputeNodes() {
    uint64 nodes = 0;
    for (Thread* th: m_Threads) nodes += th->nodes;
    return nodes;
}

void ThreadMgr::SearchSplitPoint(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread) {
    SplitPoint *activeSplitPoint = &sthread.sptable[sthread.num_sp];    

    activeSplitPoint->updatelock->lock();
    activeSplitPoint->parent = sthread.activeSplitPoint;
    activeSplitPoint->depth = depth;
    activeSplitPoint->alpha = alpha; 
    activeSplitPoint->beta = beta;
    activeSplitPoint->nodeType = nt;
    activeSplitPoint->bestvalue = ss->bestvalue;
    activeSplitPoint->bestmove = ss->bestmove;
    activeSplitPoint->played = ss->playedMoves;
    activeSplitPoint->inCheck = inCheck;
    activeSplitPoint->inRoot = inRoot;
    activeSplitPoint->cutoff = false;
    activeSplitPoint->sscurr = ss;
    activeSplitPoint->ssprev = ssprev;
    activeSplitPoint->pos[sthread.thread_id] = *p;
    activeSplitPoint->origpos = *p;
    activeSplitPoint->workersBitMask = ((uint64)1<<sthread.thread_id);
    activeSplitPoint->allWorkersBitMask = ((uint64)1<<sthread.thread_id);
    sthread.activeSplitPoint = activeSplitPoint;
    sthread.searching = true;
    sthread.stop = false;
    sthread.num_sp++;
    activeSplitPoint->updatelock->unlock();

    IdleLoop(sthread.thread_id);

    activeSplitPoint->updatelock->lock();
    sthread.num_sp--;
    ss->bestvalue = activeSplitPoint->bestvalue;
    ss->bestmove = activeSplitPoint->bestmove;
    ss->playedMoves = activeSplitPoint->played;
    sthread.activeSplitPoint = activeSplitPoint->parent;
    sthread.numsplits++;
    if (SearchMgr::Inst().Info().thinking_status != STOPPED) {
        sthread.stop = false; 
        sthread.searching = true;
    }
    activeSplitPoint->updatelock->unlock();
}
