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

std::vector<Thread*> Threads;

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

void setAllThreadsToStop(int thread) {
    SearchInfo(thread).thinking_status = STOPPED;
    for (int i = 0; i < Guci_options.threads; i++) {
        Threads[i]->doSleep = true;
        Threads[i]->stop = true;
    }
}

void checkForWork(int thread_id) { 
    int best_depth = 0;
    int master_thread = 0;
    SplitPoint *best_split_point = NULL;

    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        if (threadIdx == thread_id) continue;
        if(!Threads[threadIdx]->searching) continue; // idle thread or master waiting for other threads, no need to help
        for (int splitIdx = 0; splitIdx < Threads[threadIdx]->num_sp; splitIdx++) {
            SplitPoint* sp = &Threads[threadIdx]->sptable[splitIdx];
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
        if (Threads[master_thread]->searching && Threads[master_thread]->num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                Threads[thread_id]->split_point = best_split_point;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                Threads[thread_id]->searching = true;
                Threads[thread_id]->stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void helpfulMaster(int thread_id, SplitPoint *master_sp) { // don't call if thread is master
    int best_depth = 0;
    int master_thread = 0;
    SplitPoint *best_split_point = NULL;

    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        if (threadIdx == thread_id) continue;
        if(!Threads[threadIdx]->searching) continue; // idle thread or master waiting for other threads, no need to help
        if (!(master_sp->allWorkersBitMask & ((uint64)1<<threadIdx))) continue;
        for (int splitIdx = 0; splitIdx < Threads[threadIdx]->num_sp; splitIdx++) {
            SplitPoint* sp = &Threads[threadIdx]->sptable[splitIdx];
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
        if (Threads[master_thread]->searching && Threads[master_thread]->num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && (best_split_point->allWorkersBitMask & ((uint64)1<<master_thread))
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                Threads[thread_id]->split_point = best_split_point;
                Threads[thread_id]->searching = true;
                Threads[thread_id]->stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void idleLoop(int thread_id, SplitPoint *master_sp) {
    ASSERT(thread_id < Guci_options.threads || master_sp == NULL);
    while(!Threads[thread_id]->exit_flag) {
        if (master_sp == NULL && Threads[thread_id]->doSleep) {
            Print(1, "Thread sleeping %d\n", thread_id);
            Threads[thread_id]->sleepAndWaitForCondition();
        }
        if(Threads[thread_id]->searching) {
            ++Threads[thread_id]->started;
            SplitPoint* sp = Threads[thread_id]->split_point; // this is correctly located, don't move this, else bug
            searchFromIdleLoop(sp, thread_id);
            sp->updatelock->lock();
            sp->workersBitMask &= ~(1 << thread_id);
            Threads[thread_id]->searching = false;
            sp->updatelock->unlock();
            ++Threads[thread_id]->ended;
        }
        if(master_sp != NULL) {
            if (!master_sp->workersBitMask) return;
            else {
                if (SearchInfo(thread_id).thinking_status == STOPPED) {
                    setAllThreadsToStop(thread_id);
                    return;
                } else helpfulMaster(thread_id, master_sp);
            } 
        } else checkForWork(thread_id);
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

void initSmpVars() {
    for (int i = 0; i < MaxNumOfThreads; ++i) {
        Threads[i]->Init();
    }
}

void initThreads(void) {
    int i;
    for (i = 0; i < MaxNumOfThreads; ++i) {
        SplitPoint* sp = NULL;
        Threads.push_back(new Thread(i));
        Threads[i]->realThread = std::thread(idleLoop, i, sp);
    }
}

void stopThreads(void) {
    for(int i = 0; i < MaxNumOfThreads; i++) {
        Threads.pop_back();
    }
}

bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master) {
    SplitPoint *split_point = &Threads[master]->sptable[Threads[master]->num_sp];    

    split_point->updatelock->lock();
    split_point = &Threads[master]->sptable[Threads[master]->num_sp];  
    Threads[master]->num_sp++;
    split_point->parent = Threads[master]->split_point;
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
    split_point->pos[master] = *p;
    split_point->origpos = *p;
    split_point->workersBitMask = ((uint64)1<<master);
    split_point->allWorkersBitMask = ((uint64)1<<master);
    Threads[master]->split_point = split_point;
    Threads[master]->searching = true;
    Threads[master]->stop = false;
    split_point->updatelock->unlock();

    idleLoop(master, split_point);

    split_point->updatelock->lock();
    Threads[master]->num_sp--;
    ss->bestvalue = split_point->bestvalue;
    ss->bestmove = split_point->bestmove;
    ss->playedMoves = split_point->played;
    Threads[master]->split_point = split_point->parent;
    Threads[master]->numsplits++;
    if (SearchInfo(master).thinking_status != STOPPED) {
        Threads[master]->stop = false; 
        Threads[master]->searching = true;
    }
    split_point->updatelock->unlock();
    return true;
}
