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
#include "threads.h"
#include "search.h"
#include "attacks.h"
#include "trans.h"
#include "movegen.h"
#include "movepicker.h"
#include "position.h"
#include "utils.h"
#include "bitutils.h"
#include "uci.h"
#include "eval.h"
#include "book.h"
#include "init.h"

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


Engine CEngine;

class Search {
public:
    Search(SearchInfo& _info, TranspositionTable& _tt, PvHashTable& _pvt) :
        m_Info(_info),
        m_TransTable(_tt),
        m_PVHashTable(_pvt)
    {}
    void initNode(Thread& sthread);
    int simpleStalemate(const position_t& pos);
    void displayPV(continuation_t *pv, int multipvIdx, int depth, int alpha, int beta, int score);
    bool prevMoveAllowsThreat(const position_t& pos, basic_move_t first, basic_move_t second);
    bool moveRefutesThreat(const position_t& pos, basic_move_t first, basic_move_t second);
    void updateEvalgains(const position_t& pos, uint32 move, int before, int after, Thread& sthread);
    template<bool inPv>
    int qSearch(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread);
    template <bool inRoot, bool inSplitPoint, bool inSingular>
    int searchNode(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt);
    template<bool inRoot, bool inSplitPoint, bool inCheck, bool inSingular>
    int searchGeneric(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt);
    void extractPvMovesFromHash(position_t& pos, continuation_t& pv);
    void repopulateHash(position_t& pos, continuation_t& rootPV);
    void timeManagement(int depth);
    void stopSearch();
private:
    SearchInfo& m_Info;
    TranspositionTable& m_TransTable;
    PvHashTable& m_PVHashTable;
};

void Search::initNode(Thread& sthread) {
    int64 time2;

    if (sthread.activeSplitPoint && sthread.activeSplitPoint->cutoffOccurred()) {
        sthread.stop = true;
        return;
    }

    sthread.nodes++;
#ifdef SELF_TUNE2
    if (sthread.nodes >= m_Info.node_limit && m_Info.node_is_limited) {
        stopSearch();
    }
#endif
    if (sthread.thread_id == 0 && ++sthread.nodes_since_poll >= sthread.nodes_between_polls) {
        sthread.nodes_since_poll = 0;
        time2 = getTime();
        if (time2 - m_Info.last_time > 1000) {
            int64 time = time2 - m_Info.start_time;
            m_Info.last_time = time2;
            if (SHOW_SEARCH) {
                uint64 sum_nodes;
                Print(1, "info ");
                Print(1, "time %llu ", time);
                sum_nodes = ThreadsMgr.ComputeNodes();
                Print(1, "nodes %llu ", sum_nodes);
                Print(1, "hashfull %d ", (m_TransTable.Used() * 1000) / m_TransTable.HashSize());
                Print(1, "nps %llu ", (sum_nodes * 1000ULL) / (time));
                Print(1, "\n");
            }
        }
        if (m_Info.thinking_status == THINKING && m_Info.time_is_limited) {
            if (time2 > m_Info.time_limit_max) {
                if (time2 < m_Info.time_limit_abs) {
                    if (!m_Info.research && !m_Info.change) {
                        bool gettingWorse = m_Info.best_value != -INF && m_Info.best_value + WORSE_SCORE_CUTOFF <= m_Info.last_value;
                        if (!gettingWorse) {
                            stopSearch();
                            Print(2, "info string Aborting search: time limit 2: %d\n", time2 - m_Info.start_time);
                        }
                    }
                } else {
                    stopSearch();
                    Print(2, "info string Aborting search: time limit 1: %d\n", time2 - m_Info.start_time);
                }
            }
        }
    }
}

int Search::simpleStalemate(const position_t& pos) {
    uint32 kpos, to;
    uint64 mv_bits;
    if (MinTwoBits(pos.color[pos.side] & ~pos.pawns)) return false;
    kpos = pos.kpos[pos.side];
    if (kpos != a1 && kpos != a8 && kpos != h1 && kpos != h8) return false;
    mv_bits = KingMoves[kpos];
    while (mv_bits) {
        to = popFirstBit(&mv_bits);
        if (!isSqAtt(pos, pos.occupied, to, pos.side ^ 1)) return false;
    }
    return true;
}

