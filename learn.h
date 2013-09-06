/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#ifndef TCEC
bool learn_position(position_t *pos,int thread_id, continuation_t *variation) {
    movelist_t mvlist;
    int bestScore = -INF;
    basic_move_t bestMove = 0;
    pos_store_t undo;
    search_info_t learnSearchInfo;

    SearchInfoMap[thread_id] = &learnSearchInfo;
    memset(SearchInfo(thread_id).history, 0, sizeof(SearchInfo(thread_id).history)); //TODO this is bad to share with learning
    memset(SearchInfo(thread_id).evalgains, 0, sizeof(SearchInfo(thread_id).evalgains)); //TODO this is bad to share with learning

    initSearchThread(thread_id);

    SearchInfo(thread_id).thinking_status = THINKING;
    SearchInfo(thread_id).node_is_limited = TRUE;
    SearchInfo(thread_id).node_limit = LEARN_NODES*(1+Guci_options->learnTime)+(LEARN_NODES*(Guci_options->learnThreads-1))/2; //add a little extra when using more threads
    SearchInfo(thread_id).time_is_limited = FALSE;
    SearchInfo(thread_id).depth_is_limited = FALSE;
    SearchInfo(thread_id).easy = 0;

    SearchInfo(thread_id).pt.table = NULL;
    initPawnTab(&SearchInfo(thread_id).pt, LEARN_PAWN_HASH_SIZE);
    SearchInfo(thread_id).et.table = NULL;
    initEvalTab(&SearchInfo(thread_id).et, LEARN_EVAL_HASH_SIZE);

    Threads[thread_id].nodes = 0;
    SearchInfo(thread_id).start_time = getTime();

    //first lets get the moves, and prune out ones that are already known
    genLegal(pos, &mvlist, false); 
    SearchInfo(thread_id).legalmoves = mvlist.size;
    int moveOn=0;
    MutexLock(BookLock);
    while (moveOn < mvlist.size) {
        basic_move_t move = mvlist.list[moveOn].m;
        if (anyRepNoMove(pos,move)) { 
            int score = DrawValue[pos->side];
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
            else {
                mvlist.list[moveOn].m = mvlist.list[mvlist.size-1].m;
                mvlist.size--;
            }
        }
        else {
            makeMove(pos, &undo, move);
            int score = -current_puck_book_score(pos,&GhannibalBook);
            if (score != DEFAULT_BOOK_SCORE) {
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
                mvlist.list[moveOn].m = mvlist.list[mvlist.size-1].m;
                mvlist.size--;
            }
            else moveOn++;
            unmakeMove(pos, &undo);
        }
    }
    MutexUnlock(BookLock);
    if (bestMove && DEBUG_LEARN) Print(3,"info string tentative %d: %s     ",bestScore,move2Str(bestMove));
    if (mvlist.size) {//ok, now get iteratively search for improvements of any unscored moves
        scoreRoot(&mvlist);
        for (int depth = 1; depth < MAXPLY; depth++) {
            int alpha, beta;
            if (depth >= 6) { 
                alpha = goodAlpha(SearchInfo(thread_id).best_value-16);
                beta = goodBeta(SearchInfo(thread_id).best_value+16);
            } else {
                alpha = -INF;
                beta = INF;
            }
            searchRoot(pos, &mvlist, alpha, beta, depth, thread_id,bestScore);
            if (Threads[thread_id].nodes > SearchInfo(thread_id).node_limit/2) SearchInfo(thread_id).thinking_status = STOPPED;
            if (SearchInfo(thread_id).thinking_status == STOPPED) break;
        }
    }
    else  if (SHOW_LEARNING) Print(3,"info string no unknown moves?!\n");
    free(SearchInfo(thread_id).pt.table);
    free(SearchInfo(thread_id).et.table);

    if (Threads[thread_id].nodes > SearchInfo(thread_id).node_limit/2 ||
        (Threads[thread_id].nodes > SearchInfo(thread_id).node_limit/3 && SearchInfo(thread_id).best_value <= bestScore)) { // if we stopped for some other reason don't trust the result
            int64 usedTime = getTime() - SearchInfo(thread_id).start_time;
            variation->length++;
            if (SearchInfo(thread_id).bestmove &&SearchInfo(thread_id).best_value > bestScore ) { //if we found a better move than repetition or previously known move
                bestMove = SearchInfo(thread_id).bestmove;
                bestScore = SearchInfo(thread_id).best_value;
                variation->moves[variation->length-1] = bestMove;
                if (SHOW_LEARNING) Print(3, "info string learning %s discovered: %d (%llu)\n",pv2Str(variation).c_str(),bestScore,usedTime/1000);
                if (LOG_LEARNING) Print(2, "learning: %s discovered: %d (%llu)\n",pv2Str(variation).c_str(),bestScore,usedTime/1000);
                makeMove(pos, &undo, SearchInfo(thread_id).bestmove); //write the resulting position score if it is an unknown position (not repetition)
                insert_score_to_puck_file(&GhannibalBook, pos->hash, -bestScore);//learn this position score, and after the desired move
                unmakeMove(pos, &undo);
            }
            else {
                variation->moves[variation->length-1] = bestMove;
                if (SHOW_LEARNING) Print(3, "info string learning %s reconsidered: %d (%llu)\n",pv2Str(variation).c_str(),bestScore,usedTime/1000);
                if (LOG_LEARNING) Print(3, "learning: %s reconsidered: %d (%llu)\n",pv2Str(variation).c_str(),bestScore,usedTime/1000);
            }
            variation->length--;
            if (bestMove) { //write the current position score
                insert_score_to_puck_file(&GhannibalBook, pos->hash, bestScore);//learn this position score, and after the desired move
            }
    }
    SearchInfoMap[thread_id] = &global_search_info; //reset before local variable disappears
    if (thread_id < MaxNumOfThreads - Guci_options->learnThreads) 
    { //this thread has been cancelled
        if (SHOW_LEARNING) Print(3,"info string cancelling search by learning thread %d\n",thread_id);
        if (LOG_LEARNING) Print(2,"learning: cancelling search by learning thread %d\n",thread_id);
        fflush(stdout);
        return false; //succesfull learning
    }
    return true; 

}
#endif