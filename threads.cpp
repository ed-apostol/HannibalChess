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
#include "uci.h"

ThreadsManager ThreadsMgr;

void ThreadsManager::StartThinking(position_t* pos) {
    m_StartThinking = true;
    m_pPos = pos;
    ThreadFromIdx(0).TriggerCondition();
}

void ThreadsManager::IdleLoop(const int thread_id) {
    SplitPoint *master_sp = m_Threads[thread_id]->activeSplitPoint;
    while (!m_Threads[thread_id]->exit_flag) {
        if (master_sp == NULL && m_Threads[thread_id]->doSleep) {
            m_Threads[thread_id]->SleepAndWaitForCondition();
            if (m_StartThinking && thread_id == 0) {
                m_StartThinking = false;
                CEngine.getBestMove(m_pPos, ThreadFromIdx(thread_id));
                m_Threads[thread_id]->searching = false;
            }
        }
        if (!m_StopThreads && !m_Threads[thread_id]->searching) {
            GetWork(thread_id, master_sp);
        }
        if (!m_StopThreads && m_Threads[thread_id]->searching) {
            SplitPoint* sp = m_Threads[thread_id]->activeSplitPoint; // this is correctly located, don't move this, else bug
            CEngine.searchFromIdleLoop(sp, ThreadFromIdx(thread_id));
            sp->updatelock->lock();
            sp->workersBitMask &= ~((uint64)1 << thread_id);
            m_Threads[thread_id]->searching = false;
            sp->updatelock->unlock();
        }
        if (master_sp != NULL && (!master_sp->workersBitMask || m_StopThreads)) return;
    }
}

void ThreadsManager::GetWork(const int thread_id, SplitPoint *master_sp) {
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
            if (bitCnt(sp->allWorkersBitMask) >= m_MaxThreadsPerSplit) continue; // enough threads working, no need to help
            if (sp->depth > best_depth) {
                best_split_point = sp;
                best_depth = sp->depth;
                master_thread = th;
                break; // best split found on this thread, break
            }
        }
    }
    if (!m_StopThreads && best_split_point != NULL) {
        best_split_point->updatelock->lock();
        if (master_thread->searching && master_thread->num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && (!master_sp || (best_split_point->allWorkersBitMask & ((uint64)1<<master_thread->thread_id)))
            && bitCnt(best_split_point->allWorkersBitMask) < m_MaxThreadsPerSplit) { // redundant criteria, just to be sure
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

void ThreadsManager::SetAllThreadsToStop() {
    m_StopThreads = true;
    for (Thread* th: m_Threads) {
        th->stop = true;
    }
}

void ThreadsManager::SetAllThreadsToSleep() {
    for (Thread* th: m_Threads) {
        th->doSleep = true;
    }
}

void ThreadsManager::SetAllThreadsToWork() {
    for (Thread* th: m_Threads) {
        if (th->thread_id != 0) th->TriggerCondition(); // thread_id == 0 is triggered separately
    }
}

uint64 ThreadsManager::ComputeNodes() {
    uint64 nodes = 0;
    for (Thread* th: m_Threads) nodes += th->nodes;
    return nodes;
}

void ThreadsManager::InitVars() {
    m_StopThreads = false;
    m_MinSplitDepth = UCIOptionsMap["Min Split Depth"].GetInt();
    m_MaxThreadsPerSplit = UCIOptionsMap["Max Threads/Split"].GetInt();
    m_MaxActiveSplitsPerThread = UCIOptionsMap["Max Active Splits/Thread"].GetInt();
    for (Thread* th: m_Threads) {
        th->Init();
    }
}

void ThreadsManager::SetNumThreads(int num) {
    while (m_Threads.size() < num) {
        int id = m_Threads.size();
        m_Threads.push_back(new Thread(id));
        m_Threads[id]->NativeThread() = std::thread(&ThreadsManager::IdleLoop, this, id);
    }
    while (m_Threads.size() > num) {
        delete m_Threads.back();
        m_Threads.pop_back();
    }
    InitPawnHash(UCIOptionsMap["Pawn Hash"].GetInt(), 1);
    InitEvalHash(UCIOptionsMap["Eval Cache"].GetInt(), 1);
}

void ThreadsManager::SearchSplitPoint(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, Thread& sthread) {
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
    sthread.numsplits++;
    active_sp->updatelock->unlock();

    IdleLoop(sthread.thread_id);

    active_sp->updatelock->lock();
    sthread.num_sp--;
    ss->bestvalue = active_sp->bestvalue;
    ss->bestmove = active_sp->bestmove;
    ss->playedMoves = active_sp->played;
    sthread.activeSplitPoint = active_sp->parent;
    if (!m_StopThreads) {
        sthread.stop = false; 
        sthread.searching = true;
    }

    int cnt = bitCnt(active_sp->allWorkersBitMask);
    if (cnt > 1) {
        sthread.numsplits2++;
        sthread.workers2 += cnt;
    }
    active_sp->updatelock->unlock();
}
