/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "protos.h"

basic_move_t transGetHashMove(const uint64 hash, const int thread) {
    int hashDepth = 0;
    basic_move_t hashMove = EMPTY;
    trans_entry_t *entry;
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
    trans_entry_t *replace, *entry;

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
                pvStore(hash, move, depth, value, thread);
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
                        memset(entry, 0, sizeof(trans_entry_t));
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
        pvStore(hash, move, depth, value, thread);
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
        memset(TransTable(thread).table, 0, (TransTable(thread).size * sizeof(trans_entry_t)));
    }
}

void initTrans(uint64 target, int thread) {
    uint64 size=2;

    if (target < MIN_TRANS_SIZE) target = MIN_TRANS_SIZE;
    target *= 1024 * 1024;

    //	size should be a factor of 2 for size - 1 to work well I think -SAM
    while (size * sizeof(trans_entry_t) <= target) size*=2;
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
    TransTable(thread).table = (trans_entry_t*) malloc((TransTable(thread).size) * sizeof(trans_entry_t)); //associativity of 4 means we need the +3 in theory
    if (TransTable(thread).table == NULL) {
        Print(3, "info string Not enough memory to allocate transposition table.\n");
    }
    transClear(thread);
}

inline void pvSetHashLock (pvhash_entry_t * pve, uint32 hashlock)  { pve->hashlock = hashlock; }
inline void pvSetMove (pvhash_entry_t * pve, basic_move_t move)    { pve->move = move; }
inline void pvSetAge (pvhash_entry_t * pve, uint8 age)             { pve->age = age; }
inline void pvSetDepth (pvhash_entry_t * pve, uint8 depth)         { pve->depth = depth; }
inline void pvSetValue (pvhash_entry_t * pve, int16 value)         { pve->score = value; }

pvhash_entry_t *pvHashProbe(const uint64 hash) {
    pvhash_entry_t *entry = PVHashTable.table + (KEY(hash) & PVHashTable.mask);
    uint32 locked = LOCK(hash);
    for (int t = 0; t < PV_ASSOC; t++, entry++) {
        if (pvGetHashLock(entry) == locked) {
            return entry;
        }
    }
    return NULL;
}

pvhash_entry_t *getPvEntryFromMove(const uint64 hash, basic_move_t move) {
    pvhash_entry_t *entry = PVHashTable.table + (KEY(hash) & PVHashTable.mask);
    uint32 locked = LOCK(hash);
    for (int t = 0; t < PV_ASSOC; t++, entry++) {
        if (pvGetHashLock(entry) == locked && pvGetMove(entry) == move) {
            return entry;
        }
    }
    return NULL;
}

void pvStore(const uint64 hash, const basic_move_t move, const uint8 depth, const int16 value, const int thread) {
    int worst = -INF, t, score;
    pvhash_entry_t *replace, *entry;

    replace = entry = PVHashTable.table + (KEY(hash) & PVHashTable.mask);
    for (t = 0; t < PV_ASSOC; t++, entry++) {
        if (pvGetHashLock(entry) == LOCK(hash)) {
            pvSetAge(entry, TransTable(thread).date);
            pvSetMove(entry, move);
            pvSetDepth(entry, depth);
            pvSetValue(entry, value);
            return;
        }
        score = (TransTable(thread).age[pvGetAge(entry)] * 256) - pvGetDepth(entry);
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }
    pvSetHashLock(replace, LOCK(hash));
    pvSetAge(replace, TransTable(thread).date);
    pvSetMove(replace, move);
    pvSetDepth(replace, depth);
    pvSetValue(replace, value);
}

void pvHashTableClear(pvhashtable_t* pvt) {
    memset(pvt->table, 0, (pvt->size * sizeof(pvhash_entry_t)));
}

void initPVHashTab(pvhashtable_t* pvt, uint64 target) {
    if (pvt->table != NULL) free(pvt->table);
    pvt->size = target + PV_ASSOC - 1;
    pvt->mask = target - 1;
    pvt->table = (pvhash_entry_t*) malloc((pvt->size) * sizeof(pvhash_entry_t));
    pvHashTableClear(pvt);
}

void evalTableClear(evaltable_t* et) {
    memset(et->table, 0, (et->size * sizeof(eval_entry_t)));
}

void initEvalTab(evaltable_t* et, uint64 target) {
    uint64 size=2;
    uint64 adjSize;

    if (target < 1) target = 1;
    target *= (1024 * 1024);
    while (size * sizeof(eval_entry_t) <= target) size*=2;
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
    et->table = (eval_entry_t*) malloc(et->size * sizeof(eval_entry_t));
    evalTableClear(et);
}

void pawnTableClear(pawntable_t* pt) {
    memset(pt->table, 0, (pt->size * sizeof(pawntable_t)));
}

void initPawnTab(pawntable_t* pt, uint64 target) {
    uint64 size=2;
    uint64 adjSize;

    if (target < 1) target = 1;
    target *= 1024 * 1024;

    while (size * sizeof(pawn_entry_t) <= target) size*=2;
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
    pt->table = (pawn_entry_t*) malloc(pt->size * sizeof(pawn_entry_t));
    pawnTableClear(pt);
}