void Search::displayPV(continuation_t *pv, int multipvIdx, int depth, int alpha, int beta, int score) {
    uint64 time;
    uint64 sum_nodes = 0;

    ASSERT(pv != NULL);
    ASSERT(valueIsOk(score));

    time = getTime();
    m_Info.last_time = time;
    time = m_Info.last_time - m_Info.start_time;

    Print(1, "info depth %d ", depth);
    if (abs(score) < (INF - MAXPLY)) {
        if (score < beta) {
            if (score <= alpha) Print(1, "score cp %d upperbound ", score);
            else Print(1, "score cp %d ", score);
        } else Print(1, "score cp %d lowerbound ", score);
    } else {
        Print(1, "score mate %d ", (score>0) ? (INF - score + 1) / 2 : -(INF + score) / 2);
    }

    Print(1, "time %llu ", time);
    sum_nodes = ThreadsMgr.ComputeNodes();
    Print(1, "nodes %llu ", sum_nodes);
    Print(1, "hashfull %d ", (m_TransTable.Used() * 1000) / m_TransTable.HashSize());
    if (time > 10) Print(1, "nps %llu ", (sum_nodes * 1000) / (time));
    if (multipvIdx > 0) Print(1, "multipv %d ", multipvIdx + 1);
    Print(1, "pv ");
    for (int i = 0; i < pv->length; i++) printf("%s ", move2Str(pv->moves[i]));
    Print(1, "\n");
}

int moveIsTactical(uint32 m) {
    ASSERT(moveIsOk(m));
    return (m & 0x01fe0000UL);
}

int historyIndex(uint32 side, uint32 move) {
    return ((((side) << 9) + ((movePiece(move)) << 6) + (moveTo(move))) & 0x3ff);
}

bool Search::prevMoveAllowsThreat(const position_t& pos, basic_move_t first, basic_move_t second) {
    int m1from = moveFrom(first);
    int m2from = moveFrom(second);
    int m1to = moveTo(first);
    int m2to = moveTo(second);

    if (m1to == m2from || m2to == m1from) return true;
    if (InBetween[m2from][m2to] & BitMask[m1from]) return true;
    uint64 m1att = pieceAttacksFromBB(pos, movePiece(first), m1to, pos.occupied ^ BitMask[m2from]);
    if (m1att & BitMask[m2to]) return true;
    if (m1att & BitMask[pos.kpos[pos.side]]) return true;
    return false;
}

bool Search::moveRefutesThreat(const position_t& pos, basic_move_t first, basic_move_t second) {
    int m1from = moveFrom(first);
    int m2from = moveFrom(second);
    int m1to = moveTo(first);
    int m2to = moveTo(second);

    if (m1from == m2to) return true;
    if (moveCapture(second) && (PcValSEE[movePiece(second)] >= PcValSEE[moveCapture(second)] || movePiece(second) == KING)) {
        uint64 occ = pos.occupied ^ BitMask[m1from] ^ BitMask[m1to] ^ BitMask[m2from];
        if (pieceAttacksFromBB(pos, movePiece(first), m1to, occ) & BitMask[m2to]) return true;
        uint64 xray = rookAttacksBBX(m2to, occ) & (pos.queens | pos.rooks) & pos.color[pos.side];
        xray |= bishopAttacksBBX(m2to, occ) & (pos.queens | pos.bishops) & pos.color[pos.side];
        if (xray && (xray & ~queenAttacksBB(m2to, pos.occupied))) return true;
    }
    if (InBetween[m2from][m2to] & BitMask[m1to] && swap(pos, first) >= 0) return true;
    return false;
}

void Search::updateEvalgains(const position_t& pos, uint32 move, int before, int after, Thread& sthread) {
    if (move != EMPTY
        && before != -INF
        && after != -INF
        && !moveIsTactical(move)) {
        if (-(before + after) >= sthread.evalgains[historyIndex(pos.side ^ 1, move)])
            sthread.evalgains[historyIndex(pos.side ^ 1, move)] = -(before + after);
        else
            sthread.evalgains[historyIndex(pos.side ^ 1, move)]--;
    }
}

const int MaxPieceValue[] = { 0, PawnValueEnd, KnightValueEnd, BishopValueEnd, RookValueEnd, QueenValueEnd, 10000 };

