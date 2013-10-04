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
#include "attacks.h"
#include "trans.h"
#include "movegen.h"
#include "threads.h"
#include "movepicker.h"
#include "position.h"
#include "utils.h"
#include "bitutils.h"
#include "uci.h"
#include "eval.h"

#ifndef TCEC
search_info_t global_search_info;
search_info_t* SearchInfoMap[MaxNumOfThreads];
#define SearchInfo(thread) (*SearchInfoMap[thread])
#else
search_info_t global_search_info;
#endif

#define MAX_HDEPTH 20
#define NEW_HISTORY (10 + MAX_HDEPTH)

#define EXPLORE_CUTOFF 20
#define EXPLORE_DEPTH_PV 6
#define EXPLORE_DEPTH_NOPV 8
#define EXTEND_ONLY 0 // 3 means quiesc, pv and non-pv, 2 means both pv and non-pv, 1 means only pv (0-3)
#define Q_CHECK 1 // implies 1 check 
#define Q_PVCHECK 2 // implies 2 checks
#define MIN_REDUCTION_DEPTH 4 // default is false

#define WORSE_TIME_BONUS 20 //how many points more than 20 it takes to increase time by alloc to a maximum of 2*alloc
#define CHANGE_TIME_BONUS 50 //what percentage of alloc to increase if the last move is a change move
#define LAST_PLY_TIME 40 //what percentage of alloc remaining to be worth trying another complete ply
#define INCREASE_CHANGE 0 //what percentage of alloc to increase during a change move

const int WORSE_SCORE_CUTOFF = 20;

void ponderHit() { //no pondering in tuning
    int64 time = getTime() - SearchInfo(0).start_time;

    if ((SearchInfo(0).iteration >= 8 && (SearchInfo(0).legalmoves == 1 || SearchInfo(0).mate_found >= 3)) ||
        (time > SearchInfo(0).time_limit_abs)) {
            setAllThreadsToStop(0);
            Print(3, "info string Has searched enough the ponder move: aborting\n");
    } else {
        SearchInfo(0).thinking_status = THINKING;
        Print(2, "info string Switch from pondering to thinking\n");
    }
}

void check4Input(position_t *pos) {
    static char input[256];

    if (biosKey()) {
        if (fgets(input, 256, stdin) == NULL)
            strcpy_s(input, "quit");

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
            needReplyReady = true;
        }
    }
}

