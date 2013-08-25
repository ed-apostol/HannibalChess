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

inline void transSetHashlock(trans_entry_t * te, uint32 hashlock) { te->hashlock = hashlock; }
inline void transSetMove(trans_entry_t * te, basic_move_t move) { te->move = move; }
inline void transSetAge(trans_entry_t * te, uint8 date) { te->age = date; }
inline void transSetMask(trans_entry_t * te, uint8 mask) { te->mask |= mask; }
inline void transSetMaskRem(trans_entry_t * te, uint8 mask) { te->mask &= ~mask; }
inline void transSetMaskReplace(trans_entry_t * te, uint8 mask) { te->mask = mask; }
inline void transSetLowerDepth(trans_entry_t * te, uint8 lowerdepth) { te->lowerdepth = lowerdepth; }
inline void transSetUpperDepth(trans_entry_t * te, uint8 upperdepth) { te->upperdepth = upperdepth; }
inline void transSetLowerValue(trans_entry_t * te, int16 lowervalue) { te->lowervalue = lowervalue; }
inline void transSetUpperValue(trans_entry_t * te, int16 uppervalue) { te->uppervalue = uppervalue; }

inline int scoreFromTrans(int score, int ply) { return (score > MAXEVAL) ? (score-ply) : ((score < MAXEVAL) ? (score+ply) : score); }
inline int scoreToTrans(int score, int ply) { return (score > MAXEVAL) ? (score+ply) : ((score < MAXEVAL) ? (score-ply) : score); }

trans_entry_t *transProbe(const uint64 hash, const int thread) {
    trans_entry_t *entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);
    uint32 locked = LOCK(hash);

    for (int t = 0; t < 4; t++, entry++) {
        if (entry->hashlock == locked) {
            transSetAge(entry, TransTable(thread).date);
            return entry;
        }
    }

    return NULL;
}

template<HashType ht>
void transStore(const uint64 hash, basic_move_t move, const int depth, const int value, const int thread) {
    int worst = -INF, t, score;
    trans_entry_t *replace, *entry;

    replace = entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);

    for (t = 0; t < 4; t++, entry++) {
        if (transHashLock(entry) == LOCK(hash)) {
            if (transAge(entry) != TransTable(thread).date) TransTable(thread).used++;
            transSetAge(entry, TransTable(thread).date);
            if (ht == HTLower && depth >= transLowerDepth(entry)) {
                transSetMove(entry, move);
                transSetLowerDepth(entry, depth);
                transSetLowerValue(entry, value);
                transSetMask(entry, MLower);
                transSetMaskRem(entry, MAllLower);
            }
            if (ht == HTAllLower && depth >= transLowerDepth(entry)) {
                transSetMove(entry, move);
                transSetLowerDepth(entry, depth);
                transSetLowerValue(entry, value);
                transSetMask(entry, MLower|MAllLower);
            }
            if (ht == HTUpper && depth >= transUpperDepth(entry)) {
                transSetUpperDepth(entry, depth);
                transSetUpperValue(entry, value);
                transSetMask(entry, MUpper);
                transSetMaskRem(entry, MCutUpper);
            }
            if (ht == HTCutUpper && depth >= transUpperDepth(entry)) {
                transSetUpperDepth(entry, depth);
                transSetUpperValue(entry, value);
                transSetMask(entry, MUpper|MCutUpper);
            }
            if (ht == HTExact) {
                if (depth >= transLowerDepth(entry)) {
                    transSetMove(entry, move);
                    transSetLowerDepth(entry, depth);
                    transSetLowerValue(entry, value);
                    transSetMask(entry, MLower);
                    transSetMaskRem(entry, MAllLower);
                }
                if (depth >= transUpperDepth(entry)) {
                    transSetUpperDepth(entry, depth);
                    transSetUpperValue(entry, value);
                    transSetMask(entry, MUpper);
                    transSetMaskRem(entry, MCutUpper);
                }
            }
            return;
        }
        score = (TransTable(thread).age[transAge(entry)] * 256) - transMask(entry);
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    if (transAge(replace) != TransTable(thread).date) TransTable(thread).used++;
    transSetHashlock(replace, LOCK(hash));
    transSetAge(replace, TransTable(thread).date);
    if (ht == HTLower) {
        transSetMove(replace, move);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transSetUpperDepth(replace, 0);
        transSetUpperValue(replace, 0);
        transSetMaskReplace(replace, MLower);
    }
    if (ht == HTAllLower) {
        transSetMove(replace, move);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transSetUpperDepth(replace, 0);
        transSetUpperValue(replace, 0);
        transSetMaskReplace(replace, MLower|MAllLower);
    }
    if (ht == HTUpper) {
        transSetMove(replace, EMPTY);
        transSetLowerDepth(replace, 0);
        transSetLowerValue(replace, 0);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetMaskReplace(replace, MUpper);
    }
    if (ht == HTCutUpper) {
        transSetMove(replace, EMPTY);
        transSetLowerDepth(replace, 0);
        transSetLowerValue(replace, 0);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetMaskReplace(replace, MUpper|MCutUpper);
    }
    if (ht == HTExact) {
        transSetMove(replace, move);
        transSetLowerDepth(replace, depth);
        transSetLowerValue(replace, value);
        transSetUpperDepth(replace, depth);
        transSetUpperValue(replace, value);
        transSetMaskReplace(replace, MLower|MUpper);
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
