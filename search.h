/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#define MAX_HDEPTH 20
#define NEW_HISTORY (10 + MAX_HDEPTH)

#define EXPLORE_CUTOFF 20
#define EXPLORE_DEPTH_PV 6
#define EXPLORE_DEPTH_NOPV 8
#define EXTEND_ONLY 0 // 3 means quiesc, pv and non-pv, 2 means both pv and non-pv, 1 means only pv (0-3)
#define Q_CHECK 1 // implies 1 check 
#define Q_PVCHECK 2 // implies 2 checks
#define MIN_REDUCTION_DEPTH 4 // default is FALSE

#define WORSE_TIME_BONUS 20 //how many points more than 20 it takes to increase time by alloc to a maximum of 2*alloc
#define CHANGE_TIME_BONUS 50 //what percentage of alloc to increase if the last move is a change move
#define LAST_PLY_TIME 40 //what percentage of alloc remaining to be worth trying another complete ply
#define INCREASE_CHANGE 0 //what percentage of alloc to increase during a change move
#define TIME_DIVIDER 30 //how many moves we divide remaining time into 

void ponderHit() { //no pondering in tuning
    int64 time = getTime() - SearchInfo(0).start_time;

    if ((SearchInfo(0).iteration >= 8 && (SearchInfo(0).legalmoves == 1 || SearchInfo(0).mate_found >= 3)) ||
        (time > SearchInfo(0).time_limit_abs)) {
            setAllThreadsToStop(0);
            Print(2, "info string Has searched enough the ponder move: aborting\n");
    } else {
        SearchInfo(0).thinking_status = THINKING;
        Print(2, "info string Switch from pondering to thinking\n");
    }
}

void check4Input(position_t *pos) {
    static char input[256];

    if (biosKey()) {
        if (fgets(input, 256, stdin) == NULL)
            strcpy(input, "quit");

        Print(2, "%s\n", input);
        if (!memcmp(input, "quit", 4)) {
            quit();
        } else if (!memcmp(input, "stop", 4)) {
            setAllThreadsToStop(0);
            Print(2, "info string Aborting search: stop\n");
            return;
        } else if (!memcmp(input, "ponderhit", 9)) {
            ponderHit();
        } else if (!memcmp(input, "isready", 7)) {
            needReplyReady = TRUE;
        }
    }
}

void initNode(position_t *pos, const int thread_id) {
    int64 time2;

    Threads[thread_id].nodes++;
#ifdef SELF_TUNE2
    if (Threads[thread_id].nodes >= SearchInfo(thread_id).node_limit && SearchInfo(thread_id).node_is_limited) {
        setAllThreadsToStop(thread_id);
    }
#endif
    if (thread_id == 0 && ++Threads[thread_id].nodes_since_poll >= Threads[thread_id].nodes_between_polls) {
        Threads[thread_id].nodes_since_poll = 0;
        if (SHOW_SEARCH) check4Input(pos);
        time2 = getTime();
        if (time2 - SearchInfo(thread_id).last_time > 1000) {
            int64 time = time2 - SearchInfo(thread_id).start_time;
            SearchInfo(thread_id).last_time = time2;
            if (SHOW_SEARCH) {
                uint64 sum_nodes;
                Print(1, "info ");
                Print(1, "time %llu ", time);
                sum_nodes = 0;
                for (int i = 0; i < Guci_options->threads; ++i) sum_nodes += Threads[i].nodes;
                Print(1, "nodes %llu ", sum_nodes);
                Print(1, "hashfull %d ", (TransTable(thread_id).used*1000)/TransTable(thread_id).size);
                Print(1, "nps %llu ", (sum_nodes*1000ULL)/(time));
                Print(1, "\n");
            }
        }
        if (SearchInfo(thread_id).thinking_status == THINKING && SearchInfo(thread_id).time_is_limited) {
            if (time2 > SearchInfo(thread_id).time_limit_max) {
                if (time2 < SearchInfo(thread_id).time_limit_abs) {
                    if (!SearchInfo(thread_id).research && !SearchInfo(thread_id).change) {
                        bool gettingWorse = SearchInfo(thread_id).best_value != -INF && SearchInfo(thread_id).best_value + 30 <= SearchInfo(thread_id).last_value;
                        bool wasGettingWorse = SearchInfo(thread_id).best_value == -INF && SearchInfo(thread_id).last_value + 30 <= SearchInfo(thread_id).last_last_value;
                        if (!gettingWorse && !wasGettingWorse) {
                            setAllThreadsToStop(thread_id);
                            Print(2, "info string Aborting search: time limit 2: %d\n", time2 - SearchInfo(thread_id).start_time);
                        }
                    }
                } else {
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: time limit 1: %d\n", time2 - SearchInfo(thread_id).start_time);
                }
            }
        }
    }
}

int simpleStalemate(const position_t *pos) {
    uint32 kpos, to;
    uint64 mv_bits;
    if (MinTwoBits(pos->color[pos->side] & ~pos->pawns)) return FALSE; 
    kpos = pos->kpos[pos->side];
    if (kpos != a1 && kpos != a8 && kpos != h1 && kpos != h8) return FALSE;
    mv_bits = KingMoves[kpos];
    while (mv_bits) {
        to = popFirstBit(&mv_bits);
        if (!isSqAtt(pos,pos->occupied,to,pos->side^1)) return FALSE;
    }
    return TRUE;
}

void copyPV(continuation_t *pv, continuation_t *newpv) {
    if (newpv->length)
        memcpy(&(pv->moves[0]),&newpv->moves,newpv->length*sizeof(uint32));
    pv->length = newpv->length;
}

