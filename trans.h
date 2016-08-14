/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

template <typename Entity>
class BaseHashTable {
public:
    BaseHashTable() :
        mpTable(nullptr),
        mSize(0),
        mMask(0),
        mBucketSize(0) {}
    ~BaseHashTable() { delete[] mpTable; }
    virtual void Clear() { memset(mpTable, 0, mSize * sizeof(Entity)); }
    Entity* Entry(const uint64 hash) const { return &mpTable[KEY(hash) & mMask]; }
    void Init(size_t targetMB, const size_t bucket_size) {
        size_t size = 2;
        mBucketSize = bucket_size;
        size_t halfTarget = MAX(1, targetMB) * (1024 * 1024) / 2;
        while (size * sizeof(Entity) <= halfTarget) size *= 2;
        if (size + bucket_size - 1 == mSize) {
            Clear();
        }
        else {
            mSize = size + bucket_size - 1;
            mMask = size - 1;
            delete[] mpTable;
            mpTable = new Entity[mSize];
        }
    }
    size_t HashSize() const { return mSize; }
    size_t BucketSize() const { return mBucketSize; }
protected:
    Entity* mpTable;
    size_t mSize;
    size_t mMask;
    size_t mBucketSize;
};

struct PvHashEntry {
public:
    PvHashEntry() :
        mHashlock(0),
        mMove(EMPTY),
        mScore(0),
        mDepth(0),
        mAge(0) {}
    inline uint32 pvHashLock() const { return mHashlock; }
    inline basic_move_t pvMove() const { return mMove; }
    inline int pvAge() const { return mAge; }
    inline int pvDepth() const { return mDepth; }
    inline int pvScore() const { return mScore; }

    inline void pvSetHashLock(uint32 hashlock) { mHashlock = hashlock; }
    inline void pvSetMove(basic_move_t move) { mMove = move; }
    inline void pvSetAge(uint8 age) { mAge = age; }
    inline void pvSetDepth(int8 depth) { mDepth = depth; }
    inline void pvSetValue(int16 value) { mScore = value; }
private:
    uint32 mHashlock;
    basic_move_t mMove;
    int16 mScore;
    int8 mDepth;
    uint8 mAge;
};

class PvHashTable : public BaseHashTable < PvHashEntry > {
public:
    static const int DATESIZE = 256;
    static const int BUCKET = 8;

    virtual void Clear();
    void NewDate(int date);
    void pvStore(uint64 hash, basic_move_t move, int depth, int16 value);
    PvHashEntry *pvEntry(const uint64 hash) const;
    PvHashEntry *pvEntryFromMove(const uint64 hash, basic_move_t move) const;
    int32 Date() const { return mDate; }
    int32 Age(const int Idx) const { return mAge[Idx]; }
private:
    int32 mDate;
    int32 mAge[DATESIZE];
};

struct PawnEntry {
    PawnEntry() :
        hashlock(0),
        passedbits(0),
        score(0),
        pawnWidthBonus(0),
        locked(0) {
        shelter[0] = shelter[1] = 0;
        kshelter[0] = kshelter[1] = 0;
        qshelter[0] = qshelter[1] = 0;
    }
    uint32 hashlock;
    EvalScore score;
    uint64 passedbits;
    int8 shelter[2];
    int8 kshelter[2];
    int8 qshelter[2];
    int8 pawnWidthBonus;
    int8 locked;
};

class PawnHashTable : public BaseHashTable < PawnEntry > {
public:
    static const int BUCKET = 1;
};

