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

Spinlock SMPLock[1];
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
#ifdef SELF_TUNE2
    Threads[thread].stop = true;
#else
    SMPLock->lock();
    for (int i = 0; i < Guci_options.threads; i++) {
        Threads[i].stop = true;
    }
    SMPLock->unlock();
#endif
}

void checkForWork(int thread_id) { 
    if(Threads[thread_id].searching) return;
    if(Threads[thread_id].num_sp > 0) return;
    for(int threadIdx = 0; threadIdx < Guci_options.threads; threadIdx++) {
        SplitPoint* sp = &Threads[threadIdx].sptable[0];
        if (Threads[threadIdx].searching 
            && Threads[threadIdx].num_sp > 0
            && bitCnt(sp->allMask) < Guci_options.max_split_threads
            && bitCnt(sp->allMask) > 1 
            && sp->allMask == sp->workersBitMask) {
                SMPLock->lock();
                sp->updatelock->lock();
                if (Threads[threadIdx].searching 
                    && Threads[threadIdx].num_sp > 0
                    && !Threads[thread_id].searching
                    && Threads[thread_id].num_sp == 0
                    && bitCnt(sp->allMask) < Guci_options.max_split_threads
                    && bitCnt(sp->allMask) > 1 
                    && sp->allMask == sp->workersBitMask) {
                        sp->pos[thread_id] = sp->origpos;
                        Threads[thread_id].split_point = sp;
                        sp->workersBitMask |= ((uint64)1<<thread_id);
                        Threads[thread_id].searching = true;
                        Threads[thread_id].stop = false;
                }
                sp->updatelock->unlock();
                SMPLock->unlock();
        }
    }
}

void idleLoop(int thread_id, SplitPoint *master_sp) {
    ASSERT(thread_id < Guci_options.threads || master_sp == NULL);
    while(!Threads[thread_id].exit_flag) {
        while(master_sp == NULL && SearchInfo(thread_id).thinking_status == STOPPED && !Threads[thread_id].exit_flag) {
            Print(3, "Thread sleeping: %d\n", thread_id);
            std::unique_lock<std::mutex> lk(Threads[thread_id].threadLock);
            Threads[thread_id].idle_event.wait(lk);
        }
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
        if(master_sp != NULL) {
            if (!master_sp->workersBitMask) return;
            else {
                if (SearchInfo(thread_id).thinking_status == STOPPED) {
                    setAllThreadsToStop(thread_id);
                    return;
                }
            } 
        } else checkForWork(thread_id); // active reparenting
    }
    Print(3, "Thread quitting: %d\n", thread_id);
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

bool threadIsAvailable(int thread_id, int master) {
    if(Threads[thread_id].searching) return false;
    if(Threads[thread_id].num_sp == 0) return true;
    // the "thread_id" has this "master" as a slave in a higher split point
    // since "thread_id" is not searching and probably waiting for this "master" thread to finish,
    // it's better to help it finish the search (helpful master concept)
    if(Threads[thread_id].sptable[Threads[thread_id].num_sp-1].workersBitMask & ((uint64)1<<master)) return true;
    return false;
}

bool idleThreadExists(int master) {
    for(int i = 0; i < Guci_options.threads; i++) {
        if(i != master && threadIsAvailable(i, master)) return true;
    }
    return false;
}

void initThreads(void) {
    int i;
    for (i = 0; i < MaxNumOfThreads; ++i) {
        Threads[i].num_sp = 0;
        Threads[i].exit_flag = false;
        SearchInfo(i).thinking_status = STOPPED; // SMP HACK
    }
    for(i = 0; i < MaxNumOfThreads; i++) {
        SplitPoint* sp = NULL;
        RealThreads.push_back(std::thread(idleLoop, i, sp));
    }
    Print(1, "Gone here 3\n");
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

bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master) {
    SplitPoint *split_point = &Threads[master].sptable[Threads[master].num_sp];    
    SMPLock->lock();
    split_point->updatelock->lock();
    if(Threads[master].num_sp >= MaxNumSplitPointsPerThread || !idleThreadExists(master)) {
        split_point->updatelock->unlock();
        SMPLock->unlock();
        return false;
    }
    Threads[master].num_sp++;
    split_point->parent = Threads[master].split_point;
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
    Threads[master].split_point = split_point;

    int threadCnt = 1;
    for(int i = 0; i < Guci_options.threads; i++) {
        if(threadCnt < Guci_options.max_split_threads && threadIsAvailable(i, master)) {
            threadCnt++;
            split_point->pos[i] = *p;
            Threads[i].split_point = split_point;
            split_point->workersBitMask |= ((uint64)1<<i);
        }
    }
    for(int i = 0; i < Guci_options.threads; i++) {
        if(split_point->workersBitMask & ((uint64)1<<i)) {
            Threads[i].searching = true;
            Threads[i].stop = false;
        }
    }
    split_point->allMask = split_point->workersBitMask;
    split_point->updatelock->unlock();
    SMPLock->unlock();

    idleLoop(master, split_point);

    SMPLock->lock();
    split_point->updatelock->lock();
    ss->bestvalue = split_point->bestvalue;
    ss->bestmove = split_point->bestmove;
    ss->playedMoves = split_point->played;
    Threads[master].split_point = split_point->parent;
    Threads[master].num_sp--;
    Threads[master].numsplits++;
    if (SearchInfo(master).thinking_status != STOPPED) {
        Threads[master].stop = false; 
        Threads[master].searching = true;
    }
    split_point->updatelock->unlock();
    SMPLock->unlock();
    return true;
}
