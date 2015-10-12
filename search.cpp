/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
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
/*
void prefetch(char* addr) { //stockfish
#  if defined(__INTEL_COMPILER)
// This hack prevents prefetches from being optimized away by
// Intel compiler. Both MSVC and gcc seem not be affected by this.
__asm__("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
_mm_prefetch(addr, _MM_HINT_T0);
#  else
__builtin_prefetch(addr);
#  endif
}
*/
bool moveIsTactical(const uint32 m) { // TODO
    ASSERT(moveIsOk(m));
    return (m & 0x01fe0000UL) != 0;
}

bool moveIsDangerousPawn(const position_t& pos, const uint32 move) {
    return (movePiece(move) == PAWN && Q_DIST(moveTo(move), pos.side) <= 2);
}

int historyIndex(const uint32 side, const uint32 move) { // TODO
    return ((((side) << 9) + ((movePiece(move)) << 6) + (moveTo(move))) & 0x3ff);
}

class Search {
public:
    Search(Engine& _engine, SearchInfo& _info, TranspositionTable& _tt, PvHashTable& _pvt) :
        mEngine(_engine),
        mInfo(_info),
        mTransTable(_tt),
        mPVHashTable(_pvt) {}
    void initNode(Thread& sthread);
    bool moveRefutesThreat(const position_t& pos, basic_move_t first, basic_move_t second);
    void updateEvalgains(const position_t& pos, uint32 move, int before, int after, Thread& sthread);
    void UpdateHistory(position_t& pos, SearchStack& ss, Thread& sthread, const int depth);
    template<bool inPv>
    int qSearch(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread);
    template <bool inRoot, bool inSplitPoint, bool inSingular>
    int searchNode(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt);
    template<bool inRoot, bool inSplitPoint, bool inCheck, bool inSingular>
    int searchGeneric(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt);
private:
    inline bool inPvNode(NodeType nt) {
        return (nt == PVNode);
    }
    inline bool inCutNode(NodeType nt) {
        return (nt == CutNode);
    }
    inline bool inAllNode(NodeType nt) {
        return (nt == AllNode);
    }
    inline NodeType invertNode(NodeType nt) {
        return ((nt == PVNode) ? PVNode : ((nt == CutNode) ? AllNode : CutNode));
    }

    static const int EXPLORE_BASE_CUTOFF = 18;
    static const int EXPLORE_MULT_CUTOFF = 2;
    static const int Q_CHECK = 1; // implies 1 check
    static const int Q_PVCHECK = 2; // implies 2 checks
    static const int MIN_REDUCTION_DEPTH = 4; // default is false

    SearchInfo& mInfo;
    TranspositionTable& mTransTable;
    PvHashTable& mPVHashTable;
    Engine& mEngine;
};

