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
#include "trans.h"
#include "utils.h"

TranspositionTable TransTable;
PvHashTable PVHashTable;

void TranspositionTable::StoreLower(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->LowerDepth() && !(entry->Mask() & MExact)) {
                entry->SetAge(m_Date);
                entry->SetMove(move);
                entry->SetLowerDepth(depth);
                entry->SetLowerValue(value);
                entry->SetMask(MLower);
                entry->RemMask(MAllLower);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(m_Date);
    replace->SetMove(move);
    replace->SetUpperDepth(0);
    replace->SetUpperValue(0);
    replace->SetLowerDepth(depth);
    replace->SetLowerValue(value);
    replace->ReplaceMask(MLower);
}

void TranspositionTable::StoreUpper(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->UpperDepth() && !(entry->Mask() & MExact)) {
                entry->SetAge(m_Date);
                entry->SetUpperDepth(depth);
                entry->SetUpperValue(value);
                entry->SetMask(MUpper);
                entry->RemMask(MCutUpper);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(m_Date);
    replace->SetMove(EMPTY);
    replace->SetUpperDepth(depth);
    replace->SetUpperValue(value);
    replace->SetLowerDepth(0);
    replace->SetLowerValue(0);
    replace->ReplaceMask(MUpper);
}

void TranspositionTable::StoreAllLower(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->LowerDepth() && ((entry->LowerDepth() == 0) || (entry->Mask() & MAllLower))) {
                entry->SetAge(m_Date);
                entry->SetMove(move);
                entry->SetLowerDepth(depth);
                entry->SetLowerValue(value);
                entry->SetMask(MLower|MAllLower);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(m_Date);
    replace->SetMove(move);
    replace->SetUpperDepth(0);
    replace->SetUpperValue(0);
    replace->SetLowerDepth(depth);
    replace->SetLowerValue(value);
    replace->ReplaceMask(MLower|MAllLower);
}

void TranspositionTable::StoreCutUpper(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= entry->UpperDepth() && ((entry->UpperDepth() == 0) || (entry->Mask() & MCutUpper))) {
                entry->SetAge(m_Date);
                entry->SetUpperDepth(depth);
                entry->SetUpperValue(value);
                entry->SetMask(MUpper|MCutUpper);
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(m_Date);
    replace->SetMove(EMPTY);
    replace->SetUpperDepth(depth);
    replace->SetUpperValue(value);
    replace->SetLowerDepth(0);
    replace->SetLowerValue(0);
    replace->ReplaceMask(MUpper|MCutUpper);
}

void TranspositionTable::StoreExact(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (depth >= MAX(entry->UpperDepth(), entry->LowerDepth())) {
                PVHashTable.pvStore(hash, move, depth, value);
                entry->SetMove(move);
                entry->SetAge(m_Date);
                entry->SetUpperDepth(depth);
                entry->SetUpperValue(value);
                entry->SetLowerDepth(depth);
                entry->SetLowerValue(value);
                entry->ReplaceMask(MExact);
                for (int x = t + 1; x < m_BucketSize; x++) {
                    entry++;
                    if (entry->HashLock() == LOCK(hash)) {
                        memset(entry, 0, sizeof(TransEntry));
                        entry->SetAge((m_Date+1) % DATESIZE);
                    }
                }
                return;
            }
            m_Used++;
        }
        score = (m_Age[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(m_Date);
    PVHashTable.pvStore(hash, move, depth, value);
    replace->SetMove(move);
    replace->SetUpperDepth(depth);
    replace->SetUpperValue(value);
    replace->SetLowerDepth(depth);
    replace->SetLowerValue(value);
    replace->ReplaceMask(MExact);
}

void TranspositionTable::StoreNoMoves(const uint64 hash, basic_move_t move, const int depth, const int value) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = Entry(hash);

    for (t = 0; t < m_BucketSize; t++, entry++) {
        score = (m_Age[entry->Age()] * 256) - MAX(entry->UpperDepth(), entry->LowerDepth());
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    replace->SetHashLock(LOCK(hash));
    replace->SetAge(m_Date);
    replace->SetMove(EMPTY);
    replace->SetUpperDepth(depth);
    replace->SetUpperValue(value);
    replace->SetLowerDepth(depth);
    replace->SetLowerValue(value);
    replace->ReplaceMask(MExact|MNoMoves);
}

basic_move_t TranspositionTable::TransMove(const uint64 hash) {
    int hashDepth = 0;
    basic_move_t hashMove = EMPTY;
    TransEntry *entry = Entry(hash);
    for (int t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->HashLock() == LOCK(hash)) {
            if (entry->Move() != EMPTY && entry->LowerDepth() > hashDepth) {
                hashDepth = entry->LowerDepth();
                hashMove = entry->Move();
            }
        }
    }
    return hashMove;
}

void TranspositionTable::NewDate(int date) {
    m_Date = (date+1)%DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        m_Age[date] = m_Date - date + ((m_Date < date) ? DATESIZE:0);
    }
    m_Used = 1;
}

PvHashEntry* PvHashTable::pvEntry(const uint64 hash) const {
    PvHashEntry *entry = &m_pTable[(KEY(hash) & m_Mask)];
    for (int t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->pvHashLock() == LOCK(hash)) {
            return entry;
        }
    }
    return NULL;
}

PvHashEntry* PvHashTable::pvEntryFromMove(const uint64 hash, basic_move_t move) const {
    PvHashEntry *entry = &m_pTable[(KEY(hash) & m_Mask)];
    for (int t = 0; t < m_BucketSize; t++, entry++) {
        if (entry->pvHashLock() == LOCK(hash) && entry->pvMove() == move) {
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
        if (entry->pvHashLock() == LOCK(hash)) {
            entry->pvSetAge(TransTable.Date());
            entry->pvSetMove(move);
            entry->pvSetDepth(depth);
            entry->pvSetValue(value);
            return;
        }
        score = (TransTable.Age(entry->pvAge()) * 256) - entry->pvDepth();
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