void initNode(position_t *pos, const int thread_id) {
    int64 time2;

    if (smpCutoffOccurred(Threads[thread_id].split_point)) {
        Threads[thread_id].stop = true;
        return;
    }

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
                for (int i = 0; i < Guci_options.threads; ++i) sum_nodes += Threads[i].nodes;
                Print(1, "nodes %llu ", sum_nodes);
                Print(1, "hashfull %d ", (TransTable.Used()*1000)/TransTable.Size());
                Print(1, "nps %llu ", (sum_nodes*1000ULL)/(time));
                Print(1, "\n");
            }
        }
        if (SearchInfo(thread_id).thinking_status == THINKING && SearchInfo(thread_id).time_is_limited) {
            if (time2 > SearchInfo(thread_id).time_limit_max) {
                if (time2 < SearchInfo(thread_id).time_limit_abs) {
                    if (!SearchInfo(thread_id).research && !SearchInfo(thread_id).change) {
                        bool gettingWorse = SearchInfo(thread_id).best_value != -INF && SearchInfo(thread_id).best_value + WORSE_SCORE_CUTOFF <= SearchInfo(thread_id).last_value;
                        if (!gettingWorse) { 
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
    if (MinTwoBits(pos->color[pos->side] & ~pos->pawns)) return false; 
    kpos = pos->kpos[pos->side];
    if (kpos != a1 && kpos != a8 && kpos != h1 && kpos != h8) return false;
    mv_bits = KingMoves[kpos];
    while (mv_bits) {
        to = popFirstBit(&mv_bits);
        if (!isSqAtt(pos,pos->occupied,to,pos->side^1)) return false;
    }
    return true;
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

    for (int i = 0; i < Guci_options.threads; ++i) sum_nodes += Threads[i].nodes;
    Print(1, "nodes %llu ", sum_nodes);
    Print(1, "hashfull %d ", (TransTable.Used()*1000)/TransTable.Size());
    if (time > 10) Print(1, "nps %llu ", (sum_nodes*1000)/(time));
    Print(1, "pv ");
    for (int i = 0; i < pv->length; i++) printf("%s ", move2Str(pv->moves[i]));
    Print(1, "\n");
}

bool prevMoveAllowsThreat(const position_t* pos, basic_move_t first, basic_move_t second) {
    int m1from = moveFrom(first);
    int m2from = moveFrom(second);
    int m1to = moveTo(first);
    int m2to = moveTo(second);

    if (m1to == m2from || m2to == m1from) return true;
    if (InBetween[m2from][m2to] & BitMask[m1from]) return true;
    uint64 m1att = pieceAttacksFromBB(pos, movePiece(first), m1to, pos->occupied ^ BitMask[m2from]);
    if (m1att & BitMask[m2to]) return true;
    if (m1att & BitMask[pos->kpos[pos->side]]) return true;
    return false;
}

bool moveRefutesThreat(const position_t* pos, basic_move_t first, basic_move_t second) {
    int m1from = moveFrom(first);
    int m2from = moveFrom(second);
    int m1to = moveTo(first);
    int m2to = moveTo(second);

    if (m1from == m2to) return true;
    if (moveCapture(second) && (PcValSEE[movePiece(second)] >= PcValSEE[moveCapture(second)] || movePiece(second) == KING)) {
        uint64 occ = pos->occupied ^ BitMask[m1from] ^ BitMask[m1to] ^ BitMask[m2from];
        if (pieceAttacksFromBB(pos, movePiece(first), m1to, occ) & BitMask[m2to]) return true;
        uint64 xray = rookAttacksBBX(m2to, occ) & (pos->queens | pos->rooks) & pos->color[pos->side];
        xray |= bishopAttacksBBX(m2to, occ) & (pos->queens | pos->bishops) & pos->color[pos->side];
        if (xray && (xray & ~queenAttacksBB(m2to, pos->occupied))) return true;
    }
    if (InBetween[m2from][m2to] & BitMask[m1to] && swap(pos, first) >= 0) return true;
    return false;
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

int moveIsTactical(uint32 m) {
    ASSERT(moveIsOk(m));
    return (m & 0x01fe0000UL);
}

int historyIndex(uint32 side, uint32 move) {
    return ((((side) << 9) + ((movePiece(move)) << 6) + (moveTo(move))) & 0x3ff);
}

const int MaxPieceValue[] = {0, PawnValueEnd, KnightValueEnd, BishopValueEnd, RookValueEnd, QueenValueEnd, 10000};

template<bool inPv>
int qSearch(position_t *pos, int alpha, int beta, const int depth, SearchStack& ssprev, const int thread_id) {
    int pes = 0;
    SearchStack ss;

    ASSERT(pos != NULL);
    ASSERT(valueIsOk(alpha));
    ASSERT(valueIsOk(beta));
    ASSERT(alpha < beta);
    ASSERT(pos->ply > 0);

    initNode(pos, thread_id);
    if (Threads[thread_id].stop) return 0;

    int t = 0;
    for (TransEntry* entry = TransTable.Entry(pos->hash); t < HASH_ASSOC; t++, entry++) {
        if (entry->HashLock() == LOCK(pos->hash)) {
            entry->SetAge(TransTable.Date());
            if (!inPv) { // TODO: re-use values from here to evalvalue?
                if (entry->Move() != EMPTY && entry->LowerDepth() > 0) {
                    int score = scoreFromTrans(entry->LowerValue(), pos->ply);
                    if (score > alpha) {
                        ssprev.counterMove= entry->Move();
                        return score;
                    }
                }
                if (entry->UpperDepth() > 0) {
                    int score = scoreFromTrans(entry->UpperValue(), pos->ply);
                    if (score < beta) return score;
                }
            }
            if (entry->Move() != EMPTY && entry->LowerDepth() > ss.hashDepth ) {
                ss.hashDepth = entry->LowerDepth();
                ss.hashMove = entry->Move();
            }
        }
    }
    if (pos->ply >= MAXPLY-1) return eval(pos, thread_id, &pes);
    if (!ssprev.moveGivesCheck) {
        if (simpleStalemate(pos)) {
            return DrawValue[pos->side];
        }
        ss.evalvalue = ss.bestvalue = eval(pos, thread_id, &pes);
        updateEvalgains(pos, pos->posStore.lastmove, ssprev.evalvalue, ss.evalvalue, thread_id);
        if (ss.bestvalue > alpha) {
            if (ss.bestvalue >= beta) {
                ASSERT(valueIsOk(ss.bestvalue));
                return ss.bestvalue;
            }
            alpha = ss.bestvalue;
        }
    }

    ss.dcc = discoveredCheckCandidates(pos, pos->side);
    if (ssprev.moveGivesCheck) {
        sortInit(pos, ss.mvlist, pinnedPieces(pos, pos->side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseEvasion, thread_id);
    } else {
        if (inPv) sortInit(pos, ss.mvlist, pinnedPieces(pos, pos->side), ss.hashMove, alpha, ss.evalvalue, depth, (depth > -Q_PVCHECK) ? MoveGenPhaseQuiescenceAndChecksPV : MoveGenPhaseQuiescencePV, thread_id);
        else sortInit(pos, ss.mvlist, pinnedPieces(pos, pos->side), ss.hashMove, alpha, ss.evalvalue, depth, (depth > -Q_CHECK) ? MoveGenPhaseQuiescenceAndChecks : MoveGenPhaseQuiescence, thread_id);
    }
    bool prunable = !ssprev.moveGivesCheck && !inPv && MinTwoBits(pos->color[pos->side^1] & pos->pawns) && MinTwoBits(pos->color[pos->side^1] & ~(pos->pawns | pos->kings));
    move_t* move;
    while ((move = sortNext(NULL, pos, ss.mvlist, ss.mvlist_phase, thread_id)) != NULL) {
        int score;
        if (anyRepNoMove(pos, move->m)) { 
            score = DrawValue[pos->side];
        } else {
            pos_store_t undo;
            ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
            if (prunable && move->m != ss.hashMove && !ss.moveGivesCheck && !moveIsPassedPawn(pos, move->m)) {
                int scoreAprox = ss.evalvalue + PawnValueEnd + MaxPieceValue[moveCapture(move->m)]; 
                if (scoreAprox < beta) continue;
            }
            int newdepth = depth - !ss.moveGivesCheck;
            makeMove(pos, &undo, move->m);
            score = -qSearch<inPv>(pos, -beta, -alpha, newdepth, ss, thread_id);
            unmakeMove(pos, &undo);
        }
        if (Threads[thread_id].stop) return ss.bestvalue;
        if (score > ss.bestvalue) {
            ss.bestvalue = score;
            if (score > alpha) {
                ss.bestmove = move->m;
                if (score >= beta) {
                    TransTable.StoreLower(pos->hash, ss.bestmove, 1, scoreToTrans(ss.bestvalue, pos->ply));
                    ASSERT(valueIsOk(ss.bestvalue));
                    ssprev.counterMove = ss.bestmove;
                    return ss.bestvalue;
                }
                alpha = score;
            }
        }
    }
    if (ssprev.moveGivesCheck && ss.bestvalue == -INF) {
        ss.bestvalue = (-INF + pos->ply); 
        TransTable.StoreNoMoves(pos->hash, EMPTY, 1, scoreToTrans(ss.bestvalue, pos->ply));
        return ss.bestvalue;
    }

    ASSERT(bestvalue != -INF);

    if (inPv && ss.bestmove != EMPTY) {
        ssprev.counterMove = ss.bestmove;
        TransTable.StoreExact(pos->hash, ss.bestmove, 1, scoreToTrans(ss.bestvalue, pos->ply));
    } else TransTable.StoreUpper(pos->hash, EMPTY, 1, scoreToTrans(ss.bestvalue, pos->ply));

    ASSERT(valueIsOk(ss.bestvalue));

    return ss.bestvalue;
}

inline bool inPvNode(NodeType nt) { return (nt==PVNode);}
inline bool inCutNode(NodeType nt) { return (nt==CutNode);}
inline bool inAllNode(NodeType nt) { return (nt==AllNode);}
inline NodeType invertNode(NodeType nt) { return ((nt==PVNode) ? PVNode : ((nt==CutNode) ? AllNode : CutNode));}

template <bool inRoot, bool inSplitPoint, bool inSingular>
int searchNode(position_t *pos, int alpha, int beta, const int depth, SearchStack& ssprev, const int thread_id, NodeType nt) {
    if (depth <= 0) {
        if (inPvNode(nt)) return qSearch<true>(pos, alpha, beta, 0, ssprev, thread_id);
        else return qSearch<false>(pos, alpha, beta, 0, ssprev, thread_id);
    } else {
        if (ssprev.moveGivesCheck) return searchGeneric<inRoot, inSplitPoint, true, inSingular>(pos, alpha, beta, depth, ssprev, thread_id, nt);
        else return searchGeneric<inRoot, inSplitPoint, false, inSingular>(pos, alpha, beta, depth, ssprev, thread_id, nt);
    }
}

void searchFromIdleLoop(SplitPoint* sp, const int thread_id) {
    if (sp->inRoot) searchNode<true, true, false>(&sp->pos[thread_id], sp->alpha, sp->beta, sp->depth, *sp->ssprev, thread_id, sp->nodeType);
    else searchNode<false, true, false>(&sp->pos[thread_id], sp->alpha, sp->beta, sp->depth, *sp->ssprev, thread_id, sp->nodeType);
}

template<bool inRoot, bool inSplitPoint, bool inCheck, bool inSingular>
int searchGeneric(position_t *pos, int alpha, int beta, const int depth, SearchStack& ssprev, const int thread_id, NodeType nt) {
    int pes = 0;
    SplitPoint* sp = NULL;
    SearchStack ss;

    ASSERT(pos != NULL);
    ASSERT(valueIsOk(alpha)); 
    ASSERT(valueIsOk(beta)); 
    ASSERT(alpha < beta);
    ASSERT(depth >= 1);

    initNode(pos, thread_id);
    if (Threads[thread_id].stop) return 0;

    if (!inRoot && !inSingular && !inSplitPoint) {
        int t = 0;
        int evalDepth = 0;

        alpha = MAX(-INF + pos->ply, alpha);
        beta = MIN(INF - pos->ply - 1, beta);
        if (alpha >= beta) return alpha;

        for (TransEntry * entry = TransTable.Entry(pos->hash); t < HASH_ASSOC; t++, entry++) {
            if (entry->HashLock() == LOCK(pos->hash)) {
                entry->SetAge(TransTable.Date());
                if (entry->Mask() & MNoMoves) {
                    if (inCheck) return -INF + pos->ply;
                    else return DrawValue[pos->side];
                }
                if (!inPvNode(nt)) {
                    if ((!inCutNode(nt) || !(entry->Mask() & MAllLower)) && entry->LowerDepth() >= depth && (entry->Move() != EMPTY || pos->posStore.lastmove == EMPTY)) {
                        int score = scoreFromTrans(entry->LowerValue(), pos->ply);
                        ASSERT(valueIsOk(score));
                        if (score > alpha) {
                            ssprev.counterMove = entry->Move();
                            return score;
                        }
                    }
                    if ((!inAllNode(nt) || !(entry->Mask() & MCutUpper)) && entry->UpperDepth() >= depth) {
                        int score = scoreFromTrans(entry->UpperValue(), pos->ply);
                        ASSERT(valueIsOk(score));
                        if (score < beta) return score;
                    }
                }
                if (entry->Move() != EMPTY && entry->LowerDepth() > ss.hashDepth) {
                    ss.hashMove = entry->Move();
                    ss.hashDepth = entry->LowerDepth();
                }
                if (entry->LowerDepth() > evalDepth) {
                    evalDepth = entry->LowerDepth();
                    ss.evalvalue = scoreFromTrans(entry->LowerValue(), pos->ply);
                }
                if (entry->UpperDepth() > evalDepth) {
                    evalDepth = entry->UpperDepth();
                    ss.evalvalue = scoreFromTrans(entry->UpperValue(), pos->ply);
                }
            }
        }
        if (ss.evalvalue == -INF) ss.evalvalue = eval(pos, thread_id, &pes);

        if (pos->ply >= MAXPLY-1) return ss.evalvalue;
        updateEvalgains(pos, pos->posStore.lastmove, ssprev.evalvalue, ss.evalvalue, thread_id);

        if (!inPvNode(nt) && !inCheck) {
            const int MaxRazorDepth = 10;
            int rvalue;
            if (depth < MaxRazorDepth && (pos->color[pos->side] & ~(pos->pawns | pos->kings)) 
                && ss.evalvalue > (rvalue = beta + FutilityMarginTable[MIN(depth, MaxRazorDepth)][MIN(ssprev.playedMoves,63)])) {
                    return rvalue; 
            }
            if (depth < MaxRazorDepth && pos->posStore.lastmove != EMPTY 
                && ss.evalvalue < (rvalue = beta - FutilityMarginTable[MIN(depth, MaxRazorDepth)][MIN(ssprev.playedMoves,63)])) { 
                    int score = searchNode<false, false, false>(pos, rvalue-1, rvalue, 0, ssprev, thread_id, nt);
                    if (score < rvalue) return score; 
            }
            if (depth >= 2 && (pos->color[pos->side] & ~(pos->pawns | pos->kings)) && ss.evalvalue >= beta) {
                pos_store_t undo;
                int nullDepth = depth - (4 + depth/5 + (ss.evalvalue - beta > PawnValue));
                makeNullMove(pos, &undo);
                int score = -searchNode<false, false, false>(pos, -beta, -alpha, nullDepth, ss, thread_id, AllNode);
                ss.threatMove = ss.counterMove;
                unmakeNullMove(pos, &undo);
                if (Threads[thread_id].stop) return 0;
                if (score >= beta) {
                    if (depth >= 12) {
                        score = searchNode<false, false, false>(pos, alpha, beta, nullDepth, ssprev, thread_id, nt);
                        if (Threads[thread_id].stop) return 0;
                    }
                    if (score >= beta) {
                        if (ss.hashMove == EMPTY) {
                            if (inCutNode(nt)) TransTable.StoreLower(pos->hash, EMPTY, depth, scoreToTrans(score, pos->ply));
                            else TransTable.StoreAllLower(pos->hash, EMPTY, depth, scoreToTrans(score, pos->ply));
                        }
                        return score;
                    }
                } else if (depth < 5 && ssprev.reducedMove && ss.threatMove != EMPTY && prevMoveAllowsThreat(pos, pos->posStore.lastmove, ss.threatMove)) {
                    return alpha;
                }
            }
        }

        if (!inAllNode(nt) && !inCheck && depth >= (inPvNode(nt)?6:8)) { // IID
            int newdepth = inPvNode(nt) ? depth - 2 : depth / 2;
            if (ss.hashMove == EMPTY || ss.hashDepth < newdepth) {
                int score = searchNode<false, false, false>(pos, alpha, beta, newdepth, ssprev, thread_id, nt);
                if (Threads[thread_id].stop) return 0;
                ss.evalvalue = score;
                if (score > alpha) {
                    ss.hashMove = ssprev.counterMove;
                    ss.evalvalue = score;
                    ss.hashDepth = newdepth;
                }
            }
        }
        if (!inAllNode(nt) && !inCheck && depth >= (inPvNode(nt)?6:8)) { // singular extension
            int newdepth = depth / 2;
            if (ss.hashMove != EMPTY && ss.hashDepth >= newdepth) { 
                int targetScore = ss.evalvalue - EXPLORE_CUTOFF;
                ssprev.bannedMove = ss.hashMove;
                int score = searchNode<false, false, true>(pos, targetScore, targetScore+1, newdepth, ssprev, thread_id, nt);
                if (Threads[thread_id].stop) return 0;
                ssprev.bannedMove = EMPTY;
                if (score <= targetScore) ss.firstExtend = true;
            }
        }
    }

    if (inSplitPoint) {
        sp = Threads[thread_id].split_point;
        ss = *sp->sscurr; // ss.mvlist points to master ss.mvlist, copied the entire searchstack also
    } else {
        if (inRoot) {
            ss = ssprev; // this is correct, ss.mvlist points to the ssprev.mvlist, at the same time, ssprev resets other member vars
            if (!SearchInfo(thread_id).mvlist_initialized) {
                sortInit(pos, ss.mvlist, pinnedPieces(pos, pos->side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseRoot, thread_id);
            } else {
                ss.mvlist->pos = 0;
                ss.mvlist->phase = MoveGenPhaseRoot + 1;
            }
        } else {
            ss.dcc = discoveredCheckCandidates(pos, pos->side);
            sortInit(pos, ss.mvlist, pinnedPieces(pos, pos->side), ss.hashMove, alpha, ss.evalvalue, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), thread_id);
            ss.firstExtend = ss.firstExtend || (inCheck && ss.mvlist->size==1);
        }
    }

    int lateMove = LATE_PRUNE_MIN + (inCutNode(nt) ? ((depth * depth) / 4) : (depth * depth));
    move_t* move;
    while ((move = sortNext(sp, pos, ss.mvlist, ss.mvlist_phase, thread_id)) != NULL) {
        int score = -INF;
        if (inSplitPoint) {
            sp->updatelock->lock();
            ss.playedMoves = ++sp->played;
            sp->updatelock->unlock();
        } else ++ss.playedMoves;
        if (inSingular && move->m == ssprev.bannedMove) continue;
        if (!inSplitPoint && ss.hisCnt < 64 && !moveIsTactical(move->m)) {
            ss.hisMoves[ss.hisCnt++] = move->m;
        }
        if (anyRepNoMove(pos, move->m)) { 
            score = DrawValue[pos->side];
        } else {
            int newdepth;
            pos_store_t undo;
            ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
            if (ss.bestvalue == -INF) {
                newdepth = depth - !ss.firstExtend;
                makeMove(pos, &undo, move->m);
                score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, ss, thread_id, invertNode(nt));
            } else {
                newdepth = depth -1;
                //only reduce or prune some types of moves
                int partialReduction = 0;
                int fullReduction = 0;
                if (MoveGenPhase[ss.mvlist_phase] == PH_QUIET_MOVES && !ss.moveGivesCheck) { //never happens when in check
                    bool goodMove = (ss.threatMove && moveRefutesThreat(pos, move->m, ss.threatMove)) || moveIsPassedPawn(pos, move->m); // add countermove here
                    if (!inRoot && !inPvNode(nt)) {
                        if (ss.playedMoves > lateMove && !goodMove) continue;
                        int predictedDepth = MAX(0,newdepth - ReductionTable[1][MIN(depth,63)][MIN(ss.playedMoves,63)]);
                        int scoreAprox = ss.evalvalue + FutilityMarginTable[MIN(predictedDepth,MAX_FUT_MARGIN)][MIN(ss.playedMoves,63)]
                        + SearchInfo(thread_id).evalgains[historyIndex(pos->side, move->m)];

                        if (scoreAprox < beta) {
                            if (predictedDepth < 8 && !goodMove) continue;
                            fullReduction++;
                        }
                        if (swap(pos, move->m) < 0) {
                            if (predictedDepth < 2) continue;
                            fullReduction++; 
                        }
                    }
                    if (depth >= MIN_REDUCTION_DEPTH) { 
                        int reduction = ReductionTable[(inPvNode(nt)?0:1)][MIN(depth,63)][MIN(ss.playedMoves,63)];
                        partialReduction += goodMove ? (reduction+1)/2 : reduction; 
                    }
                }
                newdepth -= fullReduction;
                int newdepthclone = newdepth - partialReduction;
                makeMove(pos, &undo, move->m);
                if (inSplitPoint) alpha = sp->alpha;
                ss.reducedMove = (newdepthclone < newdepth);

                score = -searchNode<false, false, false>(pos, -alpha-1, -alpha, newdepthclone, ss, thread_id, CutNode);
                if (ss.reducedMove && score > alpha) {
                    ss.reducedMove = false;
                    score = -searchNode<false, false, false>(pos, -alpha-1, -alpha, newdepth, ss, thread_id, AllNode);
                }
                if (inPvNode(nt) && score > alpha) {
                    if (inRoot) SearchInfo(thread_id).research = 1;
                    score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, ss, thread_id, PVNode);
                }
            }
            unmakeMove(pos, &undo);
        }
        if (Threads[thread_id].stop) return 0;
        if (inSplitPoint) sp->updatelock->lock();
        if (inRoot) {
            move->s = score;
            if (score > SearchInfo(thread_id).rbestscore1) {
                SearchInfo(thread_id).rbestscore2 = SearchInfo(thread_id).rbestscore1;
                SearchInfo(thread_id).rbestscore1 = score;
            } else if (score > SearchInfo(thread_id).rbestscore2) {
                SearchInfo(thread_id).rbestscore2 = score;
            }
        }
        if (score > (inSplitPoint ? sp->bestvalue : ss.bestvalue)) {
            ss.bestvalue = inSplitPoint ? sp->bestvalue = score : score;
            if (inRoot) {
                SearchInfo(thread_id).best_value = ss.bestvalue;
                extractPvMovesFromHash(pos, &SearchInfo(thread_id).rootPV, move->m, true);
                if (SearchInfo(thread_id).iteration > 1 && SearchInfo(thread_id).bestmove != move->m) SearchInfo(thread_id).change = 1;
            }
            if (ss.bestvalue > (inSplitPoint ? sp->alpha : alpha)) {
                ss.bestmove = inSplitPoint ? sp->bestmove = move->m : move->m;
                if (inRoot) {
                    SearchInfo(thread_id).bestmove = ss.bestmove;
                    if (SearchInfo(thread_id).rootPV.length > 1) SearchInfo(thread_id).pondermove = SearchInfo(thread_id).rootPV.moves[1];
                    else SearchInfo(thread_id).pondermove = 0;
                }
                if (inPvNode(nt) && ss.bestvalue < beta) {
                    alpha = inSplitPoint ? sp->alpha = ss.bestvalue : ss.bestvalue;
                } else {
                    if (inRoot) {
                        if (inSplitPoint) sp->movelistlock->lock();
                        for (int i = ss.mvlist->pos; i < ss.mvlist->size; ++i) ss.mvlist->list[i].s = -INF;
                        if (inSplitPoint) sp->movelistlock->unlock();
                    }
                    if (inSplitPoint) {
                        sp->cutoff = true;
                        sp->updatelock->unlock();
                    }
                    break;
                }
            }
        }
        if (inSplitPoint) sp->updatelock->unlock();
        if (!inSplitPoint && !inSingular && !Threads[thread_id].stop && !inCheck && Threads[thread_id].num_sp < Guci_options.max_activesplits_per_thread
            && Guci_options.threads > 1 && depth >= Guci_options.min_split_depth
            //&& (!inCutNode(nt) || MoveGenPhase[ss.mvlist_phase] == PH_QUIET_MOVES)
            && splitRemainingMoves(pos, ss.mvlist, &ss, &ssprev, alpha, beta, nt, depth, inCheck, inRoot, thread_id)) {
                break;
        }
    }
    if (inRoot && !inSplitPoint && ss.playedMoves == 0) {
        if (inCheck) return -INF;
        else return DrawValue[pos->side];
    }
    if (!inRoot && !inSplitPoint && !inSingular) {
        if (ss.playedMoves == 0) {
            if (inCheck) ss.bestvalue = -INF + pos->ply;
            else ss.bestvalue = DrawValue[pos->side];
            TransTable.StoreNoMoves(pos->hash, EMPTY, depth, scoreToTrans(ss.bestvalue, pos->ply));
            return ss.bestvalue;
        }
        if (ss.bestmove != EMPTY && !moveIsTactical(ss.bestmove) && ss.bestvalue >= beta) { //> alpha account for pv better maybe? Sam
            int index = historyIndex(pos->side, ss.bestmove);
            int hisDepth = MIN(depth,MAX_HDEPTH);
            SearchInfo(thread_id).history[index] += hisDepth * hisDepth;
            for (int i=0; i < ss.hisCnt; ++i) {
                if (ss.hisMoves[i] == ss.bestmove) continue;
                index = historyIndex(pos->side, ss.hisMoves[i]);
                SearchInfo(thread_id).history[index] -= SearchInfo(thread_id).history[index]/(NEW_HISTORY-hisDepth);
            }
            if (Threads[thread_id].ts[pos->ply].killer1 != ss.bestmove) {
                Threads[thread_id].ts[pos->ply].killer2 = Threads[thread_id].ts[pos->ply].killer1;
                Threads[thread_id].ts[pos->ply].killer1 = ss.bestmove;
            }
        }
        // DEBUG
        if (inCutNode(nt)) {
            SearchInfo(thread_id).cutnodes++;
            if (ss.bestvalue < beta) SearchInfo(thread_id).cutfail++;
        }
        if (inAllNode(nt)) {
            SearchInfo(thread_id).allnodes++;
            if (ss.bestvalue >= beta) SearchInfo(thread_id).allfail++;
        }

        if (ss.bestvalue >= beta) {
            ASSERT(valueIsOk(ss.bestvalue));
            ssprev.counterMove = ss.bestmove;
            if (inCutNode(nt)) TransTable.StoreLower(pos->hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos->ply));
            else TransTable.StoreAllLower(pos->hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos->ply));
        } else {
            ASSERT(valueIsOk(bestvalue));
            if (inPvNode(nt) && ss.bestmove != EMPTY) {
                ssprev.counterMove = ss.bestmove;
                TransTable.StoreExact(pos->hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos->ply));
            }
            else if (inCutNode(nt)) TransTable.StoreCutUpper(pos->hash, EMPTY, depth, scoreToTrans(ss.bestvalue, pos->ply));
            else TransTable.StoreUpper(pos->hash, EMPTY, depth, scoreToTrans(ss.bestvalue, pos->ply));
        }
    }
    ASSERT(valueIsOk(ss.bestvalue));
    return ss.bestvalue;
}

void extractPvMovesFromHash(position_t *pos, continuation_t* pv, basic_move_t move, bool execMove) {
    PvHashEntry *entry;
    pos_store_t undo[MAXPLY];
    int ply = 0;
    basic_move_t hashMove;
    pv->length = 0;
    pv->moves[pv->length++] = move;
    if (execMove) makeMove(pos, &(undo[ply++]), move);
    while ((entry = PVHashTable.pvEntry(pos->hash)) != NULL) {
        hashMove = entry->pvMove();
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

void repopulateHash(position_t *pos, continuation_t *rootPV) {
    int moveOn;
    pos_store_t undo[MAXPLY];
    for (moveOn=0; moveOn+1 <= rootPV->length; moveOn++) {
        basic_move_t move = rootPV->moves[moveOn];
        if (!move) break;
        PvHashEntry* entry = PVHashTable.pvEntryFromMove(pos->hash, move);
        if (NULL == entry) break;
        TransTable.StoreExact(pos->hash, entry->pvMove(), entry->pvDepth(), entry->pvScore());
        makeMove(pos, &(undo[moveOn]), move);
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
            bool gettingWorse = (SearchInfo(thread_id).best_value + WORSE_SCORE_CUTOFF) <= SearchInfo(thread_id).last_value;

            if (SearchInfo(thread_id).legalmoves == 1 || SearchInfo(thread_id).mate_found >= 3) { 
                if (depth >= 8) {
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: legalmove/mate found depth >= 8\n");
                    return;
                }
            } 
            if ((time - SearchInfo(thread_id).start_time) > ((SearchInfo(thread_id).time_limit_max - SearchInfo(thread_id).start_time) * 60) / 100) {
                int64 addTime = 0;
                if (gettingWorse) {
                    int amountWorse = SearchInfo(thread_id).last_value - SearchInfo(thread_id).best_value;
                    addTime += ((amountWorse - (WORSE_SCORE_CUTOFF - 10)) * SearchInfo(thread_id).alloc_time)/WORSE_TIME_BONUS;
                    Print(2, "info string Extending time (root gettingWorse): %d\n", addTime);
                }
                if (SearchInfo(thread_id).change) {
                    addTime += (SearchInfo(thread_id).alloc_time * CHANGE_TIME_BONUS) / 100;
                }
                if (addTime > 0) {
                    SearchInfo(thread_id).time_limit_max += addTime;
                    if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) 
                        SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
                } else { // if we are unlikely to get deeper, save our time
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: root time limit 1: %d\n", time - SearchInfo(thread_id).start_time);
                    return;
                }
            }
            if (!gettingWorse && depth >= 8) {
                const int EasyTime1 = 20;
                const int EasyCutoff1 = 120;
                const int EasyTime2 = 30;
                const int EasyCutoff2 = 60;

                int64 timeLimit = SearchInfo(thread_id).time_limit_max - SearchInfo(thread_id).start_time;
                int64 timeExpended = time - SearchInfo(thread_id).start_time;

                if (timeExpended > (timeLimit * EasyTime1)/100 && SearchInfo(thread_id).rbestscore1 > SearchInfo(thread_id).rbestscore2 + EasyCutoff1) {
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: easy move1: score1: %d score2: %d time: %d\n", 
                        SearchInfo(thread_id).rbestscore1, SearchInfo(thread_id).rbestscore2, time - SearchInfo(thread_id).start_time);
                    return;
                }
                if (timeExpended > (timeLimit * EasyTime2)/100 && SearchInfo(thread_id).rbestscore1 > SearchInfo(thread_id).rbestscore2 + EasyCutoff2) {
                    setAllThreadsToStop(thread_id);
                    Print(2, "info string Aborting search: easy move2: score1: %d score2: %d time: %d\n", 
                        SearchInfo(thread_id).rbestscore1, SearchInfo(thread_id).rbestscore2, time - SearchInfo(thread_id).start_time);
                    return;
                }
            }
        }
    } 
    if (SearchInfo(thread_id).depth_is_limited && depth >= SearchInfo(thread_id).depth_limit) {
        setAllThreadsToStop(thread_id);
        Print(2, "info string Aborting search: depth limit 1\n");
        return;
    }
}