struct TransEntry {
public:
    TransEntry() :
        mHashlock(0),
        mMove(0),
        mUpperValue(0),
        mLowerValue(0),
        mMask(0),
        mAge(0),
        mUpperDepth(0),
        mLowerDepth(0),
        mEvalValue(0) {}
    inline uint32 HashLock() const { return mHashlock; }
    inline basic_move_t Move(const position_t& pos) const {
        basic_move_t nm = EMPTY;
        if (mMove != EMPTY) {
            nm = (mMove & 0xfff) | (pos.pieces[moveFrom(mMove)] << 12) | (pos.pieces[moveTo(mMove)] << 18);
            switch (mMove >> 12) {
            case H_CASTLE:  nm |= (M_CASTLE << 15);  break;
            case H_PAWN2:   nm |= (M_PAWN2 << 15);   break;
            case H_EP:      nm |= ((M_EP << 15) | (PAWN << 18)); break;
            case H_PROMN:   nm |= (M_PROMN << 15);   break;
            case H_PROMB:   nm |= (M_PROMB << 15);   break;
            case H_PROMR:   nm |= (M_PROMR << 15);   break;
            case H_PROMQ:   nm |= (M_PROMQ << 15);   break;
            }
        }
        return nm;
    }
    inline int Age() const { return mAge; }
    inline int Mask() const { return mMask; }
    inline int LowerDepth() const { return mLowerDepth; }
    inline int UpperDepth() const { return mUpperDepth; }
    inline int LowerValue() const { return mLowerValue; }
    inline int UpperValue() const { return mUpperValue; }
    inline int EvalValue() const { return mEvalValue; }
    inline void SetHashLock(const uint32 hashlock) { mHashlock = hashlock; }
    inline void SetMove(const basic_move_t move) {
        mMove = move & 0xfff;
        switch ((move >> 15) & 0x3C7) {
        case M_CASTLE:  mMove |= (H_CASTLE << 12);  break;
        case M_PAWN2:   mMove |= (H_PAWN2 << 12);   break;
        case M_EP:      mMove |= (H_EP << 12);      break;
        case M_PROMN:   mMove |= (H_PROMN << 12);   break;
        case M_PROMB:   mMove |= (H_PROMB << 12);   break;
        case M_PROMR:   mMove |= (H_PROMR << 12);   break;
        case M_PROMQ:   mMove |= (H_PROMQ << 12);   break;
        }
    }
    inline void SetAge(const uint8 date) { mAge = date; }
    inline void SetMask(const uint8 mask) { mMask |= mask; }
    inline void RemMask(const uint8 mask) { mMask &= ~mask; }
    inline void ReplaceMask(const uint8 mask) { mMask = mask; }
    inline void SetLowerDepth(const int8 lowerdepth) { mLowerDepth = lowerdepth; }
    inline void SetUpperDepth(const int8 upperdepth) { mUpperDepth = upperdepth; }
    inline void SetLowerValue(const int16 lowervalue) { mLowerValue = lowervalue; }
    inline void SetUpperValue(const int16 uppervalue) { mUpperValue = uppervalue; }
    inline void SetEvalValue(const int16 evalvalue) { mEvalValue = evalvalue; }
private:
    enum {
        H_CASTLE = 1, H_PAWN2, H_EP, H_PROMN, H_PROMB, H_PROMR, H_PROMQ
    };
    enum {
        M_CASTLE = 0x1, M_PAWN2 = 0x2, M_EP = 0x40, M_PROMN = 0x104, M_PROMB = 0x184, M_PROMR = 0x204, M_PROMQ = 0x284
    };
    uint32 mHashlock;
    uint16 mMove;
    int16 mUpperValue;
    int16 mLowerValue;
    uint8 mMask;
    uint8 mAge;
    int8 mUpperDepth;
    int8 mLowerDepth;
    int16 mEvalValue;
};

class TranspositionTable : public BaseHashTable < TransEntry > {
public:
    static const int DATESIZE = 256;
    static const int BUCKET = 4;

    virtual void Clear();
    void NewDate(int date);
    TransEntry* Probe(uint64 hash);
    void StoreEval(uint64 hash, int staticEvalValue);
    void StoreLower(uint64 hash, basic_move_t move, int depth, int value, bool singular, int staticEvalValue, bool pv);
    void StoreUpper(uint64 hash, int depth, int value, int staticEvalValue, bool pv);
    void StoreExact(uint64 hash, basic_move_t move, int depth, int value, bool singular, int staticEvalValue, bool pv);
    void StoreNoMoves(uint64 hash);

    int32 Date() const { return mDate; }
    int32 Age(const int Idx) const { return mAge[Idx]; }
private:
    int32 mDate;
    int32 mAge[DATESIZE];
};

inline int scoreFromTrans(int score, int ply) { // TODO: make static inside TranspositionTable
    return (score > MAXEVAL) ? (score - ply) : ((score < -MAXEVAL) ? (score + ply) : score);
}
inline int scoreToTrans(int score, int ply) { // TODO: make static inside TranspositionTable
    return (score > MAXEVAL) ? (score + ply) : ((score < -MAXEVAL) ? (score - ply) : score);
}
