
#pragma once

class PvHashEntry { // TODO: should this be a class with set/get or just plain struct POD?
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

    inline void pvSetHashLock (const uint32 hashlock)  { m_Hashlock = hashlock; }
    inline void pvSetMove (const basic_move_t move)    { m_Move = move; }
    inline void pvSetAge (const uint8 age)             { m_Age = age; }
    inline void pvSetDepth (const uint8 depth)         { m_Depth = depth; }
    inline void pvSetValue (const int16 value)         { m_Score = value; }
private:
    uint32 m_Hashlock;
    basic_move_t m_Move;
    int16 m_Score;
    uint8 m_Depth;
    uint8 m_Age;
};

class PvHashTable {
public:
    PvHashTable() : m_pTable(NULL) {}
    ~PvHashTable() { delete[] m_pTable; }
    void initPVHashTab(uint64 target);
    void pvStore(const uint64 hash, const basic_move_t move, const uint8 depth, const int16 value, const int thread);
    PvHashEntry *pvHashProbe(const uint64 hash);
    PvHashEntry *getPvEntryFromMove(const uint64 hash, basic_move_t move);
private:
    PvHashEntry *m_pTable;
    uint64 m_Size;
    uint64 m_Mask;
};

struct PawnEntry {
    PawnEntry () :
        hashlock(0),
        passedbits(0),
        opn(0),
        end(0)        
    {
        shelter[0] = shelter[1] = 0;
        kshelter[0] = kshelter[1] = 0;
        qshelter[0] = qshelter[1] = 0;
    }
    uint32 hashlock;
    uint64 passedbits;
    int16 opn;
    int16 end;
    int8 shelter[2];
    int8 kshelter[2];
    int8 qshelter[2];
};

class PawnHashTable {
public:
    PawnHashTable() : m_Table(NULL) {}
    ~PawnHashTable() { delete[] m_Table; }
    PawnEntry* getPawnEntry(const uint64 hash) { return &m_Table[KEY(hash) & m_Mask]; }
    void pawnTableClear() { memset(m_Table, 0, m_Size * sizeof(PawnEntry)); }

    void initPawnTab(uint64 target);
private:
    PawnEntry *m_Table;
    uint64 m_Size;
    uint64 m_Mask;
};

struct EvalEntry {
    EvalEntry() :
        hashlock(0),
        value(0),
        pessimism(0)
    {}
    uint32 hashlock;
    int16 value;
    int16 pessimism;
};

class EvalHashTable {
public:
    EvalHashTable() : m_Table(NULL) {}
    ~EvalHashTable() { delete[] m_Table; }
    EvalEntry* getEvalEntry(const uint64 hash) { return &m_Table[KEY(hash) & m_Mask]; }
    void evalTableClear() { memset(m_Table, 0, m_Size * sizeof(EvalEntry)); }

    void initEvalTab (uint64 target);
private:
    EvalEntry *m_Table;
    uint64 m_Size;
    uint64 m_Mask;
};







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

/* trans.c */
extern void initTrans(uint64 target, int thread);
extern void transClear(int thread);
extern void transNewDate(int date, int thread);
extern void transStore(HashType ht, const uint64 hash, basic_move_t move, const int depth, const int value, const int thread);

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



extern TranspositionTable global_trans_table;
//#define TransTable(thread) (SearchInfo(thread).tt)
#define TransTable(thread) global_trans_table
extern PvHashTable PVHashTable;