template<bool inPv>
int Search::qSearch(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread) {
    int pes = 0;
    SearchStack ss;

    ASSERT(pos != NULL);
    ASSERT(valueIsOk(alpha));
    ASSERT(valueIsOk(beta));
    ASSERT(alpha < beta);
    ASSERT(pos.ply > 0); // for ssprev above

    initNode(sthread);
    if (sthread.stop) return 0;

    int t = 0;
    for (TransEntry* entry = m_TransTable.Entry(pos.posStore.hash); t < m_TransTable.BucketSize(); t++, entry++) {
        if (entry->HashLock() == LOCK(pos.posStore.hash)) {
            entry->SetAge(m_TransTable.Date());
            if (!inPv) { // TODO: re-use values from here to evalvalue?
                if (entry->Move() != EMPTY && entry->LowerDepth() > 0) {
                    int score = scoreFromTrans(entry->LowerValue(), pos.ply);
                    if (score > alpha) {
                        ssprev.counterMove = entry->Move();
                        return score;
                    }
                }
                if (entry->UpperDepth() > 0) {
                    int score = scoreFromTrans(entry->UpperValue(), pos.ply);
                    if (score < beta) return score;
                }
            }
            if (entry->Move() != EMPTY && entry->LowerDepth() > ss.hashDepth) {
                ss.hashDepth = entry->LowerDepth();
                ss.hashMove = entry->Move();
            }
        }
    }
    if (pos.ply >= MAXPLY - 1) return eval(pos, sthread.thread_id, &pes);
    if (!ssprev.moveGivesCheck) {
        if (simpleStalemate(pos)) {
            return DrawValue[pos.side];
        }
        ss.evalvalue = ss.bestvalue = eval(pos, sthread.thread_id, &pes);
        updateEvalgains(pos, pos.posStore.lastmove, ssprev.evalvalue, ss.evalvalue, sthread);
        if (ss.bestvalue > alpha) {
            if (ss.bestvalue >= beta) {
                ASSERT(valueIsOk(ss.bestvalue));
                return ss.bestvalue;
            }
            alpha = ss.bestvalue;
        }
    }

    ss.dcc = discoveredCheckCandidates(pos, pos.side);
    if (ssprev.moveGivesCheck) {
        sortInit(pos, ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseEvasion, sthread);
    } else {
        if (inPv) sortInit(pos, ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, (depth > -Q_PVCHECK) ? MoveGenPhaseQuiescenceAndChecksPV : MoveGenPhaseQuiescencePV, sthread);
        else sortInit(pos, ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, (depth > -Q_CHECK) ? MoveGenPhaseQuiescenceAndChecks : MoveGenPhaseQuiescence, sthread);
    }
    bool prunable = !ssprev.moveGivesCheck && !inPv && MinTwoBits(pos.color[pos.side ^ 1] & pos.pawns) && MinTwoBits(pos.color[pos.side ^ 1] & ~(pos.pawns | pos.kings));
    move_t* move;
    while ((move = sortNext(NULL, pos, ss.mvlist, ss.mvlist_phase, sthread)) != NULL) {
        int score;
        if (anyRepNoMove(pos, move->m)) {
            score = DrawValue[pos.side];
        } else {
            pos_store_t undo;
            ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
            if (prunable && move->m != ss.hashMove && !ss.moveGivesCheck && !moveIsPassedPawn(pos, move->m)) {
                int scoreAprox = ss.evalvalue + PawnValueEnd + MaxPieceValue[moveCapture(move->m)];
                if (scoreAprox < beta) continue;
            }
            int newdepth = depth - !ss.moveGivesCheck;
            makeMove(pos, undo, move->m);
            score = -qSearch<inPv>(pos, -beta, -alpha, newdepth, ss, sthread);
            unmakeMove(pos, undo);
        }
        if (sthread.stop) return ss.bestvalue;
        if (score > ss.bestvalue) {
            ss.bestvalue = score;
            if (score > alpha) {
                ss.bestmove = move->m;
                if (score >= beta) {
                    m_TransTable.StoreLower(pos.posStore.hash, ss.bestmove, 1, scoreToTrans(ss.bestvalue, pos.ply));
                    ASSERT(valueIsOk(ss.bestvalue));
                    ssprev.counterMove = ss.bestmove;
                    return ss.bestvalue;
                }
                alpha = score;
            }
        }
    }
    if (ssprev.moveGivesCheck && ss.bestvalue == -INF) {
        ss.bestvalue = (-INF + pos.ply);
        m_TransTable.StoreNoMoves(pos.posStore.hash, EMPTY, 1, scoreToTrans(ss.bestvalue, pos.ply));
        return ss.bestvalue;
    }

    ASSERT(bestvalue != -INF);

    if (inPv && ss.bestmove != EMPTY) {
        ssprev.counterMove = ss.bestmove;
        m_TransTable.StoreExact(pos.posStore.hash, ss.bestmove, 1, scoreToTrans(ss.bestvalue, pos.ply));
        m_PVHashTable.pvStore(pos.posStore.hash, ss.bestmove, 1, scoreToTrans(ss.bestvalue, pos.ply));
    } else m_TransTable.StoreUpper(pos.posStore.hash, EMPTY, 1, scoreToTrans(ss.bestvalue, pos.ply));

    ASSERT(valueIsOk(ss.bestvalue));

    return ss.bestvalue;
}

inline bool inPvNode(NodeType nt) { return (nt == PVNode); }
inline bool inCutNode(NodeType nt) { return (nt == CutNode); }
inline bool inAllNode(NodeType nt) { return (nt == AllNode); }
inline NodeType invertNode(NodeType nt) { return ((nt == PVNode) ? PVNode : ((nt == CutNode) ? AllNode : CutNode)); }

template <bool inRoot, bool inSplitPoint, bool inSingular>
int Search::searchNode(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt) {
    if (depth <= 0) {
        if (inPvNode(nt)) return qSearch<true>(pos, alpha, beta, 0, ssprev, sthread);
        else return qSearch<false>(pos, alpha, beta, 0, ssprev, sthread);
    } else {
        if (ssprev.moveGivesCheck) return searchGeneric<inRoot, inSplitPoint, true, inSingular>(pos, alpha, beta, depth, ssprev, sthread, nt);
        else return searchGeneric<inRoot, inSplitPoint, false, inSingular>(pos, alpha, beta, depth, ssprev, sthread, nt);
    }
}

