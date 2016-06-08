/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/
/*
/GS- /GL /W4 /Gy /Zc:wchar_t /Zi /Gm /O2 /Ob2 /sdl- /Fd"x64\Release\vc120.pdb" /fp:fast /D "_MBCS"
/errorReport:prompt /GT /WX- /Zc:forScope /Gd /Oy /Oi /MT /Fa"x64\Release\" /EHsc /nologo /Fo"x64\Release\" /Ot /Fp"x64\Release\HannibalBitBucket.pch"
*/
#define SMALL_FOOTPRINT true

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "threads.h"
#include "trans.h"
#include "movegen.h"
#include "threads.h"
#include "position.h"
#include "utils.h"
#include "uci.h"
#include "search.h"
#include "init.h"
#include "material.h"
#include "utils.h"
#include "book.h"

const std::string Interface::name = "Hannibal";
const std::string Interface::author = "Sam Hamilton & Edsel Apostol";
const std::string Interface::year = "2015";
const std::string Interface::version = "0605";
const std::string Interface::arch = "x64";

void Interface::Info() {
    LogAndPrintOutput() << name << " " << version << " " << arch;
    LogAndPrintOutput() << "Copyright (C) " << year << " " << author;
    LogAndPrintOutput() << "Use UCI commands";
    LogAndPrintOutput();
}

Interface::Interface() {
    std::cout.setf(std::ios::unitbuf);

    initArr();
    initPST();
    InitMaterial();
    InitMateBoost();
    InitEval();
    setPosition(input_pos, STARTPOS);
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

    if (command == "stop") Stop(cEngine);
    else if (command == "ponderhit") PonderHit(cEngine);
    else if (command == "go") Go(cEngine, input_pos, stream);
    else if (command == "position") Position(cEngine, input_pos, stream);
    else if (command == "setoption") SetOption(cEngine, stream);
    else if (command == "ucinewgame") NewGame(cEngine);
    else if (command == "isready") IsReady(cEngine);
    else if (command == "uci") Id(cEngine);
    else if (command == "quit") { Quit(cEngine); return false; }
#ifndef SMALL_FOOTPRINT
    else if (command == "speedup") CheckSpeedup(stream);
    else if (command == "split") CheckBestSplit(stream);
    else if (command == "maxsplit") CheckMaxSplit(stream);
    else if (command == "speedtest") CheckSpeed();
#endif
    else LogAndPrintError() << "Unknown UCI command: " << command;

    return true;
}

void Interface::Quit(Engine& engine) {
    engine.StopSearch();
    engine.WaitForThink();
    LogInfo() << "Interface quit";
}

void Interface::Id(Engine& engine) {
    LogAndPrintOutput() << "id name " << name << " " << version << " " << arch;
    LogAndPrintOutput() << "id author " << author;
    engine.PrintUCIOptions();
    LogAndPrintOutput() << "uciok";
}

void Interface::Stop(Engine& engine) {
    engine.StopSearch();
    LogInfo() << "info string Aborting search: stop";
}

void Interface::PonderHit(Engine& engine) {
    engine.PonderHit();
}

void Interface::IsReady(Engine& engine) {
    LogAndPrintOutput() << "readyok";
}

void Interface::Go(Engine& engine, position_t& pos, std::istringstream& stream) {
    std::string command;
    GoCmdData data;

    stream >> command;
    while (command != "") {
        if (command == "wtime") stream >> data.wtime;
        else if (command == "btime") stream >> data.btime;
        else if (command == "winc") stream >> data.winc;
        else if (command == "binc") stream >> data.binc;
        else if (command == "movestogo") stream >> data.movestogo;
        else if (command == "ponder") data.ponder = true;
        else if (command == "depth") stream >> data.depth;
        else if (command == "movetime") stream >> data.movetime;
        else if (command == "infinite") data.infinite = true;
        else if (command == "nodes") stream >> data.nodes;
        else if (command == "mate") stream >> data.mate;
        else { LogAndPrintError() << "Wrong go command: " << command; return; }
        command = "";
        stream >> command;
    }
    engine.StartThinking(data, pos);
}