void Search::initNode(Thread& sthread) {
    if (sthread.activeSplitPoint && sthread.activeSplitPoint->cutoffOccurred()) {
        sthread.stop = true;
        return;
    }
    if (mInfo.node_is_limited && mEngine.ComputeNodes() >= mInfo.node_limit) {
        mEngine.StopSearch();
    }
}
bool Search::moveRefutesThreat(const position_t& pos, basic_move_t first, basic_move_t threat) {
    int m1from = moveFrom(first);
    int threatfrom = moveFrom(threat);
    int m1to = moveTo(first);
    int threatto = moveTo(threat);

    if (m1from == threatto) return true;
    if (moveCapture(threat) && (PcValSEE[movePiece(threat)] >= PcValSEE[moveCapture(threat)] || movePiece(threat) == KING)) {
        uint64 occ = pos.occupied ^ BitMask[m1from] ^ BitMask[m1to] ^ BitMask[threatfrom];
        if (pieceAttacksFromBB(pos, movePiece(first), m1to, occ) & BitMask[threatto]) return true;
        uint64 xray = rookAttacksBBX(threatto, occ) & (pos.queens | pos.rooks) & pos.color[pos.side];
        xray |= bishopAttacksBBX(threatto, occ) & (pos.queens | pos.bishops) & pos.color[pos.side];
        if (xray && (xray & ~queenAttacksBB(threatto, pos.occupied))) return true;
    }
    if (InBetween[threatfrom][threatto] & BitMask[m1to] && swap(pos, first) >= 0) return true;
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

void Search::UpdateHistory(position_t& pos, SearchStack& ss, Thread& sthread, const int depth) {
    if (ss.bestmove != EMPTY && !moveIsTactical(ss.bestmove)) { //> alpha account for pv better maybe? Sam
        static const int MAX_HDEPTH = 20;
        static const int NEW_HISTORY = (10 + MAX_HDEPTH);
        int index = historyIndex(pos.side, ss.bestmove);
        int hisDepth = MIN(depth, MAX_HDEPTH);
        sthread.history[index] += hisDepth * hisDepth;
        for (int i = 0; i < ss.hisCnt; ++i) {
            if (ss.hisMoves[i] == ss.bestmove) continue;
            index = historyIndex(pos.side, ss.hisMoves[i]);
            sthread.history[index] -= sthread.history[index] / (NEW_HISTORY - hisDepth);
        }
        if (sthread.ts[ss.ply].killer1 != ss.bestmove) {
            sthread.ts[ss.ply].killer2 = sthread.ts[ss.ply].killer1;
            sthread.ts[ss.ply].killer1 = ss.bestmove;
        }
    }
}

const int MaxPieceValue[] = { 0, PawnValueEnd, KnightValueEnd, BishopValueEnd, RookValueEnd, QueenValueEnd, 10000 };
template<bool inPv>
int Search::qSearch(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread) {
    SearchStack ss(ssprev.ply + 1);
    pos_store_t undo;

    ASSERT(alpha < beta);
    ASSERT(ss.ply > 0); // for ssprev above

    initNode(sthread);
    if (sthread.stop) return 0;

    for (TransEntry *entry = mTransTable.Entry(pos.posStore.hash), *end = entry + mTransTable.BucketSize(); entry != end; ++entry) {
        if (entry->HashLock() == LOCK(pos.posStore.hash)) {
            entry->SetAge(mTransTable.Date());
            if (!inPv) { // TODO: re-use values from here to evalvalue?
                if (entry->FailHighDepth() != 0) {
                    int score = scoreFromTrans(entry->FailHighValue(), ss.ply);
                    if (score > alpha) {
                        ssprev.counterMove = entry->Move();
                        return score;
                    }
                }
                if (entry->FailLowDepth() != 0) {
                    int score = scoreFromTrans(entry->FailLowValue(), ss.ply);
                    if (score < beta) return score;
                }
            }
            if (entry->Move() != EMPTY && entry->FailHighDepth() > ss.hashDepth && moveIsTactical(entry->Move())) {
                ss.hashDepth = entry->FailHighDepth();
                ss.hashMove = entry->Move();
            }
        }
    }
    if (ss.ply >= MAXPLY - 1) return eval(pos, sthread);
    if (!ssprev.moveGivesCheck) {
        ss.evalvalue = ss.bestvalue = eval(pos, sthread);
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
        sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseEvasion, sthread.ts[ss.ply]);
    }
    else {
        if (inPv) sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, (depth > -Q_PVCHECK) ? MoveGenPhaseQuiescenceAndChecksPV : MoveGenPhaseQuiescence, sthread.ts[ss.ply]);
        else sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, (depth > -Q_CHECK) ? MoveGenPhaseQuiescenceAndChecks : MoveGenPhaseQuiescence, sthread.ts[ss.ply]);
    }
    bool prunable = !ssprev.moveGivesCheck && !inPv && MinTwoBits(pos.color[pos.side ^ 1] & pos.pawns) && MinTwoBits(pos.color[pos.side ^ 1] & ~(pos.pawns | pos.kings));
    move_t* move;
    while ((move = sortNext(nullptr, mInfo, pos, *ss.mvlist, sthread)) != nullptr) {
        int score;
        if (anyRepNoMove(pos, move->m)) {
            score = DrawValue[pos.side];
        }
        else {
            ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
            if (!inPv && ssprev.moveGivesCheck &&  ss.bestvalue > -MAXEVAL && !ss.moveGivesCheck && swap(pos, move->m) < 0) continue;
            if (prunable && move->m != ss.hashMove && !ss.moveGivesCheck && !moveIsDangerousPawn(pos, move->m)) {
                int scoreAprox = ss.evalvalue + PawnValueEnd / 2 + MaxPieceValue[moveCapture(move->m)];
                if (scoreAprox < beta) continue;
            }
            int newdepth = depth - !ss.moveGivesCheck;
            makeMove(pos, undo, move->m);
            ++sthread.nodes;
            score = -qSearch<inPv>(pos, -beta, -alpha, newdepth, ss, sthread);
            unmakeMove(pos, undo);
        }
        if (sthread.stop) return 0;
        if (score > ss.bestvalue) {
            ss.bestvalue = score;
            if (score > alpha) {
                ss.bestmove = move->m;
                if (score >= beta) break;
                alpha = score;
            }
        }
    }
    if (ssprev.moveGivesCheck && ss.bestvalue == -INF) {
        ss.bestvalue = (-INF + ss.ply);
        mTransTable.StoreNoMoves(pos.posStore.hash, -1, scoreToTrans(ss.bestvalue, ss.ply));
        return ss.bestvalue;
    }
    if (ss.bestvalue >= beta) {
        ssprev.counterMove = ss.bestmove;
        mTransTable.StoreCutNodeFailHigh(pos.posStore.hash, ss.bestmove, -1, scoreToTrans(ss.bestvalue, ss.ply), false);
    }
    else {
        if (inPv && ss.bestmove != EMPTY) {
            ssprev.counterMove = ss.bestmove;
            mTransTable.StorePVNode(pos.posStore.hash, ss.bestmove, -1, scoreToTrans(ss.bestvalue, ss.ply), false);
            mPVHashTable.pvStore(pos.posStore.hash, ss.bestmove, -1, scoreToTrans(ss.bestvalue, ss.ply));
        }
        else mTransTable.StoreAllNodeFailLow(pos.posStore.hash, -1, scoreToTrans(ss.bestvalue, ss.ply));
    }
    return ss.bestvalue;
}

template <bool inRoot, bool inSplitPoint, bool inSingular>
int Search::searchNode(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt) {
    if (depth <= 0) {
        if (inPvNode(nt)) return qSearch<true>(pos, alpha, beta, 0, ssprev, sthread);
        else return qSearch<false>(pos, alpha, beta, 0, ssprev, sthread);
    }
    else {
        if (ssprev.moveGivesCheck) return searchGeneric<inRoot, inSplitPoint, true, inSingular>(pos, alpha, beta, depth, ssprev, sthread, nt);
        else return searchGeneric<inRoot, inSplitPoint, false, inSingular>(pos, alpha, beta, depth, ssprev, sthread, nt);
    }
}

