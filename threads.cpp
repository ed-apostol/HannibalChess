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

thread_t Threads[MaxNumOfThreads];
std::vector<std::thread> RealThreads;

void initSearchThread(int i) {
    Threads[i].nodes_since_poll = 0;
    Threads[i].nodes_between_polls = 8192;
    Threads[i].nodes = 0;
    for (int Idx = 0; Idx < MAXPLY; Idx++) {
        Threads[i].ts[Idx].Init();
    }
}

void setAllThreadsToStop(int thread) {
    SearchInfo(thread).thinking_status = STOPPED;
    for (int i = 0; i < Guci_options.threads; i++) {
        Threads[i].stop = true;
    }
}

void checkForWork(int thread_id) { 
    int best_depth = 0;
    int master_thread = 0;
    SplitPoint *best_split_point = NULL;

    if(Threads[thread_id].searching) return;

    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        if (threadIdx == thread_id) continue;
        if(!Threads[threadIdx].searching) continue; // idle thread or master waiting for other threads, no need to help
        for (int splitIdx = 0; splitIdx < Threads[threadIdx].num_sp; splitIdx++) {
            SplitPoint* sp = &Threads[threadIdx].sptable[splitIdx];
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (bitCnt(sp->allWorkersBitMask) >= Guci_options.max_threads_per_split) continue; // enough threads working, no need to help
            if (sp->depth > best_depth) {
                best_split_point = sp;
                best_depth = sp->depth;
                master_thread = threadIdx;
            }
        }
        if (best_split_point != NULL) break; // TODO: test commenting this out later on
    }
    if (best_split_point != NULL) {
        best_split_point->updatelock->lock();
        if (Threads[master_thread].searching && Threads[master_thread].num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                Threads[thread_id].split_point = best_split_point;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                Threads[thread_id].searching = true;
                Threads[thread_id].stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void helpfulMaster(SplitPoint *master_sp, int thread_id) { // don't call if thread is master
    int best_depth = 0;
    int master_thread = 0;
    SplitPoint *best_split_point = NULL;

    if(Threads[thread_id].searching) return;

    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        if (threadIdx == thread_id) continue;
        if(!Threads[threadIdx].searching) continue; // idle thread or master waiting for other threads, no need to help
        if (!(master_sp->allWorkersBitMask & ((uint64)1<<threadIdx))) continue;
        for (int splitIdx = 0; splitIdx < Threads[threadIdx].num_sp; splitIdx++) {
            SplitPoint* sp = &Threads[threadIdx].sptable[splitIdx];
            if (sp->workersBitMask != sp->allWorkersBitMask) continue; // only search those with all threads still searching
            if (bitCnt(sp->workersBitMask) >= Guci_options.max_threads_per_split) continue; // enough threads working, no need to help
            if (sp->depth > best_depth) {
                best_split_point = sp;
                best_depth = sp->depth;
                master_thread = threadIdx;
            }
        }
        if (best_split_point != NULL) break; // TODO: test commenting this out later on
    }
    if (best_split_point != NULL) {
        best_split_point->updatelock->lock();
        if (Threads[master_thread].searching && Threads[master_thread].num_sp > 0 && !best_split_point->cutoff
            && (best_split_point->workersBitMask == best_split_point->allWorkersBitMask)
            && (best_split_point->allWorkersBitMask & ((uint64)1<<master_thread))
            && bitCnt(best_split_point->allWorkersBitMask) < Guci_options.max_threads_per_split) { // redundant criteria, just to be sure
                best_split_point->pos[thread_id] = best_split_point->origpos;
                best_split_point->workersBitMask |= ((uint64)1<<thread_id);
                best_split_point->allWorkersBitMask |= ((uint64)1<<thread_id);
                Threads[thread_id].split_point = best_split_point;
                Threads[thread_id].searching = true;
                Threads[thread_id].stop = false;
        }
        best_split_point->updatelock->unlock();
    }
}

void helpfulMasterLoop(SplitPoint *master_sp, int thread_id) {
    while(!Threads[thread_id].exit_flag && master_sp->workersBitMask) {
        helpfulMaster(master_sp, thread_id);
        if(Threads[thread_id].searching) {
            ++Threads[thread_id].started;
            SplitPoint* sp = Threads[thread_id].split_point; // this is correctly located, don't move this, else bug
            searchFromIdleLoop(sp, thread_id);
            sp->updatelock->lock();
            sp->workersBitMask &= ~(1 << thread_id);
            Threads[thread_id].searching = false;
            sp->updatelock->unlock();
            ++Threads[thread_id].ended;
        }
    }
}

void idleLoop(int thread_id) {
    ASSERT(thread_id < Guci_options.threads);
    while(!Threads[thread_id].exit_flag) {
        while(SearchInfo(thread_id).thinking_status == STOPPED && !Threads[thread_id].exit_flag) {
            std::unique_lock<std::mutex> lk(Threads[thread_id].threadLock);
            Threads[thread_id].idle_event.wait(lk);
        }
        checkForWork(thread_id);
        if(Threads[thread_id].searching) {
            ++Threads[thread_id].started;
            SplitPoint* sp = Threads[thread_id].split_point; // this is correctly located, don't move this, else bug
            searchFromIdleLoop(sp, thread_id);
            sp->updatelock->lock();
            sp->workersBitMask &= ~(1 << thread_id);
            Threads[thread_id].searching = false;
            sp->updatelock->unlock();
            ++Threads[thread_id].ended;
        }        
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
#ifdef SELF_TUNE2
        Threads[i].playingGame = false;
#endif
        Threads[i].split_point = NULL;
        Threads[i].searching = false;
        Threads[i].stop = false;
        Threads[i].started = 0;
        Threads[i].ended = 0;
        Threads[i].numsplits = 0;
        Threads[i].num_sp = 0;
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            for (int k = 0; k < MaxNumOfThreads; ++k) {
                Threads[i].sptable[j].workersBitMask = 0;
            }
        }
    }
}

