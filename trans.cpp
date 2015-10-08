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

void TranspositionTable::StoreLower(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
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
            entry->RemMask(MAllLower);
            entry->RemMask(MSingular);
            entry->SetMask(MLower | (singular ? MSingular : 0));
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
    replace->ReplaceMask(MLower | (singular ? MSingular : 0));
}

void TranspositionTable::StoreUpper(const uint64 hash, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            entry->SetAge(mDate);
            entry->SetUpperDepth(depth);
            entry->SetUpperValue(value);
            entry->SetMask(MUpper);
            entry->RemMask(MCutUpper);
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
    replace->ReplaceMask(MUpper);
}

void TranspositionTable::StoreAllLower(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
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
            entry->RemMask(MSingular);
            entry->SetMask(MLower | MAllLower | (singular ? MSingular : 0));
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
    replace->ReplaceMask(MLower | MAllLower | (singular ? MSingular : 0));
}

void TranspositionTable::StoreCutUpper(const uint64 hash, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            entry->SetAge(mDate);
            entry->SetUpperDepth(depth);
            entry->SetUpperValue(value);
            entry->SetMask(MUpper | MCutUpper);
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
    replace->ReplaceMask(MUpper | MCutUpper);
}

void TranspositionTable::StoreExact(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
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
            entry->SetLowerValue(value);
            entry->ReplaceMask(MExact | (singular ? MSingular : 0));
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
    replace->ReplaceMask(MExact | (singular ? MSingular : 0));
}

void TranspositionTable::StoreNoMoves(const uint64 hash, const int depth, const int value) {
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
    replace->SetUpperDepth(depth);
    replace->SetUpperValue(value);
    replace->SetLowerDepth(depth);
    replace->SetLowerValue(value);
    replace->ReplaceMask(MExact | MNoMoves);
}

void TranspositionTable::NewDate(int date) {
    mDate = (date + 1) % DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        mAge[date] = mDate - date + ((mDate < date) ? DATESIZE : 0);
    }
    mUsed = 1;
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

PvHashEntry* PvHashTable::pvEntry(const uint64 hash) const {
    PvHashEntry *entry = &mpTable[(KEY(hash) & mMask)];
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->pvHashLock() == LOCK(hash)) {
            return entry;
        }
    }
    return nullptr;
}

PvHashEntry* PvHashTable::pvEntryFromMove(const uint64 hash, basic_move_t move) const {
    PvHashEntry *entry = &mpTable[(KEY(hash) & mMask)];
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->pvHashLock() == LOCK(hash) && entry->pvMove() == move) {
            return entry;
        }
    }
    return nullptr;
}

void PvHashTable::pvStore(const uint64 hash, const basic_move_t move, const int depth, const int16 value) {
    int worst = -INF, t, score;
    PvHashEntry *replace, *entry;

    replace = entry = &mpTable[(KEY(hash) & mMask)];
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