template<bool inRoot, bool inSplitPoint, bool inCheck, bool inSingular>
int Search::searchGeneric(position_t& pos, int alpha, int beta, const int depth, SearchStack& ssprev, Thread& sthread, NodeType nt) {
    SplitPoint* sp = nullptr;
    SearchStack ss(ssprev.ply + 1);
    pos_store_t undo;

    ASSERT(alpha < beta);
    ASSERT(depth >= 1);
    ASSERT(!inSingular || ssprev.bannedMove != 0);

    initNode(sthread);
    if (sthread.stop) return 0;

    if (!inRoot && !inSingular && !inSplitPoint) {
        int evalDepth = 0;

        alpha = MAX(-INF + ss.ply, alpha);
        beta = MIN(INF - ss.ply - 1, beta);
        if (alpha >= beta) return alpha;

        for (TransEntry *entry = mTransTable.Entry(pos.posStore.hash), *end = entry + mTransTable.BucketSize(); entry != end; ++entry) {
            if (entry->HashLock() == LOCK(pos.posStore.hash)) {
                entry->SetAge(mTransTable.Date());
                if (entry->Mask() & MNoMoves) {
                    if (inCheck) return -INF + ss.ply;
                    else return DrawValue[pos.side];
                }
                if (!inPvNode(nt)) {
                    if ((!inCutNode(nt) || !(entry->Mask() & MAllNodeFailHigh)) && entry->FailHighDepth() >= depth && (entry->Move() != EMPTY || pos.posStore.lastmove == EMPTY)) {
                        int score = scoreFromTrans(entry->FailHighValue(), ss.ply);
                        if (score > alpha) {
                            ss.bestmove = ssprev.counterMove = entry->Move();
                            UpdateHistory(pos, ss, sthread, depth);
                            return score;
                        }
                    }
                    if ((!inAllNode(nt) || !(entry->Mask() & MCutNodeFailLow)) && entry->FailLowDepth() >= depth) {
                        int score = scoreFromTrans(entry->FailLowValue(), ss.ply);
                        ASSERT(valueIsOk(score));
                        if (score < beta) return score;
                    }
                }
                if (entry->Move() != EMPTY && entry->FailHighDepth() > ss.hashDepth) {
                    ss.hashMove = entry->Move();
                    ss.hashDepth = entry->FailHighDepth();
                    ss.singular = (entry->Mask() & MSingular);
                }
                if (entry->FailHighDepth() > evalDepth) {
                    evalDepth = entry->FailHighDepth();
                    ss.evalvalue = scoreFromTrans(entry->FailHighValue(), ss.ply);
                }
                if (entry->FailLowDepth() > evalDepth) {
                    evalDepth = entry->FailLowDepth();
                    ss.evalvalue = scoreFromTrans(entry->FailLowValue(), ss.ply);
                }
            }
        }
        if (ss.evalvalue == -INF) ss.evalvalue = eval(pos, sthread);

        if (ss.ply >= MAXPLY - 1) return ss.evalvalue;
        updateEvalgains(pos, pos.posStore.lastmove, ssprev.evalvalue, ss.evalvalue, sthread);

        if (!inPvNode(nt) && !inCheck) {
            static const int MaxRazorDepth = 10;
            int rvalue;
            if (depth < MaxRazorDepth && (pos.color[pos.side] & ~(pos.pawns | pos.kings)) && beta <= MAXEVAL
                && ss.evalvalue >(rvalue = beta + FutilityMarginTable[depth][MIN(ssprev.playedMoves, 63)])) {
                return rvalue; //consider enforcing a max of MAXEVAL
            }
            if (depth < MaxRazorDepth && pos.posStore.lastmove != EMPTY
                && ss.evalvalue < (rvalue = beta - FutilityMarginTable[depth][MIN(ssprev.playedMoves, 63)])) {
                if (depth <= 2 && ss.evalvalue < rvalue - 100) {
                    return searchNode<false, false, false>(pos, alpha, beta, 0, ssprev, sthread, nt);
                }
                else {
                    int score = searchNode<false, false, false>(pos, rvalue - 1, rvalue, 0, ssprev, sthread, nt);
                    if (score < rvalue) return score;
                }
            }
            if (depth >= 2 && (pos.color[pos.side] & ~(pos.pawns | pos.kings)) && ss.evalvalue >= beta) {
                int nullDepth = depth - (4 + (depth / 5) + MIN(3, ((ss.evalvalue - beta) / PawnValue)));

                makeNullMove(pos, undo);
                ++sthread.nodes;
                int score = -searchNode<false, false, false>(pos, -beta, -beta + 1, nullDepth, ss, sthread, invertNode(nt)); //alpha = beta - 1 because not a PV node
                ss.threatMove = ss.counterMove;
                unmakeNullMove(pos, undo);
                if (sthread.stop) return 0;
                if (score >= beta) {
                    if (depth < 12 && abs(score) <= MAXEVAL) return score;
                    int score2 = searchNode<false, false, false>(pos, alpha, beta, nullDepth, ssprev, sthread, nt); //alpha = beta - 1 because not a PV node
                    if (sthread.stop) return 0;
                    if (score2 >= beta) return score;
                }
            }
        }
        if (!inCheck && !inPvNode(nt) && depth >= 5) { // if we have a no-brainer capture we should just do it
            sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseQuiescence, sthread.ts[ss.ply]); //h109
            move_t* move;
            int target = beta + 200;
            int minCapture = target - ss.evalvalue;
            int newdepth = depth - 5;
            while ((move = sortNext(nullptr, mInfo, pos, *ss.mvlist, sthread)) != nullptr) {
                int score;
                if (MaxPieceValue[moveCapture(move->m)] < minCapture || swap(pos, move->m) < minCapture) continue;
                ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
                makeMove(pos, undo, move->m);
                ++sthread.nodes;
                score = -searchNode<false, false, false>(pos, -target - 1, -target, newdepth, ss, sthread, inCutNode(nt) ? AllNode : CutNode);
                unmakeMove(pos, undo);
                if (sthread.stop) return 0;
                if (score > target) return score;
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
                    ss.hashDepth = newdepth;
                }
            }
        }
    }

    if (inSplitPoint) {
        sp = sthread.activeSplitPoint;
        // copy only necessary values
        ss.bestvalue = sp->sscurr->bestvalue;
        ss.evalvalue = sp->sscurr->evalvalue;
        ss.dcc = sp->sscurr->dcc;
        ss.threatMove = sp->sscurr->threatMove;
        ss.hisMoves = sp->sscurr->hisMoves;
        ss.mvlist = sp->sscurr->mvlist;
    }
    else {
        if (inRoot) {
            ss = ssprev; // this is correct, ss.mvlist points to the ssprev.mvlist, at the same time, ssprev resets other member vars
            if (!mInfo.mvlist_initialized) {
                sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, MoveGenPhaseRoot, sthread.ts[ss.ply]);
            }
            else {
                if (mInfo.multipvIdx > 0) {
                    ss.mvlist->pos = mInfo.multipvIdx - 1;
                    getMove(*ss.mvlist);
                }
                else ss.mvlist->pos = 0;
                ss.mvlist->phase = MoveGenPhaseRoot + 1;
            }
        }
        else {
            ss.dcc = discoveredCheckCandidates(pos, pos.side);
            if (inSingular) { //assumes singular only called when there is a bannedMove
                sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ssprev.bannedMove, alpha, ss.evalvalue, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), sthread.ts[ss.ply]);
                sortNext(sp, mInfo, pos, *ss.mvlist, sthread);
            }
            else sortInit(pos, *ss.mvlist, pinnedPieces(pos, pos.side), ss.hashMove, alpha, ss.evalvalue, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), sthread.ts[ss.ply]);
        }
    }
    int lateMove = LATE_PRUNE_MIN + (inCutNode(nt) ? ((depth * depth) / 2) : (depth * depth));
    move_t* move;
    basic_move_t singularMove = EMPTY;
    while ((move = sortNext(sp, mInfo, pos, *ss.mvlist, sthread)) != nullptr) {
        int score = -INF;
        int newdepth = depth - 1;
        if (inSplitPoint) {
            std::lock_guard<Spinlock> lck(sp->movesplayedlock);
            sp->played += 1;
            ss.playedMoves = sp->played;
            ss.hisCnt = sp->hisCount;
            if (ss.hisCnt < 64 && !moveIsTactical(move->m)) {
                ss.hisMoves[ss.hisCnt] = move->m;
                sp->hisCount += 1;
            }
        }
        else {
            ++ss.playedMoves;
            if (ss.hisCnt < 64 && !moveIsTactical(move->m)) {
                ss.hisMoves[ss.hisCnt++] = move->m;
            }
        }
        if (anyRepNoMove(pos, move->m)) {
            score = DrawValue[pos.side];
        }
        else {
            ss.moveGivesCheck = moveIsCheck(pos, move->m, ss.dcc);
            if (ss.bestvalue == -INF) {
                if (!inRoot && !inSingular && !inSplitPoint) {
                    int exploreDepth = depth / 2;
                    if (inCheck && ss.mvlist->size == 1) newdepth++;
                    else if (ss.hashMove == move->m  && ss.hashDepth >= exploreDepth && depth >= (inPvNode(nt) ? 6 : 8)) {
                        if (ss.singular > 0 && ss.hashDepth >= depth) {
                            singularMove = ss.hashMove;
                            newdepth++;
                            //PrintOutput() << "Gone here 1";
                        }
                        else {
                            int targetScore = ss.evalvalue - EXPLORE_BASE_CUTOFF - depth * EXPLORE_MULT_CUTOFF;
                            ssprev.bannedMove = ss.hashMove;
                            int score = searchNode<false, false, true>(pos, targetScore, targetScore + 1, exploreDepth, ssprev, sthread, nt);
                            ssprev.bannedMove = EMPTY;
                            if (sthread.stop) return 0;
                            if (score <= targetScore) {
                                singularMove = ss.hashMove;
                                newdepth++;
                            }
                            //PrintOutput() << "Gone here 2";
                        }
                        //else PrintOutput() << "Gone here 3";
                    }
                }
                makeMove(pos, undo, move->m);
                ++sthread.nodes;
                score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, ss, sthread, invertNode(nt));
            }
            else {
                //only reduce or prune some types of moves
                int partialReduction = 0;
                if ((move->m != ss.mvlist->killer1) && (move->m != ss.mvlist->killer2) && !moveIsTactical(move->m) && !ss.moveGivesCheck) {
                    if (!inRoot && !inPvNode(nt)) {
                        if (ss.playedMoves > lateMove) continue;
                        int const predictedDepth = MAX(0, newdepth - ReductionTable[1][MIN(depth, 63)][MIN(ss.playedMoves, 63)]);
                        int const gain = sthread.evalgains[historyIndex(pos.side, move->m)];
                        int const scoreAprox = ss.evalvalue + FutilityMarginTable[MIN(predictedDepth, MAX_FUT_MARGIN)][MIN(ss.playedMoves, 63)]
                            + gain;
                        if (scoreAprox < beta) {
                            if (predictedDepth < 8) continue;
                            partialReduction++;
                        }
                        if (swap(pos, move->m) < 0) {
                            if (predictedDepth < 2) continue;
                            partialReduction++;
                        }
                    }
                    if (depth >= MIN_REDUCTION_DEPTH) {
                        bool skipFutility = (inCheck || (ss.threatMove && moveRefutesThreat(pos, move->m, ss.threatMove)) || moveIsDangerousPawn(pos, move->m));
                        int reduction = ReductionTable[(inPvNode(nt) ? 0 : 1)][MIN(depth, 63)][MIN(ss.playedMoves, 63)];
                        partialReduction += skipFutility ? (reduction + 1) / 2 : reduction;
                    }
                }
                int newdepthclone = newdepth - partialReduction;
                makeMove(pos, undo, move->m);
                ++sthread.nodes;
                if (inSplitPoint) alpha = sp->alpha;
                ss.reducedMove = (newdepthclone < newdepth);
                score = -searchNode<false, false, false>(pos, -alpha - 1, -alpha, newdepthclone, ss, sthread, inCutNode(nt) ? AllNode : CutNode);
                if (!sthread.stop && ss.reducedMove && score > alpha) {
                    if (partialReduction >= 4) {
                        newdepthclone = newdepth - partialReduction / 2;
                        score = -searchNode<false, false, false>(pos, -alpha - 1, -alpha, newdepthclone, ss, sthread, inCutNode(nt) ? AllNode : CutNode);
                    }
                    if (!sthread.stop && score > alpha) {
                        ss.reducedMove = false;
                        score = -searchNode<false, false, false>(pos, -alpha - 1, -alpha, newdepth, ss, sthread, AllNode);
                    }
                }
                if (inPvNode(nt) && !sthread.stop && score > alpha) {
                    if (inRoot) mInfo.research = 1;
                    score = -searchNode<false, false, false>(pos, -beta, -alpha, newdepth, ss, sthread, PVNode);
                }
            }
            unmakeMove(pos, undo);
        }
        if (sthread.stop) return 0;
        if (inSplitPoint) sp->updatelock.lock();
        if (inRoot) {
            move->s = score;
        }
        if (score > (inSplitPoint ? sp->bestvalue : ss.bestvalue)) {
            if (inRoot) mInfo.best_value = score;
            if (inSplitPoint) sp->bestvalue = score;
            ss.bestvalue = score;
            if (ss.bestvalue > (inSplitPoint ? sp->alpha : alpha)) {
                if (inRoot) {
                    if (mInfo.iteration > 1 && mInfo.bestmove != move->m) mInfo.change = 1;
                    mInfo.bestmove = move->m;
                }
                if (inSplitPoint) sp->bestmove = move->m;
                ss.bestmove = move->m;
                if (inPvNode(nt) && ss.bestvalue < beta) {
                    if (inSplitPoint) sp->alpha = ss.bestvalue;
                    alpha = ss.bestvalue;
                }
                else {
                    if (inRoot) {
                        if (inSplitPoint) sp->movelistlock.lock();
                        for (int i = ss.mvlist->pos; i < ss.mvlist->size; ++i) ss.mvlist->list[i].s = -INF;
                        if (inSplitPoint) sp->movelistlock.unlock();
                    }
                    if (inSplitPoint) {
                        sp->cutoff = true;
                        sp->updatelock.unlock();
                    }
                    break;
                }
            }
        }
        if (inSplitPoint) sp->updatelock.unlock();
        if (!inSplitPoint && !inSingular && !sthread.stop && !inCheck && sthread.num_sp < mInfo.mMaxActiveSplitsPerThread
            && mEngine.ThreadNum() > 1 && depth >= mInfo.mMinSplitDepth
            && (!sthread.activeSplitPoint || !sthread.activeSplitPoint->workAvailable
            || ((sthread.activeSplitPoint->depth - depth <= 1) && sthread.num_sp < 2))) {
            sthread.SearchSplitPoint(pos, &ss, &ssprev, alpha, beta, nt, depth, inCheck, inRoot);
            if (sthread.stop) return 0;
            break;
        }
    }
    if (inRoot && !inSplitPoint) {
        if (ss.playedMoves == 0) {
            if (inCheck) return -INF;
            else return DrawValue[pos.side];
        }
    }
    if (!inRoot && !inSplitPoint && !inSingular) {
        if (ss.playedMoves == 0) {
            if (inCheck) ss.bestvalue = -INF + ss.ply;
            else ss.bestvalue = DrawValue[pos.side];
            mTransTable.StoreNoMoves(pos.posStore.hash, depth, scoreToTrans(ss.bestvalue, ss.ply));
            return ss.bestvalue;
        }
        if (ss.bestvalue >= beta) {
            ssprev.counterMove = ss.bestmove;
            UpdateHistory(pos, ss, sthread, depth);
            if (inCutNode(nt)) mTransTable.StoreCutNodeFailHigh(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, ss.ply), bool(singularMove == ss.bestmove));
            else mTransTable.StoreAllNodeFailHigh(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, ss.ply), bool(singularMove == ss.bestmove));
        }
        else {
            if (inPvNode(nt) && ss.bestmove != EMPTY) {
                ssprev.counterMove = ss.bestmove;
                mTransTable.StorePVNode(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, ss.ply), bool(singularMove == ss.bestmove));
                mPVHashTable.pvStore(pos.posStore.hash, ss.bestmove, depth, scoreToTrans(ss.bestvalue, ss.ply));
            }
            else if (inCutNode(nt)) mTransTable.StoreCutNodeFailLow(pos.posStore.hash, depth, scoreToTrans(ss.bestvalue, ss.ply));
            else mTransTable.StoreAllNodeFailLow(pos.posStore.hash, depth, scoreToTrans(ss.bestvalue, ss.ply));
        }
    }
    return ss.bestvalue;
}