void Interface::Position(Engine& engine, position_t& pos, std::istringstream& stream) {
    basic_move_t m;
    std::string token, fen;
    int sp = 0;

    stream >> token;
    if (token == "startpos") {
        fen = STARTPOS;
        stream >> token;
    }
    else if (token == "fen") {
        while (stream >> token && token != "moves")
            fen += token + " ";
    }
    else {
        LogWarning() << "Invalid position!";
        return;
    }

    setPosition(pos, fen.c_str());
    while (stream >> token) {
        movelist_t ml;
        genLegal(pos, ml, true);
        m = parseMove(ml, token.c_str());
        if (m) makeMove(pos, engine.UndoStack[sp++], m);
        else break;
    }
}

void Interface::SetOption(Engine& engine, std::istringstream& stream) {
    std::string token, name, value;
    stream >> token;
    while (stream >> token && token != "value") name += std::string(" ", !name.empty()) + token;
    while (stream >> token) value += std::string(" ", !value.empty()) + token;
    if (engine.uci_opt.count(name)) engine.uci_opt[name] = value;
    else LogAndPrintOutput() << "No such option: " << name;
}

void Interface::NewGame(Engine& engine) {
    engine.ClearPawnHash();
    engine.ClearTTHash();
    engine.ClearPVTTHash();
}
Interface::~Interface() {}
#ifndef SMALL_FOOTPRINT
void Interface::CheckSpeed() {
    std::istringstream streamcmd;
    std::vector<std::string> fenPos;
    std::vector<uint64> targetNodes; //when you want to compare speeds of two different implementations first you should enter # nodes for each position to ensure the two implementations are really equivalent
    std::vector<int> targetDepth;
    fenPos.push_back("q4rk1/1bp1bpp1/1pn1pn1p/r2pN3/p2P1PP1/1P1BPN1Q/PBP4P/R5RK b - -1 16");
    targetDepth.push_back(20);
    targetNodes.push_back(38613779);
    fenPos.push_back("6k1/R4p2/1r1pp2p/6p1/8/2P1P1PP/6PK/8 b - -0 38");
    targetDepth.push_back(27);
    targetNodes.push_back(13171310);
    fenPos.push_back("r3k2r/pbpnqp2/1p1ppn1p/6p1/2PP4/2PBPNB1/P4PPP/R2Q1RK1 w kq - 2 12");
    targetDepth.push_back(21);
    targetNodes.push_back(8677982);
    fenPos.push_back("2kr3r/pbpn1pq1/1p3n2/3p1R2/3P3p/2P2Q2/P1BN2PP/R3B2K w - - 4 22");
    targetDepth.push_back(21);
    targetNodes.push_back(12750335);
    fenPos.push_back("r2n1rk1/1pq2ppp/p2pbn2/8/P3Pp2/2PBB2P/2PNQ1P1/1R3RK1 w - - 0 17");
    targetDepth.push_back(20);
    targetNodes.push_back(11549240);
    fenPos.push_back("1r2r2k/1p4qp/p3bp2/4p2R/n3P3/2PB4/2PB1QPK/1R6 w - - 1 32");
    targetDepth.push_back(21);
    targetNodes.push_back(3004819);
    fenPos.push_back("1b3r1k/rb1q3p/pp2pppP/3n1n2/1P2N3/P2B1NPQ/1B3P2/2R1R1K1 b - - 1 32");
    targetDepth.push_back(20);
    targetNodes.push_back(54966429);
    int64 startTime = getTime();
    for (int idxpos = 0; idxpos < fenPos.size(); ++idxpos) {
        LogAndPrintOutput() << fenPos[idxpos] << " starting\n";
        NewGame(cEngine);
        streamcmd = std::istringstream("fen " + fenPos[idxpos]);
        Position(cEngine, input_pos, streamcmd);
        streamcmd = std::istringstream("depth " + std::to_string(targetDepth[idxpos]));
        Go(cEngine, input_pos, streamcmd);
        cEngine.WaitForThink();
        uint64 nodes = cEngine.ComputeNodes();
        if (nodes != targetNodes[idxpos]) {
            LogAndPrintOutput() << "ERROR DETECTED: node count wrong " << nodes << " != " << targetNodes[idxpos] << "\n";
            //            break;
        }
        LogAndPrintOutput() << nodes << " nodes\n";
    }
    int64 spentTime = getTime() - startTime;
    LogAndPrintOutput() << spentTime << " total time\n";
}
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
            SetOption(cEngine, streamcmd);
            NewGame(cEngine);

            streamcmd = std::istringstream("fen " + fenPos[idxpos]);
            Position(cEngine, input_pos, streamcmd);

            int64 startTime = getTime();

            streamcmd = std::istringstream("depth " + std::to_string(depth));
            Go(cEngine, input_pos, streamcmd);

            cEngine.WaitForThink();

            double timeSpeedUp;
            double nodesSpeedup;
            int64 spentTime = getTime() - startTime;
            uint64 nodes = cEngine.ComputeNodes() / spentTime;

            if (0 == idxthread) {
                nodes1 = nodes;
                spentTime1 = spentTime;
                timeSpeedUp = (double)spentTime / 1000.0;
                timeSpeedupSum[idxthread] += timeSpeedUp;
                nodesSpeedup = (double)nodes;
                nodesSpeedupSum[idxthread] += nodesSpeedup;
                LogAndPrintOutput() << "\nBase: " << std::to_string(timeSpeedUp) << "s " << std::to_string(nodes) << "knps\n";
            }
            else {
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
    SetOption(cEngine, streamcmd);

    for (int idxpos = 0; idxpos < fenPos.size(); ++idxpos) {
        LogAndPrintOutput() << "\n\nPos#" << idxpos + 1 << ": " << fenPos[idxpos];
        for (int idxsplit = 0; idxsplit < splits.size(); ++idxsplit) {
            streamcmd = std::istringstream("name Min Split Depth value " + std::to_string(splits[idxsplit]));
            SetOption(cEngine, streamcmd);
            NewGame(cEngine);

            streamcmd = std::istringstream("fen " + fenPos[idxpos]);
            Position(cEngine, input_pos, streamcmd);

            int64 startTime = getTime();

            streamcmd = std::istringstream("depth " + std::to_string(depth));
            Go(cEngine, input_pos, streamcmd);

            cEngine.WaitForThink();

            int64 spentTime = getTime() - startTime;
            uint64 nodes = cEngine.ComputeNodes() / spentTime;

            timeSum[idxsplit] += spentTime;
            nodesSum[idxsplit] += nodes;

            LogAndPrintOutput() << "\nPos#" << idxpos + 1 << " splitdepth: " << splits[idxsplit] << " time: " << (double)spentTime / 1000.0 << "s knps: " << nodes << "\n";
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

void Interface::CheckMaxSplit(std::istringstream& stream) {
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

    for (int i = MinActiveSplit; i <= MaxActiveSplit; ++i) splits.push_back(i);

    timeSum.resize(splits.size(), 0.0);
    nodesSum.resize(splits.size(), 0.0);

    streamcmd = std::istringstream("name Threads value " + std::to_string(threads));
    SetOption(cEngine, streamcmd);

    for (int idxpos = 0; idxpos < fenPos.size(); ++idxpos) {
        LogAndPrintOutput() << "\n\nPos#" << idxpos + 1 << ": " << fenPos[idxpos];
        for (int idxsplit = 0; idxsplit < splits.size(); ++idxsplit) {
            streamcmd = std::istringstream("name Max Active Splits/Thread value " + std::to_string(splits[idxsplit]));
            SetOption(cEngine, streamcmd);
            NewGame(cEngine);

            streamcmd = std::istringstream("fen " + fenPos[idxpos]);
            Position(cEngine, input_pos, streamcmd);

            int64 startTime = getTime();

            streamcmd = std::istringstream("depth " + std::to_string(depth));
            Go(cEngine, input_pos, streamcmd);

            cEngine.WaitForThink();

            int64 spentTime = getTime() - startTime;
            uint64 nodes = cEngine.ComputeNodes() / spentTime;

            timeSum[idxsplit] += spentTime;
            nodesSum[idxsplit] += nodes;

            LogAndPrintOutput() << "\nPos#" << idxpos + 1 << " active splits: " << splits[idxsplit] << " time: " << (double)spentTime / 1000.0 << "s knps: " << nodes << "\n";
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
        LogAndPrintOutput() << "active splits: " << splits[i] << " time: " << timeSum[i] / (fenPos.size() * 1000.0) << "s knps: " << nodesSum[i] / fenPos.size() << " ratio: " << nodesSum[i] / timeSum[i];
    }
    LogAndPrintOutput() << "\n\nThe best active splits is:\n";
    LogAndPrintOutput() << "active splits: " << splits[bestIdx] << " time: " << timeSum[bestIdx] / (fenPos.size() * 1000.0) << "s knps: " << nodesSum[bestIdx] / fenPos.size() << " ratio: " << nodesSum[bestIdx] / timeSum[bestIdx] << "\n";
}
#endif