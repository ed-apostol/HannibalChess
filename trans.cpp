/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "trans.h"
#include "utils.h"

TranspositionTable TransTable;
PvHashTable PVHashTable;

void TranspositionTable::transStoreLower(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        if (entry->transHashLock() == LOCK(hash)) {
            if (depth >= entry->transLowerDepth() && !(entry->transMask() & MExact)) {
                entry->transSetAge(m_Date);
                entry->transSetMove(move);
                entry->transSetLowerDepth(depth);
                entry->transSetLowerValue(value);
                entry->transSetMask(MLower);
                entry->transRemMask(MAllLower);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->transAge()] * 256) - MAX(entry->transUpperDepth(), entry->transLowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->transSetHashLock(LOCK(hash));
    replace->transSetAge(m_Date);
    replace->transSetMove(move);
    replace->transSetUpperDepth(0);
    replace->transSetUpperValue(0);
    replace->transSetLowerDepth(depth);
    replace->transSetLowerValue(value);
    replace->transReplaceMask(MLower);
}

void TranspositionTable::transStoreUpper(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        if (entry->transHashLock() == LOCK(hash)) {
            if (depth >= entry->transUpperDepth() && !(entry->transMask() & MExact)) {
                entry->transSetAge(m_Date);
                entry->transSetUpperDepth(depth);
                entry->transSetUpperValue(value);
                entry->transSetMask(MUpper);
                entry->transRemMask(MCutUpper);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->transAge()] * 256) - MAX(entry->transUpperDepth(), entry->transLowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->transSetHashLock(LOCK(hash));
    replace->transSetAge(m_Date);
    replace->transSetMove(EMPTY);
    replace->transSetUpperDepth(depth);
    replace->transSetUpperValue(value);
    replace->transSetLowerDepth(0);
    replace->transSetLowerValue(0);
    replace->transReplaceMask(MUpper);
}

void TranspositionTable::transStoreAllLower(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        if (entry->transHashLock() == LOCK(hash)) {
            if (depth >= entry->transLowerDepth() && ((entry->transLowerDepth() == 0) || (entry->transMask() & MAllLower))) {
                entry->transSetAge(m_Date);
                entry->transSetMove(move);
                entry->transSetLowerDepth(depth);
                entry->transSetLowerValue(value);
                entry->transSetMask(MLower|MAllLower);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->transAge()] * 256) - MAX(entry->transUpperDepth(), entry->transLowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->transSetHashLock(LOCK(hash));
    replace->transSetAge(m_Date);
    replace->transSetMove(move);
    replace->transSetUpperDepth(0);
    replace->transSetUpperValue(0);
    replace->transSetLowerDepth(depth);
    replace->transSetLowerValue(value);
    replace->transReplaceMask(MLower|MAllLower);
}

void TranspositionTable::transStoreCutUpper(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        if (entry->transHashLock() == LOCK(hash)) {
            if (depth >= entry->transUpperDepth() && ((entry->transUpperDepth() == 0) || (entry->transMask() & MCutUpper))) {
                entry->transSetAge(m_Date);
                entry->transSetUpperDepth(depth);
                entry->transSetUpperValue(value);
                entry->transSetMask(MUpper|MCutUpper);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->transAge()] * 256) - MAX(entry->transUpperDepth(), entry->transLowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->transSetHashLock(LOCK(hash));
    replace->transSetAge(m_Date);
    replace->transSetMove(EMPTY);
    replace->transSetUpperDepth(depth);
    replace->transSetUpperValue(value);
    replace->transSetLowerDepth(0);
    replace->transSetLowerValue(0);
    replace->transReplaceMask(MUpper|MCutUpper);
}

void TranspositionTable::transStoreExact(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        if (entry->transHashLock() == LOCK(hash)) {
            if (depth >= MAX(entry->transUpperDepth(), entry->transLowerDepth())) {
                PVHashTable.pvStore(hash, move, depth, value);
                entry->transSetMove(move);
                entry->transSetAge(m_Date);
                entry->transSetUpperDepth(depth);
                entry->transSetUpperValue(value);
                entry->transSetLowerDepth(depth);
                entry->transSetLowerValue(value);
                entry->transReplaceMask(MExact);
                for (int x = t + 1; x < HASH_ASSOC; x++) {
                    entry++;
                    if (entry->transHashLock() == LOCK(hash)) {
                        memset(entry, 0, sizeof(TransEntry));
                        entry->transSetAge((m_Date+1) % DATESIZE);
                    }
                }
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->transAge()] * 256) - MAX(entry->transUpperDepth(), entry->transLowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->transSetHashLock(LOCK(hash));
    replace->transSetAge(m_Date);
    PVHashTable.pvStore(hash, move, depth, value);
    replace->transSetMove(move);
    replace->transSetUpperDepth(depth);
    replace->transSetUpperValue(value);
    replace->transSetLowerDepth(depth);
    replace->transSetLowerValue(value);
    replace->transReplaceMask(MExact);
}

void TranspositionTable::transStoreNoMoves(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        score = (m_Age[entry->transAge()] * 256) - MAX(entry->transUpperDepth(), entry->transLowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->transSetHashLock(LOCK(hash));
    replace->transSetAge(m_Date);
    replace->transSetMove(EMPTY);
    replace->transSetUpperDepth(depth);
    replace->transSetUpperValue(value);
    replace->transSetLowerDepth(depth);
    replace->transSetLowerValue(value);
    replace->transReplaceMask(MExact|MNoMoves);
}

basic_move_t TranspositionTable::transGetHashMove(const uint64 hash) {
    int hashDepth = 0;
    basic_move_t hashMove = EMPTY;
    TransEntry *entry = Entry(hash);
    for (int t = 0; t < HASH_ASSOC; t++, entry++) {
        if (entry->transHashLock() == LOCK(hash)) {
            if (entry->transMove() != EMPTY && entry->transLowerDepth() > hashDepth) {
                hashDepth = entry->transLowerDepth();
                hashMove = entry->transMove();
            }
        }
    }
    return hashMove;
}

void TranspositionTable::transNewDate(int date) {
    m_Date = (date+1)%DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        m_Age[date] = m_Date - date + ((m_Date < date) ? DATESIZE:0);
    }
    m_Used = 1;
}

PvHashEntry* PvHashTable::pvHashProbe(const uint64 hash) const {
    PvHashEntry *entry = &m_pTable[(KEY(hash) & m_Mask)];
    for (int t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->pvGetHashLock() == LOCK(hash)) {
            return entry;
        }
    }
    return NULL;
}

PvHashEntry* PvHashTable::getPvEntryFromMove(const uint64 hash, basic_move_t move) const {
    PvHashEntry *entry = &m_pTable[(KEY(hash) & m_Mask)];
    for (int t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->pvGetHashLock() == LOCK(hash) && entry->pvGetMove() == move) {
            return entry;
        }
    }
    return NULL;
}

void PvHashTable::pvStore(const uint64 hash, const basic_move_t move, const uint8 depth, const int16 value) {
    int worst = -INF, t, score;
    PvHashEntry *replace, *entry;

    replace = entry = &m_pTable[(KEY(hash) & m_Mask)];
    for (t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->pvGetHashLock() == LOCK(hash)) {
            entry->pvSetAge(TransTable.Date());
            entry->pvSetMove(move);
            entry->pvSetDepth(depth);
            entry->pvSetValue(value);
            return;
        }
        score = (TransTable.Age(entry->pvGetAge()) * 256) - entry->pvGetDepth();
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->pvSetHashLock(LOCK(hash));
    replace->pvSetAge(TransTable.Date());
    replace->pvSetMove(move);
    replace->pvSetDepth(depth);
    replace->pvSetValue(value);
}

