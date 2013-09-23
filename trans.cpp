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

