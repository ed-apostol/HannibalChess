/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
void initSearchThread(int i) {
    memset(Threads[i].evalvalue, 0, sizeof(Threads[i].evalvalue));
    Threads[i].nodes_since_poll = 0;
    Threads[i].nodes_between_polls = 8192;
    Threads[i].nodes = 0;
    memset(Threads[i].killer1, 0, sizeof(Threads[i].killer1));
    memset(Threads[i].killer2, 0, sizeof(Threads[i].killer2));
}
void idleLoop(const int thread_id, split_point_t *master_sp) {
    ASSERT(thread_id < Guci_options->threads || master_sp == NULL);
    Threads[thread_id].running = true;
    while(1) {
        if(Threads[thread_id].exit_flag) break;

#ifndef TCEC
        if (thread_id >= MaxNumOfThreads - Guci_options->learnThreads) { //lets grab something and learn from it
            continuation_t toLearn;
            if (get_continuation_to_learn(&Glearn, &toLearn)) {
                if (LOG_LEARNING) Print(2,"learning: got continuation from file\n");
                if (learn_continuation(thread_id,&toLearn)==false)
                    Sleep(1000);
            }
            else {
                srand (getTime()+thread_id);//help get different continuations even if multiple threads call at the same time
                generateContinuation(&toLearn);
                if (learn_continuation(thread_id,&toLearn)==false)
                    Sleep(1000);
            }
        }
        else 
#endif
        {
            while(thread_id != 0 && master_sp == NULL && 
                (SearchInfo(thread_id).thinking_status == STOPPED
#ifndef TCEC
                && (thread_id >= Guci_options->threads && thread_id < MaxNumOfThreads - Guci_options->learnThreads)
#endif
                )) {
                    WaitForSingleObject(Threads[thread_id].idle_event, INFINITE);
            }
            if (thread_id < Guci_options->threads) {
                if(Threads[thread_id].work_assigned) {
                    ++Threads[thread_id].started;
                    Threads[thread_id].work_assigned = false;
                    split_point_t* sp = Threads[thread_id].split_point;
                    if (sp->inPv) searchNode<true, true>(&sp->pos[thread_id], sp->alpha, sp->beta, sp->depth, sp->inCheck, thread_id, false);
                    else searchNode<false, true>(&sp->pos[thread_id], sp->alpha, sp->beta, sp->depth, sp->inCheck, thread_id, false);
                    MutexLock(SMPLock);
                    if (Threads[thread_id].cutoff || (sp->master == thread_id && Threads[thread_id].stop)) {
                        for(int i = 0; i < Guci_options->threads; i++) {
                            if(i == sp->master || sp->slaves[i])
                                Threads[i].stop = true;
                        }
                    }
                    sp->cpus--;
                    sp->slaves[thread_id] = 0;
                    Threads[thread_id].idle = true;
                    Threads[thread_id].cutoff = false;
                    MutexUnlock(SMPLock);
                    ++Threads[thread_id].ended;
                }
                if(master_sp != NULL && master_sp->cpus == 0) return;
            }
        }
    }
    Threads[thread_id].running = false;
}

void initSmpVars() {
    for (int i = 0; i < MaxNumOfThreads; ++i) {
#ifdef SELF_TUNE2
        Threads[i].playingGame = false;
#endif
        Threads[i].work_assigned = false;
        Threads[i].idle = true;
        Threads[i].stop = false;
        Threads[i].cutoff = false;
        Threads[i].started = 0;
        Threads[i].ended = 0;
        Threads[i].numsplits = 0;
        Threads[i].num_sp = 0;
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            Threads[i].sptable[j].master = 0;
            Threads[i].sptable[j].cpus = 0;
            for (int k = 0; k < MaxNumOfThreads; ++k) {
                Threads[i].sptable[j].slaves[k] = 0;
            }
        }
    }
}

bool threadIsAvailable(int slave, int master) { // TODO: study this for improvement
    if(!Threads[slave].idle) return false;
    if(Threads[slave].num_sp == 0) 
        return true;
    if(Threads[slave].sptable[Threads[slave].num_sp-1].slaves[master])
        return true;
    return false;
}

bool idleThreadExists(int master) {
    for(int i = 0; i < Guci_options->threads; i++) {
        if(i != master && threadIsAvailable(i, master)) return true;
    }
    return false;
}

DWORD WINAPI winInitThread(LPVOID n) {
    int i = *((int*)((LPVOID*)n));
    //Print(3, "%s: thread_id:%d\n", __FUNCTION__, i);
    idleLoop(i, NULL); 
    return 0;
}