void displayPV(const position_t *pos, continuation_t *pv, int depth, int alpha, int beta, int score) {
    uint64 time;
    uint64 sum_nodes = 0;

    ASSERT(pv != NULL);
    ASSERT(valueIsOk(score));

    time = getTime();
    SearchInfo(0).last_time = time;
    time = SearchInfo(0).last_time - SearchInfo(0).start_time;

    Print(1, "info depth %d ", depth);
    if (abs(score) < (INF - MAXPLY)) {

        if (score < beta) {
            if (score <= alpha) Print(1, "score cp %d upperbound ", score);
            else Print(1, "score cp %d ", score);
        } else Print(1, "score cp %d lowerbound ", score);
    } else {
        Print(1, "score mate %d ", (score>0)? (INF-score+1)/2 : -(INF+score)/2);
    }

    Print(1, "time %llu ", time);
    for (int i = 0; i < Guci_options->threads; ++i) sum_nodes += Threads[i].nodes;
    Print(1, "nodes %llu ", sum_nodes);
    Print(1, "hashfull %d ", (TransTable(0).used*1000)/TransTable(0).size);
    if (time > 10) Print(1, "nps %llu ", (sum_nodes*1000)/(time));
    Print(1, "pv ");
    for (int i = 0; i < pv->length; i++) printf("%s ", move2Str(pv->moves[i]));
    Print(1, "\n");
}

inline int moveIsTactical(uint32 m) {
    ASSERT(moveIsOk(m));
    return (m & 0x01fe0000UL);
}

inline int historyIndex(uint32 side, uint32 move) {
    return ((((side) << 9) + ((movePiece(move)) << 6) + (moveTo(move))) & 0x3ff);
}

void updateEvalgains(const position_t *pos, uint32 move, int before, int after, const int thread_id) {
    if (move != EMPTY
        && before != -INF
        && after != -INF
        && !moveIsTactical(move)) {
            if (-(before + after) >= SearchInfo(thread_id).evalgains[historyIndex(pos->side^1, move)])
                SearchInfo(thread_id).evalgains[historyIndex(pos->side^1, move)] = -(before + after);
            else
                SearchInfo(thread_id).evalgains[historyIndex(pos->side^1, move)]--;
    }
}

const int MaxPieceValue[] = {0, PawnValueEnd, KnightValueEnd, BishopValueEnd, RookValueEnd, QueenValueEnd, 10000};

int qSearch(position_t *pos, int alpha, int beta, const int depth, const bool inCheck, const int thread_id, int inPv) {
    int bestvalue = -INF;
    int oldalpha = alpha;
    int score;
    int newdepth;
    int pes = 0, opt = 0;
    bool moveGivesCheck;
    uint32 bestmove = EMPTY;
    uint64 pinned;
    uint64 dcc;
    basic_move_t move;
    movelist_t mvlist;
    pos_store_t undo;
    trans_entry_t * entry = NULL;
    uint32 hashMove = EMPTY;
    int mvlist_phase;

    ASSERT(pos != NULL);
    ASSERT(oldPV != NULL);
    ASSERT(valueIsOk(alpha));
    ASSERT(valueIsOk(beta));
    ASSERT(alpha < beta);
    ASSERT(!kingIsInCheck(pos));

    initNode(pos, thread_id);
    if (Threads[thread_id].stop) return 0;

    entry = transProbe(pos->hash,thread_id);
    if (entry != NULL) {
        hashMove = transMove(entry);
        if (!inPv) {
            if (transMinvalue(entry) > alpha && alpha < MAXEVAL) return transMinvalue(entry);
            if (transMaxvalue(entry) < beta && beta > -MAXEVAL) return transMaxvalue(entry);
        }
    }
    if (pos->ply >= MAXPLY-1) return eval(pos, thread_id, &opt, &pes);
    if (!inCheck) {
        if (simpleStalemate(pos)) {
            bestvalue = DrawValue[pos->side];
            goto cut;
        }
        Threads[thread_id].evalvalue[pos->ply] = bestvalue = eval(pos, thread_id, &opt, &pes);
        updateEvalgains(pos, pos->posStore.lastmove, Threads[thread_id].evalvalue[pos->ply-1], Threads[thread_id].evalvalue[pos->ply], thread_id);
        if (Threads[thread_id].evalvalue[pos->ply] > alpha) {
            if (Threads[thread_id].evalvalue[pos->ply] >= beta) {
                bestvalue = Threads[thread_id].evalvalue[pos->ply];
                goto cut;
            }
            alpha = Threads[thread_id].evalvalue[pos->ply];
        }
    }
    pinned = pinnedPieces(pos, pos->side);
    dcc = discoveredCheckCandidates(pos, pos->side);
    if (inCheck) {
        sortInit(pos, &mvlist, pinned, hashMove, alpha, depth, MoveGenPhaseEvasion, thread_id);
    } else {
        if (inPv) sortInit(pos, &mvlist, pinned, hashMove, alpha, depth, (depth > -Q_PVCHECK) ? MoveGenPhaseQuiescenceAndChecksPV : MoveGenPhaseQuiescencePV, thread_id);
        else sortInit(pos, &mvlist, pinned, hashMove, alpha, depth, (depth > -Q_CHECK) ? MoveGenPhaseQuiescenceAndChecks : MoveGenPhaseQuiescence, thread_id);
    }
    bool prunable = !inCheck && !inPv && MinTwoBits(pos->color[pos->side^1] & pos->pawns) && MinTwoBits(pos->color[pos->side^1] & ~(pos->pawns | pos->kings));
    while ((move = sortNext(NULL, pos, &mvlist, &mvlist_phase, thread_id)) != EMPTY) {
        if (anyRepNoMove(pos,move)) { 
            score = DrawValue[pos->side];
        } else {
            moveGivesCheck = moveIsCheck(pos, move, dcc);
            if (prunable && move != hashMove && !moveGivesCheck && (movePiece(move) != PAWN || !moveIsPassedPawn(pos, move))) {
                int scoreAprox = Threads[thread_id].evalvalue[pos->ply] + MaxPieceValue[moveCapture(move)] + PawnValueEnd; 
                if (scoreAprox < beta) {
                    bestvalue = MAX(bestvalue, scoreAprox);
                    continue; //todo try out opt, tryout see instead of maxpiecevalue
                }
            }
            newdepth = depth - !moveGivesCheck;
            makeMove(pos, &undo, move);
            score = -qSearch(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id, inPv);
            unmakeMove(pos, &undo);
        }
        if (score > bestvalue) {
            bestvalue = score;
            if (score > alpha) {
                bestmove = move;
                if (score >= beta) break;
                alpha = score;
            }
        }
    }
    if (inCheck && bestvalue == -INF) return (-INF + pos->ply);
cut:    
    transStore(pos->hash, bestmove, 0, ((bestvalue > oldalpha) ? bestvalue : -INF), ((bestvalue < beta) ? bestvalue : INF), thread_id);
    ASSERT(valueIsOk(bestvalue));
    return bestvalue;
}