template<bool inRoot, bool inSplitPoint, bool inCheck, bool inSingular>
int Search::searchGeneric(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt) {
    int pes = 0;
    SplitPoint* sp = NULL;
    SearchStack ss;

    ASSERT(pos != NULL);
    ASSERT(valueIsOk(alpha));
    ASSERT(valueIsOk(beta));
    ASSERT(alpha < beta);
    ASSERT(depth >= 1);

    initNode(sthread);
    if (sthread.stop) return 0;

    if (!inRoot && !inSingular && !inSplitPoint) {
        int t = 0;
        int evalDepth = 0;

        alpha = MAX(-INF + pos.ply, alpha);
        beta = MIN(INF - pos.ply - 1, beta);
        if (alpha >= beta) return alpha;

        for (TransEntry * entry = m_TransTable.Entry(pos.posStore.hash); t < m_TransTable.BucketSize(); t++, entry++) {
            if (entry->HashLock() == LOCK(pos.posStore.hash)) {
                entry->SetAge(m_TransTable.Date());
                if (entry->Mask() & MNoMoves) {
                    if (inCheck) return -INF + pos.ply;
                    else return DrawValue[pos.side];
                }
                if (!inPvNode(nt)) {
                    if ((!inCutNode(nt) || !(entry->Mask() & MAllLower)) && entry->LowerDepth() >= depth && (entry->Move() != EMPTY || pos.posStore.lastmove == EMPTY)) {
                        int score = scoreFromTrans(entry->LowerValue(), pos.ply);
                        if (score > alpha) {
                            ssprev.counterMove = entry->Move();
                            return score;
                        }
                    }
                    if ((!inAllNode(nt) || !(entry->Mask() & MCutUpper)) && entry->UpperDepth() >= depth) {
                        int score = scoreFromTrans(entry->UpperValue(), pos.ply);
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
                    ss.evalvalue = scoreFromTrans(entry->LowerValue(), pos.ply);
                }
                if (entry->UpperDepth() > evalDepth) {
                    evalDepth = entry->UpperDepth();
                    ss.evalvalue = scoreFromTrans(entry->UpperValue(), pos.ply);
                }
            }
        }
        if (ss.evalvalue == -INF) ss.evalvalue = eval(pos, sthread.thread_id, &pes);

        if (pos.ply >= MAXPLY - 1) return ss.evalvalue;
        updateEvalgains(pos, pos.posStore.lastmove, ssprev.evalvalue, ss.evalvalue, sthread);

        if (!inPvNode(nt) && !inCheck) {
            const int MaxRazorDepth = 10;
            int rvalue;
            if (depth < MaxRazorDepth && (pos.color[pos.side] & ~(pos.pawns | pos.kings))
                && ss.evalvalue >(rvalue = beta + FutilityMarginTable[MIN(depth, MaxRazorDepth)][MIN(ssprev.playedMoves, 63)])) {
                return rvalue;
            }
            if (depth < MaxRazorDepth && pos.posStore.lastmove != EMPTY
                && ss.evalvalue < (rvalue = beta - FutilityMarginTable[MIN(depth, MaxRazorDepth)][MIN(ssprev.playedMoves, 63)])) {
                int score = searchNode<false, false, false>(pos, rvalue - 1, rvalue, 0, ssprev, sthread, nt);
                if (score < rvalue) return score;
            }
            if (depth >= 2 && (pos.color[pos.side] & ~(pos.pawns | pos.kings)) && ss.evalvalue >= beta) {
                pos_store_t undo;
                int nullDepth = depth - (4 + depth / 5 + (ss.evalvalue - beta > PawnValue));
                makeNullMove(pos, undo);
                int score = -searchNode<false, false, false>(pos, -beta, -alpha, nullDepth, ss, sthread, invertNode(nt));
                ss.threatMove = ss.counterMove;
                unmakeNullMove(pos, undo);
                if (sthread.stop) return 0;
                if (score >= beta) {
                    if (depth >= 12) {
                        score = searchNode<false, false, false>(pos, alpha, beta, nullDepth, ssprev, sthread, CutNode);
                        if (sthread.stop) return 0;
                    }
                    if (score >= beta) {
                        if (ss.hashMove == EMPTY) {
                            if (inCutNode(nt)) m_TransTable.StoreLower(pos.posStore.hash, EMPTY, depth, scoreToTrans(score, pos.ply));
                            else m_TransTable.StoreAllLower(pos.posStore.hash, EMPTY, depth, scoreToTrans(score, pos.ply));
                        }
                        return score;
                    }
                } else if (depth < 5 && ssprev.reducedMove && ss.threatMove != EMPTY && prevMoveAllowsThreat(pos, pos.posStore.lastmove, ss.threatMove)) {
                    return alpha;
                }
            }
        }

        if (!inAllNode(nt) && !inCheck && depth >= (inPvNode(nt) ? 6 : 8)) { // IID
            int newdepth = inPvNode(nt) ? depth - 2 : depth / 2;
            if (ss.hashMove == EMPTY || ss.hashDepth < newdepth) {
                int score = searchNode<false, false, false>(pos, alpha, beta, newdepth, ssprev, sthread, nt);
                if (sthread.stop) return 0;
                ss.evalvalue = score;
                if (score > alpha) {
                    ss.hashMove = ssprev.counterMove;
                    ss.evalvalue = score;
                    ss.hashDepth = newdepth;
                }
            }
        }
        int newdepth;
        if (ss.hashMove != EMPTY && depth >= (inPvNode(nt) ? 6 : 8) && ss.hashDepth >= (newdepth = depth / 2)) { // singular extension (adding !inRoot) 
            int targetScore = ss.evalvalue - EXPLORE_CUTOFF;
            ssprev.bannedMove = ss.hashMove;
            int score = searchNode<false, false, true>(pos, targetScore, targetScore + 1, newdepth, ssprev, sthread, nt);
            if (sthread.stop) return 0;
            ssprev.bannedMove = EMPTY;
            if (score <= targetScore) ss.firstExtend = true;
        }
    }

    if (inSplitPoint) {
        sp = sthread.activeSplitPoint;
        ss = *sp->sscurr; // ss.mvlist points to master ss.mvlist, copied the entire searchstack also
    } else {
        if (inRoot) {
            ss = ssprev; // this is correct, ss.mvlist points to the ssprev.mvlist, at the same time, ssprev resets other member vars
            if (!m_Info.mvlist_initialized) {
                sortInit(pos, ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseRoot, sthread);
            } else {
                ss.mvlist->pos = 0;
                ss.mvlist->phase = MoveGenPhaseRoot + 1;
            }
        } else {
            ss.dcc = discoveredCheckCandidates(pos, pos.side);
            sortInit(pos, ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), sthread);
            ss.firstExtend = ss.firstExtend || (inCheck && ss.mvlist->size == 1);
        }
    }

    int lateMove = LATE_PRUNE_MIN + (inCutNode(nt) ? ((depth * depth) / 4) : (depth * depth));
    move_t* move;
    while ((move = sortNext(sp, pos, ss.mvlist, ss.mvlist_phase, sthread)) != NULL) {
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
            score = DrawValue[pos.side];
        } else {
            int newdepth;
            pos_store_t undo;
            ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
            if (ss.bestvalue == -INF) { //TODO remove this from loop and do it first
                newdepth = depth - !ss.firstExtend;
                makeMove(pos, undo, move->m);
                score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, ss, sthread, invertNode(nt));
            } else {
                newdepth = depth - 1;
                //only reduce or prune some types of moves
                int partialReduction = 0;
                int fullReduction = 0;
                if (MoveGenPhase[ss.mvlist_phase] == PH_QUIET_MOVES && !ss.moveGivesCheck) { //never happens when in check
                    bool goodMove = (ss.threatMove && moveRefutesThreat(pos, move->m, ss.threatMove)) || moveIsPassedPawn(pos, move->m);
                    if (!inRoot && !inPvNode(nt)) {
                        if (ss.playedMoves > lateMove && !goodMove) continue;
                        int predictedDepth = MAX(0, newdepth - ReductionTable[1][MIN(depth, 63)][MIN(ss.playedMoves, 63)]);
                        int scoreAprox = ss.evalvalue + FutilityMarginTable[MIN(predictedDepth, MAX_FUT_MARGIN)][MIN(ss.playedMoves, 63)]
                            + sthread.evalgains[historyIndex(pos.side, move->m)];

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
                        int reduction = ReductionTable[(inPvNode(nt) ? 0 : 1)][MIN(depth, 63)][MIN(ss.playedMoves, 63)];
                        partialReduction += goodMove ? (reduction + 1) / 2 : reduction;
                    }
                } else if ((MoveGenPhase[ss.mvlist_phase] == PH_BAD_CAPTURES ||
                    (MoveGenPhase[ss.mvlist_phase] == PH_QUIET_MOVES && swap(pos, move->m) < 0)) && !inRoot && !inPvNode(nt)) fullReduction++;  //never happens when in check
                newdepth -= fullReduction;
                int newdepthclone = newdepth - partialReduction;
                makeMove(pos, undo, move->m);
                if (inSplitPoint) alpha = sp->alpha;
                ss.reducedMove = (newdepthclone < newdepth); //TODO consider taking into account full reductions
                score = -searchNode<false, false, false>(pos, -alpha - 1, -alpha, newdepthclone, ss, sthread, inCutNode(nt) ? AllNode : CutNode);
                if (ss.reducedMove && score > alpha) {
                    if (partialReduction >= 4) {
                        newdepthclone = newdepth - partialReduction / 2;
                        score = -searchNode<false, false, false>(pos, -alpha - 1, -alpha, newdepthclone, ss, sthread, inCutNode(nt) ? AllNode : CutNode);
                    }
                    if (score > alpha) {
                        ss.reducedMove = false;
                        score = -searchNode<false, false, false>(pos, -alpha - 1, -alpha, newdepth, ss, sthread, AllNode);
                    }
                }
                if (inPvNode(nt) && score > alpha) {
                    if (inRoot) m_Info.research = 1;
                    score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, ss, sthread, PVNode);
                }
            }
            unmakeMove(pos, undo);
        }
        if (sthread.stop) return 0;
        if (inSplitPoint) sp->updatelock->lock();
        if (inRoot) {
            move->s = score;
        }
        if (score > (inSplitPoint ? sp->bestvalue : ss.bestvalue)) {
            ss.bestvalue = inSplitPoint ? sp->bestvalue = score : score;
            if (inRoot) {
                m_Info.best_value = ss.bestvalue;
                if (m_Info.iteration > 1 && m_Info.bestmove != move->m) m_Info.change = 1;
            }
            if (ss.bestvalue > (inSplitPoint ? sp->alpha : alpha)) {
                ss.bestmove = inSplitPoint ? sp->bestmove = move->m : move->m;
                if (inRoot) {
                    m_Info.bestmove = ss.bestmove;
                    if (m_Info.rootPV.length > 1) m_Info.pondermove = m_Info.rootPV.moves[1];
                    else m_Info.pondermove = 0;
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
        if (!inSplitPoint && !inSingular && !sthread.stop && !inCheck && sthread.num_sp < ThreadsMgr.m_MaxActiveSplitsPerThread
            && ThreadsMgr.ThreadNum() > 1 && depth >= ThreadsMgr.m_MinSplitDepth
            && (!inCutNode(nt) || (MoveGenPhase[ss.mvlist_phase] != PH_GOOD_CAPTURES && MoveGenPhase[ss.mvlist_phase] != PH_KILLER_MOVES))) {
            ThreadsMgr.SearchSplitPoint(pos, &ss, &ssprev, alpha, beta, nt, depth, inCheck, inRoot, sthread);
            if (sthread.stop) return 0;
            break;
        }
    }
    if (inRoot && !inSplitPoint) {
        if (ss.bestmove != EMPTY && ss.bestvalue < beta) m_PVHashTable.pvStore(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos.ply));
        if (ss.playedMoves == 0) {
            if (inCheck) return -INF;
            else return DrawValue[pos.side];
        }
    }
    if (!inRoot && !inSplitPoint && !inSingular) {
        if (ss.playedMoves == 0) {
            if (inCheck) ss.bestvalue = -INF + pos.ply;
            else ss.bestvalue = DrawValue[pos.side];
            m_TransTable.StoreNoMoves(pos.posStore.hash, EMPTY, depth, scoreToTrans(ss.bestvalue, pos.ply));
            return ss.bestvalue;
        }
        if (ss.bestmove != EMPTY && !moveIsTactical(ss.bestmove) && ss.bestvalue >= beta) { //> alpha account for pv better maybe? Sam
            int index = historyIndex(pos.side, ss.bestmove);
            int hisDepth = MIN(depth, MAX_HDEPTH);
            sthread.history[index] += hisDepth * hisDepth;
            for (int i = 0; i < ss.hisCnt; ++i) {
                if (ss.hisMoves[i] == ss.bestmove) continue;
                index = historyIndex(pos.side, ss.hisMoves[i]);
                sthread.history[index] -= sthread.history[index] / (NEW_HISTORY - hisDepth);
            }
            if (sthread.ts[pos.ply].killer1 != ss.bestmove) {
                sthread.ts[pos.ply].killer2 = sthread.ts[pos.ply].killer1;
                sthread.ts[pos.ply].killer1 = ss.bestmove;
            }
        }
        if (ss.bestvalue >= beta) {
            ASSERT(valueIsOk(ss.bestvalue));
            ssprev.counterMove = ss.bestmove;
            if (inCutNode(nt)) m_TransTable.StoreLower(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos.ply));
            else m_TransTable.StoreAllLower(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos.ply));
        } else {
            ASSERT(valueIsOk(bestvalue));
            if (inPvNode(nt) && ss.bestmove != EMPTY) {
                ssprev.counterMove = ss.bestmove;
                m_TransTable.StoreExact(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos.ply));
                m_PVHashTable.pvStore(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, pos.ply));
            } else if (inCutNode(nt)) m_TransTable.StoreCutUpper(pos.posStore.hash, EMPTY, depth, scoreToTrans(ss.bestvalue, pos.ply));
            else m_TransTable.StoreUpper(pos.posStore.hash, EMPTY, depth, scoreToTrans(ss.bestvalue, pos.ply));
        }
    }
    ASSERT(valueIsOk(ss.bestvalue));
    return ss.bestvalue;
}

