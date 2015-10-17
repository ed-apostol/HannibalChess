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

TransEntry* TranspositionTable::GetHashEntry(uint64 hash) {
    TransEntry* entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) return entry;
    }
    return nullptr;
}

void TranspositionTable::StoreLowerbound(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
    int worst = -INF;
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            entry->SetAge(mDate);
            if (depth >= entry->LowerboundDepth()) {
                entry->SetMove(move);
                entry->SetLowerboundDepth(depth);
                entry->SetLowerboundValue(value);
                entry->RemMask(MSingular);
                entry->SetMask(singular ? MSingular : 0);
            }
            return;
        }
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UpperboundDepth(), entry->LowerboundDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetUpperboundDepth(0);
    replace->SetUpperboundValue(0);
    replace->SetLowerboundDepth(depth);
    replace->SetLowerboundValue(value);
    replace->ReplaceMask(singular ? MSingular : 0);
}

void TranspositionTable::StoreUpperbound(const uint64 hash, const int depth, const int value) {
    int worst = -INF;
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            entry->SetAge(mDate);
            if (depth >= entry->UpperboundDepth()) {
                entry->SetUpperboundDepth(depth);
                entry->SetUpperboundValue(value);
            }
            return;
        }
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UpperboundDepth(), entry->LowerboundDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetUpperboundDepth(depth);
    replace->SetUpperboundValue(value);
    replace->SetLowerboundDepth(0);
    replace->SetLowerboundValue(0);
    replace->ReplaceMask(0);
}

void TranspositionTable::StoreExact(const uint64 hash, basic_move_t move, const int depth, const int value, const bool singular) {
    int worst = -INF;
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            entry->SetAge(mDate);
            if (depth >= entry->LowerboundDepth()) {
                entry->SetMove(move);
                entry->SetLowerboundDepth(depth);
                entry->SetLowerboundValue(value);
                entry->RemMask(MSingular);
                entry->SetMask((singular ? MSingular : 0));
            }
            if (depth >= entry->UpperboundDepth()) {
                entry->SetUpperboundDepth(depth);
                entry->SetUpperboundValue(value);
            }
            return;
        }
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UpperboundDepth(), entry->LowerboundDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetUpperboundDepth(depth);
    replace->SetUpperboundValue(value);
    replace->SetLowerboundDepth(depth);
    replace->SetLowerboundValue(value);
    replace->ReplaceMask(singular ? MSingular : 0);
}

void TranspositionTable::StoreNoMoves(const uint64 hash, const int depth, const int value) {
    int worst = -INF;
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UpperboundDepth(), entry->LowerboundDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetUpperboundDepth(depth);
    replace->SetUpperboundValue(value);
    replace->SetLowerboundDepth(depth);
    replace->SetLowerboundValue(value);
    replace->ReplaceMask(MNoMoves);
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
    int worst = -INF;
    PvHashEntry *replace, *entry;
    replace = entry = &mpTable[(KEY(hash) & mMask)];
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->pvHashLock() == LOCK(hash)) {
            entry->pvSetAge(Date());
            entry->pvSetMove(move);
            entry->pvSetDepth(depth);
            entry->pvSetValue(value);
            return;
        }
        int score = (Age(entry->pvAge()) << 8) - entry->pvDepth();
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