template <bool inRoot, bool inSplitPoint, bool inSingular>
int searchNode(position_t *pos, int alpha, int beta, const int depth, const bool inCheck, const basic_move_t moveBanned, const int thread_id, NodeType nt) {
    if (depth <= 0) return qSearch(pos, alpha, beta, 0, inCheck, thread_id, ((nt==PVNode)?true:false));
    else {
        if (inCheck) return searchGeneric<inRoot, inSplitPoint, true, inSingular>(pos, alpha, beta, depth, moveBanned, thread_id, nt);
        else return searchGeneric<inRoot, inSplitPoint, false, inSingular>(pos, alpha, beta, depth, moveBanned, thread_id, nt);
    }
}

inline bool inPvNode(NodeType nt) { return (nt==PVNode);}
inline bool inCutNode(NodeType nt) { return (nt==CutNode);}
inline bool inAllNode(NodeType nt) { return (nt==AllNode);}
inline NodeType invertNode(NodeType nt) { return ((nt==PVNode) ? PVNode : ((nt==CutNode) ? AllNode : CutNode));}

template<bool inRoot, bool inSplitPoint, bool inCheck, bool inSingular>
int searchGeneric(position_t *pos, int alpha, int beta, const int depth, const basic_move_t bannedMove, const int thread_id, NodeType nt) {
    int bestvalue = -INF;
    int oldalpha = alpha;
    int score;
    int newdepth;
    bool moveGivesCheck;
    int played = 0;
    int pes = 0, opt = 0;
    uint32 bestmove = EMPTY;
    uint64 dcc;
    basic_move_t move;
    movelist_t movelist;
    movelist_t* mvlist = &movelist;
    pos_store_t undo;
    trans_entry_t * entry = NULL;
    int firstExtend = FALSE;
    int hashDepth;
    basic_move_t hashMove = EMPTY;
    int prune = (abs(alpha) < MAXEVAL);
    int mvlist_phase;
    split_point_t* sp = NULL;
    uint64 nullThreatMoveToBit = 0;
    basic_move_t hisMoves[64] = {0};

    ASSERT(pos != NULL);
    ASSERT(oldPV != NULL);
    ASSERT(valueIsOk(alpha));
    ASSERT(valueIsOk(beta));
    ASSERT(alpha < beta);
    ASSERT(depth >= 1);

    initNode(pos, thread_id);
    if (Threads[thread_id].stop) return 0;

    if (!inRoot && !inSingular && !inSplitPoint) {
        entry = transProbe(pos->hash, thread_id);
        if (entry != NULL) {
            hashMove = transMove(entry);
            if (!inPvNode(nt)) {
                if (depth <= transMindepth(entry) && transMinvalue(entry) > alpha && alpha < MAXEVAL) {
                    if (transMinvalue(entry) >= beta && hashMove && hashMove != Threads[thread_id].killer1[pos->ply] && !moveIsTactical(hashMove)) {
                        Threads[thread_id].killer2[pos->ply] = Threads[thread_id].killer1[pos->ply];
                        Threads[thread_id].killer1[pos->ply] = hashMove;
                    }
                    return transMinvalue(entry);
                }
                if (depth <= transMaxdepth(entry) && transMaxvalue(entry) < beta && beta > -MAXEVAL) {
                    return transMaxvalue(entry);
                }
            }
            hashDepth = transMovedepth(entry);
            if (transMinvalue(entry) != -INF) Threads[thread_id].evalvalue[pos->ply] = transMinvalue(entry);
            else if (transMaxvalue(entry) != INF) Threads[thread_id].evalvalue[pos->ply] = transMaxvalue(entry);
            else {
                Threads[thread_id].evalvalue[pos->ply] = eval(pos, thread_id, &opt, &pes);
                hashDepth = 0;
            }
        } else {
            Threads[thread_id].evalvalue[pos->ply] = eval(pos, thread_id, &opt, &pes);
            hashDepth = 0;
            hashMove = EMPTY;
        }
        if (pos->ply >= MAXPLY-1) return Threads[thread_id].evalvalue[pos->ply];
        updateEvalgains(pos, pos->posStore.lastmove, Threads[thread_id].evalvalue[pos->ply-1], Threads[thread_id].evalvalue[pos->ply], thread_id);

        if (!inPvNode(nt) && !inCheck && prune) {
            int rvalue;
            if (inCutNode(nt) && depth <= 3 && pos->color[pos->side] & ~(pos->pawns | pos->kings) 
            && beta < (rvalue = Threads[thread_id].evalvalue[pos->ply] - FutilityMarginTable[MIN(depth,MAX_FUT_MARGIN)][0]))  {
                return rvalue;
            }
            if (depth <= 3 && hashMove == EMPTY && !(Rank7ByColorBB[pos->side] & pos->pawns)
            && Threads[thread_id].evalvalue[pos->ply] < (rvalue = beta - FutilityMarginTable[MIN(depth,MAX_FUT_MARGIN)][0])) { 
                score = searchNode<false, false, false>(pos, rvalue-1, rvalue, 0, false, EMPTY, thread_id, nt);
                if (score < rvalue) return score;
            }
            if (inCutNode(nt) && depth >= 2 && (MinTwoBits(pos->color[pos->side] & ~(pos->pawns))) && Threads[thread_id].evalvalue[pos->ply] >= beta) {
                if (depth > 6) score = searchNode<false, false, false>(pos, alpha, beta, depth - 6, false, EMPTY, thread_id, AllNode);
                else score = beta;
                if (score >= beta) {
                    int nullDepth = depth - (4 + depth/5 + (Threads[thread_id].evalvalue[pos->ply] - beta > PawnValue));
                    makeNullMove(pos, &undo);
                    score = -searchNode<false, false, false>(pos, -beta, -alpha, nullDepth, false, EMPTY, thread_id, AllNode);
                    if ((entry = transProbe(pos->hash, thread_id)) != NULL && transMove(entry) != EMPTY) {
                        nullThreatMoveToBit = BitMask[moveTo(transMove(entry))];
                    }
                    unmakeNullMove(pos, &undo);
                    if (score >= beta) return score;
                }
            }
        }
        if (!inAllNode(nt) && !inCheck && depth >= (inPvNode(nt)?4:6) && prune) { // IID
            newdepth = depth / 2;
            if (hashMove == EMPTY || hashDepth < newdepth) {
                score = searchNode<false, false, false>(pos, alpha, beta, newdepth, inCheck, EMPTY, thread_id, nt);
                if (score > alpha) {
                    if ((entry = transProbe(pos->hash, thread_id)) != NULL) hashMove = transMove(entry);
                    Threads[thread_id].evalvalue[pos->ply] = score;
                    hashDepth = newdepth;
                }
            }
        }
        if (!inAllNode(nt) && !inCheck && depth >= (inPvNode(nt)?EXPLORE_DEPTH_PV:EXPLORE_DEPTH_NOPV) && prune) { // singular extension
            newdepth = depth / 2;
            if (hashMove != EMPTY && hashDepth >= newdepth) { 
                int targetScore = Threads[thread_id].evalvalue[pos->ply] - EXPLORE_CUTOFF;
                score = searchNode<false, false, true>(pos, targetScore, targetScore+1, newdepth, inCheck, hashMove, thread_id, nt);
                if (score <= targetScore) firstExtend = TRUE;
            }
        }
    }

    dcc = discoveredCheckCandidates(pos, pos->side);

    if (inSplitPoint) {
        sp = Threads[thread_id].split_point;
        bestvalue = sp->bestvalue;
        bestmove = sp->bestmove;
        mvlist = sp->parent_movestack;
        played = sp->played;
    } else {
        if (inRoot) {
            mvlist = &SearchInfo(thread_id).rootmvlist;
            if (!SearchInfo(thread_id).mvlist_initialized) {
                if ((entry = transProbe(pos->hash, thread_id)) != NULL) hashMove = transMove(entry);
                sortInit(pos, mvlist, pinnedPieces(pos, pos->side), hashMove, alpha, depth, MoveGenPhaseRoot, thread_id);
            } else {
                mvlist->pos = 0;
                mvlist->phase = MoveGenPhaseRoot + 1;
            }
        } else {
            sortInit(pos, mvlist, pinnedPieces(pos, pos->side), hashMove, alpha, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), thread_id);
            firstExtend |= (inCheck && mvlist->size==1);
        }
    }
    int pruneable =  prune && MinTwoBits(pos->color[pos->side] & ~(pos->pawns | pos->kings));
    int lateMove = LATE_PRUNE_MIN + (inCutNode(nt) ? ((depth * depth) / 4) : (depth * depth));
    int hisOn = 0;
    while ((move = sortNext(sp, pos, mvlist, &mvlist_phase, thread_id)) != EMPTY) {
        if (inSplitPoint) {
            MutexLock(sp->updatelock);
            played = ++sp->played;
            MutexUnlock(sp->updatelock);
        } else ++played;
        if (inSingular && move == bannedMove) continue;
        if (hisOn < 64 && !moveIsTactical(move)) {
            hisMoves[hisOn++] = move;
        }
        if (anyRepNoMove(pos, move)) { 
            score = DrawValue[pos->side];
        } else {
            moveGivesCheck = moveIsCheck(pos, move, dcc);
            newdepth = depth - !(firstExtend && played==1);
            if (bestvalue == -INF) {
                makeMove(pos, &undo, move);
                score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, EMPTY, thread_id, invertNode(nt));
            } else {
                int okToPruneOrReduce = (newdepth >= depth || inCheck || moveGivesCheck || MoveGenPhase[mvlist_phase] != PH_QUIET_MOVES) ? 0 : 1;
                int newdepthclone = newdepth;
                if (!inRoot && !inPvNode(nt) && okToPruneOrReduce && pruneable) {
                    if (played > lateMove && !isMoveDefence(pos, move, nullThreatMoveToBit)) continue;
                    int predictedDepth = MAX(0,newdepth - ReductionTable[1][MIN(depth,63)][MIN(played,63)]);
                    score = Threads[thread_id].evalvalue[pos->ply]
                        + FutilityMarginTable[MIN(predictedDepth,MAX_FUT_MARGIN)][MIN(played,63)]
                        + SearchInfo(thread_id).evalgains[historyIndex(pos->side, move)];
                    if (score < beta  && !moveIsPassedPawn(pos, move)) {
                        bestvalue = MAX(bestvalue, score);
                        if (inSplitPoint) {
                            MutexLock(sp->updatelock);
                            sp->bestvalue = MAX(sp->bestvalue, bestvalue);
                            MutexUnlock(sp->updatelock);
                        }
                        continue;
                    }
                    if (inCutNode(nt) && swap(pos, move) < 0) {
                        if (predictedDepth < 4) continue;
                        newdepthclone--;
                    }
                }
                if (okToPruneOrReduce && depth >= MIN_REDUCTION_DEPTH) newdepthclone -= ReductionTable[(inPvNode(nt)?0:1)][MIN(depth,63)][MIN(played,63)];
                makeMove(pos, &undo, move);
                if (inSplitPoint) alpha = sp->alpha;
                score = -searchNode<false, false, false>(pos, -alpha-1, -alpha, newdepthclone, moveGivesCheck, EMPTY, thread_id, CutNode);
                if (newdepthclone < newdepth && score > alpha) {
                    score = -searchNode<false, false, false>(pos, -alpha-1, -alpha, newdepth, moveGivesCheck, EMPTY, thread_id, AllNode);
                }
                if (inPvNode(nt) && score > alpha) {
                    if (inRoot) SearchInfo(thread_id).research = 1;
                    score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, EMPTY, thread_id, PVNode);
                    if (inRoot) SearchInfo(thread_id).research = 0;
                }
            }
            unmakeMove(pos, &undo);
        }
        if (Threads[thread_id].stop) return bestvalue;
        if (inRoot) {
            mvlist->list[mvlist->pos-1].s = score;
            if (score > SearchInfo(thread_id).rbestscore1) {
                SearchInfo(thread_id).rbestscore1 = score;
            } 
            if (score > SearchInfo(thread_id).rbestscore2 && score < SearchInfo(thread_id).rbestscore1) {
                SearchInfo(thread_id).rbestscore2 = score;
            }
        }
        if (inSplitPoint) MutexLock(sp->updatelock);
        if (score > (inSplitPoint ? sp->bestvalue : bestvalue)) {
            bestvalue = inSplitPoint ? sp->bestvalue = score : score;
            if (inRoot) {
                extractPvMovesFromHash(pos, &SearchInfo(thread_id).rootPV, move, true);
                if (SearchInfo(thread_id).iteration > 1 && SearchInfo(thread_id).bestmove != move) SearchInfo(thread_id).change = 1;
                SearchInfo(thread_id).bestmove = move;
                if (SearchInfo(thread_id).rootPV.length > 1) SearchInfo(thread_id).pondermove = SearchInfo(thread_id).rootPV.moves[1];
                else SearchInfo(thread_id).pondermove = 0;
            }
            if (bestvalue > (inSplitPoint ? sp->alpha : alpha)) {
                bestmove = inSplitPoint ? sp->bestmove = move : move;
                if (inPvNode(nt) && bestvalue < beta) {
                    alpha = inSplitPoint ? sp->alpha = bestvalue : bestvalue;
                } else {
                    if (inRoot) {
                        for (int i = mvlist->pos; i < mvlist->size; ++i) mvlist->list[i].s = -INF;
                    }
                    if (inSplitPoint) {
                        Threads[thread_id].cutoff = true;
                        MutexUnlock(sp->updatelock);
                    }
                    break;
                }
            }
        }
        if (inSplitPoint) MutexUnlock(sp->updatelock);
        if(!inRoot && !inSplitPoint && !inSingular && !Threads[thread_id].stop && !inCheck
            && Guci_options->threads > 1 && depth >= Guci_options->min_split_depth && idleThreadExists(thread_id)
            && splitRemainingMoves(pos, mvlist, &bestvalue, &bestmove, &played, alpha, beta, nt, depth, inCheck, inRoot, thread_id)) {
                break;
        }
    }
    if (inRoot) {
        if (SHOW_SEARCH && depth >= 8 && (!Threads[thread_id].stop || bestvalue != -INF))
            displayPV(pos, &SearchInfo(thread_id).rootPV, depth, oldalpha, beta, bestvalue);
        if (bestvalue <= oldalpha || bestvalue >= beta) SearchInfo(thread_id).research = 1;
    }
    if (!inSingular || bannedMove == EMPTY) {
        if (played == 0) {
            if (inCheck) return -INF + pos->ply;
            else return DrawValue[pos->side];
        }
        if (Threads[thread_id].stop) return 0;
        if (bestmove != EMPTY && !moveIsTactical(bestmove) && bestvalue >= beta) { //> alpha account for pv better maybe? Sam
            int index = historyIndex(pos->side, bestmove);
            int hisDepth = MIN(depth,MAX_HDEPTH);
            SearchInfo(thread_id).history[index] += hisDepth * hisDepth;
            for (int i=0; i < hisOn-1; ++i) { // don't include the last index, it's the bestmove
                index = historyIndex(pos->side, hisMoves[i]);
                SearchInfo(thread_id).history[index] -= SearchInfo(thread_id).history[index]/(NEW_HISTORY-hisDepth);
            }
            if (Threads[thread_id].killer1[pos->ply] != bestmove) {
                Threads[thread_id].killer2[pos->ply] = Threads[thread_id].killer1[pos->ply];
                Threads[thread_id].killer1[pos->ply] = bestmove;
            }
        }
        transStore(pos->hash, bestmove, depth, ((bestvalue > oldalpha) ? bestvalue : -INF), ((bestvalue < beta) ? bestvalue : INF),thread_id);
    }
    ASSERT(valueIsOk(bestvalue));
    return bestvalue;
}

