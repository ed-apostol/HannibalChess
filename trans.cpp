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

TranspositionTable global_trans_table;
//#define TransTable(thread) (SearchInfo(thread).tt)
#define TransTable(thread) global_trans_table
PvHashTable PVHashTable;

basic_move_t transGetHashMove(const uint64 hash, const int thread) {
    int hashDepth = 0;
    basic_move_t hashMove = EMPTY;
    TransEntry *entry;
    entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);
    for (int t = 0; t < HASH_ASSOC; t++, entry++) {
        if (transHashLock(entry) == LOCK(hash)) {
            if (transMove(entry) != EMPTY && transLowerDepth(entry) > hashDepth) {
                hashDepth = transLowerDepth(entry);
                hashMove = transMove(entry);
            }
        }
    }
    return hashMove;
}

template<HashType ht>
void transStore(const uint64 hash, basic_move_t move, const int depth, const int value, const int thread) {
    int worst = -INF, t, score;
    TransEntry *replace, *entry;

    ASSERT(valueIsOk(value));

    replace = entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);

    for (t = 0; t < HASH_ASSOC; t++, entry++) {
        if (transHashLock(entry) == LOCK(hash)) {
            if (ht == HTLower && depth >= transLowerDepth(entry) && !(transMask(entry) & MExact)) {
                transSetAge(entry, TransTable(thread).date);
                transSetMove(entry, move);
                transSetLowerDepth(entry, depth);
                transSetLowerValue(entry, value);
                transSetMask(entry, MLower);
                transRemMask(entry, MAllLower);
                return;
            }
            if (ht == HTAllLower && depth >= transLowerDepth(entry) && ((transLowerDepth(entry) == 0) || (transMask(entry) & MAllLower))) {
                transSetAge(entry, TransTable(thread).date);
                transSetMove(entry, move);
                transSetLowerDepth(entry, depth);
                transSetLowerValue(entry, value);
                transSetMask(entry, MLower|MAllLower);
                return;
            }
            if (ht == HTUpper && depth >= transUpperDepth(entry) && !(transMask(entry) & MExact)) {
                transSetAge(entry, TransTable(thread).date);
                transSetUpperDepth(entry, depth);
                transSetUpperValue(entry, value);
                transSetMask(entry, MUpper);
                transRemMask(entry, MCutUpper);
                return;
            }
            if (ht == HTCutUpper && depth >= transUpperDepth(entry) && ((transUpperDepth(entry) == 0) || (transMask(entry) & MCutUpper))) {
                transSetAge(entry, TransTable(thread).date);
                transSetUpperDepth(entry, depth);
                transSetUpperValue(entry, value);
                transSetMask(entry, MUpper|MCutUpper);
                return;
            }
            if (ht == HTExact && depth >= MAX(transUpperDepth(entry), transLowerDepth(entry))) {
                PVHashTable.pvStore(hash, move, depth, value, thread);
                transSetMove(entry, move);
                transSetAge(entry, TransTable(thread).date);
                transSetUpperDepth(entry, depth);
                transSetUpperValue(entry, value);
                transSetLowerDepth(entry, depth);
                transSetLowerValue(entry, value);
                transReplaceMask(entry, MExact);

                for (int x = t + 1; x < HASH_ASSOC; x++) {
                    entry++;
                    if (transHashLock(entry) == LOCK(hash)) {
                        memset(entry, 0, sizeof(TransEntry));
                        transSetAge(entry, (TransTable(thread).date+1) % DATESIZE);
                    }
                }
                return;
            }
            TransTable(thread).used++;
        }
        score = (TransTable(thread).age[transAge(entry)] * 256) - MAX(transUpperDepth(entry), transLowerDepth(entry));
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    transSetHashLock(replace, LOCK(hash));
    transSetAge(replace, TransTable(thread).date);
    if (ht == HTLower) {
        transSetMove(replace, move);
        transSetUpperDepth(replace, 0);
        transSetUpperValue(replace, 0);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transReplaceMask(replace, MLower);
    }
    if (ht == HTAllLower) {
        transSetMove(replace, move);
        transSetUpperDepth(replace, 0);
        transSetUpperValue(replace, 0);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transReplaceMask(replace, MLower|MAllLower);
    }
    if (ht == HTUpper) {
        transSetMove(replace, EMPTY);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetLowerDepth(replace, 0);
        transSetLowerValue(replace, 0);
        transReplaceMask(replace, MUpper);
    }
    if (ht == HTCutUpper) {
        transSetMove(replace, EMPTY);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetLowerDepth(replace, 0);
        transSetLowerValue(replace, 0);
        transReplaceMask(replace, MUpper|MCutUpper);
    }
    if (ht == HTExact) {
        PVHashTable.pvStore(hash, move, depth, value, thread);
        transSetMove(replace, move);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transReplaceMask(replace, MExact);
    }
    if (ht == HTNoMoves) {
        transSetMove(replace, EMPTY);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transReplaceMask(replace, MExact|MNoMoves);
    }
}

void transStore(HashType ht, const uint64 hash, basic_move_t move, const int depth, const int value, const int thread) {
    switch (ht) {
    case HTLower: transStore<HTLower>(hash, move, depth, value, thread); break;
    case HTUpper: transStore<HTUpper>(hash, move, depth, value, thread); break; 
    case HTCutUpper: transStore<HTCutUpper>(hash, move, depth, value, thread); break;
    case HTAllLower: transStore<HTAllLower>(hash, move, depth, value, thread); break;
    case HTExact: transStore<HTExact>(hash, move, depth, value, thread); break;
    case HTNoMoves: transStore<HTNoMoves>(hash, move, depth, value, thread); break;
    default: ASSERT(false && "Shouldn't gone here!"); break;
    }
}

