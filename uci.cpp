/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
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

const std::string Engine::name = "Hannibal";
const std::string Engine::author = "Sam Hamilton & Edsel Apostol";
const std::string Engine::year = "2014";
const std::string Engine::version = "TCEC";
const std::string Engine::arch = "x64";

UCIOptions UCIOptionsMap;

void on_hash(const Options& o) { TransTable.Init(o.GetInt(), 4); }
void on_pawn_hash(const Options& o) { ThreadsMgr.InitPawnHash(o.GetInt(), 1);}
void on_eval_hash(const Options& o) { ThreadsMgr.InitEvalHash(o.GetInt(), 1); }
void on_multi_pv(const Options& o) { SearchManager.info.multipv = o.GetInt(); }
void on_clear_hash(const Options& o) { TransTable.Clear(); }
void on_ponder(const Options& o) { }
void on_time_buffer(const Options& o) { SearchManager.info.time_buffer = o.GetInt(); }
void on_threads(const Options& o) { ThreadsMgr.SetNumThreads(o.GetInt());}
void on_splits(const Options& o) { ThreadsMgr.min_split_depth = o.GetInt(); }
void on_threads_split(const Options& o) { ThreadsMgr.max_threads_per_split = o.GetInt(); }
void on_active_splits(const Options& o) { ThreadsMgr.max_activesplits_per_thread = o.GetInt(); }
void on_contempt(const Options& o) { SearchManager.info.contempt = o.GetInt(); }

void Engine::InitUCIOptions(UCIOptions& uci_opt) {
    uci_opt["Hash"] =                       Options(64, 1, 65536, on_hash);
    uci_opt["Pawn Hash"] =                  Options(4, 1, 1024, on_pawn_hash);
    uci_opt["Eval Cache"] =                 Options(4, 1, 1024, on_eval_hash);
    uci_opt["MultiPV"] =                    Options(1, 1, 128, on_multi_pv);
    uci_opt["Clear Hash"] =                 Options(on_clear_hash);
    uci_opt["Ponder"] =                     Options(false, on_ponder);
    uci_opt["Time Buffer"] =                Options(1000, 0, 10000, on_time_buffer);
    uci_opt["Threads"] =                    Options(6, 1, MaxNumOfThreads, on_threads);
    uci_opt["Min Split Depth"] =            Options(4, 1, 12, on_splits);
    uci_opt["Max Threads/Split"] =          Options(16, 2, MaxNumOfThreads, on_threads_split);
    uci_opt["Max Active Splits/Thread"] =   Options(4, 1, 8, on_active_splits);
    uci_opt["Contempt"] =                   Options(0, -100, 100, on_contempt);
}

void Engine::PrintUCIOptions(const UCIOptions& uci_opt) {
    for (UCIOptions::const_iterator it = uci_opt.begin(); it != uci_opt.end(); ++it) {
        const Options& opt = it->second;
        Log<cOUT, true> log;
        log << "option name " << it->first << " type " << opt.m_Type;
        if (opt.m_Type != "button") log << " default " << opt.m_DefVal;
        if (opt.m_Type == "spin") log << " min " << opt.m_Min << " max " << opt.m_Max;
    }
}

void Engine::Info() {
    Log<cOUT, true>()<<name<<" "<<version<<" "<<arch;
    Log<cOUT, true>()<<"Copyright (C) "<<year<<" "<<author;
    Log<cOUT, true>()<<"Use UCI commands";
    Log<cOUT, true>();
}

Engine::Engine() {
    fopen_s(&logfile, "logfile.txt", "a+");
    fopen_s(&errfile, "errfile.txt", "a+");
    fopen_s(&dumpfile, "dumpfile.txt", "a+");

    InitUCIOptions(UCIOptionsMap);
    TransTable.Init(UCIOptionsMap["Hash"].GetInt(), 4);
    PVHashTable.Init(1, 8);
    ThreadsMgr.InitVars();
    ThreadsMgr.SetNumThreads(UCIOptionsMap["Threads"].GetInt());

    initArr();
    initPST();
    InitTrapped();
    initMaterial();
    InitMateBoost();

    setPosition(&rootpos, STARTPOS);
}

void Engine::Run() {
    std::string line;
    while(getline(std::cin,line)) {
        Log<cINFO, true>() << line;
        std::istringstream ss(line);
        if(!Input(ss)) break;
    }
}

bool Engine::Input(std::istringstream& stream) {
    std::string command;
    stream >> command;

    if(command == "stop")           { Stop(); }
    else if(command == "ponderhit") { Ponderhit(); }
    else if(command == "go")        { Go(stream); }
    else if(command == "position")  { Position(stream); }
    else if(command == "setoption") { SetOption(stream); }
    else if(command == "ucinewgame"){ NewGame(); }
    else if(command == "isready")   { Log<cOUT, true>()<<"readyok"; }
    else if(command == "quit")      { Quit(); return false; }
    else if(command == "uci")       { Id(); }
    else std::cerr<<"Unknown UCI command: "<<command<<std::endl;

    return true;
}

void Engine::Quit() {
    if (logfile) fclose(logfile);
    if (errfile) fclose(errfile);
    if (dumpfile) fclose(dumpfile);
    ThreadsMgr.SetNumThreads(0);
    Log<cINFO, true>() << "Engine quit";
}