void extractPvMovesFromHash(position_t *pos, continuation_t* pv, basic_move_t move, bool execMove) {
    trans_entry_t *entry;
    pos_store_t undo[MAXPLY];
    int ply = 0;
    basic_move_t hashMove;
    pv->length = 0;
    pv->moves[pv->length++] = move;
    if (execMove) makeMove(pos, &(undo[ply++]), move);
    while ((entry = transProbe(pos->hash,0)) != NULL) {
        hashMove = transMove(entry);
        if (hashMove == EMPTY) break;
        if (!genMoveIfLegal(pos, hashMove, pinnedPieces(pos, pos->side))) break;
        pv->moves[pv->length++] = hashMove;
        if (anyRep(pos)) break; // break on repetition to avoid long pv display
        makeMove(pos, &(undo[ply++]), hashMove);
        if (ply >= MAXPLY) break;
    }
    for (ply = ply-1; ply >= 0; --ply) {
        unmakeMove(pos, &(undo[ply]));
    }
}

void repopulateHash(position_t *pos,continuation_t *rootPV,int depth) {
    int moveOn;
    int maxMoves = rootPV->length;
    pos_store_t undo[MAXPLY];
    for (moveOn=0; moveOn+1 <= maxMoves; moveOn++) {
        int move = rootPV->moves[moveOn];
        if (!move) break;
        transStore(pos->hash, move, depth , -INF, INF,0);
        makeMove(pos, &(undo[moveOn]), move);
        if (depth > 0) depth--;
    }
    for (moveOn = moveOn-1; moveOn >= 0; moveOn--) {
        unmakeMove(pos, &(undo[moveOn]));
    }
}