void Search::extractPvMovesFromHash(position_t& pos, continuation_t& pv) {
    PvHashEntry *entry;
    pos_store_t undo[MAXPLY];
    int ply = 0;
    basic_move_t hashMove;
    pv.length = 0;
    while ((entry = m_PVHashTable.pvEntry(pos.posStore.hash)) != NULL) {
        hashMove = entry->pvMove();
        if (hashMove == EMPTY) break;
        if (!genMoveIfLegal(pos, hashMove, pinnedPieces(pos, pos.side))) break;
        pv.moves[pv.length++] = hashMove;
        if (anyRep(pos)) break; // break on repetition to avoid long pv display
        makeMove(pos, undo[ply++], hashMove);
        if (ply >= MAXPLY) break;
    }
    for (ply = ply - 1; ply >= 0; --ply) {
        unmakeMove(pos, undo[ply]);
    }
}

void Search::repopulateHash(position_t& pos, continuation_t& rootPV) {
    int moveOn;
    pos_store_t undo[MAXPLY];
    for (moveOn = 0; moveOn + 1 <= rootPV.length; moveOn++) {
        basic_move_t move = rootPV.moves[moveOn];
        if (!move) break;
        PvHashEntry* entry = m_PVHashTable.pvEntryFromMove(pos.posStore.hash, move);
        if (NULL == entry) break;
        m_TransTable.StoreExact(pos.posStore.hash, entry->pvMove(), entry->pvDepth(), entry->pvScore());
        makeMove(pos, undo[moveOn], move);
    }
    for (moveOn = moveOn - 1; moveOn >= 0; moveOn--) {
        unmakeMove(pos, undo[moveOn]);
    }
}