void getBestMove(position_t *pos, int thread_id) {
    int id;
    int alpha, beta;
    SearchStack ss;
    ss.moveGivesCheck = kingIsInCheck(pos);
    ss.dcc = discoveredCheckCandidates(pos, pos->side);

    ASSERT(pos != NULL);

    TransTable.NewDate(TransTable.Date());

    do {
        PvHashEntry *entry = PVHashTable.pvEntry(pos->hash);
        if (NULL == entry) break;
        if (SearchInfo(thread_id).thinking_status == THINKING
            && entry->pvDepth() >= SearchInfo(thread_id).lastDepthSearched - 2 
            && SearchInfo(thread_id).rootPV.moves[1] == pos->posStore.lastmove
            && SearchInfo(thread_id).rootPV.moves[2] == entry->pvMove()
            && ((moveCapture(pos->posStore.lastmove) && (moveTo(pos->posStore.lastmove) == moveTo(entry->pvMove()))) 
            || (PcValSEE[moveCapture(entry->pvMove())] > PcValSEE[movePiece(entry->pvMove())]))) {
                SearchInfo(thread_id).bestmove =  entry->pvMove();
                SearchInfo(thread_id).best_value = entry->pvScore();
                extractPvMovesFromHash(pos, &SearchInfo(thread_id).rootPV, entry->pvMove(), true);
                if (SearchInfo(thread_id).rootPV.length > 1) SearchInfo(thread_id).pondermove = SearchInfo(thread_id).rootPV.moves[1];
                else SearchInfo(thread_id).pondermove = 0;
                displayPV(pos, &SearchInfo(thread_id).rootPV, entry->pvDepth(), -INF, INF, SearchInfo(thread_id).best_value);
                setAllThreadsToStop(thread_id);
                return;
        }
        ss.hashMove = entry->pvMove();
    } while (false);

    // extend time when there is no hashmove from hashtable, this is useful when just out of the book
    if (ss.hashMove == EMPTY) { 
        SearchInfo(thread_id).time_limit_max += SearchInfo(thread_id).alloc_time / 2; //TODO optimize this, seems too large for Sam.  Perhaps something about if not move pondered instead?
        if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs)
            SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
    }

