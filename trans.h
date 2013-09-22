
#pragma once

/* the pawn hash table entry type */
struct pawn_entry_t{
    uint32 hashlock;
    uint64 passedbits;
    int16 opn;
    int16 end;
    int8 shelter[2];
    int8 kshelter[2];
    int8 qshelter[2];
    //	int8 halfPassed[2]; //currently unused
};

struct eval_entry_t{
    uint32 hashlock;
    int16 value;
    int16 pessimism;
};

/* the trans table entry type */
struct trans_entry_t{
    uint32 hashlock;
    uint32 move;
    int16 uppervalue;
    int16 lowervalue;
    uint8 mask;
    uint8 age;
    uint8 upperdepth;
    uint8 lowerdepth;
};

/* the pawn hash table type */
struct pawntable_t{
    pawntable_t() : table(NULL) {}
    ~pawntable_t() { 
        if (table) free(table); 
    }
    pawn_entry_t *table;
    uint64 size;
    uint64 mask;
};

struct evaltable_t{
    evaltable_t() : table(NULL) {}
    ~evaltable_t() { 
        if (table) free(table); 
    }
    eval_entry_t *table;
    uint64 size;
    uint64 mask;
};

/* the trans table type */
struct transtable_t{
    transtable_t() : table(NULL) {}
    ~transtable_t() { 
        if (table) free(table); 
    }
    trans_entry_t *table;
    uint64 size;
    uint64 mask;
    int32 date;
    uint64 used;
    int32 age[DATESIZE];
};

struct pvhash_entry_t {
    uint32 hashlock;
    basic_move_t move;
    int16 score;
    uint8 depth;
    uint8 age;
};

struct pvhashtable_t {
    pvhashtable_t() : table(NULL) {}
    ~pvhashtable_t() { 
        if (table) free(table); 
    }
    pvhash_entry_t *table;
    uint64 size;
    uint64 mask;
};

extern transtable_t global_trans_table;
//#define TransTable(thread) (SearchInfo(thread).tt)
#define TransTable(thread) global_trans_table
extern pvhashtable_t PVHashTable;

/* trans.c */
extern void initTrans(uint64 target, int thread);
extern void transClear(int thread);
extern pvhash_entry_t *pvHashProbe(const uint64 hash);
extern void transNewDate(int date, int thread);
extern void transStore(HashType ht, const uint64 hash, basic_move_t move, const int depth, const int value, const int thread);

extern void initPVHashTab(pvhashtable_t* pvt, uint64 target);
extern void initPawnTab(pawntable_t* pt, uint64 target);
extern void initEvalTab(evaltable_t* et, uint64 target);

extern void pawnTableClear(pawntable_t* pt);
extern void evalTableClear(evaltable_t* et);

inline uint32 pvGetHashLock (pvhash_entry_t * pve)      { return pve->hashlock; }
inline basic_move_t pvGetMove (pvhash_entry_t * pve)    { return pve->move; }
inline int pvGetAge (pvhash_entry_t * pve)              { return pve->age; }
inline int pvGetDepth (pvhash_entry_t * pve)            { return pve->depth; }
inline int pvGetValue (pvhash_entry_t * pve)            { return pve->score; }

extern pvhash_entry_t *getPvEntryFromMove(const uint64 hash, basic_move_t move);

inline void pvSetHashLock (pvhash_entry_t * pve, uint32 hashlock)  { pve->hashlock = hashlock; }
inline void pvSetMove (pvhash_entry_t * pve, basic_move_t move)    { pve->move = move; }
inline void pvSetAge (pvhash_entry_t * pve, uint8 age)             { pve->age = age; }
inline void pvSetDepth (pvhash_entry_t * pve, uint8 depth)         { pve->depth = depth; }
inline void pvSetValue (pvhash_entry_t * pve, int16 value)         { pve->score = value; }

inline const uint32 transHashLock(trans_entry_t * te) { return te->hashlock; }
inline const basic_move_t transMove(trans_entry_t * te) { return te->move; }
inline const int transAge(trans_entry_t * te) { return te->age; }
inline const int transMask(trans_entry_t * te) { return te->mask; }
inline const int transLowerDepth(trans_entry_t * te) { return te->lowerdepth; }
inline const int transUpperDepth(trans_entry_t * te) { return te->upperdepth; }
inline const int transLowerValue(trans_entry_t * te) { return te->lowervalue; }
inline const int transUpperValue(trans_entry_t * te) { return te->uppervalue; }

inline void transSetHashLock(trans_entry_t * te, const uint32 hashlock) { te->hashlock = hashlock; }
inline void transSetMove(trans_entry_t * te, const basic_move_t move) { te->move = move; }
inline void transSetAge(trans_entry_t * te, const uint8 date) { te->age = date; }
inline void transSetMask(trans_entry_t * te, const uint8 mask) { te->mask |= mask; }
inline void transRemMask(trans_entry_t * te, const uint8 mask) { te->mask &= ~mask; }
inline void transReplaceMask(trans_entry_t * te, const uint8 mask) { te->mask = mask; }
inline void transSetLowerDepth(trans_entry_t * te, const uint8 lowerdepth) { te->lowerdepth = lowerdepth; }
inline void transSetUpperDepth(trans_entry_t * te, const uint8 upperdepth) { te->upperdepth = upperdepth; }
inline void transSetLowerValue(trans_entry_t * te, const int16 lowervalue) { te->lowervalue = lowervalue; }
inline void transSetUpperValue(trans_entry_t * te, const int16 uppervalue) { te->uppervalue = uppervalue; }

inline int scoreFromTrans(int score, int ply) { return (score > MAXEVAL) ? (score-ply) : ((score < MAXEVAL) ? (score+ply) : score); }
inline int scoreToTrans(int score, int ply) { return (score > MAXEVAL) ? (score+ply) : ((score < MAXEVAL) ? (score-ply) : score); }

