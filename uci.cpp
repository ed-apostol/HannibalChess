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

UCIOptions UCIOptionsMap;

void on_hash(const Options& o) { TransTable.Init(o.GetInt(), HASH_ASSOC); }
void on_pawn_hash(const Options& o) { ThreadsMgr.pawnhashsize = o.GetInt(); ThreadsMgr.InitPawnHash(ThreadsMgr.pawnhashsize);}
void on_eval_hash(const Options& o) { ThreadsMgr.evalcachesize = o.GetInt(); ThreadsMgr.InitEvalHash(ThreadsMgr.evalcachesize); }
void on_multi_pv(const Options& o) { SearchManager.info.multipv = o.GetInt(); }
void on_clear_hash(const Options& o) { TransTable.Clear(); }
void on_ponder(const Options& o) { }
void on_time_buffer(const Options& o) { SearchManager.info.time_buffer = o.GetInt(); }
void on_threads(const Options& o) { ThreadsMgr.SetNumThreads(o.GetInt());}
void on_splits(const Options& o) { ThreadsMgr.min_split_depth = o.GetInt(); }
void on_threads_split(const Options& o) { ThreadsMgr.max_threads_per_split = o.GetInt(); }
void on_active_splits(const Options& o) { ThreadsMgr.max_activesplits_per_thread = o.GetInt(); }
void on_contempt(const Options& o) { SearchManager.info.contempt = o.GetInt(); }

void PrintUCIOptions(const UCIOptions& uci_opt) {
    for (UCIOptions::const_iterator it = uci_opt.begin(); it != uci_opt.end(); ++it) {
        const Options& opt = it->second;
        std::cout << "option name " << it->first << " type " << opt.m_Type;
        if (opt.m_Type != "button") std::cout << " default " << opt.m_DefVal;
        if (opt.m_Type == "spin") std::cout << " min " << opt.m_Min << " max " << opt.m_Max;
        std::cout << std::endl;
    }
}

void InitUCIOptions(UCIOptions& uci_opt) {
    uci_opt["Hash"] =  Options(INIT_HASH, 1, 65536, on_hash);
    uci_opt["Pawn Hash"] = Options(INIT_PAWN, 1, 1024, on_pawn_hash);
    uci_opt["Eval Cache"] = Options(INIT_EVAL, 1, 1024, on_eval_hash);
    uci_opt["MultiPV"] = Options(INIT_MULTIPV, 1, 128, on_multi_pv);
    uci_opt["Clear Hash"] = Options(on_clear_hash);
    uci_opt["Ponder"] = Options(false, on_ponder);
    uci_opt["Time Buffer"] = Options(1000, 0, 10000, on_time_buffer);
    uci_opt["Threads"] = Options(NUM_THREADS, 1, MaxNumOfThreads, on_threads);
    uci_opt["Min Split Depth"] = Options(MIN_SPLIT_DEPTH, 1, 12, on_splits);
    uci_opt["Max Threads/Split"] = Options(MAX_THREADS_PER_SPLIT, 2, 16, on_threads_split);
    uci_opt["Max Active Splits/Thread"] = Options(MAX_ACTIVE_SPLITS, 1, 8, on_active_splits);
    uci_opt["Contempt"] = Options(0, -100, 100, on_contempt);
}

void setoption(istringstream& is) {
    string token, name, value;
    is >> token;
    while (is >> token && token != "value") name += string(" ", !name.empty()) + token;
    while (is >> token) value += string(" ", !value.empty()) + token;
    if (UCIOptionsMap.count(name)) UCIOptionsMap[name] = value;
    else std::cout << "No such option: " << name << std::endl;
}

#define MIN_TIME 2
#define TIME_DIVIDER 20 //how many moves we divide remaining time into 

void uciStart(void) {
    Print(3, "id name Hannibal %s\n", VERSION);
    Print(3, "id author Sam Hamilton & Edsel Apostol\n");
    //Print(3, "option name Hash type spin min 1 max 65536 default %d\n", INIT_HASH);
    //Print(3, "option name Pawn Hash type spin min 1 max 1024 default %d\n", INIT_PAWN);
    //Print(3, "option name Eval Cache type spin min 1 max 1024 default %d\n", INIT_EVAL);
    //Print(3, "option name MultiPV type spin min 1 max 100 default %d\n", INIT_MULTIPV);
    //Print(3, "option name Clear Hash type button\n");
    //Print(3, "option name Ponder type check default false\n");
    //Print(3, "option name Time Buffer type spin min 0 max 10000 default 1000\n");
    //Print(3, "option name Threads type spin min 1 max %d default %d\n", MaxNumOfThreads, NUM_THREADS);
    //Print(3, "option name Min Split Depth type spin min 1 max 12 default %d\n", MIN_SPLIT_DEPTH);
    //Print(3, "option name Max Threads/Split type spin min 2 max 8 default %d\n", MAX_THREADS_PER_SPLIT);
    //Print(3, "option name Max Active Splits/Thread type spin min 1 max 8 default %d\n", MAX_ACTIVE_SPLITS);
    //Print(3, "option name Contempt type spin min -100 max 100 default 0\n");
    PrintUCIOptions(UCIOptionsMap);
    Print(3, "uciok\n"); //this makes sure there are no issues I hope?
}

