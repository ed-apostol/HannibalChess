/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "search.h"
#include "threads.h"
#include "trans.h"
#include "movegen.h"
#include "threads.h"
#include "position.h"
#include "utils.h"
#include "uci.h"
#include "init.h"
#include "material.h"
#include "utils.h"
#include "book.h"

const std::string Interface::name = "Hannibal";
const std::string Interface::author = "Sam Hamilton & Edsel Apostol";
const std::string Interface::year = "2014";
const std::string Interface::version = "1.5x14";
const std::string Interface::arch = "x64";

static const int MinHash = 1;
static const int MaxHash = 65536;
static const int DefaultHash = 64;

static const int MinPHash = 1;
static const int MaxPHash = 1024;
static const int DefaultPHash = 4;

static const int MinEvalCache = 1;
static const int MaxEvalCache = 1024;
static const int DefaultEvalCache = 4;

static const int MinMultiPV = 1;
static const int MaxMultiPV = 128;
static const int DefaultMultiPV = 1;

static const int MinTimeBuffer = 0;
static const int MaxTimeBuffer = 10000;
static const int DefaultTimeBuffer = 1000;

static const int MinThreads = 1;
static const int MaxThreads = 64;
static const int DefaultThreads = 6;

static const int MinSplitDepth = 2;
static const int MaxSplitDepth = 8;
static const int DefaultSplitDepth = 4;

static const int MinActiveSplit = 1;
static const int MaxActiveSplit = 8;
static const int DefaultActiveSplit = 4;

static const int MinContempt = -100;
static const int MaxContempt = 100;
static const int DefaultContempt = 0;

UCIOptions UCIOptionsMap;

void on_clear_hash(const Options& o) {
    ThreadsMgr.ClearPawnHash();
    ThreadsMgr.ClearEvalHash();
    CEngine.ClearTTHash();
    CEngine.ClearPVTTHash();
}
void on_hash(const Options& o) {
    CEngine.InitTTHash(o.GetInt());
}
void on_pawn_hash(const Options& o) {
    ThreadsMgr.InitPawnHash(o.GetInt());
}
void on_eval_hash(const Options& o) {
    ThreadsMgr.InitEvalHash(o.GetInt());
}
void on_multi_pv(const Options& o) {
    CEngine.info.multipv = o.GetInt();
}
void on_time_buffer(const Options& o) {
    CEngine.info.time_buffer = o.GetInt();
}
void on_threads(const Options& o) {
    ThreadsMgr.SetNumThreads(o.GetInt());
}
void on_splits(const Options& o) {
    ThreadsMgr.mMinSplitDepth = o.GetInt();
}
void on_active_splits(const Options& o) {
    ThreadsMgr.mMaxActiveSplitsPerThread = o.GetInt();
}
void on_contempt(const Options& o) {
    CEngine.info.contempt = o.GetInt();
}
void on_bookfile(const Options& o) {
    PolyBook.initBook(UCIOptionsMap["Book File"].GetStr());
}
void on_dummy(const Options& o) {}

void Interface::InitUCIOptions(UCIOptions& uci_opt) {
    uci_opt["Hash"] = Options(DefaultHash, MinHash, MaxHash, on_hash);
    uci_opt["Pawn Hash"] = Options(DefaultPHash, MinPHash, MaxPHash, on_pawn_hash);
    uci_opt["Eval Cache"] = Options(DefaultEvalCache, MinEvalCache, MaxEvalCache, on_eval_hash);
    uci_opt["MultiPV"] = Options(DefaultMultiPV, MinMultiPV, MaxMultiPV, on_multi_pv);
    uci_opt["Clear Hash"] = Options(on_clear_hash);
    uci_opt["OwnBook"] = Options(false, on_dummy);
    uci_opt["Book File"] = Options("Hannibal.bin", on_bookfile);
    uci_opt["Ponder"] = Options(false, on_dummy);
    uci_opt["Time Buffer"] = Options(DefaultTimeBuffer, MinTimeBuffer, MaxTimeBuffer, on_time_buffer);
    uci_opt["Threads"] = Options(DefaultThreads, MinThreads, MaxThreads, on_threads);
    uci_opt["Min Split Depth"] = Options(DefaultSplitDepth, MinSplitDepth, MaxSplitDepth, on_splits);
    uci_opt["Max Active Splits/Thread"] = Options(DefaultActiveSplit, MinActiveSplit, MaxActiveSplit, on_active_splits);
    uci_opt["Contempt"] = Options(DefaultContempt, MinContempt, MaxContempt, on_contempt);
}