void Engine::Id() {
    Log<cOUT, true>()<<"id name "<<name<<" "<<version<<" "<<arch;
    Log<cOUT, true>()<<"id author "<<author;
    PrintUCIOptions(UCIOptionsMap);
    Log<cOUT, true>()<<"uciok";
}

void Engine::Stop() {
    SearchManager.stopSearch();
    Log<cINFO, true>() <<"info string Aborting search: stop";
}

void Engine::Ponderhit() {
    SearchManager.ponderHit();
}

void Engine::Go(std::istringstream& stream) {
    std::string command;
    int64 mytime = 0, t_inc = 0;
    int wtime=0, btime=0, winc=0, binc=0, movestogo=0, upperdepth=0, nodes=0, mate=0, movetime=0;
    bool infinite = false, ponder = false;
    SearchInfo& info = SearchManager.info;

    info.Init();

    stream>>command;
    while(command != ""){
        if(command == "wtime") stream>>wtime;
        else if(command == "btime") stream>>btime;
        else if(command == "winc") stream>>winc;
        else if(command == "binc") stream>>binc;
        else if(command == "movestogo") stream>>movestogo;
        else if(command == "ponder") { ponder = true; }
        else if(command == "depth"){ stream>>upperdepth;}
        else if(command == "movetime") stream>>movetime;
        else if(command == "infinite") { infinite = true; }
        else if(command == "nodes") stream>>nodes;
        else if(command == "mate") stream>>mate;
        else { std::cerr<<"Wrong go command: "<<command<<std::endl; return; }
        command = "";
        stream>>command;
    }
    if (infinite) {
        info.depth_is_limited = true;
        info.depth_limit = MAXPLY;
        Log<cINFO, true>() <<"info string Infinite";
    }
    if (upperdepth > 0) {
        info.depth_is_limited = true;
        info.depth_limit = upperdepth;
        Log<cINFO, true>() <<"info string Depth is limited to " << info.depth_limit << " half moves";
    }
    if (mate > 0) {
        info.depth_is_limited = true;
        info.depth_limit = mate * 2 - 1;
        Log<cINFO, true>() <<"info string Mate in " << info.depth_limit << " half moves";
    }
    if (nodes > 0) {
        info.node_is_limited = true;
        info.node_limit = nodes;
        Log<cINFO, true>() <<"info string Nodes is limited to " << info.node_limit << " positions";
    }
    if (info.moves[0]) {
        info.moves_is_limited = true;
        Log<cINFO, true>() <<"info string Moves is limited";
    }

    if (movetime > 0) {
        info.time_is_limited = true;
        info.alloc_time = movetime;
        info.time_limit_max = info.start_time + movetime;
        info.time_limit_abs = info.start_time + movetime;
        Log<cINFO, true>() <<"info string Fixed time per move: " << movetime << " ms";
    }
    if (rootpos.side == WHITE) {
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

        Log<cINFO, true>() <<"info string Time is limited: ";
        Log<cINFO, true>() <<"max = " << info.time_limit_max;
        Log<cINFO, true>() <<"abs = " << info.time_limit_abs;
        info.alloc_time = info.time_limit_max;
        info.time_limit_max += info.start_time;
        info.time_limit_abs += info.start_time;
    }
    if (infinite) {
        info.thinking_status = ANALYSING;
        Log<cINFO, true>() <<"info string Search status is ANALYSING";
    } else if (ponder) {
        info.thinking_status = PONDERING;
        Log<cINFO, true>() <<"info string Search status is PONDERING";
    } else {
        info.thinking_status = THINKING;
        Log<cINFO, true>() <<"info string Search status is THINKING";
    }

    DrawValue[rootpos.side] = -info.contempt;
    DrawValue[rootpos.side^1] = info.contempt;

    ThreadsMgr.StartThinking(&rootpos);
}

void Engine::Position(std::istringstream& stream) {
    basic_move_t m;
    string token, fen;

    stream >> token;
    if (token == "startpos") {
        fen = STARTPOS;
        stream >> token;
    } else if (token == "fen") {
        while (stream >> token && token != "moves")
            fen += token + " ";
    } else return;

    setPosition(&rootpos, fen.c_str());
    while (stream >> token) {
        movelist_t ml;
        genLegal(&rootpos, &ml, true);
        m = parseMove(&ml, token.c_str());
        if (m) makeMove(&rootpos, &UndoStack[rootpos.sp], m);
        else break;
        if (rootpos.posStore.fifty==0) rootpos.sp = 0;
    }
    rootpos.ply = 0;
}

void Engine::SetOption(std::istringstream& stream) {
    string token, name, value;
    stream >> token;
    while (stream >> token && token != "value") name += string(" ", !name.empty()) + token;
    while (stream >> token) value += string(" ", !value.empty()) + token;
    if (UCIOptionsMap.count(name)) UCIOptionsMap[name] = value;
    else Log<cOUT, true>() << "No such option: " << name;
}

void Engine::NewGame() {
    PVHashTable.Clear();
    TransTable.Clear();
    ThreadsMgr.ClearPawnHash();
    ThreadsMgr.ClearEvalHash();
    SearchManager.info.lastDepthSearched = MAXPLY;
}

Engine::~Engine() {

}