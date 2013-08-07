/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/


void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int depth, int type, int thread_id) {
	mvlist->transmove =  hashmove;
	mvlist->killer1 = Threads[thread_id].killer1[pos->ply];
	mvlist->killer2 = Threads[thread_id].killer2[pos->ply];
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

basic_move_t getMove(movelist_t *mvlist) {
	move_t *best, *temp, *start, *end;

	ASSERT(mvlist != NULL);

	if (mvlist->pos >= mvlist->size) return EMPTY;
	start = &mvlist->list[mvlist->pos++];
	end = &mvlist->list[mvlist->size];
	best = start;
	for (temp = start+1; temp < end; temp++) {
		if (temp->s > best->s) best = temp;
	}
	if (best == start) return start->m;
	*temp = *start;
	*start = *best;
	*best = *temp;
	return start->m;
}

BOOL moveIsPassedPawn(const position_t * pos, uint32 move) {
	if (movePiece(move) == PAWN && !((*FillPtr[pos->side])(BitMask[moveTo(move)]) & pos->pawns)) {
		if (!(pos->pawns & pos->color[pos->side^1] & PassedMask[pos->side][moveTo(move)])) return TRUE;
	}
	return FALSE;
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
	if (prom != EMPTY && prom != QUEEN) return FALSE;
	if (capt != EMPTY) {
		if (prom != EMPTY) return TRUE;
		if (capt >= pc) return TRUE;
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

void scoreNonCaptures(const position_t *pos, movelist_t *mvlist, int thread_id) {
	move_t *m;

	ASSERT(mvlist != NULL);

	for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
		m->s = SearchInfo(0).history[historyIndex(mvlist->side, m->m)]; // TODO: change history with evalgains
	}
}

void scoreAll(const position_t *pos, movelist_t *mvlist, int thread_id) {
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
			m->s = SearchInfo(0).history[historyIndex(mvlist->side, m->m)]; // TODO: change history with evalgains
		}
	}
}