#ifndef TCEC
    if (SearchInfo(thread_id).thinking_status == THINKING && opt->try_book && pos->sp <= opt->book_limit && !anyRep(pos) && SearchInfo(thread_id).outOfBook < 8) {
        if (DEBUG_BOOK) Print(3,"info string num moves %d\n",mvlist.size);
        book_t *book = opt->usehannibalbook ? &GhannibalBook : &GpolyglotBook;
        if ((SearchInfo(thread_id).bestmove = getBookMove(pos,book,&mvlist,true,Guci_options.bookExplore*5)) != 0) {
            SearchInfo(thread_id).outOfBook = 0;
            return;
        }
        if (opt->usehannibalbook /*&& Guci_options.try_book*/ && SearchInfo(thread_id).outOfBook < MAXLEARN_OUT_OF_BOOK && movesSoFar.length > 0) 
            add_to_learn_begin(&Glearn,&movesSoFar);
    }
    if (SearchInfo(thread_id).thinking_status != PONDERING) SearchInfo(thread_id).outOfBook++;
#endif

    // SMP 
    initSmpVars();
    for (int i = 1; i < Guci_options.threads; ++i) {
        std::unique_lock<std::mutex>(Threads[thread_id].threadLock);
        Threads[i].idle_event.notify_one();
    }
    Threads[thread_id].split_point = NULL;
    SearchInfo(thread_id).mvlist_initialized = false;

    for (id = 1; id < MAXPLY; id++) {
        const int AspirationWindow = 24;
        int faillow = 0, failhigh = 0;
        SearchInfo(thread_id).iteration = id;
        SearchInfo(thread_id).best_value = -INF;
        SearchInfo(thread_id).change = 0;
        SearchInfo(thread_id).research = 0;
        if (id < 6) {
            alpha = -INF;
            beta = INF;
        } else {
            alpha = SearchInfo(thread_id).last_value-AspirationWindow;
            beta = SearchInfo(thread_id).last_value+AspirationWindow;
            if (alpha < -RookValue) alpha = -INF;
            if (beta > RookValue) beta = INF;
        }
        while (true) {
            SearchInfo(thread_id).rbestscore1 = -INF;
            SearchInfo(thread_id).rbestscore2 = -INF;

            searchNode<true, false, false>(pos, alpha, beta, id, ss, thread_id, PVNode);

            if (!Threads[thread_id].stop || SearchInfo(thread_id).best_value != -INF) {
                if (SHOW_SEARCH && id >= 8)
                    displayPV(pos, &SearchInfo(thread_id).rootPV, id, alpha, beta, SearchInfo(thread_id).best_value);
                if (SearchInfo(thread_id).best_value > alpha && SearchInfo(thread_id).best_value < beta)
                    repopulateHash(pos, &SearchInfo(thread_id).rootPV);
            }
            if (SearchInfo(thread_id).thinking_status == STOPPED) break;
            if (SearchInfo(thread_id).best_value <= alpha) {
                if (SearchInfo(thread_id).best_value <= alpha) alpha = SearchInfo(thread_id).best_value-(AspirationWindow<<++faillow);
                if (alpha < -RookValue) alpha = -INF;
                SearchInfo(thread_id).research = 1;
            } else if(SearchInfo(thread_id).best_value >= beta) {
                if (SearchInfo(thread_id).best_value >= beta)  beta = SearchInfo(thread_id).best_value+(AspirationWindow<<++failhigh);
                if (beta > RookValue) beta = INF;
                SearchInfo(thread_id).research = 1;
            } else break;
        }
        if (SearchInfo(thread_id).thinking_status == STOPPED) break;
        timeManagement(id, thread_id);
        if (SearchInfo(thread_id).thinking_status == STOPPED) break;
        if (SearchInfo(thread_id).best_value != -INF) {
            SearchInfo(thread_id).last_last_value = SearchInfo(thread_id).last_value;
            SearchInfo(thread_id).last_value = SearchInfo(thread_id).best_value;
        }
        //////Print(1, "info string cutfails: %d %% cutnodes: %d  allfails: %d %% allnodes: %d\n", 
        //////    (SearchInfo(0).cutfail * 100)/SearchInfo(0).cutnodes, SearchInfo(0).cutnodes, (SearchInfo(0).allfail * 100)/SearchInfo(0).allnodes, SearchInfo(0).allnodes);
    }
    SearchInfo(thread_id).lastDepthSearched = id;
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

    Print(2, "================================================================\n");
    for (int i = 0; i < Guci_options.threads; ++i) {
        Print(2, "%s: thread_id:%d, num_sp:%d searching:%d stop:%d started:%d ended:%d nodes:%d numsplits:%d\n", __FUNCTION__, i, 
            Threads[i].num_sp, Threads[i].searching, Threads[i].stop, 
            Threads[i].started, Threads[i].ended, Threads[i].nodes, Threads[i].numsplits);
    }
}