void Search::timeManagement(int depth) {
    if (m_Info.best_value > INF - MAXPLY) m_Info.mate_found++;
    if (m_Info.thinking_status == THINKING) {
        if (m_Info.time_is_limited) {
            int64 time = getTime();
            bool gettingWorse = (m_Info.best_value + WORSE_SCORE_CUTOFF) <= m_Info.last_value;

            if (m_Info.legalmoves == 1 || m_Info.mate_found >= 3) {
                if (depth >= 8) {
                    stopSearch();
                    Print(2, "info string Aborting search: legalmove/mate found depth >= 8\n");
                    return;
                }
            }
            if ((time - m_Info.start_time) > ((m_Info.time_limit_max - m_Info.start_time) * 60) / 100) {
                int64 addTime = 0;
                if (gettingWorse) {
                    int amountWorse = m_Info.last_value - m_Info.best_value;
                    addTime += ((amountWorse - (WORSE_SCORE_CUTOFF - 10)) * m_Info.alloc_time) / WORSE_TIME_BONUS;
                    Print(2, "info string Extending time (root gettingWorse): %d\n", addTime);
                }
                if (m_Info.change) {
                    addTime += (m_Info.alloc_time * CHANGE_TIME_BONUS) / 100;
                }
                if (addTime > 0) {
                    m_Info.time_limit_max += addTime;
                    if (m_Info.time_limit_max > m_Info.time_limit_abs)
                        m_Info.time_limit_max = m_Info.time_limit_abs;
                } else { // if we are unlikely to get deeper, save our time
                    stopSearch();
                    Print(2, "info string Aborting search: root time limit 1: %d\n", time - m_Info.start_time);
                    return;
                }
            }
        }
    }
    if (m_Info.depth_is_limited && depth >= m_Info.depth_limit) {
        stopSearch();
        Print(2, "info string Aborting search: depth limit 1\n");
        return;
    }
}