void scoreAllQ(movelist_t *mvlist, int thread_id) {
	move_t *m;

	ASSERT(mvlist != NULL);

	for (m = &mvlist->list[mvlist->pos]; m < &mvlist->list[mvlist->size]; m++) {
		if (moveIsTactical(m->m))  m->s = MAXHIST + (moveCapture(m->m) * 6) + movePromote(m->m) - movePiece(m->m);
		else m->s = SearchInfo(0).history[historyIndex(mvlist->side, m->m)]; // TODO: change history with evalgains
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

basic_move_t sortNext(split_point_t* sp, position_t *pos, movelist_t *mvlist, int *phase, int thread_id) {
	basic_move_t move;
	if (sp != NULL) MutexLock(sp->lock);
	*phase = mvlist->phase;
	if (MoveGenPhase[mvlist->phase] == PH_END) {  // SMP hack
		if (sp != NULL) MutexUnlock(sp->lock);
		return EMPTY;
	}
	while (TRUE) {
		while (mvlist->pos < mvlist->size) {
			move = getMove(mvlist);
			switch (MoveGenPhase[mvlist->phase]) {
			case PH_ROOT:
				break;
			case PH_EVASION:
				ASSERT(kingIsInCheck(pos));
				break;
			case PH_TRANS:
				if (!genMoveIfLegal(pos, move, mvlist->pinned)) {
					continue;
				}
				if (mvlist->depth <= 0 && !moveIsTactical(mvlist->transmove)) { // TODO: test this
					mvlist->transmove = EMPTY;
					continue;
				}
				break;
			case PH_ALL_CAPTURES:
				if (move == mvlist->transmove) continue;
			case PH_ALL_CAPTURES_PURE:
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_GOOD_CAPTURES:
				if (move == mvlist->transmove) continue;
				if (!captureIsGood(pos, move)) {
					mvlist->list[--(mvlist->startBad)].m = move;
					continue;
				}
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_GOOD_CAPTURES_PURE:
				if (move == mvlist->transmove) continue;
				if (!captureIsGood(pos, move)) continue;
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_BAD_CAPTURES:
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_KILLER_MOVES:
				if (move == mvlist->transmove) continue;
				if (!genMoveIfLegal(pos, move, mvlist->pinned)) continue;
				break;
			case PH_QUIET_MOVES:
				if (move == mvlist->transmove) continue;
				if (move == mvlist->killer1) continue;
				if (move == mvlist->killer2) continue;
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_NONTACTICAL_CHECKS:
				if (move == mvlist->transmove) continue;
				if (move == mvlist->killer1) continue;
				if (move == mvlist->killer2) continue;
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_NONTACTICAL_CHECKS_WIN:
				if (swap(pos, move) < 0) continue;
			case PH_NONTACTICAL_CHECKS_PURE:
				if (move == mvlist->transmove) continue;
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			case PH_GAINING:
				if (move == mvlist->transmove) continue;
				if (moveIsCheck(pos, move, discoveredCheckCandidates(pos, pos->side))) continue;
				if (!moveIsLegal(pos, move, mvlist->pinned, FALSE)) continue;
				break;
			default:
				// can't get here
				if (sp != NULL) MutexUnlock(sp->lock);
				return EMPTY;
			}
			if (sp != NULL) MutexUnlock(sp->lock);
			return move;
		}

		mvlist->phase++;
		*phase = mvlist->phase;

		switch (MoveGenPhase[mvlist->phase]) {
		case PH_ROOT:
			if (SearchInfo(thread_id).mvlist_initialized) break;
			if (SearchInfo(thread_id).moves_is_limited == TRUE) {
				for (mvlist->size = 0; SearchInfo(thread_id).moves[mvlist->size] != EMPTY; mvlist->size++) {
					mvlist->list[mvlist->size].m = SearchInfo(thread_id).moves[mvlist->size];
				}
			} else {
				// generate all under promotion ONLY IF IN ANALYZE MODE
				genLegal(pos, mvlist, (SearchInfo(thread_id).depth_is_limited && SearchInfo(thread_id).depth_limit == MAXPLY)); 
			}
			scoreRoot(mvlist);
			SearchInfo(thread_id).mvlist_initialized = true;
			break;
		case PH_EVASION:
			genEvasions(pos, mvlist);
			if (mvlist->depth <= 0) scoreAllQ(mvlist, thread_id);
			else scoreAll(pos, mvlist, thread_id);
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
			for (int i = MAXMOVES-1; i >= mvlist->startBad; --i) {
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
				mvlist->list[mvlist->size].s = MAXHIST-1;
				mvlist->size++;
			}
			break;
		case PH_QUIET_MOVES:
			genNonCaptures(pos, mvlist);
			scoreNonCaptures(pos, mvlist, thread_id);
			break;
		case PH_NONTACTICAL_CHECKS:
		case PH_NONTACTICAL_CHECKS_WIN:
		case PH_NONTACTICAL_CHECKS_PURE:
			genQChecks(pos, mvlist);
			scoreNonCaptures(pos, mvlist, thread_id);
			break;
		case PH_GAINING:
			if (mvlist->scout - Threads[thread_id].evalvalue[pos->ply] > 150) continue; // TODO: test other values
			genGainingMoves(pos, mvlist, mvlist->scout - Threads[thread_id].evalvalue[pos->ply], thread_id);
			// Print(3, "delta = %d, mvlist->size = %d\n", mvlist->scout - SearchInfo.evalvalue[pos->ply], mvlist->size);
			scoreNonCaptures(pos, mvlist, thread_id);
			break;
		default:
			ASSERT(MoveGenPhase[mvlist->phase] == PH_END);
			if (sp != NULL) MutexUnlock(sp->lock);
			return EMPTY;
		}
	}
}


