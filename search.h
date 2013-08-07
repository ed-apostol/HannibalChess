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
//#define Q_HSEARCH 5 // implies searches hash move up to 2 depth
#define NO_ASP FALSE
#define MIN_REDUCTION_DEPTH 4 // default is FALSE

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
			Print(2, "info string Aborting search\n");
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
#ifndef TCEC
	if (thread_id >= Guci_options->threads) { //if a learningthread
		if (Threads[thread_id].nodes >= SearchInfo(thread_id).node_limit) //enforce node limit
			SearchInfo(thread_id).thinking_status = STOPPED;
	}
	else
#endif
		if (thread_id == 0 && ++Threads[thread_id].nodes_since_poll >= Threads[thread_id].nodes_between_polls) {
			Threads[thread_id].nodes_since_poll = 0;
			if (SHOW_SEARCH) check4Input(pos);
			time2 = getTime();
			if (time2 - SearchInfo(thread_id).last_time > 1000) {
				int64 time = time2 - SearchInfo(thread_id).start_time;
				SearchInfo(thread_id).last_time = time2;
				if (SHOW_SEARCH  && thread_id < Guci_options->threads) {
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
#ifdef W_EASY
				int64 easyTime = (SearchInfo(thread_id).alloc_time - (((SearchInfo(thread_id).alloc_time * (int64)SearchInfo(thread_id).easy) / EASY_MAX) * EASY_TIME) / EASY_DIVTIME); 
				if (SearchInfo(thread_id).easy && SearchInfo(thread_id).research && time2 >= SearchInfo(thread_id).start_time + easyTime) {
					setAllThreadsToStop(thread_id);
#ifdef DEBUG_EASY
					Print(2, "info string Easy QUICK1 Move: time spent = %d\n", time2 - SearchInfo(thread_id).start_time);
#endif
				}
				else
#endif
					if (time2 > SearchInfo(thread_id).time_limit_max) {
						if (time2 < SearchInfo(thread_id).time_limit_abs) {
							if (SearchInfo(thread_id).best_value != -INF && SearchInfo(thread_id).best_value + 30 <= SearchInfo(thread_id).last_value) {
								SearchInfo(thread_id).time_limit_max += ((SearchInfo(thread_id).last_value - SearchInfo(thread_id).best_value - 20) / 10) * SearchInfo(thread_id).alloc_time / 2;
								if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
								Print(2, "info string Time is extended in search: time = %d\n", time2 - SearchInfo(thread_id).start_time);
							} else if (SearchInfo(thread_id).best_value == -INF && SearchInfo(thread_id).last_value + 30 <= SearchInfo(thread_id).last_last_value) {
								SearchInfo(thread_id).time_limit_max += ((SearchInfo(thread_id).last_last_value - SearchInfo(thread_id).last_value - 20) / 10) * SearchInfo(thread_id).alloc_time / 2;
								if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
								Print(2, "info string Time is extended in search: time = %d\n", time2 - SearchInfo(thread_id).start_time);
							} else if (SearchInfo(thread_id).research || SearchInfo(thread_id).change) {
								SearchInfo(thread_id).time_limit_max += SearchInfo(thread_id).alloc_time / 2;
								if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
								Print(2, "info string Time is extended in search: time = %d\n", time2 - SearchInfo(thread_id).start_time);
							} else {
								setAllThreadsToStop(thread_id);
								Print(2, "info string Aborting search: time spent = %d\n", time2 - SearchInfo(thread_id).start_time);
							}
						} else {
							setAllThreadsToStop(thread_id);
							Print(2, "info string Aborting search: time spent = %d\n", time2 - SearchInfo(thread_id).start_time);
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
			if (-(before + after) >= SearchInfo(0).evalgains[historyIndex(pos->side^1, move)])
				SearchInfo(0).evalgains[historyIndex(pos->side^1, move)] = -(before + after);
			else
				SearchInfo(0).evalgains[historyIndex(pos->side^1, move)]--;
	}
}

const int MaxPieceValue[] = {0, PawnValueEnd, KnightValueEnd, BishopValueEnd, RookValueEnd, QueenValueEnd, 10000};
template <bool inPv>
int qSearch(position_t *pos, int alpha, int beta, int depth, const int inCheck, const int thread_id) {
	int bestvalue = -INF;
	int oldalpha = alpha;
	int score;
	int newdepth;
	int moveGivesCheck;
	int opt, pes;
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
		if (!inPv) {
			if (transMinvalue(entry) > alpha && alpha < MAXEVAL) return transMinvalue(entry);
			if (transMaxvalue(entry) < beta && beta > -MAXEVAL) return transMaxvalue(entry);
		}
		hashMove = transMove(entry);
		//		hashMove = (depth > -Q_HSEARCH || moveIsTactical(transMove(entry))) ? transMove(entry) : 0; //080213opt1
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
	bool prunable = !inCheck && !inPv && MinTwoBits(pos->color[pos->side^1] & pos->pawns) &&
		MinTwoBits(pos->color[pos->side^1] & ~(pos->pawns | pos->kings));
	while ((move = sortNext(NULL, pos, &mvlist, &mvlist_phase, thread_id)) != EMPTY) {
		if (anyRepNoMove(pos,move)) { 
			score = DrawValue[pos->side];
		} else {
			moveGivesCheck = moveIsCheck(pos, move, dcc);
			//pruning
			if (prunable && move!=hashMove && !moveGivesCheck && !moveIsPassedPawn(pos, move)) {
				int scoreAprox = Threads[thread_id].evalvalue[pos->ply] + PawnValueEnd/*+ PawnValueEnd/2 + opt*/ + MaxPieceValue[moveCapture(move)]; //opt1 removed opt
				if (scoreAprox <= alpha) continue; //todo try out opt, tryout see instead of maxpiecevalue
			}
			newdepth = depth - !moveGivesCheck;
			makeMove(pos, &undo, move);
			score = -qSearch<inPv>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id);
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
	if (inCheck && bestvalue == -INF) bestvalue = -INF + pos->ply;
cut:    
	transStore(pos->hash, bestmove, 0, ((bestvalue > oldalpha) ? bestvalue : -INF), ((bestvalue < beta) ? bestvalue : INF),thread_id);
	ASSERT(valueIsOk(bestvalue));
	return bestvalue;
}

template <bool inPv, bool inSplitPoint>
inline int searchNode(position_t *pos, int alpha, int beta, int depth, int inCheck, const int thread_id, const bool cutNode) {
	if (depth <= 0) return qSearch<inPv>(pos, alpha, beta, 0, inCheck, thread_id);
	else return searchGeneric<inPv, inSplitPoint>(pos, alpha, beta, depth, inCheck, thread_id, cutNode);
}

template<bool firstBanned>
int searchSelective(position_t *pos, int beta, int depth, int inCheck, uint32 *firstMove, const int thread_id) {
	int score;
	int newdepth;
	int moveGivesCheck;
	int played = 0;
	uint64 pinned;
	uint64 dcc;
	basic_move_t move;
	movelist_t mvlist;
	pos_store_t undo;
	int mvlist_phase;

	ASSERT(pos != NULL);
	ASSERT(valueIsOk(beta));

	initNode(pos, thread_id);
	if (Threads[thread_id].stop) return 0;

	pinned = pinnedPieces(pos, pos->side);
	dcc = discoveredCheckCandidates(pos, pos->side);

	sortInit(pos, &mvlist, pinned, *firstMove, beta, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), thread_id);
	if (firstBanned) { //just skipping the hashed move off if we know it is banned so we don't have to keep checking SAM
		mvlist.phase++;
	}
	while ((move = sortNext(NULL, pos, &mvlist, &mvlist_phase, thread_id)) != EMPTY) {
		int okToPruneOrReduce ;
		int newdepthclone;
		played++;
		if (anyRepNoMove(pos,move)) { 
			score = DrawValue[pos->side];
		} else {
			moveGivesCheck = moveIsCheck(pos, move, dcc);
			newdepth = depth-1;
			okToPruneOrReduce = (newdepth < depth && !moveGivesCheck && MoveGenPhase[mvlist.phase] == PH_QUIET_MOVES &&
				(depth >= MIN_REDUCTION_DEPTH || !MIN_REDUCTION_DEPTH)); //TODO update this given new search (passed pawn pushes?)
			makeMove(pos, &undo, move);
			newdepthclone = newdepth;
			if (okToPruneOrReduce) newdepthclone = newdepth - ReductionTable[1][MIN(depth,63)][MIN(played,63)];
			score = -searchNode<false, false>(pos, -beta, 1-beta, newdepthclone, moveGivesCheck, thread_id,true); //TODO consider not doing pruning on played1
			if (newdepthclone < newdepth && score >= beta) {
				score = -searchNode<false, false>(pos, -beta, 1-beta, newdepth, moveGivesCheck, thread_id,true);
			}
			unmakeMove(pos, &undo);
		}
		if (score >= beta) {
			if (!firstBanned) *firstMove = move;
			return score;
		}
	}
	return (!firstBanned && played == 0 && !inCheck) ? DrawValue[pos->side] : -INF;
}

template<bool inPv, bool inSplitPoint>
int searchGeneric(position_t *pos, int alpha, int beta, int depth, int inCheck, const int thread_id, const bool cutNode) {
	int bestvalue = -INF;
	int oldalpha = alpha;
	int score;
	int newdepth;
	int moveGivesCheck;
	int played = 0;
	int opt=0, pes = 0;
	uint32 bestmove = EMPTY;
	uint64 pinned;
	uint64 dcc;
	basic_move_t move;
	movelist_t movelist;
	movelist_t* mvlist = &movelist;
	pos_store_t undo;
	trans_entry_t * entry = NULL;
	int firstExtend = FALSE;
	int hashDepth;
	basic_move_t hashMove;
	int prune = (abs(alpha) < MAXEVAL);
	int mvlist_phase;
	split_point_t* sp = NULL;
	uint64 nullThreatMoveToBit = 0;
	basic_move_t hisMoves[64];
	int hisOn;

	ASSERT(pos != NULL);
	ASSERT(oldPV != NULL);
	ASSERT(valueIsOk(alpha));
	ASSERT(valueIsOk(beta));
	ASSERT(alpha < beta);
	ASSERT(depth >= 1);

	initNode(pos, thread_id);
	if (Threads[thread_id].stop) return 0;

	if (!inSplitPoint) {
		entry = transProbe(pos->hash,thread_id);
		if (entry != NULL) {
			hashMove = transMove(entry);
			if (!inPv) {
				if (depth <= transMindepth(entry) && transMinvalue(entry) >= beta && beta < MAXEVAL) {
					if (hashMove && hashMove != Threads[thread_id].killer1[pos->ply] && !moveIsTactical(hashMove)) { //good move, so killer move 07/25/13 NEW
						Threads[thread_id].killer2[pos->ply] = Threads[thread_id].killer1[pos->ply];
						Threads[thread_id].killer1[pos->ply] = hashMove;
					}
					return transMinvalue(entry);
				}
				if (depth <= transMaxdepth(entry) && transMaxvalue(entry) < beta && beta >= -MAXEVAL) {
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
		if (pos->ply >= MAXPLY-3) return Threads[thread_id].evalvalue[pos->ply]; // -3 ensures the next 2 lines do not cause problems
		Threads[thread_id].killer1[pos->ply+2] = 0;
		Threads[thread_id].killer2[pos->ply+2] = 0;
		updateEvalgains(pos, pos->posStore.lastmove, Threads[thread_id].evalvalue[pos->ply-1], Threads[thread_id].evalvalue[pos->ply], thread_id);

		if (!inPv && !inCheck && prune) {
			int rvalue;
			if (cutNode && pos->color[pos->side] & ~(pos->pawns | pos->kings)
				&& beta < (rvalue = Threads[thread_id].evalvalue[pos->ply] - FutilityMarginTable[MIN(depth,MAX_FUT_MARGIN)][0] - pes))  {
					if (hashMove && hashMove != Threads[thread_id].killer1[pos->ply] && !moveIsTactical(hashMove)) { //good move, so killer move 07/25/13 NEW
						Threads[thread_id].killer2[pos->ply] = Threads[thread_id].killer1[pos->ply];
						Threads[thread_id].killer1[pos->ply] = hashMove;
					}
					return rvalue;
			}
			if (pos->posStore.lastmove != EMPTY && hashMove == EMPTY
				&& Threads[thread_id].evalvalue[pos->ply] < (rvalue = beta - FutilityMarginTable[MIN(depth,MAX_FUT_MARGIN)][0] - opt)) { 
					score = qSearch<FALSE>(pos, rvalue-1, rvalue, 0, FALSE, thread_id);
					if (score < rvalue) return score;
			}

			if (cutNode && depth >= 2 && (MinTwoBits(pos->color[pos->side] & ~(pos->pawns))) && Threads[thread_id].evalvalue[pos->ply] >= beta) {
				if (anyRepNoMove(pos,0)) {
					if (DrawValue[pos->side] >= beta) return DrawValue[pos->side];
				} else {
					int R = 4 + depth/5  + (Threads[thread_id].evalvalue[pos->ply] - beta > PawnValue);
					int nullDepth = depth - R;
					makeNullMove(pos, &undo);
					score = -searchNode<false, false>(pos, -beta, -alpha, nullDepth, FALSE, thread_id,false);
					if ((entry = transProbe(pos->hash,thread_id)) != NULL && transMove(entry) != EMPTY) {
						nullThreatMoveToBit = BitMask[moveTo(transMove(entry))];
					}
					unmakeNullMove(pos, &undo);
					if (score >= beta) {
						if (depth <= 6 || MinTwoBits(pos->color[pos->side] & ~(pos->pawns | pos->kings))) {
							return score;
						}
						score = searchNode<false, false>(pos, alpha, beta, depth-6, FALSE, thread_id,false);
						if (score >= beta) 
							return score;
					}
				}

			}
		}


		if (depth >= (inPv?EXPLORE_DEPTH_PV:EXPLORE_DEPTH_NOPV) && prune) {
			newdepth = depth / 2;
			if (hashMove == EMPTY || hashDepth < newdepth) {
				int targetScore = alpha;
				score =	searchSelective<false>(pos, targetScore, newdepth, inCheck, &hashMove, thread_id);
				//				score = searchNode<false, false>(pos, alpha, beta, newdepth, inCheck, thread_id,false);
				if (score >= targetScore) {
					Threads[thread_id].evalvalue[pos->ply] = score;//consider reseting opt and pes
					hashDepth = newdepth;
				}
			}
			if (hashMove != EMPTY && hashDepth >= newdepth) { 
				int targetscore = Threads[thread_id].evalvalue[pos->ply] - EXPLORE_CUTOFF;
				int dif = alpha - Threads[thread_id].evalvalue[pos->ply];
				//				if (dif > 0) targetscore += dif / 4;
				score = searchSelective<true>(pos, targetscore, newdepth, inCheck, &hashMove, thread_id);
				if (score < targetscore) firstExtend = TRUE;
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
		pinned = pinnedPieces(pos, pos->side);
		sortInit(pos, mvlist, pinned, hashMove, alpha, depth, (inCheck ? MoveGenPhaseEvasion : MoveGenPhaseStandard), thread_id);
		firstExtend |= (inCheck && mvlist->size==1);
	}
	int pruneable =  prune && MinTwoBits(pos->color[pos->side] & ~(pos->pawns | pos->kings));//TODO try having minimum # pawns as well
	int lateMove = LATE_PRUNE_MIN + ((cutNode && !inPv) ? ((depth * depth) / 4) : (depth * depth));
	//	if (!firstExtend) lateMove += LATE_PRUNE_MIN;
	hisOn = 0;
	while ((move = sortNext(sp, pos, mvlist, &mvlist_phase, thread_id)) != EMPTY) {
		played++;
		if (anyRepNoMove(pos,move)) { 
			score = DrawValue[pos->side];
		} else {
			moveGivesCheck = moveIsCheck(pos, move, dcc);
			newdepth = depth - !(firstExtend && played==1);
			//maybe should check legality here, since its an expensive check and might have been pruned earlier?
			if (/*inPv && */bestvalue == -INF) { //TODO remove this from loop and do it first
				makeMove(pos, &undo, move);
				score = -searchNode<inPv, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id,(!inPv && !cutNode));
			} 
			else {
				int PruneReductionLevel = (inCheck || newdepth >= depth || moveGivesCheck || MoveGenPhase[mvlist_phase] != PH_QUIET_MOVES ) ? 0 : 
					(moveIsPassedPawn(pos, move) ? 1 : 2);
				if (PruneReductionLevel && !inPv && pruneable) {
					if (played > lateMove && !isMoveDefence(pos, move, nullThreatMoveToBit)) continue;
					int predictedDepth = MAX(0,newdepth - ReductionTable[1][MIN(depth,63)][MIN(played,63)]);
					score = Threads[thread_id].evalvalue[pos->ply]
					+ FutilityMarginTable[MIN(predictedDepth,MAX_FUT_MARGIN)][MIN(played,63)]
					+ SearchInfo(0).evalgains[historyIndex(pos->side, move)];
					if (score < beta) {
						if (predictedDepth < 8 && PruneReductionLevel > 1) continue;
						if (swap(pos, move) < 0 && predictedDepth < 2) continue;
						newdepth--;
					}
					if (swap(pos, move) < 0) {
						newdepth--; 
					}
				}
				int newdepthclone = newdepth;
				if (PruneReductionLevel==1) {
					newdepthclone -= ((ReductionTable[1][MIN(depth,63)][MIN(played,63)] >= 3) ? (ReductionTable[1][MIN(depth,63)][MIN(played,63)] - 2) : 0);
				}
				else if (PruneReductionLevel==2) {
					newdepthclone -= ReductionTable[(inPv?0:1)][MIN(depth,63)][MIN(played,63)];
				}

				makeMove(pos, &undo, move);
				score = -searchNode<false, false>(pos, -alpha-1, -alpha, newdepthclone, moveGivesCheck, thread_id, true);
				if (newdepthclone < newdepth && score > alpha) {
					score = -searchNode<false, false>(pos, -alpha-1, -alpha, newdepth, moveGivesCheck, thread_id, false);
				}
				if (inPv && score > alpha) {
					score = -searchNode<true, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id, false);
				}
			}
			unmakeMove(pos, &undo);
		}
		if (Threads[thread_id].stop) return bestvalue;
		if (score > bestvalue) {
			bestvalue = score;
			if (inSplitPoint) {
				if (bestvalue > sp->bestvalue) sp->bestvalue = bestvalue;
				else bestvalue = sp->bestvalue;
			}
			if (bestvalue > alpha) {
				bestmove = move;
				if (inSplitPoint) sp->bestmove = move;
				alpha = bestvalue;
				if (alpha >= beta) {
					if (inSplitPoint) Threads[thread_id].cutoff = true;
					break;
				}
				if (inSplitPoint) {
					if (alpha > sp->alpha) sp->alpha = alpha;
					else alpha = sp->alpha;
				}
			}
		}
		if (hisOn < 64 && !moveIsTactical(move)) { //putting it after best move check ensures we do not mess with best move
			hisMoves[hisOn++] = move;
		}
		if(!inSplitPoint && SearchInfo(thread_id).thinking_status != STOPPED && !inCheck  
#ifndef TCEC
			&& thread_id < Guci_options->threads //SAM only necessary for learning threads, which are start at maxthreads and count down
#endif
			&& Guci_options->threads > 1 && depth >= Guci_options->min_split_depth && idleThreadExists(thread_id)
			&& splitRemainingMoves(pos, mvlist, &bestvalue, &bestmove, &played, alpha, beta, inPv, depth, inCheck, thread_id)) {
				break;
		}
	}
	if(inSplitPoint) sp->played = played;
	else {
		if (played == 0) {
			if (inCheck) return -INF + pos->ply;
			else return DrawValue[pos->side];
		}
		if (Threads[thread_id].stop) return 0;
		if (bestmove != EMPTY && !moveIsTactical(bestmove) && bestvalue >= beta) { //> alpha account for pv better maybe? Sam
			int index = historyIndex(pos->side, bestmove);
			int hisDepth = MIN(depth,MAX_HDEPTH);
			int hisValue = depth * depth;

			SearchInfo(0).history[index] += hisValue;
			for (int i=0; i < hisOn; ++i) {
				index = historyIndex(pos->side, hisMoves[i]);
				SearchInfo(0).history[index] -= SearchInfo(0).history[index]/(NEW_HISTORY-hisDepth);
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

void extractPvMovesFromHash(position_t *pos, continuation_t* pv, basic_move_t move) {
	trans_entry_t *entry;
	pos_store_t undo[MAXPLY];
	int ply = 0;
	basic_move_t hashMove;
	pv->length = 0;
	pv->moves[pv->length++] = move;
	makeMove(pos, &(undo[ply++]), move);
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

void repopulateHash(position_t *pos,continuation_t *rootPV,int depth, int score) {
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

int goodAlpha(int alpha) {
	if (abs(alpha) > 500) return -INF;
	else return alpha;
}

int goodBeta(int beta) {
	if (abs(beta) > 500) return INF;
	else return beta;
}


void searchRoot(position_t *pos, movelist_t *mvlist, int alpha, int beta, int depth, const int thread_id,const int minAlpha) {
	int64 time;
	basic_move_t move;
	pos_store_t undo;
	continuation_t newPV;
	continuation_t rootPV;
	int64 lastnodes;
	int64 sum_nodes;
	int64 maxnodes;
	uint64 dcc;
	int score;
	int bestmoveindex;
	int newdepth;
	int moveGivesCheck;
	int fail_low = 0;
	int fail_high = 0;
	int old_alpha = alpha;
	int played = 0;
	time = getTime();
	pos->ply = 0;
	rootPV.length = 0;
	newPV.length = 0;
	Threads[thread_id].evalvalue[pos->ply] = -INF;
	SearchInfo(thread_id).best_value = -INF;
	SearchInfo(thread_id).iteration = depth;
	SearchInfo(thread_id).change = 0;
	SearchInfo(thread_id).research = 0;

	maxnodes = 0;
	bestmoveindex = 0;

	dcc = discoveredCheckCandidates(pos, pos->side);

	if (SHOW_SEARCH  && thread_id < Guci_options->threads) Print(1, "info depth %d\n", depth);
	while(1) {
		if (alpha < minAlpha) alpha = minAlpha;
		SearchInfo(thread_id).best_value = -INF;
		played = 0;
		mvlist->pos = 0;
		while ((move = getMove(mvlist)) != NULL) {
			int okToPruneOrReduce;
			SearchInfo(thread_id).currmovenumber = mvlist->pos;
			if (SHOW_SEARCH && getTime() - SearchInfo(thread_id).start_time > 10000  && thread_id < Guci_options->threads)
				Print(1, "info currmove %s currmovenumber %d\n", move2Str(move), mvlist->pos);

			played++;
			moveGivesCheck = moveIsCheck(pos, move, dcc);
			newdepth = depth - (!moveGivesCheck);

			okToPruneOrReduce =
				(newdepth < depth
				&& !moveGivesCheck
				&& !moveIsTactical(move)
				&& !isCastle(move)
				&& !moveIsPassedPawn(pos, move));

			if (thread_id >= Guci_options->threads) lastnodes = Threads[thread_id].nodes; //for learning
			else {
				lastnodes = 0;
				for (int i = 0; i < Guci_options->threads; ++i) lastnodes += Threads[i].nodes;
			}

			if (anyRepNoMove(pos,move)) { 
				score = DrawValue[pos->side];
				newPV.length = 0;
			} else {
				score = -INF;
				makeMove(pos, &undo, move);
				if (SearchInfo(thread_id).best_value == -INF || score > alpha) {
					score = -searchNode<true, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id,false);
				} else {
					int newdepthclone = newdepth;
					if (okToPruneOrReduce) {

						//SAM not sure how easy interacts with fail lows
#ifdef W_EASY
						newdepthclone -= ReductionTable[0][MIN(depth,63)][MIN(played + (SearchInfo(thread_id).easy/EASY_INC)*EASY_PLAYED ,63)];
					}
					int easyAlpha = alpha - SearchInfo(thread_id).easy;
					score = -searchNode<false, false>(pos, -easyAlpha-1, -easyAlpha, newdepthclone, moveGivesCheck, thread_id,true);
					if (SearchInfo(thread_id).thinking_status != STOPPED && newdepthclone < newdepth && score > easyAlpha) {
						int64 tempEasy = SearchInfo(thread_id).easy;
						SearchInfo(thread_id).easy = 0; //we do this so that easy moves are not detected during researches
						//SearchInfo(thread_id).research = 1;
						bool first = true;
						do {
							if (tempEasy) {
								if (first) tempEasy -= EASY_INC;
								else tempEasy /= 2;
								first = false;
								if (tempEasy < EASY_MIN) {
									tempEasy = 0;
								}
								easyAlpha = alpha - tempEasy;
								SearchInfo(thread_id).maxEasy = tempEasy;
#ifdef DEBUG_EASY
								Print(3, "info string Easy %d\n",tempEasy);
#endif
							}
							if (SearchInfo(thread_id).easy) score = -searchNode<false, false>(pos, -easyAlpha-1, -easyAlpha, newdepthclone, moveGivesCheck, thread_id,false);
							else score = -searchNode<false, false>(pos, -easyAlpha-1, -easyAlpha, newdepth, moveGivesCheck, thread_id,false);
						} while (SearchInfo(thread_id).thinking_status != STOPPED && tempEasy > 0 && score > easyAlpha);
						SearchInfo(thread_id).easy = tempEasy;
					}

					if (SearchInfo(thread_id).thinking_status != STOPPED && score > alpha) {
						SearchInfo(thread_id).research = 1;
						if (SHOW_SEARCH  && thread_id < Guci_options->threads && getTime() - SearchInfo(thread_id).start_time > 10000) Print(1, "info currmove %s currmovenumber %d\n", move2Str(move), mvlist->pos);
						score = -searchNode<true, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id, false);
						SearchInfo(thread_id).research = 0;
					}
#else
						newdepthclone -= ReductionTable[0][MIN(depth,63)][MIN(played,63)];
					}
					score = -searchNode<false, false>(pos, -alpha-1, -alpha, newdepthclone, moveGivesCheck, thread_id,true);
					if (SearchInfo(thread_id).thinking_status != STOPPED && newdepthclone < newdepth && score > alpha) {
						score = -searchNode<false, false>(pos, -alpha-1, -alpha, newdepth, moveGivesCheck, thread_id,false);
					}
					if (SearchInfo(thread_id).thinking_status != STOPPED && score > alpha) {
						SearchInfo(thread_id).research = 1;
						if (SHOW_SEARCH && getTime() - SearchInfo(thread_id).start_time > 10000) Print(1, "info currmove %s currmovenumber %d\n", move2Str(move), mvlist->pos);
						score = -searchNode<true, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id, false);
						SearchInfo(thread_id).research = 0;
					}
#endif
				}

				if (!NO_ASP && SearchInfo(thread_id).thinking_status != STOPPED && score >= beta) {
					int oldMove = SearchInfo(thread_id).bestmove;
					int oldPonder = SearchInfo(thread_id).pondermove;
					do { //TODO rewrite this to do alpha and beta adjustments in the same loop
						if (mvlist->pos > 1) {
							SearchInfo(thread_id).change = 1;
						}
						SearchInfo(thread_id).bestmove = move;
						fail_high++;
						beta = goodBeta(beta+24*(1<<fail_high));
						score = -searchNode<true, false>(pos, -beta, -alpha, newdepth, moveGivesCheck, thread_id, false);
					} while (score >= beta && SearchInfo(thread_id).thinking_status != STOPPED);
					if (!NO_ASP && SearchInfo(thread_id).thinking_status != STOPPED && score <= alpha) {
						score = -searchNode<true, false>(pos, -INF, INF, newdepth, moveGivesCheck, thread_id, false);//this ensures our best estimate of the score
						if (SearchInfo(thread_id).thinking_status != STOPPED && score <= SearchInfo(thread_id).best_value) { // perhaps we should not have switched moves?
							SearchInfo(thread_id).bestmove = oldMove;
							SearchInfo(thread_id).pondermove = oldPonder;
						}
					}
				}
				unmakeMove(pos, &undo);
			}
			if (Threads[thread_id].stop) break;
#ifdef SELF_TUNE2
			sum_nodes = Threads[thread_id].nodes;
#else
			sum_nodes = 0;
			for (int i = 0; i < Guci_options->threads; ++i) sum_nodes += Threads[i].nodes;
#endif
			mvlist->list[mvlist->pos-1].s = ((sum_nodes - lastnodes) >> 13);
			if (mvlist->list[mvlist->pos-1].s > maxnodes) maxnodes = mvlist->list[mvlist->pos-1].s;
			if (score > SearchInfo(thread_id).best_value) {
				bestmoveindex = mvlist->pos-1;
				SearchInfo(thread_id).best_value = score;
				extractPvMovesFromHash(pos, &rootPV, move);
				SearchInfo(thread_id).bestmove = move;
				if (rootPV.length > 1) SearchInfo(thread_id).pondermove = rootPV.moves[1];
				else SearchInfo(thread_id).pondermove = 0;
				if (mvlist->pos > 1) {
					SearchInfo(thread_id).change = 1;
				}
				if (score > alpha) {
					alpha = score;
				}
			}
		}
		if (Threads[thread_id].stop) break;
		if (alpha == old_alpha) {
			if (alpha == minAlpha) {
				SearchInfo(thread_id).bestmove = 0;
				return;
			}
			SearchInfo(thread_id).change = 1;  // TODO: review time management
			if (SHOW_SEARCH  && thread_id < Guci_options->threads && depth >= 8) displayPV(pos, &rootPV, depth, old_alpha, beta, SearchInfo(thread_id).best_value);
			fail_low++;
			old_alpha = alpha = goodAlpha(alpha-(24*(1<<(fail_low))));

		} else break;
	}
	mvlist->list[bestmoveindex].s = (int32) maxnodes + 1024;

	if (SearchInfo(thread_id).best_value > INF - MAXPLY) SearchInfo(thread_id).mate_found++;
	if (SearchInfo(thread_id).thinking_status == THINKING || SearchInfo(thread_id).thinking_status == PONDERING) {
		if (SearchInfo(thread_id).time_is_limited) {
			time = getTime();
#ifdef W_EASY
			int64 easyTime = (SearchInfo(thread_id).alloc_time - (((SearchInfo(thread_id).alloc_time * (int64)SearchInfo(thread_id).easy) / EASY_MAX) * EASY_TIME) / EASY_DIVTIME)/2; 
			if (depth >= 8 && (SearchInfo(thread_id).legalmoves == 1 || SearchInfo(thread_id).mate_found >= 3) && SearchInfo(thread_id).thinking_status == THINKING) {
				setAllThreadsToStop(thread_id);
			} 
			else if (SearchInfo(thread_id).easy && time >= SearchInfo(thread_id).start_time + easyTime) {
				setAllThreadsToStop(thread_id);
#ifdef DEBUG_EASY
				Print(3, "info string Easy QUICK2 Move: time spent = %d(%d)\n", time - SearchInfo(thread_id).start_time,easyTime);
#endif
				Print(2, "info string Easy QUICK2 Move: time spent = %d(%d)\n", time - SearchInfo(thread_id).start_time,easyTime);
			}
#endif

			if (SearchInfo(thread_id).thinking_status == THINKING && time >= SearchInfo(thread_id).start_time + (SearchInfo(thread_id).alloc_time / 2)) {
				if (SearchInfo(thread_id).best_value + 30 <= SearchInfo(thread_id).last_value) {
					SearchInfo(thread_id).time_limit_max += ((SearchInfo(thread_id).last_value - SearchInfo(thread_id).best_value - 20) / 10) * SearchInfo(thread_id).alloc_time / 2;
					if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
				} else if (SearchInfo(thread_id).change) {
					SearchInfo(thread_id).time_limit_max += SearchInfo(thread_id).alloc_time / 2;
					if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs) SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
				} else if (SearchInfo(thread_id).thinking_status == THINKING) { // if we are unlikely to get deeper, save our time
					setAllThreadsToStop(thread_id);
				}
			}
		} else if (SearchInfo(thread_id).depth_is_limited && depth >= SearchInfo(thread_id).depth_limit && SearchInfo(thread_id).thinking_status == THINKING) {
			setAllThreadsToStop(thread_id);
		}
	}

	if (SearchInfo(thread_id).best_value != -INF) {
		SearchInfo(thread_id).last_last_value = SearchInfo(thread_id).last_value;
		SearchInfo(thread_id).last_value = SearchInfo(thread_id).best_value;
		repopulateHash(pos, &rootPV, depth, SearchInfo(thread_id).best_value);
	} else if (SearchInfo(thread_id).thinking_status != STOPPED) {
		Print(8, "SearchInfo.thinking_status != STOPPED Failure!\n");
		displayBoard(pos, 8);
	}

	if (SHOW_SEARCH  && thread_id < Guci_options->threads && SearchInfo(thread_id).best_value != -INF && (depth >= 8 || SearchInfo(thread_id).thinking_status == STOPPED))
		displayPV(pos, &rootPV, depth, old_alpha, beta, SearchInfo(thread_id).best_value);
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
#ifdef W_EASY
	SearchInfo(thread_id).easy = 0;
#endif
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
	movelist_t mvlist;
	trans_entry_t *entry;
	uci_option_t *opt = Guci_options;
	basic_move_t hashMove;
	int alpha, beta;

	ASSERT(pos != NULL);
#ifdef EVAL_DEBUG
	showEval = true;
	{
		int opt, pes;
		Print(3,"info string eval %d",eval(pos,0, &opt, &pes));
		Print(3,"opt %d pes %d\n",opt,pes);
	}
	showEval = false;
#endif

	hashMove = 0;

	transNewDate(TransTable(thread_id).date,thread_id);
	entry = transProbe(pos->hash,thread_id);
	if (entry != NULL) hashMove = transMove(entry);
	sortInit(pos, &mvlist, pinnedPieces(pos, pos->side), hashMove, -INF, 1, MoveGenPhaseStandard, 0);
	if (hashMove == EMPTY) { // extend time when there is no hashmove from hashtable, this is useful when just out of the book
		SearchInfo(thread_id).time_limit_max += SearchInfo(thread_id).alloc_time / 2;
		if (SearchInfo(thread_id).time_limit_max > SearchInfo(thread_id).time_limit_abs)
			SearchInfo(thread_id).time_limit_max = SearchInfo(thread_id).time_limit_abs;
	}

	if (SearchInfo(thread_id).moves_is_limited == TRUE) { // limit moves, might be useful with IDea
		for (mvlist.size = 0; SearchInfo(thread_id).moves[mvlist.size] != 0; mvlist.size++) {
			mvlist.list[mvlist.size].m = SearchInfo(thread_id).moves[mvlist.size];
		}
	} else {
		genLegal(pos, &mvlist, (SearchInfo(thread_id).depth_is_limited && SearchInfo(thread_id).depth_limit == MAXPLY)); // generate all under promotion ONLY IF IN ANALYZE MODE
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

	scoreRoot(&mvlist);
	if (mvlist.size == 0) {
		if (SHOW_SEARCH && thread_id < Guci_options->threads) {
			Print(10, "info string No legal moves found!\n");
			displayBoard(pos, 8);
		}
		return;
	}
	SearchInfo(thread_id).legalmoves = mvlist.size;

	initSmpVars();

	// wake up sleeping threads
	for (int i = 1; i < Guci_options->threads; ++i) SetEvent(Threads[i].idle_event); // SMP 

	Threads[thread_id].split_point = &Threads[thread_id].sptable[thread_id];

#ifdef W_EASY
	SearchInfo(thread_id).easy = 0;
	SearchInfo(thread_id).maxEasy = EASY_MAX;

	for (id = 1; id < MAXPLY; id++) {
		if (id >= 6) { // TODO: to be tuned
#ifdef OPT_EASY
			if (SearchInfo(thread_id).easy || getTime() < SearchInfo(thread_id).start_time + (SearchInfo(thread_id).alloc_time / 4)) {
				if (id == EASY_MIN_DEPTH) SearchInfo(thread_id).easy = EASY_MIN; 
				else if (id > EASY_MIN_DEPTH && SearchInfo(thread_id).easy < SearchInfo(thread_id).maxEasy) SearchInfo(thread_id).easy += EASY_INC;
			}
#else
			if (id == EASY_MIN_DEPTH) SearchInfo(thread_id).easy = EASY_MIN; 
			else if (id > EASY_MIN_DEPTH && SearchInfo(thread_id).easy < SearchInfo(thread_id).maxEasy) SearchInfo(thread_id).easy += EASY_INC;
#endif
			alpha = goodAlpha(SearchInfo(thread_id).best_value-16);
			beta = goodBeta(SearchInfo(thread_id).best_value+16);
		} else {
			alpha = -INF;
			beta = INF;
		}
#ifdef DEBUG_EASY
		Print(3, "info string EasyRoot %d\n",SearchInfo(thread_id).easy);
#endif
#else
	for (id = 1; id < MAXPLY; id++) {
		if (id >= 6) { // TODO: to be tuned
			alpha = goodAlpha(SearchInfo(thread_id).best_value-16);
			beta = goodBeta(SearchInfo(thread_id).best_value+16);
		} else {
			alpha = -INF;
			beta = INF;
		}
#endif

		searchRoot(pos, &mvlist, alpha, beta, id, thread_id,-INF);

		if (SearchInfo(thread_id).thinking_status == STOPPED) break;
	}
	if (SearchInfo(thread_id).thinking_status != STOPPED) {
		if ((SearchInfo(thread_id).node_is_limited || SearchInfo(thread_id).depth_is_limited || SearchInfo(thread_id).time_is_limited) && SearchInfo(thread_id).thinking_status == THINKING) {
			Print(3,"info string max depth, but can stop\n");
			if (SearchInfo(thread_id).depth_is_limited) Print(3,"info string depth limited %d\n",SearchInfo(thread_id).depth_limit);
			if (SearchInfo(thread_id).time_is_limited) Print(3,"info string time limited\n");
			setAllThreadsToStop(thread_id);
		} else {
			Print(3, "info string Waiting for stop, quit, or ponderhit\n");
			do {
				check4Input(pos);
			} while (SearchInfo(thread_id).thinking_status != STOPPED);
			setAllThreadsToStop(thread_id);
		}
	}

	//Print(8, "================================================================\n");
	//for (int i = 0; i < Guci_options->threads; ++i) {
	//	Print(8, "%s: thread_id:%d, num_sp:%d work:%d idle:%d stop:%d started:%d ended:%d nodes:%d numsplits:%d\n", __FUNCTION__, i, 
	//		Threads[i].num_sp, Threads[i].work_assigned, Threads[i].idle, Threads[i].stop, 
	//		Threads[i].started, Threads[i].ended, Threads[i].nodes, Threads[i].numsplits);
	//}
}