/**************************************************************************************************/
/**************************************************************************************************/

Engine::Engine() {
    mThinking = false;
    search = new Search(*this, info, transtable, pvhashtable);
    mTimerThread = new TimerThread(std::bind(&Engine::CheckTime, this));

    InitUCIOptions();
    SetNumThreads(uci_opt[ThreadsStr].GetInt());
    InitVars();
    InitTTHash(uci_opt[HashStr].GetInt());
    InitPVTTHash(1);
}

Engine::~Engine() {
    delete search;
    SetNumThreads(0);
    delete mTimerThread;
}

void Engine::StopSearch() {
    info.stop_search = true;
    SetAllThreadsToStop();
}

void Engine::PonderHit() { //no pondering in tuning
    info.thinking_status = THINKING;
    LogInfo() << "info string Switch from pondering to thinking";
}

void Engine::SearchFromIdleLoop(SplitPoint& sp, Thread& sthread) {
    position_t pos = *sp.origpos;
    if (sp.inRoot) search->searchNode<true, true, false>(pos, sp.alpha, sp.beta, sp.depth, *sp.ssprev, sthread, sp.nodeType);
    else search->searchNode<false, true, false>(pos, sp.alpha, sp.beta, sp.depth, *sp.ssprev, sthread, sp.nodeType);
}