void Search::stopSearch() {
    m_Info.stop_search = true;
    ThreadsMgr.SetAllThreadsToStop();
}

Engine::Engine() { search = new Search(info, transtable, pvhashtable); }
Engine::~Engine() { delete search; }

void Engine::stopSearch() {
    search->stopSearch();
}

void Engine::ponderHit() { //no pondering in tuning
    int64 time = getTime() - info.start_time;

    if ((info.iteration >= 8 && (info.legalmoves == 1 || info.mate_found >= 3)) || (time > info.time_limit_abs)) {
        stopSearch();
        Print(3, "info string Has searched enough the ponder move: aborting\n");
    } else {
        info.thinking_status = THINKING;
        Print(2, "info string Switch from pondering to thinking\n");
    }
}

void Engine::searchFromIdleLoop(SplitPoint& sp, Thread& sthread) {
    if (sp.inRoot) search->searchNode<true, true, false>(sp.pos[sthread.thread_id], sp.alpha, sp.beta, sp.depth, *sp.ssprev, sthread, sp.nodeType);
    else search->searchNode<false, true, false>(sp.pos[sthread.thread_id], sp.alpha, sp.beta, sp.depth, *sp.ssprev, sthread, sp.nodeType);
}

void Engine::sendBestMove() {
    if (!info.bestmove) {
        if (RETURN_MOVE)
            Print(3, "info string No legal move found. Start a new game.\n\n");
    } else {
        if (RETURN_MOVE) {
            Print(3, "bestmove %s", move2Str(info.bestmove));
            if (info.pondermove) Print(3, " ponder %s", move2Str(info.pondermove));
            Print(3, "\n\n");
        }
        origScore = info.last_value; // just to be safe
    }
    ThreadsMgr.SetAllThreadsToSleep();
    //ThreadsMgr.PrintDebugData();
}