void checkSpeedUp(position_t* pos, char string[]) {
    const int NUMPOS = 4;
    const int MAXTHREADS = 32;
    char* fenPos[NUMPOS] = {
        "r2qkb1r/pp3p1pp2n2/nB1P4/3P1Qb1N2p2/PPP3PP/R1B1R1K1 b kq - 2 12",
        "r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - 2 18",
        "4qRr1/p3b2p/1kn5/1p2B3/3P4P2Q2/PP4PP/4R1K1 b - - 0 24",
        "3q4/p3b2p/1k6P1Q3/p2P4/8/1P4PP/6K1 b - - 0 30",
    };
    double timeSpeedupSum[MAXTHREADS];
    double nodesSpeedupSum[MAXTHREADS];
    char tempStr[256] = {0};
    int depth = 12;
    int threads = 1;
    char command[1024];

    sscanf_s(string, "%d %d", &threads, &depth);

    for (int j = 0; j < MAXTHREADS; ++j) {
        timeSpeedupSum[j] = 0.0;
        nodesSpeedupSum[j] = 0.0;
    }

    for (int i = 0; i < NUMPOS; ++i) {
        uciSetOption("name Threads value 1");
        TransTable.Clear();
        for (int k = 0; k < Guci_options.threads; ++k) {
            initSearchThread(k);
        }
        Print(5, "\n\nPos#%d: %s\n", i+1, fenPos[i]);
        uciSetPosition(pos, fenPos[i]);
        int64 startTime = getTime();
        sprintf_s(command, "movedepth %d", depth);
        uciGo(pos, command);
        int64 spentTime1 = getTime() - startTime;
        uint64 nodes1 = Threads[0].nodes / spentTime1;
        double timeSpeedUp = (double)spentTime1/1000.0;
        timeSpeedupSum[0] += timeSpeedUp;
        double nodesSpeedup = (double)nodes1;
        nodesSpeedupSum[0] += nodesSpeedup;
        Print(5, "Base: %0.2fs %dknps\n", timeSpeedUp, nodes1);

        for (int j = 2; j <= threads; ++j) {
            sprintf_s(tempStr, "name Threads value %d\n", j);
            uciSetOption(tempStr);
            TransTable.Clear();
            for (int k = 0; k < Guci_options.threads; ++k) {
                initSearchThread(k);
            }
            uciSetPosition(pos, fenPos[i]);
            startTime = getTime();
            sprintf_s(command, "movedepth %d", depth);
            uciGo(pos, command);
            int64 spentTime = getTime() - startTime;
            uint64 nodes = 0;
            for (int i = 0; i < Guci_options.threads; ++i) nodes += Threads[i].nodes;
            nodes /= spentTime;
            timeSpeedUp = (double)spentTime1 / (double)spentTime;
            timeSpeedupSum[j] += timeSpeedUp;
            nodesSpeedup = (double)nodes / (double)nodes1;
            nodesSpeedupSum[j] += nodesSpeedup;
            Print(5, "Thread: %d SpeedUp: %0.2f %0.2f\n", j, timeSpeedUp, nodesSpeedup);
        }
    }
    Print(5, "\n\n");
    Print(5, "\n\nAvg Base: %0.2fs %dknps\n", timeSpeedupSum[0]/NUMPOS, (int)nodesSpeedupSum[0]/NUMPOS);
    for (int j = 2; j <= threads; ++j) {
        Print(5, "Thread: %d Avg SpeedUp: %0.2f %0.2f\n", j, timeSpeedupSum[j]/NUMPOS, nodesSpeedupSum[j]/NUMPOS);
    }
    Print(5, "\n\n");
}

