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

extern int moveIsTactical(uint32 m);
extern int historyIndex(uint32 side, uint32 move);

void sortInit(const position_t& pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, Thread& sthread) {
    mvlist->transmove = hashmove;
    mvlist->killer1 = sthread.ts[pos.ply].killer1;
    mvlist->killer2 = sthread.ts[pos.ply].killer2;
    mvlist->evalvalue = eval;
    mvlist->pos = 0;
    mvlist->size = 0;
    mvlist->pinned = pinned;
    mvlist->scout = scout;
    mvlist->ply = pos.ply;
    mvlist->side = pos.side;
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
    for (temp = start + 1; temp < end; temp++) {
        if (temp->s > best->s) best = temp;
    }
    if (best == start) return start;
    *temp = *start;
    *start = *best;
    *best = *temp;
    return start;
}
inline int scoreNonTactical(uint32 side, uint32 move, Thread& sthread) {
    int score = sthread.history[historyIndex(side, move)];
    return score;
}
bool moveIsPassedPawn(const position_t& pos, uint32 move) {
    if (movePiece(move) == PAWN && !((*FillPtr[pos.side])(BitMask[moveTo(move)]) & pos.pawns)) {
        if (!(pos.pawns & pos.color[pos.side ^ 1] & PassedMask[pos.side][moveTo(move)])) return true;
    }
    return false;
}

uint32 captureIsGood(const position_t& pos, const basic_move_t m) {
    uint32 prom = movePromote(m);
    uint32 capt = moveCapture(m);
    uint32 pc = movePiece(m);

    ASSERT(moveIsOk(m));
    ASSERT(moveIsTactical(m));

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

void scoreNonCaptures(const position_t& pos, movelist_t *mvlist, Thread& sthread) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        m->s = scoreNonTactical(mvlist->side, m->m, sthread);
    }
}

void scoreAll(const position_t& pos, movelist_t *mvlist, Thread& sthread) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        if (m->m == mvlist->transmove) m->s = MAXHIST * 3;
        else if (moveIsTactical(m->m)) {
            m->s = (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
            if (captureIsGood(pos, m->m)) m->s += MAXHIST * 2;
            else m->s -= MAXHIST;
        } else if (m->m == mvlist->killer1) m->s = MAXHIST + 4;
        else if (m->m == mvlist->killer2) m->s = MAXHIST + 2;
        else {
            m->s = scoreNonTactical(mvlist->side, m->m, sthread);
        }
    }
}

void scoreAllQ(movelist_t *mvlist, Thread& sthread) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
        else m->s = scoreNonTactical(mvlist->side, m->m, sthread);
    }
}

void scoreRoot(movelist_t *mvlist) {
    move_t *m;

    ASSERT(mvlist != NULL);

    for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
        if (m->m == mvlist->transmove) m->s = MAXHIST * 3;
        else if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
        else m->s = 0;
    }
}

