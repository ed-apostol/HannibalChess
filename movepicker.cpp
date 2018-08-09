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
#include "attacks.h"
#include "movegen.h"
#include "threads.h"
#include "search.h"
#include "movepicker.h"

extern bool moveIsTactical(uint32 m);
extern int historyIndex(uint32 side, uint32 move);

void sortInit(const position_t& pos, movelist_t& mvlist, uint64 pinned, uint32 hashmove, int depth, int type, ThreadStack& ts) {
	mvlist.transmove = hashmove;
	mvlist.killer1 = ts.killer1;
	mvlist.killer2 = ts.killer2;
	mvlist.pos = 0;
	mvlist.size = 0;
	mvlist.pinned = pinned;
	mvlist.side = pos.side;
	mvlist.depth = depth;
	mvlist.phase = type;
	mvlist.startBad = MAXMOVES;
}

move_t* getMove(movelist_t& mvlist) {
	move_t *best, *temp, *start, *end;

	if (mvlist.pos >= mvlist.size) return nullptr;
	start = &mvlist.list[mvlist.pos++];
	end = &mvlist.list[mvlist.size];
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
inline int scoreNonTactical(const uint32 side, const int32 move, Thread& sthread) {
	int score = sthread.history[historyIndex(side, move)] + sthread.evalgains[historyIndex(side, move)];
	return score;
}

bool captureIsGood(const position_t& pos, const basic_move_t m) {
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

void scoreCapturesPure(movelist_t& mvlist) {
	move_t *m;
	for (m = &mvlist.list[mvlist.pos]; m < &mvlist.list[mvlist.size]; m++) {
		m->s = (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
	}
}

void scoreNonCaptures(movelist_t& mvlist, Thread& sthread) {
	move_t *m;
	for (m = &mvlist.list[mvlist.pos]; m < &mvlist.list[mvlist.size]; m++) {
		m->s = scoreNonTactical(mvlist.side, m->m, sthread);
	}
}

template<bool isQuiesc>
void scoreEvasion(const position_t& pos, movelist_t& mvlist, Thread& sthread) {
	move_t *m;

	ASSERT(mvlist != NULL);
	for (m = &mvlist.list[mvlist.pos]; m < &mvlist.list[mvlist.size]; m++) {
		if (m->m == mvlist.transmove) m->s = MAXHIST * 3; //need trans to go first since there is no trans phase
		else {
			if (isQuiesc) {
				if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
				else m->s = scoreNonTactical(mvlist.side, m->m, sthread);
			}
			else {
				if (moveIsTactical(m->m)) {
					m->s = (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
					if (captureIsGood(pos, m->m)) m->s += MAXHIST * 2;
					else m->s -= MAXHIST;
				}
				else if (m->m == mvlist.killer1) m->s = MAXHIST + 4;
				else if (m->m == mvlist.killer2) m->s = MAXHIST + 2;
				else m->s = scoreNonTactical(mvlist.side, m->m, sthread);
			}
		}
	}
}

void scoreRoot(movelist_t& mvlist) {
	move_t *m;

	for (m = &mvlist.list[mvlist.pos]; m < &mvlist.list[mvlist.size]; m++) {
		if (m->m == mvlist.transmove) m->s = MAXHIST * 3;
		else if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
		else m->s = 0;
	}
}

move_t* sortNext(SplitPoint* sp, SearchInfo& info, position_t& pos, movelist_t& mvlist, Thread& sthread) {
	move_t* move;
	if (sp != nullptr) sp->movelistlock.lock();
	if (MoveGenPhase[mvlist.phase] == PH_END) {  // SMP hack
		if (sp != nullptr) sp->movelistlock.unlock();
		return nullptr;
	}
	while (true) {
		while (mvlist.pos < mvlist.size) {
			move = getMove(mvlist);
			switch (MoveGenPhase[mvlist.phase]) {
			case PH_ROOT:
				break;
			case PH_EVASION:
				ASSERT(kingIsInCheck(pos));
				break;
			case PH_TRANS:
				if (!genMoveIfLegal(pos, move->m, mvlist.pinned)) continue;
				break;
			case PH_ALL_CAPTURES:
				if (move->m == mvlist.transmove) continue;
			case PH_ALL_CAPTURES_PURE:
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			case PH_GOOD_CAPTURES:
				if (move->m == mvlist.transmove) continue;
				if (!captureIsGood(pos, move->m)) {
					mvlist.list[--(mvlist.startBad)].m = move->m;
					continue;
				}
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			case PH_GOOD_CAPTURES_PURE:
				if (move->m == mvlist.transmove) continue;
				if (!captureIsGood(pos, move->m)) continue;
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			case PH_BAD_CAPTURES:
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			case PH_KILLER_MOVES:
				if (move->m == mvlist.transmove) continue;
				if (!genMoveIfLegal(pos, move->m, mvlist.pinned)) continue;
				break;
			case PH_QUIET_MOVES:
				if (move->m == mvlist.transmove) continue;
				if (move->m == mvlist.killer1) continue;
				if (move->m == mvlist.killer2) continue;
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			case PH_NONTACTICAL_CHECKS:
				if (move->m == mvlist.transmove) continue;
				if (move->m == mvlist.killer1) continue;
				if (move->m == mvlist.killer2) continue;
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			case PH_NONTACTICAL_CHECKS_WIN:
				if (swap(pos, move->m) < 0) continue;
			case PH_NONTACTICAL_CHECKS_PURE:
				if (move->m == mvlist.transmove) continue;
				if (!moveIsLegal(pos, move->m, mvlist.pinned, false)) continue;
				break;
			default:
				// can't get here
				if (sp != nullptr) sp->movelistlock.unlock();
				return nullptr;
			}
			if (sp != nullptr) sp->movelistlock.unlock();
			return move;
		}
		mvlist.phase++;
		switch (MoveGenPhase[mvlist.phase]) {
		case PH_ROOT:
			if (info.mvlist_initialized) break;
			if (info.moves_is_limited == true) {
				for (mvlist.size = 0; info.moves[mvlist.size] != EMPTY; mvlist.size++) {
					mvlist.list[mvlist.size].m = info.moves[mvlist.size];
				}
			}
			else {
				// generate all legal moves at least in the root
				genLegal(pos, mvlist, true);
				info.bestmove = mvlist.list[0].m; // to avoid time losses on very fast TC
			}
			scoreRoot(mvlist);
			info.mvlist_initialized = true;
			info.legalmoves = mvlist.size;
			break;
		case PH_EVASION:
			genEvasions(pos, mvlist);
			if (mvlist.depth <= 0) scoreEvasion<true>(pos, mvlist, sthread);
			else scoreEvasion<false>(pos, mvlist, sthread);
			break;
		case PH_TRANS:
			if (mvlist.transmove != EMPTY) {
				mvlist.list[mvlist.size++].m = mvlist.transmove;
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
			for (int i = MAXMOVES - 1; i >= mvlist.startBad; --i) {
				mvlist.list[mvlist.size] = mvlist.list[i];
				mvlist.size++;
			}
			break;
		case PH_KILLER_MOVES:
			if (mvlist.killer1 != EMPTY) {
				mvlist.list[mvlist.size].m = mvlist.killer1;
				mvlist.list[mvlist.size].s = MAXHIST;
				mvlist.size++;
			}
			if (mvlist.killer2 != EMPTY) {
				mvlist.list[mvlist.size].m = mvlist.killer2;
				mvlist.list[mvlist.size].s = MAXHIST - 1;
				mvlist.size++;
			}
			break;
		case PH_QUIET_MOVES:
			genNonCaptures(pos, mvlist);
			scoreNonCaptures(mvlist, sthread);
			break;
		case PH_NONTACTICAL_CHECKS:
		case PH_NONTACTICAL_CHECKS_WIN:
		case PH_NONTACTICAL_CHECKS_PURE:
			genQChecks(pos, mvlist);
			scoreNonCaptures(mvlist, sthread);
			break;
		default:
			ASSERT(MoveGenPhase[mvlist.phase] == PH_END);
			if (sp != nullptr) sp->movelistlock.unlock();
			return nullptr;
		}
	}
}