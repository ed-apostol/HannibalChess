/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

basic_move_t transMove(trans_entry_t * te) {
    return (basic_move_t)(te->data & 0x1ffffff);
}
/*
int transMateThreat(trans_entry_t * te) {
return (1 & (te->data >> 25));
}
*/
int transDate(trans_entry_t * te) {
    return (0x3f & (te->data >> 26));
}

int transDepth(trans_entry_t * te) {
    return te->depth;
}
int transMovedepth(trans_entry_t * te) {
    return te->movedepth;
}

int transMindepth(trans_entry_t * te) {
    return te->mindepth;
}
int transMaxdepth(trans_entry_t * te) {
    return te->maxdepth;
}
uint32 transHashlock(trans_entry_t * te) {
    return te->hashlock;
}
int transMinvalue(trans_entry_t * te) {
    return te->minvalue;
}
int transMaxvalue(trans_entry_t * te) {
    return te->maxvalue;
}

void transSetMove(trans_entry_t * te, uint32 move) {
    te->data &= 0xfe000000;
    te->data |= (move & 0x1ffffff);
}
/*
void transSetMateThreat(trans_entry_t * te, uint32 mthreat) {
te->data &= 0xfdffffff;
te->data |= ((mthreat & 1) << 25);
}
*/
void transSetDate(trans_entry_t * te, uint32 date) {
    te->data &= 0x3ffffff;
    te->data |= ((date & 0x3f) << 26);
}

void transSetDepth(trans_entry_t * te, uint32 depth) {
    te->depth = depth;
}
void transSetMovedepth(trans_entry_t * te, uint32 movedepth) {
    te->movedepth = movedepth;
}

void transSetMindepth(trans_entry_t * te, uint32 mindepth) {
    te->mindepth = mindepth;
}
void transSetMaxdepth(trans_entry_t * te, uint32 maxdepth) {
    te->maxdepth = maxdepth;
}
void transSetHashlock(trans_entry_t * te, uint32 hashlock) {
    te->hashlock = hashlock;
}
void transSetMinvalue(trans_entry_t * te, int minvalue) {
    te->minvalue = minvalue;
}
void transSetMaxvalue(trans_entry_t * te, int maxvalue) {
    te->maxvalue = maxvalue;
}

trans_entry_t *transProbe(const uint64 hash, const int thread) {

    trans_entry_t *entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);
    uint32 locked = LOCK(hash);

    if (entry->hashlock == locked) {
        transSetDate(entry, TransTable(thread).date);
        return entry;
    }
    entry++;
    if (entry->hashlock == locked) {
        transSetDate(entry,TransTable(thread).date);
        return entry;
    }
    entry++;
    if (entry->hashlock == locked) {
        transSetDate(entry, TransTable(thread).date);
        return entry;
    }
    entry++;
    if (entry->hashlock == locked) {
        transSetDate(entry, TransTable(thread).date);
        return entry;
    }
    return NULL;
}

void transStore(const uint64 hash,uint32 bm, const int d, const int min, const int max, const int thread) {

    int worst = -INF, t, score;
    trans_entry_t *replace, *entry;

    replace = entry = TransTable(thread).table + (KEY(hash) & TransTable(thread).mask);

    for (t = 0; t < 4; t++, entry++) {
        if (transHashlock(entry) == LOCK(hash)) {
            if (transDate(entry) != TransTable(thread).date) TransTable(thread).used++;
            transSetDate(entry, TransTable(thread).date);
            if (d > transDepth(entry)) transSetDepth(entry, d);
            if (bm && d > transMovedepth(entry)) {
                transSetMovedepth(entry, d);
                transSetMove(entry, bm);
            }
            if (max < INF && d >= transMaxdepth(entry)) {
                transSetMaxdepth(entry, d);
                transSetMaxvalue(entry, max);

            }
            if (min > -INF && d >= transMindepth(entry)) {
                transSetMindepth(entry, d);
                transSetMinvalue(entry, min);

            }
            return;
        }
        score = (TransTable(thread).age[transDate(entry)] * 256) - transDepth(entry);
        if (score > worst) {
            worst = score;
            replace = entry;
        }
    }

    if (transDate(replace) != TransTable(thread).date) TransTable(thread).used++;
    transSetHashlock(replace, LOCK(hash));
    transSetMove(replace, bm);
    //    transSetMateThreat(replace, mt);
    transSetDate(replace, TransTable(thread).date);
    transSetDepth(replace, d);
    transSetMovedepth(replace, (bm ? d : 0));
    transSetMindepth(replace, ((min > -INF) ? d : 0));
    transSetMaxdepth(replace, ((max < INF) ? d : 0));
    transSetMinvalue(replace, min);
    transSetMaxvalue(replace, max);

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
            transSetMinvalue(te, -INF); // not important now, but perhaps if we use in qsearch or something
            transSetMaxvalue(te, INF); // not important now, but perhaps if we use in qsearch or something
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
    TransTable(thread).mask = TransTable(thread).size - 1; //size needs to be a power of 2 for the masking to work
    TransTable(thread).table = (trans_entry_t*) malloc((TransTable(thread).size+3) * sizeof(trans_entry_t)); //associativity of 4 means we need the +3 in theory
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