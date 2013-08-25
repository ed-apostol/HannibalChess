/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

inline uint32 transHashLock(trans_entry_t * te) { return te->hashlock; }
inline basic_move_t transMove(trans_entry_t * te) { return te->move; }
inline int transAge(trans_entry_t * te) { return te->age; }
inline int transMask(trans_entry_t * te) { return te->mask; }
inline int transLowerDepth(trans_entry_t * te) { return te->lowerdepth; }
inline int transUpperDepth(trans_entry_t * te) { return te->upperdepth; }
inline int transLowerValue(trans_entry_t * te) { return te->lowervalue; }
inline int transUpperValue(trans_entry_t * te) { return te->uppervalue; }

inline void transSetHashLock(trans_entry_t * te, uint32 hashlock) { te->hashlock = hashlock; }
inline void transSetMove(trans_entry_t * te, basic_move_t move) { te->move = move; }
inline void transSetAge(trans_entry_t * te, uint8 date) { te->age = date; }
inline void transSetMask(trans_entry_t * te, uint8 mask) { te->mask |= mask; }
inline void transRemMask(trans_entry_t * te, uint8 mask) { te->mask &= ~mask; }
inline void transReplaceMask(trans_entry_t * te, uint8 mask) { te->mask = mask; }
inline void transSetLowerDepth(trans_entry_t * te, uint8 lowerdepth) { te->lowerdepth = lowerdepth; }
inline void transSetUpperDepth(trans_entry_t * te, uint8 upperdepth) { te->upperdepth = upperdepth; }
inline void transSetLowerValue(trans_entry_t * te, int16 lowervalue) { te->lowervalue = lowervalue; }
inline void transSetUpperValue(trans_entry_t * te, int16 uppervalue) { te->uppervalue = uppervalue; }

inline int scoreFromTrans(int score, int ply) { return (score > MAXEVAL) ? (score-ply) : ((score < MAXEVAL) ? (score+ply) : score); }
inline int scoreToTrans(int score, int ply) { return (score > MAXEVAL) ? (score+ply) : ((score < MAXEVAL) ? (score-ply) : score); }

basic_move_t transGetHashMove(const uint64 hash, const int thread) {
    int hashDepth = 0;
    basic_move_t hashMove = EMPTY;
    trans_entry_t *entry;
    entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);
    for (int t = 0; t < 4; t++, entry++) {
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

    replace = entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);

    for (t = 0; t < 4; t++, entry++) {
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
                for (int x = t + 1; x < 4; x++) {
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
        score = (TransTable(thread).age[transAge(entry)] * 256) - transMask(entry);
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
        transReplaceMask(replace, MNoMoves);
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
    TransTable(thread).mask = TransTable(thread).size - 4; //size needs to be a power of 2 for the masking to work
    TransTable(thread).table = (trans_entry_t*) malloc((TransTable(thread).size) * sizeof(trans_entry_t)); //associativity of 4 means we need the +3 in theory
    if (TransTable(thread).table == NULL) {
        Print(3, "info string Not enough memory to allocate transposition table.\n");
    }
    transClear(thread);
}

inline uint32 pvGetHashLock (pvhash_entry_t * pve)      { return pve->hashlock; }
inline basic_move_t pvGetMove (pvhash_entry_t * pve)    { return pve->move; }
inline int pvGetAge (pvhash_entry_t * pve)              { return pve->age; }
inline int pvGetDepth (pvhash_entry_t * pve)            { return pve->depth; }
inline int pvGetValue (pvhash_entry_t * pve)            { return pve->score; }

inline void pvSetHashLock (pvhash_entry_t * pve, uint32 hashlock)  { pve->hashlock = hashlock; }
inline void pvSetMove (pvhash_entry_t * pve, basic_move_t move)    { pve->move = move; }
inline void pvSetAge (pvhash_entry_t * pve, uint8 age)             { pve->age = age; }
inline void pvSetDepth (pvhash_entry_t * pve, uint8 depth)         { pve->depth = depth; }
inline void pvSetValue (pvhash_entry_t * pve, int16 value)         { pve->score = value; }

pvhash_entry_t *pvHashProbe(const uint64 hash, const int thread) {
    pvhash_entry_t *entry = PVHashTable.table + (KEY(hash) & PVHashTable.mask);
    uint32 locked = LOCK(hash);
    for (int t = 0; t < 8; t++, entry++) {
        if (pvGetHashLock(entry) == locked) {
            pvSetAge(entry, TransTable(thread).date);
            return entry;
        }
    }
    return NULL;
}

void pvStore(const uint64 hash, const basic_move_t move, const uint8 depth, const int16 value, const int thread) {
    int worst = -INF, t, score;
    pvhash_entry_t *replace, *entry;

    replace = entry = PVHashTable.table + (KEY(hash) & PVHashTable.mask);
    for (t = 0; t < 8; t++, entry++) {
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

void initPVHashTab(pvhashtable_t* pvt, uint64 size) {
    if (pvt->table != NULL) free(pvt->table);
    pvt->size = size;
    pvt->mask = pvt->size - 1;
    pvt->table = (pvhash_entry_t*) malloc(pvt->size * sizeof(pvhash_entry_t));
    pvHashTableClear(pvt);
}

void evalTableClear(evaltable_t* et) {
    memset(et->table, 0, (et->size * sizeof(eval_entry_t)));
}

void initEvalTab(evaltable_t* et, uint64 target) {
    uint64 size=2;

    if (target < 1) target = 1;
    target *= (1024 * 1024);
    size = size/2;


    while (size * sizeof(eval_entry_t) <= target) size*=2;
    if (et->table != NULL) {
        if (size==et->size) {
            Print(3,"info string no change in eval table\n");
            return; //don't reallocate if there is not change
        }
        free(et->table);
    }
    et->size = size;
    et->mask = et->size - 1;
    et->table = (eval_entry_t*) malloc(et->size * sizeof(eval_entry_t));
    evalTableClear(et);
}

void pawnTableClear(pawntable_t* pt) {
    memset(pt->table, 0, (pt->size * sizeof(pawntable_t)));
}
void initPawnTab(pawntable_t* pt, uint64 target) {
    uint64 size=2;

    if (target < 1) target = 1;
    target *= 1024 * 1024;

    while (size * sizeof(pawn_entry_t) <= target) size*=2;
    size = size/2;
    if (pt->table != NULL) {
        if (size==pt->size) {
            Print(3,"info string no change in pawn table\n");
            return; //don't reallocate if there is not change
        }
        free(pt->table);
    }
    pt->size = size;
    pt->mask = pt->size - 1;
    pt->table = (pawn_entry_t*) malloc(pt->size * sizeof(pawn_entry_t));
    pawnTableClear(pt);
}