// TODO: encapsulate
bool isLegal(const position_t& pos, const  basic_move_t move, const bool inCheck) { //this ensures perfect legality for moves that might actually be chosen by the engine
    if (!genMoveIfLegal(pos, move, pinnedPieces(pos, pos.side))) return false;
    if (inCheck && movePiece(move) != KING) { //MoveIfLegal handles all king moves pretty well already
        int ksq = pos.kpos[pos.side];
        uint64 checkers = attackingPiecesSide(pos, ksq, pos.side ^ 1);
        if (MinTwoBits(checkers)) return false; //need to move king if two checkers
        int sqchecker = getFirstBit(checkers);
        if (!((InBetween[sqchecker][ksq] | checkers) & BitMask[moveTo(move)])) return false;
    }
    return true;
}
void Engine::ExtractPvMovesFromHash(position_t& pos, continuation_t& pv, basic_move_t move) {
    PvHashEntry *entry;
    pos_store_t undo[MAXPLY];
    int ply = 0;
    pv.length = 0;
    pv.moves[pv.length++] = move;
    makeMove(pos, undo[ply++], move);
    while ((entry = pvhashtable.pvEntry(pos.posStore.hash)) != nullptr) {
        basic_move_t hashMove = entry->pvMove();
        if (hashMove == EMPTY) break;
        if (!isLegal(pos, hashMove, kingIsInCheck(pos))) break;
        pv.moves[pv.length++] = hashMove;
        if (anyRep(pos)) break; // break on repetition to avoid long pv display
        makeMove(pos, undo[ply++], hashMove);
        if (ply >= MAXPLY) break;
    }
    for (ply = ply - 1; ply >= 0; --ply) {
        unmakeMove(pos, undo[ply]);
    }
}