void Interface::PrintUCIOptions(UCIOptions& uci_opt) {
    uci_opt.Print();
}

void Interface::Info() {
    LogAndPrintOutput() << name << " " << version << " " << arch;
    LogAndPrintOutput() << "Copyright (C) " << year << " " << author;
    LogAndPrintOutput() << "Use UCI commands";
    LogAndPrintOutput();
}

Interface::Interface() {
    std::cout.setf(std::ios::unitbuf);

    InitUCIOptions(UCIOptionsMap);
    ThreadsMgr.InitVars();
    ThreadsMgr.SetNumThreads(UCIOptionsMap["Threads"].GetInt());
    CEngine.InitTTHash(UCIOptionsMap["Hash"].GetInt());
    CEngine.InitPVTTHash(1);

    initArr();
    initPST();
    InitTrapped();
    initMaterial();
    InitMateBoost();

    setPosition(CEngine.rootpos, STARTPOS);
}

void Interface::Run() {
    std::string line;
    while (getline(std::cin, line)) {
        LogInfo() << line;
        std::istringstream ss(line);
        if (!Input(ss)) break;
    }
}

bool Interface::Input(std::istringstream& stream) {
    std::string command;
    stream >> command;

    if (command == "stop") {
        Stop();
    } else if (command == "ponderhit") {
        Ponderhit();
    } else if (command == "go") {
        Go(stream);
    } else if (command == "position") {
        Position(stream);
    } else if (command == "setoption") {
        SetOption(stream);
    } else if (command == "ucinewgame") {
        NewGame();
    } else if (command == "isready") {
        LogAndPrintOutput() << "readyok";
    } else if (command == "uci") {
        Id();
    } else if (command == "quit") {
        Quit();
        return false;
    } else if (command == "speedup") {
        CheckSpeedup(stream);
    } else if (command == "split") {
        CheckBestSplit(stream);
    } else {
        LogAndPrintError() << "Unknown UCI command: " << command;
    }

    return true;
}

void Interface::Quit() {
    if (ThreadsMgr.StillThinking()) {
        Stop();
        while (ThreadsMgr.StillThinking());
    }
    ThreadsMgr.SetNumThreads(0);
    LogInfo() << "Interface quit";
}

void Interface::Id() {
    LogAndPrintOutput() << "id name " << name << " " << version << " " << arch;
    LogAndPrintOutput() << "id author " << author;
    PrintUCIOptions(UCIOptionsMap);
    LogAndPrintOutput() << "uciok";
}

void Interface::Stop() {
    CEngine.stopSearch();
    LogInfo() << "info string Aborting search: stop";
}

void Interface::Ponderhit() {
    CEngine.ponderHit();
}