void initThreads(void) {
    int i;
    for (i = 0; i < MaxNumOfThreads; ++i) {
        Threads[i].num_sp = 0;
        Threads[i].exit_flag = false;
        SearchInfo(i).thinking_status = STOPPED; // SMP HACK
    }
    for(i = 0; i < MaxNumOfThreads; i++) {
        RealThreads.push_back(std::thread(idleLoop, i));
    }
}

void stopThreads(void) {
    for(int i = 0; i < MaxNumOfThreads; i++) {
        Threads[i].stop = true;
        Threads[i].exit_flag = true;
        Threads[i].searching = false;        
    }
    for(int i = 0; i < MaxNumOfThreads; i++) {
        std::unique_lock<std::mutex>(Threads[i].threadLock);
        Threads[i].idle_event.notify_one();
        RealThreads[i].join();
        Print(1, "Gone here\n");
    }
    Print(1, "Gone here end\n");
}

void putUpSplitPoint(const position_t* p, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int thread_id) {
    SplitPoint *split_point = &Threads[thread_id].sptable[Threads[thread_id].num_sp];   

    split_point->updatelock->lock();
    split_point = &Threads[thread_id].sptable[Threads[thread_id].num_sp];  
    Threads[thread_id].num_sp++;
    split_point->parent = Threads[thread_id].split_point;
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
    split_point->pos[thread_id] = *p;
    split_point->origpos = *p;
    split_point->workersBitMask = ((uint64)1<<thread_id);
    split_point->allWorkersBitMask = ((uint64)1<<thread_id);
    Threads[thread_id].split_point = split_point;
    Threads[thread_id].searching = true;
    Threads[thread_id].stop = false;
    split_point->updatelock->unlock();
}

void putDownSplitPoint(SplitPoint *split_point, SearchStack* ss, const int thread_id) {
    split_point->updatelock->lock();
    Threads[thread_id].num_sp -= 1;
    ss->bestvalue = split_point->bestvalue;
    ss->bestmove = split_point->bestmove;
    ss->playedMoves = split_point->played;
    Threads[thread_id].split_point = split_point->parent;
    Threads[thread_id].numsplits++;
    if (SearchInfo(thread_id).thinking_status != STOPPED) {
        Threads[thread_id].stop = false; 
        Threads[thread_id].searching = true;
    }
    split_point->updatelock->unlock();
}