void timeManagement(int depth, int thread_id) {
    if (SearchInfo(thread_id).best_value > INF - MAXPLY) SearchInfo(thread_id).mate_found++;
    if (SearchInfo(thread_id).thinking_status == THINKING) {
        if (SearchInfo(thread_id).time_is_limited) {
            int64 time = getTime();
            bool gettingWorse = (SearchInfo(thread_id).best_value + 30) <= SearchInfo(thread_id).last_value;
            if (SearchInfo(thread_id).legalmoves == 1 || SearchInfo(thread_id).mate_found >= 3) { 
                if (depth >= 8) {
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: legalmove/mate found depth >= 8\n");
                }
            } 
            if (time + (SearchInfo(thread_id).alloc_time * LAST_PLY_TIME)/100 >= SearchInfo(thread_id).time_limit_max) {
                int64 addTime = 0;
                if (gettingWorse) {
                    int amountWorse = SearchInfo(thread_id).last_value - SearchInfo(thread_id).best_value;
                    addTime += ((amountWorse - 20) * SearchInfo(thread_id).alloc_time)/WORSE_TIME_BONUS;
                    Print(2, "info string Extending time (root gettingWorse): %d\n", addTime);
                }
                if (SearchInfo(thread_id).change) {
                    addTime += (SearchInfo(thread_id).alloc_time * CHANGE_TIME_BONUS) / 100;
                    Print(2, "info string Extending time (root change): %d\n", addTime);
                }
                if (addTime) {
                    SearchInfo(thread_id).time_limit_max += addTime;
                    if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
                    if (time + (SearchInfo(thread_id).alloc_time * LAST_PLY_TIME)/100 >= SearchInfo(thread_id).time_limit_abs) {
                        setAllThreadsToStop(thread_id);
                        Print(2, "info string Aborting search: root time limit 2: %d\n", time - SearchInfo(thread_id).start_time);
                    }
                }
                else { // if we are unlikely to get deeper, save our time
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: root time limit 1: %d\n", time - SearchInfo(thread_id).start_time);
                }
            } else if (!gettingWorse 
            && SearchInfo(thread_id).try_easy 
            && SearchInfo(thread_id).iteration >= 12
            && SearchInfo(thread_id).best_value > -MAXEVAL
            && SearchInfo(thread_id).rbestscore2 != -INF
            && time + (SearchInfo(thread_id).alloc_time * 80)/100 >= SearchInfo(thread_id).time_limit_max) {
                SearchInfo(thread_id).try_easy = false;
                Print(3, "info string Trying easy: score1: %d, score2: %d\n", SearchInfo(thread_id).rbestscore1, SearchInfo(thread_id).rbestscore2);
                if (SearchInfo(thread_id).rbestscore1 >= SearchInfo(thread_id).rbestscore2 + PawnValueEnd) { // TODO: to be tuned
                    setAllThreadsToStop(thread_id);
                    Print(3, "info string Aborting search: easy move: score1: %d, score2: %d, %d ms\n",
                        SearchInfo(thread_id).rbestscore1, SearchInfo(thread_id).rbestscore2, time - SearchInfo(thread_id).start_time);
                }
            }
        }
    } 
    if (SearchInfo(thread_id).depth_is_limited && depth >= SearchInfo(thread_id).depth_limit) {
        setAllThreadsToStop(thread_id);
        Print(2, "info string Aborting search: depth limit 1\n");
    }
}

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
void getBestMove(position_t *pos, int thread_id) {
    int id;
    trans_entry_t *entry;
    uci_option_t *opt = Guci_options;
    basic_move_t hashMove;
    int alpha, beta;
    bool inCheck = kingIsInCheck(pos);
    ASSERT(pos != NULL);

    hashMove = EMPTY;
    transNewDate(TransTable(thread_id).date, thread_id);
    entry = transProbe(pos->hash, thread_id);
    if (entry != NULL) hashMove = transMove(entry);
    // extend time when there is no hashmove from hashtable, this is useful when just out of the book
    if (hashMove == EMPTY) { 
        SearchInfo(thread_id).time_limit_max += SearchInfo(thread_id).alloc_time / 2;
        if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs)
            SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
    }

