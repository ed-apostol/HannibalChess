/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

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
    MutexLock(SMPLock);
    for (int i = 0; i < Guci_options->threads; i++) {
        Threads[i].stop = true;
    }
    MutexUnlock(SMPLock);
#endif
}

void idleLoop(const int thread_id, SplitPoint *master_sp) {
    ASSERT(thread_id < Guci_options->threads || master_sp == NULL);
    //Print(3, "%s: thread_id:%d\n", __FUNCTION__, thread_id);
    Threads[thread_id].running = true;
    while(!Threads[thread_id].exit_flag) {
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
            while(master_sp == NULL && 
                (SearchInfo(thread_id).thinking_status == STOPPED
#ifndef TCEC
                && (thread_id >= Guci_options->threads && thread_id < MaxNumOfThreads - Guci_options->learnThreads)
#endif
                )) {
                    Print(2, "Thread sleeping: %d\n", thread_id);
                    WaitForSingleObject(Threads[thread_id].idle_event, INFINITE);
            }
            if(Threads[thread_id].searching) {
                ++Threads[thread_id].started;
                SplitPoint* sp = Threads[thread_id].split_point;
                if (sp->inRoot) searchNode<true, true, false>(&sp->pos[thread_id], sp->alpha, sp->beta, sp->depth, *sp->ssprev, thread_id, sp->nodeType);
                else searchNode<false, true, false>(&sp->pos[thread_id], sp->alpha, sp->beta, sp->depth, *sp->ssprev, thread_id, sp->nodeType);
                MutexLock(sp->updatelock);
                //if (sp->cutoff || (thread_id == sp->master && Threads[thread_id].stop)) {
                //    for(int i = 0; i < Guci_options->threads; i++) {
                //        if(i == sp->master || sp->slaves[i])
                //            Threads[i].stop = true;
                //    }
                //}
                sp->cpus--;
                sp->slaves[thread_id] = 0;
                Threads[thread_id].searching = false;
                MutexUnlock(sp->updatelock);
                ++Threads[thread_id].ended;
            }
            if(master_sp != NULL && master_sp->cpus == 0) {
                if (SearchInfo(thread_id).thinking_status != STOPPED) {
                    Threads[thread_id].stop = false; 
                    Threads[thread_id].searching = true;
                }
                return;
            }
            if (master_sp != NULL && master_sp->cpus != 0 && SearchInfo(thread_id).thinking_status == STOPPED) {
                Print(2, "Master Thread waiting: %d cpus: %d\n", thread_id, master_sp->cpus);
                //// HACK: no need to wait for other threads, let's kill them all, and quit the search ASAP
                setAllThreadsToStop(thread_id);
                return;
            }
    }
    Threads[thread_id].running = false;
}

bool smpCutoffOccurred(SplitPoint *splitpoint) {
    for (SplitPoint* sp = splitpoint; sp; sp = sp->parent) {
        if (sp->cutoff) return true;
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
            Threads[i].sptable[j].master = 0;
            Threads[i].sptable[j].cpus = 0;
            for (int k = 0; k < MaxNumOfThreads; ++k) {
                Threads[i].sptable[j].slaves[k] = 0;
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
    if(Threads[thread_id].sptable[Threads[thread_id].num_sp-1].slaves[master]) return true;
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
    int i;
    DWORD iID[MaxNumOfThreads];
    for (i = 0; i < MaxNumOfThreads; ++i) {
        Threads[i].num_sp = 0;
        Threads[i].exit_flag = false;
        Threads[i].running = false;
        Threads[i].idle_event = CreateEvent(0, FALSE, FALSE, 0);
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            MutexInit(Threads[i].sptable[j].movelistlock, NULL);
            MutexInit(Threads[i].sptable[j].updatelock, NULL);
        }
        SearchInfo(i).thinking_status = STOPPED; // SMP HACK
    }

    MutexInit(SMPLock, NULL);
#ifdef SELF_TUNE2
    for(i = 0; i < MaxNumOfThreads; i++) {
#else
    for(i = 1; i < MaxNumOfThreads; i++) {
#endif
        CreateThread(NULL, 0, winInitThread, (LPVOID)&i, 0, &iID[i]);
        while(!Threads[i].running);
    }
}

void stopThreads(void) {
    for(int i = 0; i < MaxNumOfThreads; i++) {
        Threads[i].stop = true;
        Threads[i].exit_flag = true;
        Threads[i].searching = false;
        SetEvent(Threads[i].idle_event);
        for (int j = 0; j < MaxNumSplitPointsPerThread; ++j) {
            MutexDestroy(Threads[i].sptable[j].movelistlock);
            MutexDestroy(Threads[i].sptable[j].updatelock);
        }
    }
    MutexDestroy(SMPLock);
}

bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master) {
    SplitPoint *split_point;
    MutexLock(SMPLock);
    if(!idleThreadExists(master) || Threads[master].num_sp >= MaxNumSplitPointsPerThread) {
        MutexUnlock(SMPLock); 
        return false;
    }
    split_point = &Threads[master].sptable[Threads[master].num_sp];
    Threads[master].num_sp++;
    split_point->parent = Threads[master].split_point;
    split_point->depth = depth;
    split_point->alpha = alpha; 
    split_point->beta = beta;
    split_point->nodeType = nt;
    split_point->bestvalue = ss->bestvalue;
    split_point->bestmove = ss->bestmove;
    split_point->played = ss->playedMoves;
    split_point->master = master;
    split_point->inCheck = inCheck;
    split_point->inRoot = inRoot;
    split_point->cpus = 0;
    split_point->cutoff = false;
    split_point->sscurr = ss;
    split_point->ssprev = ssprev;
    for(int i = 0; i < Guci_options->threads; i++) {
        split_point->slaves[i] = 0;
        if(master == i || threadIsAvailable(i, master)) {
            split_point->pos[i] = *p;
            Threads[i].split_point = split_point;
            if(i != master) split_point->slaves[i] = 1;
            split_point->cpus++;
        }
    }
    for(int i = 0; i < Guci_options->threads; i++) {
        if(i == master || split_point->slaves[i]) {
            Threads[i].searching = true;
            Threads[i].stop = false;
        }
    }
    MutexUnlock(SMPLock);

    if (SearchInfo(0).thinking_status==STOPPED) {
        setAllThreadsToStop(0);
        return false;
    }

    idleLoop(master, split_point);

    MutexLock(SMPLock);
    ss->bestvalue = split_point->bestvalue;
    ss->bestmove = split_point->bestmove;
    ss->playedMoves = split_point->played;
    Threads[master].split_point = split_point->parent;
    Threads[master].num_sp--;
    Threads[master].numsplits++;
    MutexUnlock(SMPLock);
    return true;
}
