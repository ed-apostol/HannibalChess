
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
    void Clear() { memset(m_pTable, 0, m_Size * sizeof(Entity)); }
    Entity* Entry(const uint64 hash) const { return &m_pTable[KEY(hash) & m_Mask]; }
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
    uint64 Size() const { return m_Size; }
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
    void pvStore(uint64 hash, basic_move_t move, uint8 depth, int16 value);
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
public:
    TransEntry () :
        m_Hashlock(0),
        m_Move(0),
        m_UpperValue(0),
        m_LowerValue(0),
        m_Mask(0),
        m_Age(0),
        m_UpperDepth(0),
        m_LowerDepth(0)
    {}
    inline const uint32 transHashLock() { return m_Hashlock; }
    inline const basic_move_t transMove() { return m_Move; }
    inline const int transAge() { return m_Age; }
    inline const int transMask() { return m_Mask; }
    inline const int transLowerDepth() { return m_LowerDepth; }
    inline const int transUpperDepth() { return m_UpperDepth; }
    inline const int transLowerValue() { return m_LowerValue; }
    inline const int transUpperValue() { return m_UpperValue; }

    inline void transSetHashLock(const uint32 hashlock) { m_Hashlock = hashlock; }
    inline void transSetMove(const basic_move_t move) { m_Move = move; }
    inline void transSetAge(const uint8 date) { m_Age = date; }
    inline void transSetMask(const uint8 mask) { m_Mask |= mask; }
    inline void transRemMask(const uint8 mask) { m_Mask &= ~mask; }
    inline void transReplaceMask(const uint8 mask) { m_Mask = mask; }
    inline void transSetLowerDepth(const uint8 lowerdepth) { m_LowerDepth = lowerdepth; }
    inline void transSetUpperDepth(const uint8 upperdepth) { m_UpperDepth = upperdepth; }
    inline void transSetLowerValue(const int16 lowervalue) { m_LowerValue = lowervalue; }
    inline void transSetUpperValue(const int16 uppervalue) { m_UpperValue = uppervalue; }
private:
    uint32 m_Hashlock;
    uint32 m_Move;
    int16 m_UpperValue;
    int16 m_LowerValue;
    uint8 m_Mask;
    uint8 m_Age;
    uint8 m_UpperDepth;
    uint8 m_LowerDepth;
};

class TranspositionTable : public BaseHashTable<TransEntry>  {
public:
    enum {
        DATESIZE = 16
    };
    void transNewDate (int date);
    template<HashType ht> void transStore (uint64 hash, basic_move_t move, int depth, int value) {
        switch (ht) {
        case HTLower: transStoreLower(hash, move, depth, value); break;
        case HTUpper: transStoreUpper(hash, move, depth, value); break;
        case HTCutUpper: transStoreCutUpper(hash, move, depth, value); break; 
        case HTAllLower: transStoreAllLower(hash, move, depth, value); break;
        case HTExact: transStoreExact(hash, move, depth, value); break;
        case HTNoMoves: transStoreNoMoves(hash, move, depth, value); break;
        default: Print(1, "Can't be here!\n"); break;
        }
    }
    void transStoreLower (uint64 hash, basic_move_t move, int depth, int value);
    void transStoreUpper (uint64 hash, basic_move_t move, int depth, int value);
    void transStoreCutUpper (uint64 hash, basic_move_t move, int depth, int value);
    void transStoreAllLower (uint64 hash, basic_move_t move, int depth, int value);
    void transStoreExact (uint64 hash, basic_move_t move, int depth, int value);
    void transStoreNoMoves (uint64 hash, basic_move_t move, int depth, int value);

    basic_move_t TranspositionTable::transGetHashMove(uint64 hash);
    int32 Date () const { return m_Date; }
    uint64 Used () const { return m_Used; }
    int32 Age (const int Idx) const { return m_Age[Idx]; }
private:
    int32 m_Date;
    uint64 m_Used;
    int32 m_Age[DATESIZE];
};

inline int scoreFromTrans(int score, int ply) { return (score > MAXEVAL) ? (score-ply) : ((score < MAXEVAL) ? (score+ply) : score); }
inline int scoreToTrans(int score, int ply) { return (score > MAXEVAL) ? (score+ply) : ((score < MAXEVAL) ? (score-ply) : score); }

extern TranspositionTable TransTable;
extern PvHashTable PVHashTable;















