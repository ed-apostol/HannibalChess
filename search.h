/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include <functional>
#include <map>
#include "typedefs.h"
#include "threads.h"
#include "trans.h"
#include "utils.h"
#include "book.h"

static const std::string HashStr = "Hash";
static const int MinHash = 1;
static const int MaxHash = 65536;
static const int DefaultHash = 128;

static const std::string PawnHashStr = "Pawn Hash";
static const int MinPHash = 1;
static const int MaxPHash = 1024;
//static const int DefaultPHash = 64; //TCEC
static const int DefaultPHash = 32;

static const std::string EvalCacheStr = "Eval Cache";
static const int MinEvalCache = 1;
static const int MaxEvalCache = 1024;
//static const int DefaultEvalCache = 128; //TCEC
static const int DefaultEvalCache = 64;

static const std::string MultiPVStr = "MultiPV";
static const int MinMultiPV = 1;
static const int MaxMultiPV = 128;
static const int DefaultMultiPV = 1;

static const std::string TimeBufferStr = "Time Buffer";
static const int MinTimeBuffer = 0;
static const int MaxTimeBuffer = 10000;
static const int DefaultTimeBuffer = 1000;

static const std::string ThreadsStr = "Threads";
static const int MinThreads = 1;
static const int MaxThreads = 64;
static const int DefaultThreads = 1;

static const std::string MinSplitDepthStr = "Min Split Depth";
static const int MinSplitDepth = 1;
static const int MaxSplitDepth = 8;
static const int DefaultSplitDepth = 4;

static const std::string ActiveSplitsStr = "Max Active Splits/Thread";
static const int MinActiveSplit = 1;
static const int MaxActiveSplit = 8;
static const int DefaultActiveSplit = 4;

static const std::string ContemptStr = "Contempt";
static const int MinContempt = -100;
static const int MaxContempt = 100;
static const int DefaultContempt = 0;

static const std::string ClearHashStr = "Clear Hash";
static const std::string OwnBookStr = "OwnBook";
static const std::string BookFileStr = "Book File";
static const std::string PonderStr = "Ponder";

struct Options {
    typedef std::function<void()> CallbackFunc;
    Options() {}
    Options(const char* v, CallbackFunc f) : mType("string"), mMin(0), mMax(0), OnChange(f) {
        mDefVal = mCurVal = v;
    }
    Options(bool v, CallbackFunc f) : mType("check"), mMin(0), mMax(0), OnChange(f) {
        mDefVal = mCurVal = (v ? "true" : "false");
    }
    Options(CallbackFunc f) : mType("button"), mMin(0), mMax(0), OnChange(f) {}
    Options(int v, int minv, int maxv, CallbackFunc f) : mType("spin"), mMin(minv), mMax(maxv), OnChange(f) {
        std::ostringstream ss; ss << v; mDefVal = mCurVal = ss.str();
    }
    int GetInt() const {
        return (mType == "spin" ? atoi(mCurVal.c_str()) : mCurVal == "true");
    }
    std::string GetStr() {
        return mCurVal;
    }
    Options& operator=(const std::string& val) {
        if (mType != "button") mCurVal = val;
        OnChange();
        return *this;
    }
    std::string mDefVal, mCurVal, mType;
    int mMin, mMax;
    CallbackFunc OnChange;
};

typedef std::map<std::string, Options> UCIOptionsBasic;

class UCIOptions : public UCIOptionsBasic {
public:
    Options& operator[] (std::string const& key) {
        UCIOptionsBasic::iterator opt = find(key);
        if (opt != end()) {
            return opt->second;
        }
        else {
            mKeys.push_back(key);
            return (insert(std::make_pair(key, Options())).first)->second;
        }
    }
    void Print() const {
        for (int idx = 0; idx < mKeys.size(); ++idx) {
            UCIOptionsBasic::const_iterator itr = find(mKeys[idx]);
            if (itr != end()) {
                const Options& opt = itr->second;
                LogAndPrintOutput log;
                log << "option name " << mKeys[idx] << " type " << opt.mType;
                if (opt.mType != "button") log << " default " << opt.mDefVal;
                if (opt.mType == "spin") log << " min " << opt.mMin << " max " << opt.mMax;
            }
        }
    }
private:
    std::vector<std::string> mKeys;
};