#ifndef TCEC
    if (SearchInfo(thread_id).thinking_status == THINKING && opt->try_book && pos->sp <= opt->book_limit && !anyRep(pos) && SearchInfo(thread_id).outOfBook < 8) {
        if (DEBUG_BOOK) Print(3,"info string num moves %d\n",mvlist.size);
        book_t *book = opt->usehannibalbook ? &GhannibalBook : &GpolyglotBook;
        if ((SearchInfo(thread_id).bestmove = getBookMove(pos,book,&mvlist,true,Guci_options->bookExplore*5)) != 0) {
            SearchInfo(thread_id).outOfBook = 0;
            return;
        }
        if (opt->usehannibalbook /*&& Guci_options->try_book*/ && SearchInfo(thread_id).outOfBook < MAXLEARN_OUT_OF_BOOK && movesSoFar.length > 0) 
            add_to_learn_begin(&Glearn,&movesSoFar);
    }
    if (SearchInfo(thread_id).thinking_status != PONDERING) SearchInfo(thread_id).outOfBook++;
#endif

    // SMP 
    initSmpVars();
    for (int i = 1; i < Guci_options->threads; ++i) SetEvent(Threads[i].idle_event);
    Threads[thread_id].split_point = &Threads[thread_id].sptable[0];
    SearchInfo(thread_id).mvlist_initialized = false;

    SearchInfo(thread_id).try_easy = true;
    for (id = 1; id < MAXPLY; id++) {
        int faillow = 0, failhigh = 0;
        SearchInfo(thread_id).iteration = id;
        SearchInfo(thread_id).best_value = -INF;
        SearchInfo(thread_id).change = 0;
        SearchInfo(thread_id).research = 0;
        if (id < 6) {
            alpha = -INF;
            beta = INF;
        } else {
            alpha = SearchInfo(thread_id).last_value-16;
            beta = SearchInfo(thread_id).last_value+16;
            if (alpha < -RookValue) alpha = -INF;
            if (beta > RookValue) beta = INF;
        }
        while (true) {
            SearchInfo(thread_id).rbestscore1 = -INF;
            SearchInfo(thread_id).rbestscore2 = -INF;
            SearchInfo(thread_id).best_value = searchNode<true, false, false>(pos, alpha, beta, id, inCheck, EMPTY, thread_id, PVNode);
            if (SearchInfo(thread_id).thinking_status == STOPPED) break;
            if(SearchInfo(thread_id).best_value <= alpha) {
                if (SearchInfo(thread_id).best_value <= alpha) alpha = SearchInfo(thread_id).best_value-(16<<++faillow);
                if (alpha < -RookValue) alpha = -INF;
            } else if(SearchInfo(thread_id).best_value >= beta) {
                if (SearchInfo(thread_id).best_value >= beta)  beta = SearchInfo(thread_id).best_value+(16<<++failhigh);
                if (beta > RookValue) beta = INF;
            } else break;
        }
        if (SearchInfo(thread_id).thinking_status == STOPPED) break;
        repopulateHash(pos, &SearchInfo(thread_id).rootPV, id);
        timeManagement(id, thread_id);
        if (SearchInfo(thread_id).thinking_status == STOPPED) break;
        if (SearchInfo(thread_id).best_value != -INF) {
            SearchInfo(thread_id).last_last_value = SearchInfo(thread_id).last_value;
            SearchInfo(thread_id).last_value = SearchInfo(thread_id).best_value;
        }
    }
    if (SearchInfo(thread_id).thinking_status != STOPPED) {
        if ((SearchInfo(thread_id).depth_is_limited || SearchInfo(thread_id).time_is_limited) && SearchInfo(thread_id).thinking_status == THINKING) {
            SearchInfo(thread_id).thinking_status = STOPPED;
            setAllThreadsToStop(thread_id);
            Print(2, "info string Aborting search: end of getBestMove: id=%d, best_value = %d sp = %d, ply = %d\n", 
                id, SearchInfo(thread_id).best_value, pos->sp, pos->ply);
        } else {
            Print(3, "info string Waiting for stop, quit, or ponderhit\n");
            do {
                check4Input(pos);
            } while (SearchInfo(thread_id).thinking_status != STOPPED);
            setAllThreadsToStop(thread_id);
            Print(2, "info string Aborting search: end of waiting for stop/quit/ponderhit\n");
        }
    }

    //////Print(8, "================================================================\n");
    //////for (int i = 0; i < Guci_options->threads; ++i) {
    //////    Print(8, "%s: thread_id:%d, num_sp:%d work:%d idle:%d stop:%d started:%d ended:%d nodes:%d numsplits:%d\n", __FUNCTION__, i, 
    //////    Threads[i].num_sp, Threads[i].work_assigned, Threads[i].idle, Threads[i].stop, 
    //////    Threads[i].started, Threads[i].ended, Threads[i].nodes, Threads[i].numsplits);
    //////}
}