void benchMinSplitDepth(position_t* pos, char string[]) {
    const int NUMPOS = 4;
    const int MAXSPLIT = 11;
    char* fenPos[NUMPOS] = {
        "r2qkb1r/pp3p1p/2p2n2/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b kq - 2 12",
        "r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - 2 18",
        "4qRr1/p3b2p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - 0 24",
        "3q4/p3b2p/1k6/2P1Q3/p2P4/8/1P4PP/6K1 b - - 0 30",
    };
    char command[1024] = {0};
    uint64 timeSum[MAXSPLIT];
    uint64 nodesSum[MAXSPLIT];
    int threads = 1, depth = 12;
    for (int i = 1; i < MAXSPLIT; ++i) {
        timeSum[i] = nodesSum[i] = 0;
    }

    sscanf_s(string, "%d %d", &threads, &depth);

    sprintf_s(command, "name Threads value %d", threads);
    uciSetOption(command);
    for (int posIdx = 0; posIdx < NUMPOS; ++posIdx) {
        Print(5, "\n\nPos#%d: %s\n", posIdx+1, fenPos[posIdx]);
        for (int i = 1; i < MAXSPLIT; ++i) {
            sprintf_s(command, "name Min Split Depth value %d", i);
            uciSetOption(command);
            TransTable.Clear();
            for (int k = 0; k < Guci_options.threads; ++k) {
                initSearchThread(k);
            }
            uciSetPosition(pos, fenPos[posIdx]);
            int64 startTime = getTime();
            sprintf_s(command, "movedepth %d", depth);
            uciGo(pos, command);
            int64 spentTime = getTime() - startTime;
            timeSum[i] += spentTime;
            uint64 nodes = 0;
            for (int k = 0; k < Guci_options.threads; ++k) nodes += Threads[k].nodes;
            nodes = nodes*1000/spentTime;
            nodesSum[i] += nodes;
            Print(5, "Threads: %d Depth: %d SplitDepth: %d Time: %d Nps: %d\n", threads, depth, i, spentTime, nodes);
        }
    }
    int bestIdx = 1;
    double bestSplit = double(nodesSum[bestIdx])/double(timeSum[bestIdx]);
    Print(5, "\n\nAverage Statistics (Threads: %d Depth: %d)\n\n", threads, depth);
    for (int i = 1; i < MAXSPLIT; ++i) {
        if (double(nodesSum[i])/double(timeSum[i]) > bestSplit) {
            bestSplit = double(nodesSum[i])/double(timeSum[i]);
            bestIdx = i;
        }
        Print(5, "SplitDepth: %d Time: %d Nps: %d Ratio: %0.2f\n", i, timeSum[i]/NUMPOS, nodesSum[i]/NUMPOS, double(nodesSum[i])/double(timeSum[i]));
    }
    Print(5, "\n\nThe best SplitDepth for Threads: %d Depth: %d is:\n\nSplitDepth: %d Time: %d Nps: %d Ratio: %0.2f\n\n\n",
        threads, depth, bestIdx, timeSum[bestIdx]/NUMPOS, nodesSum[bestIdx]/NUMPOS, double(nodesSum[bestIdx])/double(timeSum[bestIdx]));
}

