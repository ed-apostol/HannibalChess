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

void TranspositionTable::StoreCutNodeFailHigh(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->FailHighDepth() && !(entry->Mask() & MExact)) {
                entry->SetAge(mDate);
                entry->SetMove(move);
                entry->SetFailHighDepth(depth);
                entry->SetFailHighValue(value);
                entry->RemMask(MAllNodeFailHigh);
                entry->RemMask(MSingular);
                entry->SetMask(MCutNodeFailHigh | (singular ? MSingular : 0));
                return;
            }
            mUsed++;
        }
        score = (mAge[entry->Age()] * 256) - MAX(entry->FailLowDepth(), entry->FailHighDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetFailLowDepth(0);
    replace->SetFailLowValue(0);
    replace->SetFailHighDepth(depth);
    replace->SetFailHighValue(value);
    replace->ReplaceMask(MCutNodeFailHigh | (singular ? MSingular : 0));
}

void TranspositionTable::StoreAllNodeFailLow(const uint64 hash, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->FailLowDepth() && !(entry->Mask() & MExact)) {
                entry->SetAge(mDate);
                entry->SetFailLowDepth(depth);
                entry->SetFailLowValue(value);
                entry->SetMask(MAllNodeFailLow);
                entry->RemMask(MCutNodeFailLow);
                return;
            }
            mUsed++;
        }
        score = (mAge[entry->Age()] * 256) - MAX(entry->FailLowDepth(), entry->FailHighDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetFailLowDepth(depth);
    replace->SetFailLowValue(value);
    replace->SetFailHighDepth(0);
    replace->SetFailHighValue(0);
    replace->ReplaceMask(MAllNodeFailLow);
}

void TranspositionTable::StoreAllNodeFailHigh(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->FailHighDepth() && ((entry->FailHighDepth() == 0) || (entry->Mask() & MAllNodeFailHigh))) {
                entry->SetAge(mDate);
                entry->SetMove(move);
                entry->SetFailHighDepth(depth);
                entry->SetFailHighValue(value);
                entry->RemMask(MSingular);
                entry->SetMask(MCutNodeFailHigh | MAllNodeFailHigh | (singular ? MSingular : 0));
                return;
            }
            mUsed++;
        }
        score = (mAge[entry->Age()] * 256) - MAX(entry->FailLowDepth(), entry->FailHighDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetFailLowDepth(0);
    replace->SetFailLowValue(0);
    replace->SetFailHighDepth(depth);
    replace->SetFailHighValue(value);
    replace->ReplaceMask(MCutNodeFailHigh | MAllNodeFailHigh | (singular ? MSingular : 0));
}

void TranspositionTable::StoreCutNodeFailLow(const uint64 hash, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->FailLowDepth() && ((entry->FailLowDepth() == 0) || (entry->Mask() & MCutNodeFailLow))) {
                entry->SetAge(mDate);
                entry->SetFailLowDepth(depth);
                entry->SetFailLowValue(value);
                entry->SetMask(MAllNodeFailLow | MCutNodeFailLow);
                return;
            }
            mUsed++;
        }
        score = (mAge[entry->Age()] * 256) - MAX(entry->FailLowDepth(), entry->FailHighDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetFailLowDepth(depth);
    replace->SetFailLowValue(value);
    replace->SetFailHighDepth(0);
    replace->SetFailHighValue(0);
    replace->ReplaceMask(MAllNodeFailLow | MCutNodeFailLow);
}

void TranspositionTable::StorePVNode(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= MAX(entry->FailLowDepth(), entry->FailHighDepth())) {
                entry->SetMove(move);
                entry->SetAge(mDate);
                entry->SetFailLowDepth(depth);
                entry->SetFailLowValue(value);
                entry->SetFailHighDepth(depth);
                entry->SetFailHighValue(value);
                entry->ReplaceMask(MExact | (singular ? MSingular : 0));
                for (int x = t + 1; x < mBucketSize; x++) {
                    entry++;
                    if (entry->HashLock() == LOCK(hash)) {
                        memset(entry, 0, sizeof(TransEntry));
                        entry->SetAge((mDate + 1) % DATESIZE);
                    }
                }
                return;
            }
            mUsed++;
        }
        score = (mAge[entry->Age()] * 256) - MAX(entry->FailLowDepth(), entry->FailHighDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetFailLowDepth(depth);
    replace->SetFailLowValue(value);
    replace->SetFailHighDepth(depth);
    replace->SetFailHighValue(value);
    replace->ReplaceMask(MExact | (singular ? MSingular : 0));
}

void TranspositionTable::StoreNoMoves(const uint64 hash, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < mBucketSize; t++, entry++) {
        score = (mAge[entry->Age()] * 256) - MAX(entry->FailLowDepth(), entry->FailHighDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetFailLowDepth(depth);
    replace->SetFailLowValue(value);
    replace->SetFailHighDepth(depth);
    replace->SetFailHighValue(value);
    replace->ReplaceMask(MExact | MNoMoves);
}

void TranspositionTable::NewDate(int date) {
    mDate = (date + 1) % DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        mAge[date] = (DATESIZE + mDate - date) % DATESIZE;
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
        mAge[date] = (DATESIZE + mDate - date) % DATESIZE;
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