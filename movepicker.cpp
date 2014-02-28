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
#include "attacks.h"
#include "movegen.h"
#include "threads.h"
#include "movepicker.h"
#include "search.h"

void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, Thread& sthread) {
    mvlist->transmove =  hashmove;
    mvlist->killer1 = sthread.ts[pos->ply].killer1;
    mvlist->killer2 = sthread.ts[pos->ply].killer2;
    mvlist->evalvalue = eval;
    mvlist->pos = 0;
    mvlist->size = 0;
    mvlist->pinned = pinned;
    mvlist->scout = scout;
    mvlist->ply = pos->ply;
    mvlist->side = pos->side;
    mvlist->depth = depth;
    mvlist->phase = type;
    mvlist->startBad = MAXMOVES;
}

move_t* getMove(movelist_t *mvlist) {
    move_t *best, *temp, *start, *end;

    ASSERT(mvlist != NULL);

    if (mvlist->pos >= mvlist->size) return NULL;
    start = &mvlist->list[mvlist->pos++];
    end = &mvlist->list[mvlist->size];
    best = start;
    for (temp = start+1; temp < end; temp++) {
        if (temp->s > best->s) best = temp;
    }
    if (best == start) return start;
    *temp = *start;
    *start = *best;
    *best = *temp;
    return start;
}
inline int scoreNonTactical(uint32 side, uint32 move) {
    int score = SearchManager.info.history[historyIndex(side,move)];
    return score;
}
bool moveIsPassedPawn(const position_t * pos, uint32 move) {
    if (movePiece(move) == PAWN && !((*FillPtr[pos->side])(BitMask[moveTo(move)]) & pos->pawns)) {
        if (!(pos->pawns & pos->color[pos->side^1] & PassedMask[pos->side][moveTo(move)])) return true;
    }
    return false;
}

uint32 captureIsGood(const position_t *pos, const basic_move_t m) {
    uint32 prom = movePromote(m);
    uint32 capt = moveCapture(m);
    uint32 pc = movePiece(m);

    ASSERT(pos != NULL);
    ASSERT(moveIsOk(m));
    ASSERT(moveIsTactical(m));

#ifdef DEBUG
    if (!moveIsTactical(m)) Print(8, "%s: pc: %d, cap: %d, prom: %d\n", __FUNCTION__, pc, capt, prom);
#endif
    if (prom != EMPTY && prom != QUEEN) return false;
    if (capt != EMPTY) {
        if (prom != EMPTY) return true;
        if (capt >= pc) return true;
    }
    return (swap(pos, m) >= 0);
}


void scoreCapturesPure(movelist_t *mvlist) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        m->s = (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
    }
}

void scoreCaptures(movelist_t *mvlist) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        m->s = (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
        if (m->m == mvlist->transmove) m->s += MAXHIST;
    }
}

void scoreNonCaptures(const position_t *pos, movelist_t *mvlist) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        m->s = scoreNonTactical(mvlist->side, m->m);
    }
}

void scoreAll(const position_t *pos, movelist_t *mvlist) {
    move_t *m;

    ASSERT(pos != NULL);
    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        if (m->m == mvlist->transmove) m->s = MAXHIST * 3;
        else if (moveIsTactical(m->m)) {
            m->s = (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
            if (captureIsGood(pos, m->m)) m->s += MAXHIST * 2;
            else m->s -= MAXHIST;
        }
        else if (m->m == mvlist->killer1) m->s = MAXHIST + 4;
        else if (m->m == mvlist->killer2) m->s = MAXHIST + 2;
        else {
            m->s = scoreNonTactical(mvlist->side, m->m);
        }
    }
}

void scoreAllQ(movelist_t *mvlist) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
        else m->s = scoreNonTactical(mvlist->side, m->m);
    }
}

void scoreRoot(movelist_t *mvlist) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        if (m->m == mvlist->transmove) m->s = MAXHIST * 3;
        else if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
        else m-> s = 0;
    }
}