void Engine::RepopulateHash(position_t& pos, continuation_t& rootPV) {
    int moveOn;
    pos_store_t undo[MAXPLY];
    for (moveOn = 0; moveOn + 1 <= rootPV.length; moveOn++) {
        basic_move_t move = rootPV.moves[moveOn];
        if (!move) break;
        PvHashEntry* entry = pvhashtable.pvEntryFromMove(pos.posStore.hash, move);
        if (nullptr == entry) break;
        transtable.StorePVNode(pos.posStore.hash, entry->pvMove(), entry->pvDepth(), entry->pvScore(), false);
        makeMove(pos, undo[moveOn], move);
    }
    for (moveOn = moveOn - 1; moveOn >= 0; moveOn--) {
        unmakeMove(pos, undo[moveOn]);
    }
}

void Engine::DisplayPV(continuation_t& pv, int multipvIdx, int depth, int alpha, int beta, int score) {
    uint64 time;
    uint64 sumnodes = 0;
    PrintOutput log;
    int scoreToDisplay = (score * 100) / PawnValueEnd;
    time = getTime();
    info.last_time = time;
    time = info.last_time - info.start_time;

    log << "info depth " << depth;
    if (abs(score) < (INF - MAXPLY)) {
        if (score < beta) {
            if (score <= alpha) log << " score cp " << scoreToDisplay << " upperbound";
            else log << " score cp " << scoreToDisplay;
        }
        else log << " score cp " << scoreToDisplay << " lowerbound";
    }
    else {
        log << " score mate " << ((score > 0) ? (INF - score + 1) / 2 : -(INF + score) / 2);
    }

    log << " time " << time;
    sumnodes = ComputeNodes();
    log << " nodes " << sumnodes;
    log << " hashfull " << (transtable.Used() * 1000) / transtable.HashSize();
    if (time > 10) log << " nps " << (sumnodes * 1000) / (time);

    if (multipvIdx > 0) log << " multipv " << multipvIdx + 1;
    log << " pv ";
    for (int i = 0; i < pv.length; i++) log << move2Str(pv.moves[i]) << " ";
}

void Engine::TimeManagement(int depth) {
    static const int WORSE_TIME_BONUS = 20; //how many points more than 20 it takes to increase time by alloc to a maximum of 2*alloc
    static const int CHANGE_TIME_BONUS = 50; //what percentage of alloc to increase if the last move is a change move
    static const int EASYMOVE_TIME_CUTOFF = 30;
    static const int EXTEND_OR_STOP_TIME_CUTOFF = 65;

    if (info.best_value > INF - MAXPLY) info.mate_found++;
    if (info.thinking_status == THINKING && info.time_is_limited) {
        int64 timeElapsed = getTime();
        if (info.legalmoves == 1 || info.mate_found >= 3) {
            if (depth >= 8) {
                StopSearch();
                LogInfo() << "info string Aborting search: legalmove/mate found depth >= 8";
                return;
            }
        }
        bool easymovecutoff = false;
        bool extendcutoff = false;
        if ((info.legalmoves < 3 || info.is_easymove) && timeElapsed >(info.start_time + (((info.time_limit_max - info.start_time) * EASYMOVE_TIME_CUTOFF) / 100))) easymovecutoff = true;
        else if (timeElapsed > (info.start_time + (((info.time_limit_max - info.start_time) * EXTEND_OR_STOP_TIME_CUTOFF) / 100))) extendcutoff = true;
        if (easymovecutoff || extendcutoff) {
            int64 addTime = 0;
            if (timeElapsed < info.time_limit_abs) {
                if ((info.best_value + WORSE_SCORE_CUTOFF) <= info.last_value) {
                    int amountWorse = info.last_value - info.best_value;
                    addTime += ((amountWorse - (WORSE_SCORE_CUTOFF - 10)) * info.alloc_time) / WORSE_TIME_BONUS;
                    LogInfo() << "info string Extending time (root gettingWorse): " << addTime;
                }
                if (info.change) {
                    addTime += (info.alloc_time * CHANGE_TIME_BONUS) / 100;
                    LogInfo() << "info string Extending time (root change): " << addTime;
                }
            }
            if (addTime <= 0) { // if we are unlikely to get deeper, save our time
                StopSearch();
                if (easymovecutoff) {
                    info.easymoves.Reset(); // no 2 consecutive easy moves
                    LogAndPrintOutput() << "info string Aborting search: Easymove: " << timeElapsed - info.start_time;
                }
                else LogInfo() << "info string Aborting search: root time limit 1: " << timeElapsed - info.start_time;
                return;
            }
            else {
                if (extendcutoff) {
                    info.time_limit_max += addTime;
                    if (info.time_limit_max > info.time_limit_abs)
                        info.time_limit_max = info.time_limit_abs;
                }
            }
        }
    }
    if (info.depth_is_limited && depth >= info.depth_limit) {
        StopSearch();
        LogInfo() << "info string Aborting search: depth limit: " << depth;
        return;
    }
}