void checkSpeedUp(position_t* pos) {
    const int NUMPOS = 4;
    const int NUMVAL = 5;
    char* fenPos[NUMPOS] = {
        "r2qkb1r/pp3p1p/2p2n2/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b kq - 2 12",
        "r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - 2 18",
        "4qRr1/p3b2p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - 0 24",
        "3q4/p3b2p/1k6/2P1Q3/p2P4/8/1P4PP/6K1 b - - 0 30",
    };
    float timeSpeedupSum[NUMVAL];
    float nodesSpeedupSum[NUMVAL];
    char tempStr[256] = {0};
    char* command = "movedepth 21";

    for (int j = 0; j < NUMVAL; ++j) {
        timeSpeedupSum[j] = 0.0;
        nodesSpeedupSum[j] = 0.0;
    }

    for (int i = 0; i < NUMPOS; ++i) {
        uciSetOption("name Threads value 1");
        transClear(0);
        for (int k = 0; k < Guci_options->threads; ++k) {
            pawnTableClear(&SearchInfo(0).pt);
            evalTableClear(&SearchInfo(0).et);
        }
        Print(5, "\n\nPos#%d: %s\n", i+1, fenPos[i]);
        uciSetPosition(pos, fenPos[i]);
        int64 startTime = getTime();
        uciGo(pos, command);
        int64 spentTime1 = getTime() - startTime;
        uint64 nodes1 = Threads[0].nodes / spentTime1;
        float timeSpeedUp = (float)spentTime1/1000.0;
        timeSpeedupSum[0] += timeSpeedUp;
        float nodesSpeedup = (float)nodes1;
        nodesSpeedupSum[0] += nodesSpeedup;
        Print(5, "Base: %0.2fs %dknps\n", timeSpeedUp, nodes1);

        for (int j = 2; j <= 8; j += 2) {
            sprintf(tempStr, "name Threads value %d\n", j);
            uciSetOption(tempStr);
            transClear(0);
            for (int k = 0; k < Guci_options->threads; ++k) {
                pawnTableClear(&SearchInfo(0).pt);
                evalTableClear(&SearchInfo(0).et);
            }
            uciSetPosition(pos, fenPos[i]);
            startTime = getTime();
            uciGo(pos, command);
            int64 spentTime = getTime() - startTime;
            uint64 nodes = 0;
            for (int i = 0; i < Guci_options->threads; ++i) nodes += Threads[i].nodes;
            nodes /= spentTime;
            timeSpeedUp = (float)spentTime1 / (float)spentTime;
            timeSpeedupSum[j/2] += timeSpeedUp;
            nodesSpeedup = (float)nodes / (float)nodes1;
            nodesSpeedupSum[j/2] += nodesSpeedup;
            Print(5, "Thread: %d SpeedUp: %0.2f %0.2f\n", j, timeSpeedUp, nodesSpeedup);
        }
    }
    Print(5, "\n\n");
    Print(5, "\n\nAvg Base: %0.2fs %dknps\n", timeSpeedupSum[0]/NUMPOS, (int)nodesSpeedupSum[0]/NUMPOS);
    for (int j = 2; j <= 8; j += 2) {
        Print(5, "Thread: %d Avg SpeedUp: %0.2f %0.2f\n", j, timeSpeedupSum[j/2]/NUMPOS, nodesSpeedupSum[j/2]/NUMPOS);
    }
    Print(5, "\n\n");
}

