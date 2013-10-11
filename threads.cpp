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

void Thread::Init() {
    searching = false;
    stop = false;
    exit_flag = false;
    nodes = 0;
    nodes_since_poll = 0;
    nodes_between_polls = 8192;
    started = 0;
    ended = 0;
    numsplits = 0;
    num_sp = 0;
    split_point = NULL;
    for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
        for (int k = 0; k < MaxNumOfThreads; ++k) {
            sptable[j].workersBitMask = 0;
        }
    }
    for (int Idx = 0; Idx < MAXPLY; Idx++) {
        ts[Idx].Init();
    }
}

void ThreadMgr::idleLoop(int thread_id) {
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

void ThreadMgr::setAllThreadsToStop() {
    SearchMgr::Inst().Info().thinking_status = STOPPED;
    for (int i = 0; i < Guci_options.threads; i++) {
        m_Threads[i]->stop = true;
    }
}

void ThreadMgr::setAllThreadsToSleep() {
    for (int i = 0; i < Guci_options.threads; i++) {
        m_Threads[i]->doSleep = true;
    }
}

void ThreadMgr::checkForWork(int thread_id) { 
    int best_depth = 0;
    int master_thread = 0;
    SplitPoint *best_split_point = NULL;

    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        if (threadIdx == thread_id) continue;
        if(!m_Threads[threadIdx]->searching) continue; // idle thread or master waiting for other threads, no need to help
        for (int splitIdx = 0; splitIdx < m_Threads[threadIdx]->num_sp; splitIdx++) {
            SplitPoint* sp = &m_Threads[threadIdx]->sptable[splitIdx];
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (bitCnt(sp->allWorkersBitMask) >= Guci_options.max_threads_per_split) continue; // enough threads working, no need to help
            if (sp->depth > best_depth) {
                best_split_point = sp;
                best_depth = sp->depth;
                master_thread = threadIdx;
                break; // best split found on this thread, break
            }
        }
    }
    if (best_split_point != NULL) {
        best_split_point->updatelock->lock();
        if (m_Threads[master_thread]->searching && m_Threads[master_thread]->num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                m_Threads[thread_id]->split_point = best_split_point;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                m_Threads[thread_id]->searching = true;
                m_Threads[thread_id]->stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void ThreadMgr::helpfulMaster(int thread_id, SplitPoint *master_sp) { // don't call if thread is master
    int best_depth = 0;
    int master_thread = 0;
    SplitPoint *best_split_point = NULL;

    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        if (threadIdx == thread_id) continue;
        if(!m_Threads[threadIdx]->searching) continue; // idle thread or master waiting for other threads, no need to help
        if (!(master_sp->allWorkersBitMask & ((uint64)1<<threadIdx))) continue;
        for (int splitIdx = 0; splitIdx < m_Threads[threadIdx]->num_sp; splitIdx++) {
            SplitPoint* sp = &m_Threads[threadIdx]->sptable[splitIdx];
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (bitCnt(sp->workersBitMask) >= Guci_options.max_threads_per_split) continue; // enough threads working, no need to help
            if (sp->depth > best_depth) {
                best_split_point = sp;
                best_depth = sp->depth;
                master_thread = threadIdx;
                break; // best split found on this thread, break
            }
        }
    }
    if (best_split_point != NULL) {
        best_split_point->updatelock->lock();
        if (m_Threads[master_thread]->searching && m_Threads[master_thread]->num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && (best_split_point->allWorkersBitMask & ((uint64)1<<master_thread))
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

bool smpCutoffOccurred(SplitPoint *sp) {
    if (NULL == sp) return false;
    if (sp->cutoff) return true;
    if (smpCutoffOccurred(sp->parent)) {
        sp->cutoff = true;
        return true;
    }
    return false;
}

void ThreadMgr::initSmpVars() {
    for (int i = 0; i < MaxNumOfThreads; ++i) {
        m_Threads[i]->Init();
    }
}

void ThreadMgr::initThreads(void) {
    int i;
    for (i = 0; i < MaxNumOfThreads; ++i) {
        Thread* th = new Thread(i);
        m_Threads.push_back(th);
        //m_Threads.push_back(new Thread(i));
        m_Threads[i]->realThread = std::thread(&ThreadMgr::idleLoop, this, i);
    }
}

void ThreadMgr::stopThreads(void) {
    for(int i = 0; i < MaxNumOfThreads; i++) {
        m_Threads.pop_back();
    }
}

void ThreadMgr::wakeUpThreads() {
    for (int i = 1; i < Guci_options.threads; ++i) {
        m_Threads[i]->triggerCondition();
    }
}

uint64 ThreadMgr::computeNodes() {
    uint64 nodes = 0;
    for (int i = 0; i < Guci_options.threads; ++i) nodes += m_Threads[i]->nodes;
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

    ThreadsMgr.idleLoop(sthread.thread_id);

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