void Engine::CheckTime() {
    int64 time2 = getTime();
    if (time2 - info.last_time > 1000) {
        int64 time = time2 - info.start_time;
        info.last_time = time2;
        if (SHOW_SEARCH) {
            uint64 sumnodes = ComputeNodes();
            PrintOutput() << "info time " << time
                << " nodes " << sumnodes
                << " hashfull " << (transtable.Used() * 1000) / transtable.HashSize()
                << " nps " << (sumnodes * 1000ULL) / (time);
        }
    }
    if (info.thinking_status == THINKING && info.time_is_limited) {
        if (time2 > info.time_limit_max) {
            if (time2 < info.time_limit_abs) {
                if (!info.research && !info.change) {
                    bool gettingWorse = info.best_value != -INF && info.best_value + WORSE_SCORE_CUTOFF <= info.last_value;
                    if (!gettingWorse) {
                        StopSearch();
                        LogInfo() << "info string Aborting search: time limit 2: " << time2 - info.start_time;
                    }
                }
            }
            else {
                StopSearch();
                LogInfo() << "info string Aborting search: time limit 1: " << time2 - info.start_time;
            }
        }
    }
    if (info.thinking_status == THINKING && info.time_is_fixed) {
        if (time2 > info.time_limit_max) {
            StopSearch();
            LogInfo() << "info string Aborting search: fixed time: " << time2 - info.start_time;
        }
    }
}

void Engine::SendBestMove() {
    if (!info.bestmove) {
        LogAndPrintOutput() << "info string No legal move found. Start a new game.";
    }
    else {
        LogAndPrintOutput log;
        log << "bestmove " << move2Str(info.bestmove);
        if (info.rootPV.moves[0] == info.bestmove && info.rootPV.length > 1 && info.rootPV.moves[1])
            log << " ponder " << move2Str(info.rootPV.moves[1]);
    }
    //PrintThreadStats();
    SetThinkFinished();
}