move_t* sortNext(SplitPoint* sp, position_t *pos, SearchStack& ss, int thread_id) {
    move_t* move;
    if (sp != NULL) sp->movelistlock->lock();
    ss.mvlist_phase = ss.mvlist->phase;
    if (MoveGenPhase[ss.mvlist->phase] == PH_END) {  // SMP hack
        if (sp != NULL) sp->movelistlock->unlock();
        return NULL;
    }
    while (true) {
        while (ss.mvlist->pos < ss.mvlist->size) {
            move = getMove(ss.mvlist);
            switch (MoveGenPhase[ss.mvlist->phase]) {
            case PH_ROOT:
                break;
            case PH_EVASION:
                ASSERT(kingIsInCheck(pos));
                break;
            case PH_TRANS:
                if (!genMoveIfLegal(pos, move->m, ss.mvlist->pinned)) {
                    continue;
                }
                if (ss.mvlist->depth <= 0 && !moveIsTactical(ss.mvlist->transmove)) { // TODO: test this
                    ss.mvlist->transmove = EMPTY;
                    continue;
                }
                break;
            case PH_ALL_CAPTURES:
                if (move->m == ss.mvlist->transmove) continue;
            case PH_GOOD_CAPTURES:
                if (move->m == ss.mvlist->transmove) continue;
                if (!captureIsGood(pos, move->m)) {
                    ss.mvlist->list[--(ss.mvlist->startBad)].m = move->m;
                    continue;
                }
                if (!moveIsLegal(pos, move->m, ss.mvlist->pinned, false)) continue;
                break;
            case PH_GOOD_CAPTURES_PURE:
                if (move->m == ss.mvlist->transmove) continue;
                if (!captureIsGood(pos, move->m)) continue;
                if (!moveIsLegal(pos, move->m, ss.mvlist->pinned, false)) continue;
                break;
            case PH_BAD_CAPTURES:
                if (!moveIsLegal(pos, move->m, ss.mvlist->pinned, false)) continue;
                break;
            case PH_KILLER_MOVES:
                if (move->m == ss.mvlist->transmove) continue;
                if (!genMoveIfLegal(pos, move->m, ss.mvlist->pinned)) continue;
                break;
            case PH_QUIET_MOVES:
                if (move->m == ss.mvlist->transmove) continue;
                if (move->m == ss.mvlist->killer1) continue;
                if (move->m == ss.mvlist->killer2) continue;
                if (!moveIsLegal(pos, move->m, ss.mvlist->pinned, false)) continue;
                break;
            case PH_NONTACTICAL_CHECKS_WIN:
                if (swap(pos, move->m) < 0) continue;
            case PH_NONTACTICAL_CHECKS_PURE:
                if (move->m == ss.mvlist->transmove) continue;
                if (!moveIsLegal(pos, move->m, ss.mvlist->pinned, false)) continue;
                break;
            case PH_GAINING:
                if (move->m == ss.mvlist->transmove) continue;
                if (moveIsCheck(pos, move->m, discoveredCheckCandidates(pos, pos->side))) continue;
                if (!moveIsLegal(pos, move->m, ss.mvlist->pinned, false)) continue;
                break;
            default:
                // can't get here
                if (sp != NULL) sp->movelistlock->unlock();
                return NULL;
            }
            if (sp != NULL) sp->movelistlock->unlock();
            return move;
        }

        ss.mvlist->phase++;
        ss.mvlist_phase = ss.mvlist->phase;

        switch (MoveGenPhase[ss.mvlist->phase]) {
        case PH_ROOT:
            if (SearchManager.info.mvlist_initialized) break;
            if (SearchManager.info.moves_is_limited == true) {
                for (ss.mvlist->size = 0; SearchManager.info.moves[ss.mvlist->size] != EMPTY; ss.mvlist->size++) {
                    ss.mvlist->list[ss.mvlist->size].m = SearchManager.info.moves[ss.mvlist->size];
                }
            } else {
                // generate all legal moves at least in the root
                genLegal(pos, ss.mvlist, true); 
            }
            scoreRoot(ss.mvlist);
            SearchManager.info.mvlist_initialized = true;
            SearchManager.info.legalmoves = ss.mvlist->size;
            break;
        case PH_EVASION:
            genEvasions(pos, ss.mvlist);
            if (ss.mvlist->depth <= 0) scoreAllQ(ss.mvlist);
            else scoreAll(pos, ss.mvlist);
            break;
        case PH_TRANS:
            if (ss.mvlist->transmove != EMPTY) {
                ss.mvlist->list[ss.mvlist->size++].m = ss.mvlist->transmove;
            }
            break;
        case PH_ALL_CAPTURES:
            genCaptures(pos, ss.mvlist);
            scoreCapturesPure(ss.mvlist);
            break;
        case PH_GOOD_CAPTURES:
        case PH_GOOD_CAPTURES_PURE:
            genCaptures(pos, ss.mvlist);
            scoreCapturesPure(ss.mvlist);
            break;
        case PH_BAD_CAPTURES:
            for (int i = MAXMOVES-1; i >= ss.mvlist->startBad; --i) {
                ss.mvlist->list[ss.mvlist->size] = ss.mvlist->list[i];
                ss.mvlist->size++;
            }
            break;
        case PH_KILLER_MOVES:
            if (ss.mvlist->killer1 != EMPTY) {
                ss.mvlist->list[ss.mvlist->size].m = ss.mvlist->killer1;
                ss.mvlist->list[ss.mvlist->size].s = MAXHIST;
                ss.mvlist->size++;
            }
            if (ss.mvlist->killer2 != EMPTY) {
                ss.mvlist->list[ss.mvlist->size].m = ss.mvlist->killer2;
                ss.mvlist->list[ss.mvlist->size].s = MAXHIST-1;
                ss.mvlist->size++;
            }
            break;
        case PH_QUIET_MOVES:
            genNonCaptures(pos, ss.mvlist);
            scoreNonCaptures(pos, ss.mvlist);
            break;
        case PH_NONTACTICAL_CHECKS_WIN:
        case PH_NONTACTICAL_CHECKS_PURE:
            genQChecks(pos, ss.mvlist);
            scoreNonCaptures(pos, ss.mvlist);
            break;
        case PH_GAINING:
            if (ss.mvlist->scout - ss.mvlist->evalvalue > 150) continue; // TODO: test other values
            genGainingMoves(pos, ss.mvlist, ss.mvlist->scout - ss.mvlist->evalvalue, thread_id);
            // Print(3, "delta = %d, ss.mvlist->size = %d\n", ss.mvlist->scout - SearchInfo.evalvalue[pos->ply], ss.mvlist->size);
            scoreNonCaptures(pos, ss.mvlist);
            break;
        default:
            ASSERT(MoveGenPhase[ss.mvlist->phase] == PH_END);
            if (sp != NULL) sp->movelistlock->unlock();
            return NULL;
        }
    }
}


