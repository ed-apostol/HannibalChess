
#pragma once

template <typename Entity>
class BaseHashTable {
public:
    BaseHashTable () :
        m_pTable(NULL),
        m_Size(0),
        m_Mask(0),
        m_BucketSize(0)
    {}
    ~BaseHashTable() { delete[] m_pTable; }
    virtual Entity* Entry(const uint64 hash) const { return &m_pTable[KEY(hash) & m_Mask]; }
    void Clear() { memset(m_pTable, 0, m_Size * sizeof(Entity)); }
    void Init(uint64 target, const int bucket_size) {
        uint64 size = 2;
        m_BucketSize = bucket_size;
        if (target < 1) target = 1;
        target *= 1024 * 1024;
        while (size * sizeof(Entity) <= target) size*=2;
        size = size / 2;
        if (size + bucket_size - 1 == m_Size) {
            Clear();
        } else {
            m_Size = size + bucket_size - 1;;
            m_Mask = size - 1;
            delete [] m_pTable;
            m_pTable = new Entity[m_Size];
        }
    }
protected:
    Entity* m_pTable;
    uint64 m_Size;
    uint64 m_Mask;
    uint64 m_BucketSize;
};

struct PvHashEntry {
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

class PvHashTable : public BaseHashTable<PvHashEntry> {
public:
    void pvStore(uint64 hash, basic_move_t move, uint8 depth, int16 value, int thread);
    PvHashEntry *pvHashProbe(const uint64 hash) const;
    PvHashEntry *getPvEntryFromMove(const uint64 hash, basic_move_t move) const;
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

class PawnHashTable : public BaseHashTable<PawnEntry> {};

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

class EvalHashTable : public BaseHashTable<EvalEntry> {};








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
#define TransTable(thread) global_trans_table
extern PvHashTable PVHashTable;