/* the search data structure */
struct SearchInfo {
    void Init() {
        thinking_status = THINKING;
        singular = false;
        pondering = false;
        stop_search = false;
        depth_is_limited = false;
        depth_limit = MAXPLY;
        moves_is_limited = false;
        time_is_limited = false;
        time_limit_max = 0;
        time_limit_abs = 0;
        node_is_limited = false;
        node_limit = 0;
        start_time = last_time = getTime();
        alloc_time = 0;
        best_value = -INF;
        last_value = -INF;
        change = 0;
        research = 0;
        iteration = 0;
        bestmove = EMPTY;
        mate_found = 0;
        currmovenumber = 0;
        multipvIdx = 0;
        legalmoves = 0;
        mvlist_initialized = false;
        memset(moves, 0, sizeof(moves));
        time_buffer = 0;
        contempt = 0;
        multipv = 0;
        mMinSplitDepth = 0;
        mMaxActiveSplitsPerThread = 0;
    }
    volatile int thinking_status;
    volatile bool stop_search; // TODO: replace with sthread.stop?
    bool pondering;
    bool singular;

    int time_buffer;
    int contempt;
    int multipv;
    int mMinSplitDepth;
    int mMaxActiveSplitsPerThread;

    bool depth_is_limited;
    int depth_limit;
    bool moves_is_limited;
    bool time_is_limited;

    volatile int64 time_limit_max;
    int64 time_limit_abs;
    bool node_is_limited;
    uint64 node_limit;

    int64 start_time;
    volatile int64 last_time;
    int64 alloc_time;

    volatile int last_value;
    volatile int best_value;

    int mate_found;
    int currmovenumber;
    volatile int change;
    volatile int research;
    int iteration;

    int multipvIdx;

    int legalmoves;
    basic_move_t bestmove;
    basic_move_t expectedmove;
    basic_move_t easymove;

    basic_move_t moves[MAXMOVES];
    bool mvlist_initialized;
    continuation_t rootPV;
};

class Search;

class Engine {
public:
    Engine();
    ~Engine();
    void StopSearch();
    void PonderHit();
    void SendBestMove();
    void ExtractPvMovesFromHash(position_t& pos, continuation_t& pv, basic_move_t move);
    void RepopulateHash(position_t& pos, continuation_t& rootPV);
    void DisplayPV(continuation_t& pv, int multipvIdx, int depth, int alpha, int beta, int score);
    void TimeManagement(int depth);
    void CheckTime();
    void SearchFromIdleLoop(SplitPoint& sp, Thread& sthread);
    void GetBestMove(Thread& sthread);
    void StartThinking(GoCmdData& data, position_t& pos);

    // hash
    void InitTTHash(size_t size) {
        transtable.Init(size, transtable.BUCKET);
    }
    void InitPVTTHash(size_t size) {
        pvhashtable.Init(size, pvhashtable.BUCKET);
    }
    void InitPawnHash(size_t size) {
        for (Thread* th : mThreads) th->pt.Init(size, th->pt.BUCKET);
    }
    void InitEvalHash(size_t size) {
        for (Thread* th : mThreads) th->et.Init(size, th->et.BUCKET);
    }
    void ClearTTHash() {
        transtable.Clear();
    }
    void ClearPVTTHash() {
        pvhashtable.Clear();
    }
    void ClearPawnHash() {
        for (Thread* th : mThreads) th->pt.Clear();
    }
    void ClearEvalHash() {
        for (Thread* th : mThreads) th->et.Clear();
    }