void initThreads(void) {
    volatile int i;
    DWORD iID[MaxNumOfThreads];
#ifndef TCEC
    MutexInit(LearningLock,NULL);
    MutexInit(BookLock,NULL);
#endif
    for (int i = 0; i < MaxNumOfThreads; ++i) {
        Threads[i].num_sp = 0;
        Threads[i].exit_flag = false;
        Threads[i].idle_event = CreateEvent(0, FALSE, FALSE, 0);
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            MutexInit(Threads[i].sptable[j].movelistlock, NULL);
            MutexInit(Threads[i].sptable[j].updatelock, NULL);
        }
        SearchInfo(i).thinking_status = STOPPED; // SMP HACK
    }

    MutexInit(SMPLock, NULL);
    //TODO consider add i=0 when dealing with auto-tuning code
    for(i = 1; i < MaxNumOfThreads; i++) {
        CreateThread(NULL, 0, winInitThread, (LPVOID)&i, 0, &iID[i]);
        while(!Threads[i].running);
    }
}

void stopThreads(void) {
    for(int i = 0; i < MaxNumOfThreads; i++) {
        Threads[i].stop = true;
        Threads[i].exit_flag = true;
        Threads[i].work_assigned = false;
        SetEvent(Threads[i].idle_event);
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            MutexDestroy(Threads[i].sptable[j].movelistlock);
            MutexDestroy(Threads[i].sptable[j].updatelock);
        }
    }
    MutexDestroy(SMPLock);
#ifndef TCEC
    MutexDestroy(LearningLock);
    MutexDestroy(BookLock);
#endif
}

void setAllThreadsToStop(int thread) {
    SearchInfo(thread).thinking_status = STOPPED;

    MutexLock(SMPLock);
    for (int i = 0; i < Guci_options->threads; i++) {
        Threads[i].stop = true;
    }
    MutexUnlock(SMPLock);
}

bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, int* bestvalue, basic_move_t* bestmove, int* played, 
                         int alpha, int beta, bool pvnode, int depth, int inCheck, const int master) {
                             split_point_t *split_point;
                             MutexLock(SMPLock);
                             if(!idleThreadExists(master) || Threads[master].num_sp >= MaxNumSplitPointsPerThread) {
                                 MutexUnlock(SMPLock); 
                                 return false;
                             }
                             split_point = &Threads[master].sptable[Threads[master].num_sp];//SAM awful lot of things happening here, maybe a cheaper way?
                             Threads[master].num_sp++;
                             split_point->parent = Threads[master].split_point;
                             split_point->depth = depth;
                             split_point->alpha = alpha; 
                             split_point->beta = beta;
                             split_point->inPv = pvnode;
                             split_point->bestvalue = *bestvalue;
                             split_point->bestmove = *bestmove;
                             split_point->played = *played;
                             split_point->master = master;
                             split_point->inCheck = inCheck;
                             split_point->cpus = 0;
                             split_point->parent_movestack = mvlist;
                             for(int i = 0; i < Guci_options->threads; i++) {
                                 split_point->slaves[i] = 0;
                                 if(threadIsAvailable(i, master) || i == master) {
                                     split_point->pos[i] = *p;
                                     Threads[i].split_point = split_point;
                                     if(i != master) split_point->slaves[i] = 1;
                                     split_point->cpus++;
                                 }
                             }
                             for(int i = 0; i < Guci_options->threads; i++) {
                                 if(i == master || split_point->slaves[i]) {
                                     Threads[i].work_assigned = true;
                                     Threads[i].idle = false;
                                     Threads[i].stop = false;
                                     Threads[i].cutoff = false;
                                 }
                                 if (split_point->slaves[i]) { // copy search stack from master to slaves
                                     Threads[i].evalvalue[p->ply] = Threads[master].evalvalue[p->ply];
                                 }
                             }
                             MutexUnlock(SMPLock);

                             if (SearchInfo(0).thinking_status==STOPPED) {
                                 setAllThreadsToStop(0);
                                 return false;
                             }

                             idleLoop(master, split_point);

                             MutexLock(SMPLock);
                             *bestvalue = split_point->bestvalue;
                             *bestmove = split_point->bestmove;
                             *played = split_point->played;
                             if (SearchInfo(0).thinking_status != STOPPED) {
                                 Threads[master].stop = false; 
                                 Threads[master].idle = false;
                             }
                             Threads[master].split_point = split_point->parent;
                             Threads[master].num_sp--;
                             Threads[master].numsplits++;
                             MutexUnlock(SMPLock);
                             return true;
}
