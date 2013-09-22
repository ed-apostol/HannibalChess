
#pragma once

/* the pawn hash table entry type */
struct PawnEntry {
    uint32 hashlock;
    uint64 passedbits;
    int16 opn;
    int16 end;
    int8 shelter[2];
    int8 kshelter[2];
    int8 qshelter[2];
    //	int8 halfPassed[2]; //currently unused
};

struct EvalEntry {
    uint32 hashlock;
    int16 value;
    int16 pessimism;
};

/* the trans table entry type */
struct TransEntry {
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
struct PawnHashTable {
    PawnHashTable() : table(NULL) {}
    ~PawnHashTable() { 
        if (table) free(table); 
    }
    PawnEntry *table;
    uint64 size;
    uint64 mask;
};

struct EvalHashTable {
    EvalHashTable() : table(NULL) {}
    ~EvalHashTable() { 
        if (table) free(table); 
    }
    EvalEntry *table;
    uint64 size;
    uint64 mask;
};

/* the trans table type */
struct TranspositionTable {
    TranspositionTable() : table(NULL) {}
    ~TranspositionTable() { 
        if (table) free(table); 
    }
    TransEntry *table;
    uint64 size;
    uint64 mask;
    int32 date;
    uint64 used;
    int32 age[DATESIZE];
};

class PvHashEntry {
public:
    PvHashEntry() :
        m_Hashlock(0),
        m_Move(EMPTY),
        m_Score(0),
        m_Depth(0),
        m_Age(0)
    {}
    inline uint32 pvGetHashLock () const { return m_Hashlock; }
    inline basic_move_t pvGetMove () const { return m_Move; }
    inline int pvGetAge () const { return m_Age; }
    inline int pvGetDepth () const { return m_Depth; }
    inline int pvGetScore () const { return m_Score; }

    inline void pvSetHashLock (uint32 hashlock)  { m_Hashlock = hashlock; }
    inline void pvSetMove (basic_move_t move)    { m_Move = move; }
    inline void pvSetAge (uint8 age)             { m_Age = age; }
    inline void pvSetDepth (uint8 depth)         { m_Depth = depth; }
    inline void pvSetValue (int16 value)         { m_Score = value; }
private:
    uint32 m_Hashlock;
    basic_move_t m_Move;
    int16 m_Score;
    uint8 m_Depth;
    uint8 m_Age;
};

class PvHashTable {
public:
    PvHashTable() : table(NULL) {}
    ~PvHashTable() { delete[] table; }
    void initPVHashTab(uint64 target);
    void pvStore(const uint64 hash, const basic_move_t move, const uint8 depth, const int16 value, const int thread);
    PvHashEntry *pvHashProbe(const uint64 hash);
    PvHashEntry *getPvEntryFromMove(const uint64 hash, basic_move_t move);
private:
    PvHashEntry *table;
    uint64 size;
    uint64 mask;
};

extern TranspositionTable global_trans_table;
//#define TransTable(thread) (SearchInfo(thread).tt)
#define TransTable(thread) global_trans_table
extern PvHashTable PVHashTable;

/* trans.c */
extern void initTrans(uint64 target, int thread);
extern void transClear(int thread);
extern void transNewDate(int date, int thread);
extern void transStore(HashType ht, const uint64 hash, basic_move_t move, const int depth, const int value, const int thread);

extern void initPawnTab(PawnHashTable* pt, uint64 target);
extern void initEvalTab(EvalHashTable* et, uint64 target);

extern void pawnTableClear(PawnHashTable* pt);
extern void evalTableClear(EvalHashTable* et);



inline const uint32 transHashLock(TransEntry * te) { return te->hashlock; }
inline const basic_move_t transMove(TransEntry * te) { return te->move; }
inline const int transAge(TransEntry * te) { return te->age; }
inline const int transMask(TransEntry * te) { return te->mask; }
inline const int transLowerDepth(TransEntry * te) { return te->lowerdepth; }
inline const int transUpperDepth(TransEntry * te) { return te->upperdepth; }
inline const int transLowerValue(TransEntry * te) { return te->lowervalue; }
inline const int transUpperValue(TransEntry * te) { return te->uppervalue; }

inline void transSetHashLock(TransEntry * te, const uint32 hashlock) { te->hashlock = hashlock; }
inline void transSetMove(TransEntry * te, const basic_move_t move) { te->move = move; }
inline void transSetAge(TransEntry * te, const uint8 date) { te->age = date; }
inline void transSetMask(TransEntry * te, const uint8 mask) { te->mask |= mask; }
inline void transRemMask(TransEntry * te, const uint8 mask) { te->mask &= ~mask; }
inline void transReplaceMask(TransEntry * te, const uint8 mask) { te->mask = mask; }
inline void transSetLowerDepth(TransEntry * te, const uint8 lowerdepth) { te->lowerdepth = lowerdepth; }
inline void transSetUpperDepth(TransEntry * te, const uint8 upperdepth) { te->upperdepth = upperdepth; }
inline void transSetLowerValue(TransEntry * te, const int16 lowervalue) { te->lowervalue = lowervalue; }
inline void transSetUpperValue(TransEntry * te, const int16 uppervalue) { te->uppervalue = uppervalue; }

inline int scoreFromTrans(int score, int ply) { return (score > MAXEVAL) ? (score-ply) : ((score < MAXEVAL) ? (score+ply) : score); }
inline int scoreToTrans(int score, int ply) { return (score > MAXEVAL) ? (score+ply) : ((score < MAXEVAL) ? (score-ply) : score); }