void Engine::getBestMove(Thread& sthread) {
    int id;
    int alpha, beta;
    SearchStack ss;
    SplitPoint rootsp;
    ss.moveGivesCheck = kingIsInCheck(rootpos);
    ss.dcc = discoveredCheckCandidates(rootpos, rootpos.side);

    transtable.NewDate(transtable.Date());
    pvhashtable.NewDate(pvhashtable.Date());

    do {
        PvHashEntry *entry = pvhashtable.pvEntry(rootpos.posStore.hash);
        if (NULL == entry) break;
        if (info.thinking_status == THINKING
            && info.rootPV.moves[1] == rootpos.posStore.lastmove
            && info.rootPV.moves[2] == entry->pvMove()
            && ((moveCapture(rootpos.posStore.lastmove) && (moveTo(rootpos.posStore.lastmove) == moveTo(entry->pvMove())))
            || (PcValSEE[moveCapture(entry->pvMove())] > PcValSEE[movePiece(entry->pvMove())]))) {
            info.bestmove = entry->pvMove();
            info.best_value = entry->pvScore();
            search->extractPvMovesFromHash(rootpos, info.rootPV);
            if (info.rootPV.length > 1) info.pondermove = info.rootPV.moves[1];
            else info.pondermove = 0;
            search->displayPV(&info.rootPV, info.multipvIdx, entry->pvDepth(), -INF, INF, info.best_value);
            stopSearch();
            sendBestMove();
            return;
        }
        ss.hashMove = entry->pvMove();
    } while (false);

    // extend time when there is no hashmove from hashtable, this is useful when just out of the book
    if (ss.hashMove == EMPTY) {
        info.time_limit_max += info.alloc_time / 2;
        if (info.time_limit_max > info.time_limit_abs)
            info.time_limit_max = info.time_limit_abs;
    }

    // SMP 
    ThreadsMgr.InitVars();
    ThreadsMgr.SetAllThreadsToWork();

    for (id = 1; id < MAXPLY; id++) {
        const int AspirationWindow = 24;
        int faillow = 0, failhigh = 0;
        info.iteration = id;
        info.best_value = -INF;
        info.change = 0;
        info.research = 0;
        for (info.multipvIdx = 0; info.multipvIdx < info.multipv; ++info.multipvIdx) {
            if (id < 6) {
                alpha = -INF;
                beta = INF;
            } else {
                alpha = info.last_value - AspirationWindow;
                beta = info.last_value + AspirationWindow;
                if (alpha < -QueenValue) alpha = -INF;
                if (beta > QueenValue) beta = INF;
            }
            while (true) {

                search->searchNode<true, false, false>(rootpos, alpha, beta, id, ss, sthread, PVNode);

                if (!sthread.stop || info.best_value != -INF) {
                    search->extractPvMovesFromHash(rootpos, info.rootPV);
                    search->repopulateHash(rootpos, info.rootPV);
                    if (SHOW_SEARCH && id >= 8) {
                        search->displayPV(&info.rootPV, info.multipvIdx, id, alpha, beta, info.best_value);
                    }
                }
                if (info.stop_search) break;
                if (info.best_value <= alpha) {
                    if (info.best_value <= alpha) alpha = info.best_value - (AspirationWindow << ++faillow);
                    if (alpha < -QueenValue) alpha = -INF;
                    info.research = 1;
                } else if (info.best_value >= beta) {
                    if (info.best_value >= beta)  beta = info.best_value + (AspirationWindow << ++failhigh);
                    if (beta > QueenValue) beta = INF;
                    info.research = 1;
                } else break;
            }
            if (info.stop_search) break; // TODO: refactor this
        }
        if (info.stop_search) break; // TODO: refactor this
        search->timeManagement(id);
        if (info.stop_search) break; // TODO: refactor this
        if (info.best_value != -INF) {
            info.last_last_value = info.last_value;
            info.last_value = info.best_value;
        }
    }
    if (!info.stop_search) {
        if ((info.depth_is_limited || info.time_is_limited) && info.thinking_status == THINKING) {
            info.stop_search = true;;
            stopSearch();
            Print(2, "info string Aborting search: end of getBestMove: id=%d, best_value = %d sp = %d, ply = %d\n",
                id, info.best_value, rootpos.sp, rootpos.ply);
        } else {
            Print(3, "info string Waiting for stop, quit, or ponderhit\n");
            while (!info.stop_search);
            stopSearch();
            Print(2, "info string Aborting search: end of waiting for stop/quit/ponderhit\n");
        }
    }

    sendBestMove();
}


