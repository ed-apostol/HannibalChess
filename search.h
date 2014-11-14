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

static const int MinHash = 1;
static const int MaxHash = 65536;
static const int DefaultHash = 128;

static const int MinPHash = 1;
static const int MaxPHash = 1024;
static const int DefaultPHash = 32;

static const int MinEvalCache = 1;
static const int MaxEvalCache = 1024;
static const int DefaultEvalCache = 64;

static const int MinMultiPV = 1;
static const int MaxMultiPV = 128;
static const int DefaultMultiPV = 1;

static const int MinTimeBuffer = 0;
static const int MaxTimeBuffer = 10000;
static const int DefaultTimeBuffer = 1000;

static const int MinThreads = 1;
static const int MaxThreads = 64;
static const int DefaultThreads = 1;

static const int MinSplitDepth = 2;
static const int MaxSplitDepth = 8;
static const int DefaultSplitDepth = 4;

static const int MinActiveSplit = 1;
static const int MaxActiveSplit = 8;
static const int DefaultActiveSplit = 4;

static const int MinContempt = -100;
static const int MaxContempt = 100;
static const int DefaultContempt = 0;

struct Options {
    typedef std::function<void()> CallbackFunc;
    Options() {}
    Options(const std::string& v, CallbackFunc f) : mType("string"), mMin(0), mMax(0), OnChange(f) {
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
        last_last_value = -INF;
        change = 0;
        research = 0;
        iteration = 0;
        bestmove = 0;
        pondermove = 0;
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
        nodes_since_poll = 0;
        nodes_between_polls = 8192;
    }
    volatile int thinking_status;
    volatile bool stop_search; // TODO: replace with sthread.stop?

    int time_buffer;
    int contempt;
    int multipv;
    int mMinSplitDepth;
    int mMaxActiveSplitsPerThread;

    bool depth_is_limited;
    int depth_limit;
    bool moves_is_limited;
    bool time_is_limited;

    int64 time_limit_max;
    int64 time_limit_abs;
    bool node_is_limited;
    uint64 node_limit;

    int64 start_time;
    int64 last_time;
    int64 alloc_time;

    int last_last_value;
    int last_value;
    int best_value;

    int mate_found;
    int currmovenumber;
    int change;
    int research;
    int iteration;

    int multipvIdx;

    uint64 nodes_since_poll;
    uint64 nodes_between_polls;

    int legalmoves;
    basic_move_t bestmove;
    basic_move_t pondermove;

    basic_move_t moves[MAXMOVES];
    bool mvlist_initialized;
    continuation_t rootPV;
};

class Search;

class Engine {
public:
    Engine();
    ~Engine();
    void PonderHit();
    void SendBestMove();
    void SearchFromIdleLoop(SplitPoint& sp, Thread& sthread);
    void GetBestMove(Thread& sthread);
    void StopSearch();
    void StartThinking(GoCmdData& data, position_t& pos);