void Interface::Go(std::istringstream& stream) {
    std::string command;
    int64 mytime = 0, t_inc = 0;
    int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0, upperdepth = 0, nodes = 0, mate = 0, movetime = 0;
    bool infinite = false, ponder = false;
    SearchInfo& info = CEngine.info;

    info.Init();

    stream >> command;
    while (command != "") {
        if (command == "wtime") {
            stream >> wtime;
        } else if (command == "btime") {
            stream >> btime;
        } else if (command == "winc") {
            stream >> winc;
        } else if (command == "binc") {
            stream >> binc;
        } else if (command == "movestogo") {
            stream >> movestogo;
        } else if (command == "ponder") {
            ponder = true;
        } else if (command == "depth") {
            stream >> upperdepth;
        } else if (command == "movetime") {
            stream >> movetime;
        } else if (command == "infinite") {
            infinite = true;
        } else if (command == "nodes") {
            stream >> nodes;
        } else if (command == "mate") {
            stream >> mate;
        } else {
            LogAndPrintError() << "Wrong go command: " << command;
            return;
        }
        command = "";
        stream >> command;
    }
    if (infinite) {
        info.depth_is_limited = true;
        info.depth_limit = MAXPLY;
        LogInfo() << "info string Infinite";
    }
    if (upperdepth > 0) {
        info.depth_is_limited = true;
        info.depth_limit = upperdepth;
        LogInfo() << "info string Depth is limited to " << info.depth_limit << " half moves";
    }
    if (mate > 0) {
        info.depth_is_limited = true;
        info.depth_limit = mate * 2 - 1;
        LogInfo() << "info string Mate in " << info.depth_limit << " half moves";
    }
    if (nodes > 0) {
        info.node_is_limited = true;
        info.node_limit = nodes;
        LogInfo() << "info string Nodes is limited to " << info.node_limit << " positions";
    }
    if (info.moves[0]) {
        info.moves_is_limited = true;
        LogInfo() << "info string Moves is limited";
    }

    if (movetime > 0) {
        info.time_is_limited = true;
        info.alloc_time = movetime;
        info.time_limit_max = info.start_time + movetime;
        info.time_limit_abs = info.start_time + movetime;
        LogInfo() << "info string Fixed time per move: " << movetime << " ms";
    }
    if (CEngine.rootpos.side == WHITE) {
        mytime = wtime;
        t_inc = winc;
    } else {
        mytime = btime;
        t_inc = binc;
    }
    if (mytime > 0) {
        info.time_is_limited = true;
        mytime = ((mytime * 95) / 100) - info.time_buffer;
        if (mytime  < 0) mytime = 0;
        if (movestogo <= 0 || movestogo > 20) movestogo = 20;
        info.time_limit_max = (mytime / movestogo) + ((t_inc * 4) / 5);
        if (ponder) info.time_limit_max += info.time_limit_max / 4;

        if (info.time_limit_max > mytime) info.time_limit_max = mytime;
        info.time_limit_abs = ((mytime * 2) / 5) + ((t_inc * 4) / 5);
        if (info.time_limit_abs < info.time_limit_max) info.time_limit_abs = info.time_limit_max;
        if (info.time_limit_abs > mytime) info.time_limit_abs = mytime;

        if (info.time_limit_max < 2) info.time_limit_max = 2;
        if (info.time_limit_abs < 2) info.time_limit_abs = 2;

        LogInfo() << "info string Time is limited: ";
        LogInfo() << "max = " << info.time_limit_max;
        LogInfo() << "abs = " << info.time_limit_abs;
        info.alloc_time = info.time_limit_max;
        info.time_limit_max += info.start_time;
        info.time_limit_abs += info.start_time;
    }
    if (infinite) {
        info.thinking_status = ANALYSING;
        LogInfo() << "info string Search status is ANALYSING";
    } else if (ponder) {
        info.thinking_status = PONDERING;
        LogInfo() << "info string Search status is PONDERING";
    } else {
        info.thinking_status = THINKING;
        LogInfo() << "info string Search status is THINKING";
    }

    DrawValue[CEngine.rootpos.side] = -info.contempt;
    DrawValue[CEngine.rootpos.side ^ 1] = info.contempt;

    ThreadsMgr.StartThinking();
}

void Interface::Position(std::istringstream& stream) {
    basic_move_t m;
    std::string token, fen;

    stream >> token;
    if (token == "startpos") {
        fen = STARTPOS;
        stream >> token;
    } else if (token == "fen") {
        while (stream >> token && token != "moves")
            fen += token + " ";
    } else {
        LogWarning() << "Invalid position!";
        return;
    }

    setPosition(CEngine.rootpos, fen.c_str());
    while (stream >> token) {
        movelist_t ml;
        genLegal(CEngine.rootpos, &ml, true);
        m = parseMove(&ml, token.c_str());
        if (m) makeMove(CEngine.rootpos, UndoStack[CEngine.rootpos.sp], m);
        else break;
        if (CEngine.rootpos.posStore.fifty == 0) CEngine.rootpos.sp = 0;
    }
    CEngine.rootpos.ply = 0;
}

