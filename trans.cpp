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
#include "trans.h"
#include "utils.h"

TransEntry* TranspositionTable::Probe(const uint64 hash) {
	TransEntry *entry = Entry(hash);
	for (int t = 0; t < mBucketSize; t++, entry++) {
		if (entry->HashLock() == LOCK(hash)) {
			entry->SetAge(mDate);
			return entry;
		}
	}
	return nullptr;
}

void TranspositionTable::StoreEval(uint64 hash, int staticEvalValue) {
	int worst = -INF, t, score;
	TransEntry *replace, *entry;

	ASSERT(valueIsOk(value));

	replace = entry = Entry(hash);

	for (t = 0; t < mBucketSize; t++, entry++) {
		if (entry->HashLock() == LOCK(hash)) {
			entry->SetAge(mDate);
			entry->SetEvalValue(staticEvalValue);
			return;
		}
		score = (mAge[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
		if (score > worst) {
			worst = score;
			replace = entry;
		}
	}

	replace->SetHashLock(LOCK(hash));
	replace->SetAge(mDate);
	replace->SetMove(EMPTY);
	replace->SetUpperDepth(0);
	replace->SetUpperValue(0);
	replace->SetLowerDepth(0);
	replace->SetLowerValue(0);
	replace->SetEvalValue(staticEvalValue);
	replace->ReplaceMask(0);
}

void TranspositionTable::StoreLower(uint64 hash, basic_move_t move, int depth, int value, bool singular, int staticEvalValue, bool pv) {
	int worst = -INF, t, score;
	TransEntry *replace, *entry;

	ASSERT(valueIsOk(value));

	replace = entry = Entry(hash);

	for (t = 0; t < mBucketSize; t++, entry++) {
		if (entry->HashLock() == LOCK(hash)) {
			entry->SetAge(mDate);
			entry->SetMove(move);
			entry->SetLowerDepth(depth);
			entry->SetLowerValue(value);
			entry->SetEvalValue(staticEvalValue);
			entry->RemMask(MSingular | MPV);
			entry->SetMask((singular ? MSingular : 0) | (pv ? MPV : 0));
			return;
		}
		score = (mAge[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
		if (score > worst) {
			worst = score;
			replace = entry;
		}
	}

	replace->SetHashLock(LOCK(hash));
	replace->SetAge(mDate);
	replace->SetMove(move);
	replace->SetUpperDepth(0);
	replace->SetUpperValue(0);
	replace->SetLowerDepth(depth);
	replace->SetLowerValue(value);
	replace->SetEvalValue(staticEvalValue);
	replace->ReplaceMask((singular ? MSingular : 0) | (pv ? MPV : 0));
}

void TranspositionTable::StoreUpper(uint64 hash, int depth, int value, int staticEvalValue, bool pv) {
	int worst = -INF, t, score;
	TransEntry *replace, *entry;

	ASSERT(valueIsOk(value));

	replace = entry = Entry(hash);

	for (t = 0; t < mBucketSize; t++, entry++) {
		if (entry->HashLock() == LOCK(hash)) {
			entry->SetAge(mDate);
			entry->SetUpperDepth(depth);
			entry->SetUpperValue(value);
			entry->SetEvalValue(staticEvalValue);
			entry->SetMask(pv ? MPV : 0);
			return;
		}
		score = (mAge[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
		if (score > worst) {
			worst = score;
			replace = entry;
		}
	}

	replace->SetHashLock(LOCK(hash));
	replace->SetAge(mDate);
	replace->SetMove(EMPTY);
	replace->SetUpperDepth(depth);
	replace->SetUpperValue(value);
	replace->SetLowerDepth(0);
	replace->SetLowerValue(0);
	replace->SetEvalValue(staticEvalValue);
	replace->ReplaceMask(0);
}

void TranspositionTable::StoreExact(uint64 hash, basic_move_t move, int depth, int value, bool singular, int staticEvalValue, bool pv) {
	int worst = -INF, t, score;
	TransEntry *replace, *entry;

	ASSERT(valueIsOk(value));

	replace = entry = Entry(hash);

	for (t = 0; t < mBucketSize; t++, entry++) {
		if (entry->HashLock() == LOCK(hash)) {
			entry->SetMove(move);
			entry->SetAge(mDate);
			entry->SetUpperDepth(depth);
			entry->SetUpperValue(value);
			entry->SetLowerDepth(depth);
			entry->SetEvalValue(staticEvalValue);
			entry->SetLowerValue(value);
			entry->ReplaceMask((singular ? MSingular : 0) | (pv ? MPV : 0));
			return;
		}
		score = (mAge[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
		if (score > worst) {
			worst = score;
			replace = entry;
		}
	}

	replace->SetHashLock(LOCK(hash));
	replace->SetAge(mDate);
	replace->SetMove(move);
	replace->SetUpperDepth(depth);
	replace->SetUpperValue(value);
	replace->SetLowerDepth(depth);
	replace->SetLowerValue(value);
	replace->SetEvalValue(staticEvalValue);
	replace->ReplaceMask((singular ? MSingular : 0) | (pv ? MPV : 0));
}

void TranspositionTable::StoreNoMoves(uint64 hash) {
	int worst = -INF, t, score;
	TransEntry *replace, *entry;

	ASSERT(valueIsOk(value));

	replace = entry = Entry(hash);

	for (t = 0; t < mBucketSize; t++, entry++) {
		score = (mAge[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
		if (score > worst) {
			worst = score;
			replace = entry;
		}
	}

	replace->SetHashLock(LOCK(hash));
	replace->SetAge(mDate);
	replace->SetMove(EMPTY);
	replace->SetUpperDepth(0);
	replace->SetUpperValue(0);
	replace->SetLowerDepth(0);
	replace->SetLowerValue(0);
	replace->ReplaceMask(MNoMoves);
}

void TranspositionTable::NewDate(int date) {
	mDate = (date + 1) % DATESIZE;
	for (date = 0; date < DATESIZE; date++) {
		mAge[date] = mDate - date + ((mDate < date) ? DATESIZE : 0);
	}
}

void TranspositionTable::Clear() {
	BaseHashTable<TransEntry>::Clear();
	mDate = 0;
}

void PvHashTable::NewDate(int date) {
	mDate = (date + 1) % DATESIZE;
	for (date = 0; date < DATESIZE; date++) {
		mAge[date] = mDate - date + ((mDate < date) ? DATESIZE : 0);
	}
}

void PvHashTable::Clear() {
	BaseHashTable<PvHashEntry>::Clear();
	mDate = 0;
}

PvHashEntry* PvHashTable::pvEntry(uint64 hash) const {
	PvHashEntry *entry = Entry(hash);
	for (int t = 0; t < mBucketSize; t++, entry++) {
		if (entry->pvHashLock() == LOCK(hash)) {
			entry->pvSetAge(mDate);
			return entry;
		}
	}
	return nullptr;
}

PvHashEntry* PvHashTable::pvEntryFromMove(uint64 hash, basic_move_t move) const {
	PvHashEntry *entry = Entry(hash);
	for (int t = 0; t < mBucketSize; t++, entry++) {
		if (entry->pvHashLock() == LOCK(hash) && entry->pvMove() == move) {
			entry->pvSetAge(mDate);
			return entry;
		}
	}
	return nullptr;
}

void PvHashTable::pvStore(uint64 hash, basic_move_t move, int depth, int16 value) {
	int worst = -INF, t, score;
	PvHashEntry *replace, *entry;

	replace = entry = Entry(hash);
	for (t = 0; t < mBucketSize; t++, entry++) {
		if (entry->pvHashLock() == LOCK(hash)) {
			entry->pvSetAge(Date());
			entry->pvSetMove(move);
			entry->pvSetDepth(depth);
			entry->pvSetValue(value);
			return;
		}
		score = (Age(entry->pvAge()) * 256) - entry->pvDepth();
		if (score > worst) {
			worst = score;
			replace = entry;
		}
	}
	replace->pvSetHashLock(LOCK(hash));
	replace->pvSetAge(Date());
	replace->pvSetMove(move);
	replace->pvSetDepth(depth);
	replace->pvSetValue(value);
}