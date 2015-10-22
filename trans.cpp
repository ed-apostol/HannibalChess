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

TransEntry* TransTable::GetHashEntry(uint64 hash) {
    TransEntry* entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->LockFound(hash)) return entry;
    }
    return nullptr;
}

void TransTable::StoreLB(const uint64 hash, const basic_move_t move, const int depth, const int value, const int ply, const int evalvalue, const bool singular) {
    int worst = -INF;
    ASSERT(evalvalue != -INF);
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->LockFound(hash)) {
            entry->SetAge(mDate);
            entry->SetEvalValue(evalvalue);
            if (depth >= entry->LBDepth()) {
                entry->SetMove(move);
                entry->SetLBDepth(depth);
                entry->SetLBValue(value, ply);
                entry->RemMask(MSingular);
                entry->SetMask(MLowerbound | (singular ? MSingular : 0));
            }
            return;
        }
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UBDepth(), entry->LBDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetUBDepth(0);
    replace->SetUBValue(0, ply);
    replace->SetLBDepth(depth);
    replace->SetLBValue(value, ply);
    replace->SetEvalValue(evalvalue);
    replace->ReplaceMask(MLowerbound | (singular ? MSingular : 0));
}

void TransTable::StoreUB(const uint64 hash, const int depth, const int value, const int ply, const int evalvalue) {
    int worst = -INF;
    ASSERT(evalvalue != -INF);
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->LockFound(hash)) {
            entry->SetAge(mDate);
            entry->SetEvalValue(evalvalue);
            if (depth >= entry->UBDepth()) {
                entry->SetUBDepth(depth);
                entry->SetUBValue(value, ply);
                entry->SetMask(MUpperbound);
            }
            return;
        }
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UBDepth(), entry->LBDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetUBDepth(depth);
    replace->SetUBValue(value, ply);
    replace->SetLBDepth(0);
    replace->SetLBValue(0, ply);
    replace->SetEvalValue(evalvalue);
    replace->ReplaceMask(MUpperbound);
}

void TransTable::StoreExact(const uint64 hash, const basic_move_t move, const int depth, const int value, const int ply, const int evalvalue, const bool singular) {
    int worst = -INF;
    ASSERT(evalvalue != -INF);
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        if (entry->LockFound(hash)) {
            entry->SetAge(mDate);
            entry->SetEvalValue(evalvalue);
            if (depth >= entry->LBDepth()) {
                entry->SetMove(move);
                entry->SetLBDepth(depth);
                entry->SetLBValue(value, ply);
                entry->RemMask(MSingular);
                entry->SetMask(MLowerbound | (singular ? MSingular : 0));
            }
            if (depth >= entry->UBDepth()) {
                entry->SetUBDepth(depth);
                entry->SetUBValue(value, ply);
                entry->SetMask(MUpperbound);
            }
            return;
        }
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UBDepth(), entry->LBDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(move);
    replace->SetUBDepth(depth);
    replace->SetUBValue(value, ply);
    replace->SetLBDepth(depth);
    replace->SetLBValue(value, ply);
    replace->SetEvalValue(evalvalue);
    replace->ReplaceMask(MLowerbound | MUpperbound | (singular ? MSingular : 0));
}

void TransTable::StoreNoMoves(const uint64 hash, const int depth, const int value, const int ply) {
    int worst = -INF;
    ASSERT(evalvalue != -INF);
    TransEntry *replace, *entry;
    replace = entry = Entry(hash);
    for (int t = 0; t < mBucketSize; t++, entry++) {
        int score = (mAge[entry->Age()] << 8) - MAX(entry->UBDepth(), entry->LBDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->SetHashLock(LOCK(hash));
    replace->SetAge(mDate);
    replace->SetMove(EMPTY);
    replace->SetUBDepth(depth);
    replace->SetUBValue(value, ply);
    replace->SetLBDepth(depth);
    replace->SetLBValue(value, ply);
    replace->SetEvalValue(-MAXEVAL);
    replace->ReplaceMask(MNoMoves);
}

void TransTable::NewDate(int date) {
    mDate = (date + 1) % DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        mAge[date] = (DATESIZE + mDate - date) % DATESIZE;
    }
    mUsed = 1;
}

void TransTable::Clear() {
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
        if (entry->LockFound(hash) && entry->pvMove() == move) {
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
        if (entry->LockFound(hash)) {
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