void uciSetOption(char string[]) {
    char *name, *value;

    name = strstr(string,"name ");
    value = strstr(string,"value ");

    name += 5;
    value += 6;

    //if (!memcmp(name,"Hash",4)) {
    //    TransTable.Init(atoi(value), HASH_ASSOC);
    //} else if (!memcmp(name,"Pawn Hash",9)) {
    //    Guci_options.pawnhashsize = atoi(value);
    //    ThreadsMgr.InitPawnHash(Guci_options.pawnhashsize);

    //} else if (!memcmp(name,"Eval Cache",10)) {
    //    Guci_options.evalcachesize = atoi(value);
    //    ThreadsMgr.InitEvalHash(Guci_options.evalcachesize);
    //} else if (!memcmp(name,"Clear Hash",10)) {
    //    TransTable.Clear();
    //} else if (!memcmp(name,"Time Buffer",11)) {
    //    Guci_options.time_buffer = atoi(value);
    //} else if (!memcmp(name,"Contempt",8)) {
    //    Guci_options.contempt = atoi(value);
    //} else if (!memcmp(name,"Threads",7)) {
    //    ThreadsMgr.SetNumThreads(atoi(value));
    //} else if (!memcmp(name,"Min Split Depth",15)) {
    //    Guci_options.min_split_depth = atoi(value);
    //} else if (!memcmp(name,"Max Threads/Split",17)) {
    //    Guci_options.max_threads_per_split = atoi(value);
    //} else if (!memcmp(name,"Max Active Splits/Thread",24)) {
    //    Guci_options.max_activesplits_per_thread = atoi(value);
    //} else if (!memcmp(name,"MultiPV",7)) {
    //    Guci_options.multipv = atoi(value);
    //}
}

void uciParseSearchmoves(movelist_t *ml, char *str, uint32 moves[]) {
    char *c, movestr[10];
    int i;
    int m;
    uint32 *move = moves;

    ASSERT(ml != NULL);
    ASSERT(moves != NULL);

    c = str;
    while (isspace(*c)) c++;
    while (*c != '\0') {
        i = 0;
        while (*c != '\0' && !isspace(*c) && i < 9) movestr[i++] = *c++;
        if (i >= 4 && 'a' <= movestr[0] && movestr[0] <= 'h' &&
            '1' <= movestr[1] && movestr[1] <= '8' &&
            'a' <= movestr[2] && movestr[2] <= 'h' &&
            '1' <= movestr[3] && movestr[3] <= '8') {
                m = parseMove(ml, movestr);
                if (m) *move++ = m;
        } else break;
        while (isspace(*c)) c++;
    }
    *move = 0;
}