void benchThreadsperSplit(position_t* pos, char string[]) {
    const int NUMPOS = 4;
    const int MAXSPLIT = 9;
    char* fenPos[NUMPOS] = {
        "r2qkb1r/pp3p1p/2p2n2/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b kq - 2 12",
        "r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - 2 18",
        "4qRr1/p3b2p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - 0 24",
        "3q4/p3b2p/1k6/2P1Q3/p2P4/8/1P4PP/6K1 b - - 0 30",
    };
    char command[1024] = {0};
    uint64 timeSum[MAXSPLIT];
    uint64 nodesSum[MAXSPLIT];
    int threads = 1, depth = 12;
    for (int i = 1; i < MAXSPLIT; ++i) {
        timeSum[i] = nodesSum[i] = 0;
    }

    sscanf_s(string, "%d %d", &threads, &depth);

    sprintf_s(command, "name Threads value %d", threads);
    uciSetOption(command);
    for (int posIdx = 0; posIdx < NUMPOS; ++posIdx) {
        Print(5, "\n\nPos#%d: %s\n", posIdx+1, fenPos[posIdx]);
        for (int i = 2; i < MIN(MAXSPLIT, threads+1); ++i) {
            sprintf_s(command, "name Max Threads/Split value %d", i);
            uciSetOption(command);
            TransTable.Clear();
            for (int k = 0; k < Guci_options.threads; ++k) {
                initSearchThread(k);
            }
            uciSetPosition(pos, fenPos[posIdx]);
            int64 startTime = getTime();
            sprintf_s(command, "movedepth %d", depth);
            uciGo(pos, command);
            int64 spentTime = getTime() - startTime;
            timeSum[i] += spentTime;
            uint64 nodes = 0;
            for (int k = 0; k < Guci_options.threads; ++k) nodes += Threads[k].nodes;
            nodes = nodes*1000/spentTime;
            nodesSum[i] += nodes;
            Print(5, "Threads: %d Depth: %d Threads/Split: %d Time: %d Nps: %d\n", threads, depth, i, spentTime, nodes);
        }
    }
    int bestIdx = 2;
    double bestSplit = double(nodesSum[bestIdx])/double(timeSum[bestIdx]);
    Print(5, "\n\nAverage Statistics (Threads: %d Depth: %d)\n\n", threads, depth);
    for (int i = 2; i <  MIN(MAXSPLIT, threads+1); ++i) {
        if (double(nodesSum[i])/double(timeSum[i]) > bestSplit) {
            bestSplit = double(nodesSum[i])/double(timeSum[i]);
            bestIdx = i;
        }
        Print(5, "Threads/Split: %d Time: %d Nps: %d Ratio: %0.2f\n", i, timeSum[i]/NUMPOS, nodesSum[i]/NUMPOS, double(nodesSum[i])/double(timeSum[i]));
    }
    Print(5, "\n\nThe best Threads/Split for Threads: %d Depth: %d is:\n\nThreads/Split: %d Time: %d Nps: %d Ratio: %0.2f\n\n\n",
        threads, depth, bestIdx, timeSum[bestIdx]/NUMPOS, nodesSum[bestIdx]/NUMPOS, double(nodesSum[bestIdx])/double(timeSum[bestIdx]));
}