    // threads
    uint64 ComputeNodes() {
        uint64 nodes = 0;
        for (Thread* th : mThreads) nodes += th->nodes;
        return nodes;
    }
    void InitVars() {
        for (Thread* th : mThreads) {
            th->Init();
        }
        mThreads[0]->stop = false;
    }
    void SetNumThreads(int num) {
        while (mThreads.size() < num) {
            using namespace std::placeholders;
            int id = (int)mThreads.size();
            mThreads.push_back(new Thread(id, mThreads, std::bind(&Engine::GetBestMove, this, _1),
                std::bind(&Engine::SearchFromIdleLoop, this, _1, _2)));
        }
        while (mThreads.size() > num) {
            delete mThreads.back();
            mThreads.pop_back();
        }
        InitPawnHash(uci_opt[PawnHashStr].GetInt());
        InitEvalHash(uci_opt[EvalCacheStr].GetInt());
    }
    void WaitForThink() {
        while (mThinking) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    void SetThinkStarted() {
        mThinking = true;
    }
    void SetThinkFinished() {
        mThinking = false;
    }
    void SetAllThreadsToStop() {
        for (Thread* th : mThreads) {
            th->stop = true;
            th->doSleep = true;
        }
        mTimerThread->stop = true;
    }
    void SetAllThreadsToWork() {
        for (Thread* th : mThreads) {
            if (th->thread_id != 0)
                th->TriggerCondition(); // thread_id == 0 is triggered separately
        }
        mTimerThread->stop = false;
        mTimerThread->TriggerCondition();
    }
    Thread& ThreadFromIdx(int thread_id) {
        return *mThreads[thread_id];
    }
    size_t ThreadNum() const {
        return mThreads.size();
    }

    // UCI options
    void OnClearHash() {
        ClearPawnHash();
        ClearEvalHash();
        ClearTTHash();
        ClearPVTTHash();
    }
    void OnHashChange() {
        InitTTHash(uci_opt[HashStr].GetInt());
    }
    void OnPawnHashChange() {
        InitPawnHash(uci_opt[PawnHashStr].GetInt());
    }
    void OnEvalCacheChange() {
        InitEvalHash(uci_opt[EvalCacheStr].GetInt());
    }
    void OnMultiPvChange() {
        info.multipv = uci_opt[MultiPVStr].GetInt();
    }
    void OnTimeBufferChange() {
        info.time_buffer = uci_opt[TimeBufferStr].GetInt();
    }
    void OnThreadsChange() {
        SetNumThreads(uci_opt[ThreadsStr].GetInt());
    }
    void OnSplitsChange() {
        info.mMinSplitDepth = uci_opt[MinSplitDepthStr].GetInt();
    }
    void OnActiveSplitsChange() {
        info.mMaxActiveSplitsPerThread = uci_opt[ActiveSplitsStr].GetInt();
    }
    void OnContemptChange() {
        info.contempt = uci_opt[ContemptStr].GetInt();
    }
    void OnBookfileChange() {
        mPolyBook.initBook(uci_opt[BookFileStr].GetStr());
    }
    void OnDummyChange() {}

    void InitUCIOptions() {
        uci_opt[HashStr] = Options(DefaultHash, MinHash, MaxHash, std::bind(&Engine::OnHashChange, this));
        uci_opt[PawnHashStr] = Options(DefaultPHash, MinPHash, MaxPHash, std::bind(&Engine::OnPawnHashChange, this));
        uci_opt[EvalCacheStr] = Options(DefaultEvalCache, MinEvalCache, MaxEvalCache, std::bind(&Engine::OnEvalCacheChange, this));
        uci_opt[MultiPVStr] = Options(DefaultMultiPV, MinMultiPV, MaxMultiPV, std::bind(&Engine::OnMultiPvChange, this));
        uci_opt[ClearHashStr] = Options(std::bind(&Engine::OnClearHash, this));
        uci_opt[OwnBookStr] = Options(false, std::bind(&Engine::OnDummyChange, this));
        uci_opt[BookFileStr] = Options("Hannibal.bin", std::bind(&Engine::OnBookfileChange, this));
        uci_opt[PonderStr] = Options(false, std::bind(&Engine::OnDummyChange, this));
        uci_opt[TimeBufferStr] = Options(DefaultTimeBuffer, MinTimeBuffer, MaxTimeBuffer, std::bind(&Engine::OnTimeBufferChange, this));
        uci_opt[ThreadsStr] = Options(DefaultThreads, MinThreads, MaxThreads, std::bind(&Engine::OnThreadsChange, this));
        uci_opt[MinSplitDepthStr] = Options(DefaultSplitDepth, MinSplitDepth, MaxSplitDepth, std::bind(&Engine::OnSplitsChange, this));
        uci_opt[ActiveSplitsStr] = Options(DefaultActiveSplit, MinActiveSplit, MaxActiveSplit, std::bind(&Engine::OnActiveSplitsChange, this));
        uci_opt[ContemptStr] = Options(DefaultContempt, MinContempt, MaxContempt, std::bind(&Engine::OnContemptChange, this));
    }

    void PrintUCIOptions() {
        uci_opt.Print();
    }

    UCIOptions uci_opt;
    pos_store_t UndoStack[MAX_HASH_STORE];
private:
    static const int WORSE_SCORE_CUTOFF = 20;

    std::atomic<bool> mThinking;
    position_t rootpos;
    SearchInfo info;
    Search* search;
    TranspositionTable transtable;
    PvHashTable pvhashtable;
    std::vector<Thread*> mThreads;
    TimerThread* mTimerThread;
    Book mPolyBook;
};

extern inline bool moveIsTactical(uint32 m);
extern inline int historyIndex(uint32 side, uint32 move);