move_t* sortNext(SplitPoint* sp, position_t& pos, movelist_t *mvlist, int& phase, Thread& sthread) {
    move_t* move;
    if (sp != NULL) sp->movelistlock->lock();
    phase = mvlist->phase;
    if (MoveGenPhase[mvlist->phase] == PH_END) {  // SMP hack
        if (sp != NULL) sp->movelistlock->unlock();
        return NULL;
    }
    while (true) {
        while (mvlist->pos < mvlist->size) {
            move = getMove(mvlist);
            switch (MoveGenPhase[mvlist->phase]) {
            case PH_ROOT:
                break;
            case PH_EVASION:
                ASSERT(kingIsInCheck(pos));
                break;
            case PH_TRANS:
                if (!genMoveIfLegal(pos, move->m, mvlist->pinned)) {
                    continue;
                }
                if (mvlist->depth <= 0 && !moveIsTactical(mvlist->transmove)) { // TODO: test this
                    mvlist->transmove = EMPTY;
                    continue;
                }
                break;
            case PH_ALL_CAPTURES:
                if (move->m == mvlist->transmove) continue;
            case PH_ALL_CAPTURES_PURE:
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_GOOD_CAPTURES:
                if (move->m == mvlist->transmove) continue;
                if (!captureIsGood(pos, move->m)) {
                    mvlist->list[--(mvlist->startBad)].m = move->m;
                    continue;
                }
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_GOOD_CAPTURES_PURE:
                if (move->m == mvlist->transmove) continue;
                if (!captureIsGood(pos, move->m)) continue;
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_BAD_CAPTURES:
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_KILLER_MOVES:
                if (move->m == mvlist->transmove) continue;
                if (!genMoveIfLegal(pos, move->m, mvlist->pinned)) continue;
                break;
            case PH_QUIET_MOVES:
                if (move->m == mvlist->transmove) continue;
                if (move->m == mvlist->killer1) continue;
                if (move->m == mvlist->killer2) continue;
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_NONTACTICAL_CHECKS:
                if (move->m == mvlist->transmove) continue;
                if (move->m == mvlist->killer1) continue;
                if (move->m == mvlist->killer2) continue;
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_NONTACTICAL_CHECKS_WIN:
                if (swap(pos, move->m) < 0) continue;
            case PH_NONTACTICAL_CHECKS_PURE:
                if (move->m == mvlist->transmove) continue;
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            case PH_GAINING:
                if (move->m == mvlist->transmove) continue;
                if (moveIsCheck(pos, move->m, discoveredCheckCandidates(pos, pos.side))) continue;
                if (!moveIsLegal(pos, move->m, mvlist->pinned, false)) continue;
                break;
            default:
                // can't get here
                if (sp != NULL) sp->movelistlock->unlock();
                return NULL;
            }
            if (sp != NULL) sp->movelistlock->unlock();
            return move;
        }

        mvlist->phase++;
        phase = mvlist->phase;

        switch (MoveGenPhase[mvlist->phase]) {
        case PH_ROOT:
            if (CEngine.info.mvlist_initialized) break;
            if (CEngine.info.moves_is_limited == true) {
                for (mvlist->size = 0; CEngine.info.moves[mvlist->size] != EMPTY; mvlist->size++) {
                    mvlist->list[mvlist->size].m = CEngine.info.moves[mvlist->size];
                }
            } else {
                // generate all legal moves at least in the root
                genLegal(pos, mvlist, true);
            }
            scoreRoot(mvlist);
            CEngine.info.mvlist_initialized = true;
            CEngine.info.legalmoves = mvlist->size;
            break;
        case PH_EVASION:
            genEvasions(pos, mvlist);
            if (mvlist->depth <= 0) scoreAllQ(mvlist, sthread);
            else scoreAll(pos, mvlist, sthread);
            break;
        case PH_TRANS:
            if (mvlist->transmove != EMPTY) {
                mvlist->list[mvlist->size++].m = mvlist->transmove;
            }
            break;
        case PH_ALL_CAPTURES:
        case PH_ALL_CAPTURES_PURE:
            genCaptures(pos, mvlist);
            scoreCapturesPure(mvlist);
            break;
        case PH_GOOD_CAPTURES:
        case PH_GOOD_CAPTURES_PURE:
            genCaptures(pos, mvlist);
            scoreCapturesPure(mvlist);
            break;
        case PH_BAD_CAPTURES:
            for (int i = MAXMOVES - 1; i >= mvlist->startBad; --i) {
                mvlist->list[mvlist->size] = mvlist->list[i];
                mvlist->size++;
            }
            break;
        case PH_KILLER_MOVES:
            if (mvlist->killer1 != EMPTY) {
                mvlist->list[mvlist->size].m = mvlist->killer1;
                mvlist->list[mvlist->size].s = MAXHIST;
                mvlist->size++;
            }
            if (mvlist->killer2 != EMPTY) {
                mvlist->list[mvlist->size].m = mvlist->killer2;
                mvlist->list[mvlist->size].s = MAXHIST - 1;
                mvlist->size++;
            }
            break;
        case PH_QUIET_MOVES:
            genNonCaptures(pos, mvlist);
            scoreNonCaptures(pos, mvlist, sthread);
            break;
        case PH_NONTACTICAL_CHECKS:
        case PH_NONTACTICAL_CHECKS_WIN:
        case PH_NONTACTICAL_CHECKS_PURE:
            genQChecks(pos, mvlist);
            scoreNonCaptures(pos, mvlist, sthread);
            break;
        case PH_GAINING:
            if (mvlist->scout - mvlist->evalvalue > 150) continue; // TODO: test other values
            genGainingMoves(pos, mvlist, mvlist->scout - mvlist->evalvalue, sthread);
            scoreNonCaptures(pos, mvlist, sthread);
            break;
        default:
            ASSERT(MoveGenPhase[mvlist->phase] == PH_END);
            if (sp != NULL) sp->movelistlock->unlock();
            return NULL;
        }
    }
}