void benchSplitDepth(position_t* pos) {
    const int NUMPOS = 4;
    char* fenPos[NUMPOS] = {
        "r2qkb1r/pp3p1p/2p2n2/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b kq - 2 12",
        "r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - 2 18",
        "4qRr1/p3b2p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - 0 24",
        "3q4/p3b2p/1k6/2P1Q3/p2P4/8/1P4PP/6K1 b - - 0 30",
    };
    char command[1024] = {0};
    uint64 timeSum[15];
    uint64 nodesSum[15];
    for (int i = 1; i <= 14; ++i) {
        timeSum[i] = nodesSum[i] = 0;
    }

    uciSetOption("name Threads value 7");
    for (int posIdx = 0; posIdx < NUMPOS; ++posIdx) {
        Print(5, "\n\nPos#%d: %s\n", posIdx+1, fenPos[posIdx]);
        for (int i = 1; i <= 14; ++i) {
            sprintf(command, "name Min Split Depth value %d", i);
            uciSetOption(command);
            transClear(0);
            for (int k = 0; k < Guci_options->threads; ++k) {
                pawnTableClear(&SearchInfo(0).pt);
                evalTableClear(&SearchInfo(0).et);
            }
            uciSetPosition(pos, fenPos[posIdx]);
            int64 startTime = getTime();
            uciGo(pos, "movedepth 20");
            int64 spentTime = getTime() - startTime;
            timeSum[i] += spentTime;
            uint64 nodes = 0;
            for (int k = 0; k < Guci_options->threads; ++k) nodes += Threads[k].nodes;
            nodes /= spentTime;
            nodesSum[i] += nodes;
            Print(5, "Threads: 7: SplitDepth: %d Time: %d Knps: %d\n", i, spentTime, nodes);
        }
    }
    Print(5, "\n\nAverage:\n");
    for (int i = 1; i <= 14; ++i) {
        Print(5, "SplitDepth: %d Time: %d Knps: %d\n", i, timeSum[i]/NUMPOS, nodesSum[i]/NUMPOS);
    }
    Print(5, "\n\n");
}

