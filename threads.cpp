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

void ThreadMgr::idleLoop(const int thread_id) {
    SplitPoint *master_sp = m_Threads[thread_id]->split_point;
    while(!m_Threads[thread_id]->exit_flag) {
        if (master_sp == NULL && m_Threads[thread_id]->doSleep) {
            m_Threads[thread_id]->sleepAndWaitForCondition();
        }
        if(m_Threads[thread_id]->searching) {
            ++m_Threads[thread_id]->started;
            SplitPoint* sp = m_Threads[thread_id]->split_point; // this is correctly located, don't move this, else bug
            SearchMgr::Inst().searchFromIdleLoop(sp, *m_Threads[thread_id]);
            sp->updatelock->lock();
            sp->workersBitMask &= ~(1 << thread_id);
            m_Threads[thread_id]->searching = false;
            sp->updatelock->unlock();
            ++m_Threads[thread_id]->ended;
        }
        if(master_sp != NULL) {
            if (!master_sp->workersBitMask) return;
            else helpfulMaster(thread_id, master_sp);
        } else checkForWork(thread_id);
    }
}

void ThreadMgr::checkForWork(const int thread_id) { 
    int best_depth = 0;
    Thread* master_thread = NULL;
    SplitPoint *best_split_point = NULL;

    for (Thread* th: m_Threads) {
        if (th->thread_id == thread_id) continue;
        if(!th->searching) continue; // idle thread or master waiting for other threads, no need to help
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
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                m_Threads[thread_id]->split_point = best_split_point;
                m_Threads[thread_id]->searching = true;
                m_Threads[thread_id]->stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void ThreadMgr::helpfulMaster(const int thread_id, SplitPoint *master_sp) { // don't call if thread is master
    int best_depth = 0;
    Thread* master_thread = NULL;
    SplitPoint *best_split_point = NULL;

    for (Thread* th: m_Threads) {
        if (th->thread_id == thread_id) continue;
        if(!th->searching) continue; // idle thread or master waiting for other threads, no need to help
        if (!(master_sp->allWorkersBitMask & ((uint64)1<<th->thread_id))) continue;
        for (int splitIdx = 0; splitIdx < th->num_sp; splitIdx++) {
            SplitPoint* sp = &th->sptable[splitIdx];
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (bitCnt(sp->workersBitMask) >= Guci_options.max_threads_per_split) continue; // enough threads working, no need to help
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
            && (best_split_point->allWorkersBitMask & ((uint64)1<<master_thread->thread_id))
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                m_Threads[thread_id]->split_point = best_split_point;
                m_Threads[thread_id]->searching = true;
                m_Threads[thread_id]->stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void ThreadMgr::setAllThreadsToStop() {
    SearchMgr::Inst().Info().thinking_status = STOPPED;
    for (Thread* th: m_Threads) {
        th->stop = true;
    }
}

void ThreadMgr::setAllThreadsToSleep() {
    for (Thread* th: m_Threads) {
        th->doSleep = true;
    }
}

void ThreadMgr::initSmpVars() {
    for (Thread* th: m_Threads) {
        th->Init();
    }
}

void ThreadMgr::initThreads(int num) {
    while (m_Threads.size() < num) {
        int id = m_Threads.size();
        m_Threads.push_back(new Thread(id));
        m_Threads[id]->RThread() = std::thread(&ThreadMgr::idleLoop, this, id);
    }
    while (m_Threads.size() > num) {
        delete m_Threads.back();
        m_Threads.pop_back();
    }
}

void ThreadMgr::stopThreads(void) {
    while (m_Threads.size() > 0) {
        delete m_Threads.back();
        m_Threads.pop_back();
    }
}

void ThreadMgr::wakeUpThreads() {
    for (int i = 1; i < m_Threads.size(); ++i) {
        m_Threads[i]->triggerCondition();
    }
}

uint64 ThreadMgr::computeNodes() {
    uint64 nodes = 0;
    for (Thread* th: m_Threads) nodes += th->nodes;
    return nodes;
}

bool ThreadMgr::splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread) {
    SplitPoint *split_point = &sthread.sptable[sthread.num_sp];    

    split_point->updatelock->lock();
    split_point = &sthread.sptable[sthread.num_sp];  
    sthread.num_sp++;
    split_point->parent = sthread.split_point;
    split_point->depth = depth;
    split_point->alpha = alpha; 
    split_point->beta = beta;
    split_point->nodeType = nt;
    split_point->bestvalue = ss->bestvalue;
    split_point->bestmove = ss->bestmove;
    split_point->played = ss->playedMoves;
    split_point->inCheck = inCheck;
    split_point->inRoot = inRoot;
    split_point->cutoff = false;
    split_point->sscurr = ss;
    split_point->ssprev = ssprev;
    split_point->pos[sthread.thread_id] = *p;
    split_point->origpos = *p;
    split_point->workersBitMask = ((uint64)1<<sthread.thread_id);
    split_point->allWorkersBitMask = ((uint64)1<<sthread.thread_id);
    sthread.split_point = split_point;
    sthread.searching = true;
    sthread.stop = false;
    split_point->updatelock->unlock();

    idleLoop(sthread.thread_id);

    split_point->updatelock->lock();
    sthread.num_sp--;
    ss->bestvalue = split_point->bestvalue;
    ss->bestmove = split_point->bestmove;
    ss->playedMoves = split_point->played;
    sthread.split_point = split_point->parent;
    sthread.numsplits++;
    if (SearchMgr::Inst().Info().thinking_status != STOPPED) {
        sthread.stop = false; 
        sthread.searching = true;
    }
    split_point->updatelock->unlock();
    return true;
}