void benchActiveSplits(position_t* pos, char string[]) {
    const int NUMPOS = 4;
    const int MAXSPLIT = MaxNumSplitPointsPerThread+1;
    char* fenPos[NUMPOS] = {
        "r2qkb1r/pp3p1p/2p2n2/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b kq - 2 12",
        "r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - 2 18",
        "4qRr1/p3b2p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - 0 24",
        "3q4/p3b2p/1k6/2P1Q3/p2P4/8/1P4PP/6K1 b - - 0 30",
    };
    char command[1024] = {0};
    uint64 timeSum[MAXSPLIT];
    uint64 nodesSum[MAXSPLIT];
    int threads = 1, depth = 12;
    for (int i = 1; i < MAXSPLIT; ++i) {
        timeSum[i] = nodesSum[i] = 0;
    }

    sscanf_s(string, "%d %d", &threads, &depth);

    sprintf_s(command, "name Threads value %d", threads);
    uciSetOption(command);
    for (int posIdx = 0; posIdx < NUMPOS; ++posIdx) {
        Print(5, "\n\nPos#%d: %s\n", posIdx+1, fenPos[posIdx]);
        for (int i = 1; i <= MaxNumSplitPointsPerThread; ++i) {
            sprintf_s(command, "name Max Active Splits/Thread value %d", i);
            uciSetOption(command);
            TransTable.Clear();
            for (int k = 0; k < Guci_options.threads; ++k) {
                initSearchThread(k);
            }
            uciSetPosition(pos, fenPos[posIdx]);
            int64 startTime = getTime();
            sprintf_s(command, "movedepth %d", depth);
            uciGo(pos, command);
            int64 spentTime = getTime() - startTime;
            timeSum[i] += spentTime;
            uint64 nodes = 0;
            for (int k = 0; k < Guci_options.threads; ++k) nodes += Threads[k].nodes;
            nodes = nodes*1000/spentTime;
            nodesSum[i] += nodes;
            Print(5, "Threads: %d Depth: %d Active Splits: %d Time: %d Nps: %d\n", threads, depth, i, spentTime, nodes);
        }
    }
    int bestIdx = 2;
    double bestSplit = double(nodesSum[bestIdx])/double(timeSum[bestIdx]);
    Print(5, "\n\nAverage Statistics (Threads: %d Depth: %d)\n\n", threads, depth);
    for (int i = 1; i <= MaxNumSplitPointsPerThread; ++i) {
        if (double(nodesSum[i])/double(timeSum[i]) > bestSplit) {
            bestSplit = double(nodesSum[i])/double(timeSum[i]);
            bestIdx = i;
        }
        Print(5, "Active Splits: %d Time: %d Nps: %d Ratio: %0.2f\n", i, timeSum[i]/NUMPOS, nodesSum[i]/NUMPOS, double(nodesSum[i])/double(timeSum[i]));
    }
    Print(5, "\n\nThe best Active Splits for Threads: %d Depth: %d is:\n\nActive Splits: %d Time: %d Nps: %d Ratio: %0.2f\n\n\n",
        threads, depth, bestIdx, timeSum[bestIdx]/NUMPOS, nodesSum[bestIdx]/NUMPOS, double(nodesSum[bestIdx])/double(timeSum[bestIdx]));
}