void Interface::SetOption(std::istringstream& stream) {
    std::string token, name, value;
    stream >> token;
    while (stream >> token && token != "value") name += std::string(" ", !name.empty()) + token;
    while (stream >> token) value += std::string(" ", !value.empty()) + token;
    if (UCIOptionsMap.count(name)) UCIOptionsMap[name] = value;
    else LogAndPrintOutput() << "No such option: " << name;
}

void Interface::NewGame() {
    ThreadsMgr.ClearPawnHash();
    ThreadsMgr.ClearEvalHash();
    CEngine.ClearTTHash();
    CEngine.ClearPVTTHash();
}

Interface::~Interface() {}

void Interface::CheckSpeedup(std::istringstream& stream) {
    std::istringstream streamcmd;
    std::vector<std::string> fenPos;
    fenPos.push_back("r3k2r/pbpnqp2/1p1ppn1p/6p1/2PP4/2PBPNB1/P4PPP/R2Q1RK1 w kq - 2 12");
    fenPos.push_back("2kr3r/pbpn1pq1/1p3n2/3p1R2/3P3p/2P2Q2/P1BN2PP/R3B2K w - - 4 22");
    fenPos.push_back("r2n1rk1/1pq2ppp/p2pbn2/8/P3Pp2/2PBB2P/2PNQ1P1/1R3RK1 w - - 0 17");
    fenPos.push_back("1r2r2k/1p4qp/p3bp2/4p2R/n3P3/2PB4/2PB1QPK/1R6 w - - 1 32");
    fenPos.push_back("1b3r1k/rb1q3p/pp2pppP/3n1n2/1P2N3/P2B1NPQ/1B3P2/2R1R1K1 b - - 1 32");
    std::vector<double> timeSpeedupSum(2, 0.0);
    std::vector<double> nodesSpeedupSum(2, 0.0);
    std::vector<int> threads(2, 1);
    int depth = 12;

    stream >> threads[1];
    stream >> depth;

    for (int idxpos = 0; idxpos < fenPos.size(); ++idxpos) {
        LogAndPrintOutput() << "\n\nPos#" << idxpos + 1 << ": " << fenPos[idxpos];
        uint64 nodes1 = 0;
        uint64 spentTime1 = 0;
        for (int idxthread = 0; idxthread < threads.size(); ++idxthread) {
            streamcmd = std::istringstream("name Threads value " + std::to_string(threads[idxthread]));
            SetOption(streamcmd);
            NewGame();

            streamcmd = std::istringstream("fen " + fenPos[idxpos]);
            Position(streamcmd);

            int64 startTime = getTime();

            streamcmd = std::istringstream("depth " + std::to_string(depth));
            Go(streamcmd);

            while (ThreadsMgr.StillThinking());

            double timeSpeedUp;
            double nodesSpeedup;
            int64 spentTime = getTime() - startTime;
            uint64 nodes = ThreadsMgr.ComputeNodes() / spentTime;

            if (0 == idxthread) {
                nodes1 = nodes;
                spentTime1 = spentTime;
                timeSpeedUp = (double)spentTime / 1000.0;
                timeSpeedupSum[idxthread] += timeSpeedUp;
                nodesSpeedup = (double)nodes;
                nodesSpeedupSum[idxthread] += nodesSpeedup;
                LogAndPrintOutput() << "\nBase: " << std::to_string(timeSpeedUp) << "s " << std::to_string(nodes) << "knps\n";
            } else {
                timeSpeedUp = (double)spentTime1 / (double)spentTime;
                timeSpeedupSum[idxthread] += timeSpeedUp;
                nodesSpeedup = (double)nodes / (double)nodes1;
                nodesSpeedupSum[idxthread] += nodesSpeedup;
                LogAndPrintOutput() << "\nThread: " << std::to_string(threads[idxthread]) << " time: " << std::to_string(timeSpeedUp) << " nodes: " << std::to_string(nodesSpeedup) << "\n";
            }
        }
    }

    LogAndPrintOutput() << "\n\n";
    LogAndPrintOutput() << "\n\nAvg Base: " << std::to_string(timeSpeedupSum[0] / fenPos.size()) << "s " << std::to_string(nodesSpeedupSum[0] / fenPos.size()) << "knps\n";
    LogAndPrintOutput() << "Threads: " << std::to_string(threads[1]) << " time: " << std::to_string(timeSpeedupSum[1] / fenPos.size()) << " nodes: " << std::to_string(nodesSpeedupSum[1] / fenPos.size()) << "\n";
    LogAndPrintOutput() << "\n\n";
}