    // hash
    void InitTTHash(int size) {
        transtable.Init(size, transtable.BUCKET);
    }
    void InitPVTTHash(int size) {
        pvhashtable.Init(size, pvhashtable.BUCKET);
    }
    void ClearTTHash() {
        transtable.Clear();
    }
    void ClearPVTTHash() {
        pvhashtable.Clear();
    }
    void InitPawnHash(int size) {
        for (Thread* th : mThreads) th->pt.Init(size, th->pt.BUCKET);
    }
    void InitEvalHash(int size) {
        for (Thread* th : mThreads) th->et.Init(size, th->et.BUCKET);
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
            int id = (int)mThreads.size();
            mThreads.push_back(new Thread(id, mThreads, *this));
        }
        while (mThreads.size() > num) {
            delete mThreads.back();
            mThreads.pop_back();
        }
        InitPawnHash(uci_opt["Pawn Hash"].GetInt());
        InitEvalHash(uci_opt["Eval Cache"].GetInt());
    }
    void PrintThreadStats() {
        LogInfo() << "================================================================";
        for (Thread* th : mThreads) {
            LogInfo() << "thread_id: " << th->thread_id
                << " nodes: " << th->nodes
                << " joined_split: " << double(th->numsplits2 * 100.0) / double(th->numsplits)
                << " threads_per_split: " << double(th->workers2) / double(th->numsplits2);
        }
        LogInfo() << "================================================================";
    }
    void WaitForThinkFinished() {
        while (mThinking.test_and_set(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    void SetThinkFinished() {
        mThinking.clear(std::memory_order_release);
    }
    void WaitForThreadsToSleep() {
        for (Thread* th : mThreads) {
            while (!th->doSleep);
        }
    }
    void SetAllThreadsToStop() {
        for (Thread* th : mThreads) {
            th->stop = true;
            th->doSleep = true;
        }
    }
    void SetAllThreadsToWork() {
        for (Thread* th : mThreads) {
            if (th->thread_id != 0) th->TriggerCondition(); // thread_id == 0 is triggered separately
        }
    }
    Thread& ThreadFromIdx(int thread_id) {
        return *mThreads[thread_id];
    }
    size_t ThreadNum() const {
        return mThreads.size();
    }

    // uci options
    void OnClearHash(const std::string& str) {
        ClearPawnHash();
        ClearEvalHash();
        ClearTTHash();
        ClearPVTTHash();
    }
    void OnHashChange(const std::string& str) {
        InitTTHash(uci_opt[str].GetInt());
    }
    void OnPawnHashChange(const std::string& str) {
        InitPawnHash(uci_opt[str].GetInt());
    }
    void OnEvalCacheChange(const std::string& str) {
        InitEvalHash(uci_opt[str].GetInt());
    }
    void OnMultiPvChange(const std::string& str) {
        info.multipv = uci_opt[str].GetInt();
    }
    void OnTimeBufferChange(const std::string& str) {
        info.time_buffer = uci_opt[str].GetInt();
    }
    void OnThreadsChange(const std::string& str) {
        SetNumThreads(uci_opt[str].GetInt());
    }
    void OnSplitsChange(const std::string& str) {
        info.mMinSplitDepth = uci_opt[str].GetInt();
    }
    void OnActiveSplitsChange(const std::string& str) {
        info.mMaxActiveSplitsPerThread = uci_opt[str].GetInt();
    }
    void OnContemptChange(const std::string& str) {
        info.contempt = uci_opt[str].GetInt();
    }
    void OnBookfileChange(const std::string& str) {
        mPolyBook.initBook(uci_opt[str].GetStr());
    }
    void OnDummyChange(const std::string& str) {}

    void InitUCIOptions() {
        uci_opt["Hash"] = Options(DefaultHash, MinHash, MaxHash, std::bind(&Engine::OnHashChange, this, "Hash"));
        uci_opt["Pawn Hash"] = Options(DefaultPHash, MinPHash, MaxPHash, std::bind(&Engine::OnPawnHashChange, this, "Pawn Hash"));
        uci_opt["Eval Cache"] = Options(DefaultEvalCache, MinEvalCache, MaxEvalCache, std::bind(&Engine::OnEvalCacheChange, this, "Eval Cache"));
        uci_opt["MultiPV"] = Options(DefaultMultiPV, MinMultiPV, MaxMultiPV, std::bind(&Engine::OnMultiPvChange, this, "MultiPV"));
        uci_opt["Clear Hash"] = Options(std::bind(&Engine::OnClearHash, this, "Clear Hash"));
        uci_opt["OwnBook"] = Options(false, std::bind(&Engine::OnDummyChange, this, "OwnBook"));
        uci_opt["Book File"] = Options("Hannibal.bin", std::bind(&Engine::OnBookfileChange, this, "Book File"));
        uci_opt["Ponder"] = Options(false, std::bind(&Engine::OnDummyChange, this, "Ponder"));
        uci_opt["Time Buffer"] = Options(DefaultTimeBuffer, MinTimeBuffer, MaxTimeBuffer, std::bind(&Engine::OnTimeBufferChange, this, "Time Buffer"));
        uci_opt["Threads"] = Options(DefaultThreads, MinThreads, MaxThreads, std::bind(&Engine::OnThreadsChange, this, "Threads"));
        uci_opt["Min Split Depth"] = Options(DefaultSplitDepth, MinSplitDepth, MaxSplitDepth, std::bind(&Engine::OnSplitsChange, this, "Min Split Depth"));
        uci_opt["Max Active Splits/Thread"] = Options(DefaultActiveSplit, MinActiveSplit, MaxActiveSplit, std::bind(&Engine::OnActiveSplitsChange, this, "Max Active Splits/Thread"));
        uci_opt["Contempt"] = Options(DefaultContempt, MinContempt, MaxContempt, std::bind(&Engine::OnContemptChange, this, "Contempt"));
    }

    void PrintUCIOptions() {
        uci_opt.Print();
    }

    UCIOptions uci_opt;
private:
    std::atomic_flag mThinking;
    position_t rootpos;
    SearchInfo info;
    Search* search;
    TranspositionTable transtable;
    PvHashTable pvhashtable;
    std::vector<Thread*> mThreads;
    Book mPolyBook;
};

extern inline bool moveIsTactical(uint32 m);
extern inline int historyIndex(uint32 side, uint32 move);
