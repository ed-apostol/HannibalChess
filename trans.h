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
    ~BaseHashTable() {
        delete[] mpTable;
    }
    virtual void Clear() {
        memset(mpTable, 0, mSize * sizeof(Entity));
    }
    Entity* Entry(const uint64 hash) const {
        return &mpTable[KEY(hash) & mMask];
    }
    void Init(size_t targetMB, const int bucket_size) {
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
    size_t HashSize() const {
        return mSize;
    }
    uint64 BucketSize() const {
        return mBucketSize;
    }
protected:
    Entity* mpTable;
    size_t mSize;
    uint64 mMask;
    uint64 mBucketSize;
};

struct PvHashEntry {
public:
    PvHashEntry() :
        mHashlock(0),
        mMove(EMPTY),
        mScore(0),
        mDepth(0),
        mAge(0) {}
    inline uint32 pvHashLock() const {
        return mHashlock;
    }
    inline basic_move_t pvMove() const {
        return mMove;
    }
    inline int pvAge() const {
        return mAge;
    }
    inline int pvDepth() const {
        return mDepth;
    }
    inline int pvScore() const {
        return mScore;
    }

    inline void pvSetHashLock(const uint32 hashlock) {
        mHashlock = hashlock;
    }
    inline void pvSetMove(const basic_move_t move) {
        mMove = move;
    }
    inline void pvSetAge(const uint8 age) {
        mAge = age;
    }
    inline void pvSetDepth(const int8 depth) {
        mDepth = depth;
    }
    inline void pvSetValue(const int16 value) {
        mScore = value;
    }
private:
    uint32 mHashlock;
    basic_move_t mMove;
    int16 mScore;
    uint8 mDepth;
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
    int32 Date() const {
        return mDate;
    }
    int32 Age(const int Idx) const {
        return mAge[Idx];
    }
private:
    int32 mDate;
    int32 mAge[DATESIZE];
};

struct PawnEntry {
    PawnEntry() :
        hashlock(0),
        passedbits(0),
        opn(0),
        end(0) {
        shelter[0] = shelter[1] = 0;
        kshelter[0] = kshelter[1] = 0;
        qshelter[0] = qshelter[1] = 0;
    }
    uint32 hashlock;
    int16 opn;
    int16 end;
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

struct EvalEntry {
    EvalEntry() :
        hashlock(0),
        value(0) {}
    uint32 hashlock;
    int32 value;
};

class EvalHashTable : public BaseHashTable < EvalEntry > {
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
        mLowerDepth(0) {}
    inline uint32 HashLock() const {
        return mHashlock;
    }
    inline basic_move_t Move() const {
        return mMove;
    }
    inline int Age() const {
        return mAge;
    }
    inline int Mask() const {
        return mMask;
    }
    inline int LowerDepth() const {
        return mLowerDepth;
    }
    inline int UpperDepth() const {
        return mUpperDepth;
    }
    inline int LowerValue() const {
        return mLowerValue;
    }
    inline int UpperValue() const {
        return mUpperValue;
    }

    inline void SetHashLock(const uint32 hashlock) {
        mHashlock = hashlock;
    }
    inline void SetMove(const basic_move_t move) {
        mMove = move;
    }
    inline void SetAge(const uint8 date) {
        mAge = date;
    }
    inline void SetMask(const uint8 mask) {
        mMask |= mask;
    }
    inline void RemMask(const uint8 mask) {
        mMask &= ~mask;
    }
    inline void ReplaceMask(const uint8 mask) {
        mMask = mask;
    }
    inline void SetLowerDepth(const int8 lowerdepth) {
        mLowerDepth = lowerdepth;
    }
    inline void SetUpperDepth(const int8 upperdepth) {
        mUpperDepth = upperdepth;
    }
    inline void SetLowerValue(const int16 lowervalue) {
        mLowerValue = lowervalue;
    }
    inline void SetUpperValue(const int16 uppervalue) {
        mUpperValue = uppervalue;
    }
private:
    uint32 mHashlock;
    uint32 mMove;
    int16 mUpperValue;
    int16 mLowerValue;
    uint8 mMask;
    uint8 mAge;
    int8 mUpperDepth;
    int8 mLowerDepth;
};

class TranspositionTable : public BaseHashTable < TransEntry > {
public:
    static const int DATESIZE = 256;
    static const int BUCKET = 4;

    virtual void Clear();
    void NewDate(int date);
    void StoreLower(uint64 hash, basic_move_t move, int depth, int value);
    void StoreUpper(uint64 hash, int depth, int value);
    void StoreCutUpper(uint64 hash, int depth, int value);
    void StoreAllLower(uint64 hash, basic_move_t move, int depth, int value);
    void StoreExact(uint64 hash, basic_move_t move, int depth, int value);
    void StoreNoMoves(uint64 hash, int depth, int value);

    int32 Date() const {
        return mDate;
    }
    uint64 Used() const {
        return mUsed;
    }
    int32 Age(const int Idx) const {
        return mAge[Idx];
    }
private:
    int32 mDate;
    uint64 mUsed;
    int32 mAge[DATESIZE];
};

inline int scoreFromTrans(int score, int ply) { // TODO: make static inside TranspositionTable
    return (score > MAXEVAL) ? (score - ply) : ((score < -MAXEVAL) ? (score + ply) : score);
}
inline int scoreToTrans(int score, int ply) { // TODO: make static inside TranspositionTable
    return (score > MAXEVAL) ? (score + ply) : ((score < -MAXEVAL) ? (score - ply) : score);
}
