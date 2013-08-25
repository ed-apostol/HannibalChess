/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

uint32 transHashLock(trans_entry_t * te) { return te->hashlock; }
basic_move_t transMove(trans_entry_t * te) { return te->move; }
int transAge(trans_entry_t * te) { return te->age; }
int transMask(trans_entry_t * te) { return te->mask; }
int transLowerDepth(trans_entry_t * te) { return te->lowerdepth; }
int transUpperDepth(trans_entry_t * te) { return te->upperdepth; }
int transLowerValue(trans_entry_t * te) { return te->lowervalue; }
int transUpperValue(trans_entry_t * te) { return te->uppervalue; }

void transSetHashlock(trans_entry_t * te, uint32 hashlock) { te->hashlock = hashlock; }
void transSetMove(trans_entry_t * te, basic_move_t move) { te->move = move; }
void transSetAge(trans_entry_t * te, uint32 date) { te->age = date; }
void transSetMask(trans_entry_t * te, uint8 mask) { te->mask = mask; }
void transSetLowerDepth(trans_entry_t * te, uint8 lowerdepth) { te->lowerdepth = lowerdepth; }
void transSetUpperDepth(trans_entry_t * te, uint8 upperdepth) { te->upperdepth = upperdepth; }
void transSetLowerValue(trans_entry_t * te, int16 lowervalue) { te->lowervalue = lowervalue; }
void transSetUpperValue(trans_entry_t * te, int16 uppervalue) { te->uppervalue = uppervalue; }

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

void transStore(const uint64 hash,uint32 bm, const int d, const int min, const int max, const int thread) {
    int worst = -INF, t, score;
    trans_entry_t *replace, *entry;

    replace = entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);

    for (t = 0; t < 4; t++, entry++) {
        if (transHashLock(entry) == LOCK(hash)) {
            if (transAge(entry) != TransTable(thread).date) TransTable(thread).used++;
            transSetAge(entry, TransTable(thread).date);
            if (max < INF && d >= transUpperDepth(entry)) {
                transSetUpperDepth(entry, d);
                transSetUpperValue(entry, max);
            }
            if (min > -INF && d >= transLowerDepth(entry)) {
                transSetMove(entry, bm);
                transSetLowerDepth(entry, d);
                transSetLowerValue(entry, min);
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
    transSetMove(replace, bm);
    transSetAge(replace, TransTable(thread).date);
    transSetLowerDepth(replace, ((min > -INF) ? d : 0));
    transSetUpperDepth(replace, ((max < INF) ? d : 0));
    transSetLowerValue(replace, min);
    transSetUpperValue(replace, max);
}

void transNewDate(int date, int thread) {
    TransTable(thread).date = (date+1)%DATESIZE;
    for (date = 0; date < DATESIZE; date++) {
        TransTable(thread).age[date] = TransTable(thread).date - date + ((TransTable(thread).date < date) ? DATESIZE:0);
    }
    TransTable(thread).used = 1;
}

void transClear(int thread) {
    trans_entry_t *te;

    transNewDate(-1,thread);
    if (TransTable(thread).table != NULL) {
        memset(TransTable(thread).table, 0, (TransTable(thread).size * sizeof(trans_entry_t)));
        for (te = &TransTable(thread).table[0]; te < &TransTable(thread).table[TransTable(thread).size]; te++) {
            transSetLowerValue(te, -INF); // not important now, but perhaps if we use in qsearch or something
            transSetUpperValue(te, INF); // not important now, but perhaps if we use in qsearch or something
        }
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