void Engine::GetBestMove(Thread& sthread) {
    int id;
    int alpha, beta;
    SearchStack ss(0);
    SplitPoint rootsp;
    ss.moveGivesCheck = kingIsInCheck(rootpos);
    ss.dcc = discoveredCheckCandidates(rootpos, rootpos.side);

    transtable.NewDate(transtable.Date());
    pvhashtable.NewDate(pvhashtable.Date());

    if (info.thinking_status == THINKING && uci_opt[OwnBookStr].GetInt() && !anyRep(rootpos)) {
        if ((info.bestmove = mPolyBook.getBookMove(rootpos)) != EMPTY) {
            info.rootPV.length = 0;
            StopSearch();
            SendBestMove();
            return;
        }
    }
#ifdef EVAL_DEBUG
    SHOW_EVAL = true;
    int tscore = eval(rootpos, sthread);
    SHOW_EVAL = false;
    PrintOutput() << "info string score = " << tscore << "\n";
#endif
    PvHashEntry *entry = pvhashtable.pvEntry(rootpos.posStore.hash);
    if (nullptr != entry && entry->pvMove() && isLegal(rootpos, entry->pvMove(), ss.moveGivesCheck)) {
        ss.hashMove = entry->pvMove();
        bool easycapture = ((moveCapture(rootpos.posStore.lastmove) && (moveTo(rootpos.posStore.lastmove) == moveTo(ss.hashMove)))
            || (PcValSEE[moveCapture(ss.hashMove)] > PcValSEE[movePiece(ss.hashMove)]));
        bool singularmove = false;
        for (TransEntry *hentry = transtable.Entry(rootpos.posStore.hash), *end = hentry + transtable.BucketSize(); hentry != end; ++hentry) {
            if (hentry->HashLock() == LOCK(rootpos.posStore.hash)) {
                if (ss.hashMove == hentry->Move() && (hentry->Mask() & MSingular)) {
                    singularmove = true;
                }
            }
        }
        bool expectedmove = (info.easymoves.m[1] == rootpos.posStore.lastmove && info.easymoves.m[2] == ss.hashMove);
        if (info.thinking_status == THINKING && info.time_is_limited && (easycapture || singularmove) && expectedmove) {
            info.bestmove = ss.hashMove;
            info.best_value = entry->pvScore();
            info.easymoves.Reset(); // no 2 consecutive instant moves
            ExtractPvMovesFromHash(rootpos, info.rootPV, ss.hashMove);
            DisplayPV(info.rootPV, info.multipvIdx, entry->pvDepth(), -INF, INF, info.best_value);
            StopSearch();
            SendBestMove();
            PrintOutput() << "info string Easy move at the root!!!";
            return;
        }
        if (easycapture || singularmove || expectedmove) info.is_easymove = true;
    }

    // extend time when there is no hashmove from hashtable, this is useful when just out of the book
    if (ss.hashMove == EMPTY || (info.easymoves.m[1] != rootpos.posStore.lastmove)) {
        info.time_limit_max += info.alloc_time / 4; // 25%
        if (info.time_limit_max > info.time_limit_abs)
            info.time_limit_max = info.time_limit_abs;
    }

    // easy moves
    if (!info.pondering) info.easymoves.Reset();

    // SMP
    InitVars();
    SetAllThreadsToWork();
    for (id = 1; id < MAXPLY; id++) {
        const int AspirationWindow = 12;
        int faillow = 0, failhigh = 0;
        info.iteration = id;
        info.best_value = -INF;
        info.change = 0;
        info.research = 0;
        for (info.multipvIdx = 0; info.multipvIdx < info.multipv; ++info.multipvIdx) {
            if (id < 6) {
                alpha = -INF;
                beta = INF;
            }
            else {
                alpha = info.last_value - AspirationWindow;
                beta = info.last_value + AspirationWindow;
                if (alpha < -QueenValue) alpha = -INF;
                if (beta > QueenValue) beta = INF;
            }
            while (true) {
                search->searchNode<true, false, false>(rootpos, alpha, beta, id, ss, sthread, PVNode);

                if (!info.stop_search || info.best_value != -INF) {
                    if (info.best_value > alpha && info.best_value < beta) {
                        ExtractPvMovesFromHash(rootpos, info.rootPV, info.bestmove);
                        RepopulateHash(rootpos, info.rootPV);
                    }
                    if (SHOW_SEARCH && id >= 8) {
                        DisplayPV(info.rootPV, info.multipvIdx, id, alpha, beta, info.best_value);
                    }
                }
                if (info.stop_search) break;
                if (info.best_value <= alpha) {
                    beta = (alpha + beta) / 2;
                    alpha = info.best_value - (AspirationWindow << ++faillow);
                    if (alpha < -QueenValue) alpha = -INF;
                    info.research = 1;
                }
                else if (info.best_value >= beta) {
                    alpha = (alpha + beta) / 2;
                    beta = info.best_value + (AspirationWindow << ++failhigh);
                    if (beta > QueenValue) beta = INF;
                    info.research = 1;
                }
                else break;
            }
            if (info.stop_search) break;
        }
        if (!info.pondering) info.easymoves.Update(info.rootPV);
        if (!info.stop_search) TimeManagement(id);
        if (info.stop_search) break;
        if (info.best_value != -INF) info.last_value = info.best_value;
    }

    // easy moves
    if (!info.pondering && info.easymoves.cnt < 3)
        info.easymoves.Reset();

    if (!info.stop_search) {
        if ((info.depth_is_limited || info.time_is_limited) && info.thinking_status == THINKING) {
            LogInfo() << "info string Aborting search: end of GetBestMove: id = " << id << ", best_value = " << info.best_value << " ply = " << ss.ply;
        }
        else {
            LogAndPrintInfo() << "info string Waiting for stop, quit, or PonderHit";
            while (!info.stop_search && info.thinking_status != THINKING);
            LogInfo() << "info string Aborting search: end of waiting for stop/quit/PonderHit";
        }
        StopSearch();
    }

    SendBestMove();
}
void Engine::StartThinking(GoCmdData& data, position_t& pos) {
    WaitForThink();
    SetThinkStarted();

    rootpos = pos;
    info.Init();

    info.time_buffer = uci_opt[TimeBufferStr].GetInt();
    info.contempt = uci_opt[ContemptStr].GetInt();
    info.multipv = uci_opt[MultiPVStr].GetInt();
    info.mMinSplitDepth = uci_opt[MinSplitDepthStr].GetInt();
    info.mMaxActiveSplitsPerThread = uci_opt[ActiveSplitsStr].GetInt();

    int mytime = 0, t_inc = 0;
    if (data.infinite) {
        info.depth_is_limited = true;
        info.depth_limit = MAXPLY;
        LogInfo() << "info string Infinite";
    }
    if (data.depth > 0) {
        info.depth_is_limited = true;
        info.depth_limit = data.depth;
        LogInfo() << "info string Depth is limited to " << info.depth_limit << " half moves";
    }
    if (data.mate > 0) {
        info.depth_is_limited = true;
        info.depth_limit = data.mate * 2 - 1;
        LogInfo() << "info string Mate in " << info.depth_limit << " half moves";
    }
    if (data.nodes > 0) {
        info.node_is_limited = true;
        info.node_limit = data.nodes;
        LogInfo() << "info string Nodes is limited to " << info.node_limit << " positions";
    }
    if (info.moves[0]) {
        info.moves_is_limited = true;
        LogInfo() << "info string Moves is limited";
    }

    if (data.movetime > 0) {
        info.time_is_fixed = true;
        info.alloc_time = data.movetime;
        info.time_limit_max = info.start_time + data.movetime;
        info.time_limit_abs = info.start_time + data.movetime;
        LogInfo() << "info string Fixed time per move: " << data.movetime << " ms";
    }
    if (rootpos.side == WHITE) {
        mytime = data.wtime;
        t_inc = data.winc;
    }
    else {
        mytime = data.btime;
        t_inc = data.binc;
    }
    if (mytime > 0) {
        info.time_is_limited = true;
        mytime -= info.time_buffer;
        if (mytime < 0) mytime = 0;
        if (data.movestogo <= 0 || data.movestogo > 30) data.movestogo = 30;
        info.time_limit_max = (mytime / data.movestogo) + ((t_inc * 4) / 5);
        if (data.ponder) {
            info.pondering = true;
            info.time_limit_max += info.time_limit_max / 4;
        }

        if (info.time_limit_max > mytime) info.time_limit_max = mytime;
        info.time_limit_abs = ((mytime * 3) / 10) + ((t_inc * 4) / 5);
        if (info.time_limit_abs < info.time_limit_max) info.time_limit_abs = info.time_limit_max;
        if (info.time_limit_abs > mytime) info.time_limit_abs = mytime;

        if (info.time_limit_max < 2) info.time_limit_max = 2;
        if (info.time_limit_abs < 2) info.time_limit_abs = 2;

        LogInfo() << "info string Time is limited: ";
        LogInfo() << "max = " << info.time_limit_max;
        LogInfo() << "abs = " << info.time_limit_abs;
        info.alloc_time = info.time_limit_max;
        info.time_limit_max += info.start_time;
        info.time_limit_abs += info.start_time;
    }
    if (data.infinite) {
        info.thinking_status = ANALYSING;
        LogInfo() << "info string Search status is ANALYSING";
    }
    else if (data.ponder) {
        info.thinking_status = PONDERING;
        LogInfo() << "info string Search status is PONDERING";
    }
    else {
        info.thinking_status = THINKING;
        LogInfo() << "info string Search status is THINKING";
    }

    DrawValue[rootpos.side] = -info.contempt;
    DrawValue[rootpos.side ^ 1] = info.contempt;

    ThreadFromIdx(0).TriggerCondition();
}