void uciGo(position_t *pos, char *options) {
    char *ponder, *infinite;
    char *c;
    int64 mytime = 0, t_inc = 0;
    int wtime=0, btime=0, winc=0, binc=0, movestogo=0, upperdepth=0, nodes=0, mate=0, movetime=0;
    movelist_t ml;
    SearchInfo& info = SearchManager.info;

    ASSERT(pos != NULL);
    ASSERT(options != NULL);

    info.Init();

    infinite = strstr(options, "infinite");
    ponder = strstr(options, "ponder");
    c = strstr(options, "wtime");
    if (c != NULL) sscanf_s(c+6, "%d", &wtime);
    c = strstr(options, "btime");
    if (c != NULL) sscanf_s(c+6, "%d", &btime);
    c = strstr(options, "winc");
    if (c != NULL) sscanf_s(c + 5, "%d", &winc);
    c = strstr(options, "binc");
    if (c != NULL) sscanf_s(c + 5, "%d", &binc);
    c = strstr(options, "movestogo");
    if (c != NULL) sscanf_s(c + 10, "%d", &movestogo);
    c = strstr(options, "depth");
    if (c != NULL) sscanf_s(c + 6, "%d", &upperdepth);
    c = strstr(options, "nodes");
    if (c != NULL) sscanf_s(c + 6, "%d", &nodes);
    c = strstr(options, "mate");
    if (c != NULL) sscanf_s(c + 5, "%d", &mate);
    c = strstr(options, "movetime");
    if (c != NULL) sscanf_s(c + 9, "%d", &movetime);
    c = strstr(options, "searchmoves");
    if (c != NULL) {
        genLegal(pos, &ml,true);
        uciParseSearchmoves(&ml, c + 12, &(info.moves[0]));
    }

    if (infinite) {
        info.depth_is_limited = true;
        info.depth_limit = MAXPLY;
        Print(2, "info string Infinite\n");
    }
    if (upperdepth > 0) {
        info.depth_is_limited = true;
        info.depth_limit = upperdepth;
        Print(2, "info string Depth is limited to %d half moves\n", info.depth_limit);
    }
    if (mate > 0) {
        info.depth_is_limited = true;
        info.depth_limit = mate * 2 - 1;
        Print(2, "info string Mate in %d half moves\n", info.depth_limit);
    }
    if (nodes > 0) {
        info.node_is_limited = true;
        info.node_limit = nodes;
        Print(2, "info string Nodes is limited to %d positions\n", info.node_limit);
    }
    if (info.moves[0]) {
        info.moves_is_limited = true;
        Print(2, "info string Moves is limited\n");
    }

    if (movetime > 0) {
        info.time_is_limited = true;
        info.alloc_time = movetime;
        info.time_limit_max = info.start_time + movetime;
        info.time_limit_abs = info.start_time + movetime;
        Print(2, "info string Fixed time per move: %d ms\n", movetime);
    }
    if (pos->side == WHITE) {
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
        if (movestogo <= 0 || movestogo > TIME_DIVIDER) movestogo = TIME_DIVIDER;
        info.time_limit_max = (mytime / movestogo) + ((t_inc * 4) / 5);
        if (ponder) info.time_limit_max += info.time_limit_max / 4;

        if (info.time_limit_max > mytime) info.time_limit_max = mytime;
        info.time_limit_abs = ((mytime * 2) / 5) + ((t_inc * 4) / 5);
        if (info.time_limit_abs < info.time_limit_max) info.time_limit_abs = info.time_limit_max;
        if (info.time_limit_abs > mytime) info.time_limit_abs = mytime;

        if (info.time_limit_max < MIN_TIME) info.time_limit_max = MIN_TIME; //SAM added to deail with issues when time < time_buffer
        if (info.time_limit_abs < MIN_TIME) info.time_limit_abs = MIN_TIME;

        Print(2, "info string Time is limited: ");
        Print(2, "max = %d, ", info.time_limit_max);
        Print(2, "abs = %d\n", info.time_limit_abs);
        info.alloc_time = info.time_limit_max;
        info.time_limit_max += info.start_time;
        info.time_limit_abs += info.start_time;
    }
    if (infinite) {
        info.thinking_status = ANALYSING;
        Print(2, "info string Search status is ANALYSING\n");
    } else if (ponder) {
        info.thinking_status = PONDERING;
        Print(2, "info string Search status is PONDERING\n");
    } else {
        info.thinking_status = THINKING;
        Print(2, "info string Search status is THINKING\n");
    }
    /* initialize UCI parameters to be used in search */
    DrawValue[pos->side] = - info.contempt;
    DrawValue[pos->side^1] = info.contempt;

    ThreadsMgr.StartThinking(pos);
}
void uciSetPosition(position_t *pos, char *str) {
    char *c = str, *m;
    char movestr[10];
    basic_move_t move;
    movelist_t ml;
    bool startPos = false;

#ifndef TCEC
    movesSoFar.length = 0; //I don't trust this position to be from book
#endif
    ASSERT(pos != NULL);
    ASSERT(str != NULL);

    while (isspace(*c)) c++;
    if (!memcmp(c, "startpos", 8)) {
        c += 8;
        while (isspace(*c)) c++;
        setPosition(pos, STARTPOS);
        startPos = true;
    } else if (!memcmp(c, "fen", 3)) {
        c += 3;
        while (isspace(*c)) c++;
        setPosition(pos, c);
        while (*c != '\0' && memcmp(c, "moves", 5)) c++;
    }
    while (isspace(*c)) c++;
    if (!memcmp(c, "moves", 5)) {
        c += 5;
        while (isspace(*c)) c++;
        while (*c != '\0') {
            m = movestr;
            while (*c != '\0' && !isspace(*c)) *m++ = *c++;
            *m = '\0';
            genLegal(pos, &ml,true);
            move = parseMove(&ml, movestr);
            if (!move) {
                Print(3, "info string Illegal move: %s\n", movestr);
                return;
            } else makeMove(pos, &UndoStack[pos->sp], move);
#ifndef TCEC
            if (startPos && movesSoFar.length < MAXPLY-1) {
                movesSoFar.moves[movesSoFar.length] = move;
                movesSoFar.length++;
            }
#endif
            // this is to allow any number of moves in a game by only keeping the last relevant ones for rep detection
            if (pos->posStore.fifty==0) {
                pos->sp = 0;
            }
            while (isspace(*c)) c++;
        }
    }
    pos->ply = 0;
}