void transNewDate(int date, int thread) {
    TransTable(thread).date = (date+1)%DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        TransTable(thread).age[date] = TransTable(thread).date - date + ((TransTable(thread).date < date) ? DATESIZE:0);
    }
    TransTable(thread).used = 1;
}

void transClear(int thread) {
    transNewDate(-1,thread);
    if (TransTable(thread).table != NULL) {
        memset(TransTable(thread).table, 0, (TransTable(thread).size * sizeof(TransEntry)));
    }
}

void initTrans(uint64 target, int thread) {
    uint64 size=2;

    if (target < MIN_TRANS_SIZE) target = MIN_TRANS_SIZE;
    target *= 1024 * 1024;

    //	size should be a factor of 2 for size - 1 to work well I think -SAM
    while (size * sizeof(TransEntry) <= target) size*=2;
    size = size/2;
    if (TransTable(thread).table != NULL) {
        if (size==TransTable(thread).size) {
            Print(3,"info string no change in transition table\n");
            return; //don't reallocate if there is not change
        }
        free(TransTable(thread).table);
    }
    TransTable(thread).size = size;
    TransTable(thread).mask = size - 4; //size needs to be a power of 2 for the masking to work
    TransTable(thread).table = (TransEntry*) malloc((TransTable(thread).size) * sizeof(TransEntry)); //associativity of 4 means we need the +3 in theory
    if (TransTable(thread).table == NULL) {
        Print(3, "info string Not enough memory to allocate transposition table.\n");
    }
    transClear(thread);
}



PvHashEntry* PvHashTable::pvHashProbe(const uint64 hash) {
    PvHashEntry *entry = &table[(KEY(hash) & mask)];
    for (int t = 0; t < PV_ASSOC; t++, entry++) {
        if (entry->pvGetHashLock() == LOCK(hash)) {
            return entry;
        }
    }
    return NULL;
}

PvHashEntry* PvHashTable::getPvEntryFromMove(const uint64 hash, basic_move_t move) {
    PvHashEntry *entry = &table[(KEY(hash) & mask)];
    for (int t = 0; t < PV_ASSOC; t++, entry++) {
        if (entry->pvGetHashLock() == LOCK(hash) && entry->pvGetMove() == move) {
            return entry;
        }
    }
    return NULL;
}

void PvHashTable::pvStore(const uint64 hash, const basic_move_t move, const uint8 depth, const int16 value, const int thread) {
    int worst = -INF, t, score;
    PvHashEntry *replace, *entry;

    replace = entry = &table[(KEY(hash) & mask)];
    for (t = 0; t < PV_ASSOC; t++, entry++) {
        if (entry->pvGetHashLock() == LOCK(hash)) {
            entry->pvSetAge(TransTable(thread).date);
            entry->pvSetMove(move);
            entry->pvSetDepth(depth);
            entry->pvSetValue(value);
            return;
        }
        score = (TransTable(thread).age[entry->pvGetAge()] * 256) - entry->pvGetDepth();
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    replace->pvSetHashLock(LOCK(hash));
    replace->pvSetAge(TransTable(thread).date);
    replace->pvSetMove(move);
    replace->pvSetDepth(depth);
    replace->pvSetValue(value);
}

void PvHashTable::initPVHashTab(uint64 target) {
    delete[] table;
    size = target + PV_ASSOC - 1;
    mask = target - 1;
    table = new PvHashEntry[size];
}




void evalTableClear(EvalHashTable* et) {
    memset(et->table, 0, (et->size * sizeof(EvalEntry)));
}

void initEvalTab(EvalHashTable* et, uint64 target) {
    uint64 size=2;
    uint64 adjSize;

    if (target < 1) target = 1;
    target *= (1024 * 1024);
    while (size * sizeof(EvalEntry) <= target) size*=2;
    size /= 2;
    adjSize = size + EVAL_ASSOC-1;
    if (et->table != NULL) {
        if (adjSize==et->size) {
            Print(3,"info string no change in eval table\n");
            return; //don't reallocate if there is not change
        }
        free(et->table);
    }
    et->size = adjSize;
    et->mask = size - 1;
    et->table = (EvalEntry*) malloc(et->size * sizeof(EvalEntry));
    evalTableClear(et);
}

void pawnTableClear(PawnHashTable* pt) {
    memset(pt->table, 0, (pt->size * sizeof(PawnHashTable)));
}

void initPawnTab(PawnHashTable* pt, uint64 target) {
    uint64 size=2;
    uint64 adjSize;

    if (target < 1) target = 1;
    target *= 1024 * 1024;

    while (size * sizeof(PawnEntry) <= target) size*=2;
    size = size/2;
    adjSize = size + PAWN_ASSOC - 1;
    if (pt->table != NULL) {
        if (adjSize==pt->size) {
            Print(3,"info string no change in pawn table\n");
            return; //don't reallocate if there is not change
        }
        free(pt->table);
    }
    pt->size = adjSize;
    pt->mask = size - 1;
    pt->table = (PawnEntry*) malloc(pt->size * sizeof(PawnEntry));
    pawnTableClear(pt);
}