void Interface::CheckBestSplit(std::istringstream& stream) {
    std::istringstream streamcmd;
    std::vector<std::string> fenPos;
    fenPos.push_back("r3k2r/pbpnqp2/1p1ppn1p/6p1/2PP4/2PBPNB1/P4PPP/R2Q1RK1 w kq - 2 12");
    fenPos.push_back("2kr3r/pbpn1pq1/1p3n2/3p1R2/3P3p/2P2Q2/P1BN2PP/R3B2K w - - 4 22");
    fenPos.push_back("r2n1rk1/1pq2ppp/p2pbn2/8/P3Pp2/2PBB2P/2PNQ1P1/1R3RK1 w - - 0 17");
    fenPos.push_back("1r2r2k/1p4qp/p3bp2/4p2R/n3P3/2PB4/2PB1QPK/1R6 w - - 1 32");
    fenPos.push_back("1b3r1k/rb1q3p/pp2pppP/3n1n2/1P2N3/P2B1NPQ/1B3P2/2R1R1K1 b - - 1 32");
    std::vector<double> timeSum;
    std::vector<double> nodesSum;
    std::vector<int> splits;
    int threads = 1;
    int depth = 12;

    stream >> threads;
    stream >> depth;

    for (int i = MinSplitDepth; i <= MaxSplitDepth; ++i) splits.push_back(i);

    timeSum.resize(splits.size(), 0.0);
    nodesSum.resize(splits.size(), 0.0);

    streamcmd = std::istringstream("name Threads value " + std::to_string(threads));
    SetOption(streamcmd);

    for (int idxpos = 0; idxpos < fenPos.size(); ++idxpos) {
        LogAndPrintOutput() << "\n\nPos#" << idxpos + 1 << ": " << fenPos[idxpos];
        for (int idxsplit = 0; idxsplit < splits.size(); ++idxsplit) {
            streamcmd = std::istringstream("name Min Split Depth value " + std::to_string(splits[idxsplit]));
            SetOption(streamcmd);
            NewGame();

            streamcmd = std::istringstream("fen " + fenPos[idxpos]);
            Position(streamcmd);

            int64 startTime = getTime();

            streamcmd = std::istringstream("depth " + std::to_string(depth));
            Go(streamcmd);

            while (ThreadsMgr.StillThinking());

            int64 spentTime = getTime() - startTime;
            uint64 nodes = ThreadsMgr.ComputeNodes() / spentTime;

            timeSum[idxsplit] += spentTime;
            nodesSum[idxsplit] += nodes;

            LogAndPrintOutput() << "\nPos#" << idxpos + 1 << " splitdepth: " << splits[idxsplit] << " time: " << (double)spentTime/1000.0 << "s knps: " << nodes << "\n";
        }
    }

    int bestIdx = 0;
    double bestSplit = double(nodesSum[bestIdx]) / double(timeSum[bestIdx]);
    LogAndPrintOutput() << "\n\nAverage Statistics (threads: " << threads << " depth: " << depth << ")\n";
    for (int i = 0; i < splits.size(); ++i) {
        if (double(nodesSum[i]) / double(timeSum[i]) > bestSplit) {
            bestSplit = double(nodesSum[i]) / double(timeSum[i]);
            bestIdx = i;
        }
        LogAndPrintOutput() << "splitdepth: " << splits[i] << " time: " << timeSum[i] / (fenPos.size() * 1000.0) << "s knps: " << nodesSum[i] / fenPos.size() << " ratio: " << nodesSum[i] / timeSum[i];
    }
    LogAndPrintOutput() << "\n\nThe best splitdepth is:\n";
    LogAndPrintOutput() << "splitdepth: " << splits[bestIdx] << " time: " << timeSum[bestIdx] / (fenPos.size() * 1000.0) << "s knps: " << nodesSum[bestIdx] / fenPos.size() << " ratio: " << nodesSum[bestIdx] / timeSum[bestIdx] << "\n";
}
