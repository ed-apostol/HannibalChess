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
#include "trans.h"
#include "threads.h"
#include "position.h"
#include "utils.h"
#include "search.h"
#include "init.h"
#include "uci.h"
#include "material.h"

#include <cstdlib>

void quit(void) {
    if (logfile) fclose(logfile);
    if (errfile) fclose(errfile);
    if (dumpfile) fclose(dumpfile);

#ifndef TCEC
    closeBook(&GpolyglotBook);
    closeLearn(&Glearn);
    closeBook(&GhannibalBook);
#endif 
    ThreadsMgr.SetNumThreads(0);
}

int main(void) {
    position_t pos;
    char command[8192];
    char* ptr;

    //////setbuf(stdout, NULL);
    //////setbuf(stdin, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    fopen_s(&logfile, "logfile.txt", "a+");
    fopen_s(&errfile, ERROR_FILE, "a+");
    fopen_s(&dumpfile, "dumpfile.txt", "a+");

    Print(3, "Hannibal %s by Sam Hamilton & Edsel Apostol\n", VERSION);
    Print(3, "Use Universal Chess Interface(UCI) commands\n");

    InitUCIOptions(UCIOptionsMap);

    TransTable.Init(INIT_HASH, HASH_ASSOC);
    PVHashTable.Init(INIT_PVHASH, PV_ASSOC);

    initArr();
    initPST();
    InitTrapped();

    ThreadsMgr.InitVars();
    ThreadsMgr.SetNumThreads(NUM_THREADS);

    initMaterial();
    InitMateBoost();

    setPosition(&pos,STARTPOS);
    needReplyReady = false;
    while (true) {
        if (needReplyReady) {
            Print(3, "readyok\n");
            needReplyReady = false;
        }
        if (fgets(command, 8192, stdin) == NULL)
            strcpy_s(command, "quit\n");

        ptr = strchr(command, '\n');
        if (ptr != NULL) *ptr = '\0';

        Print(2, "Command: %s\n", command);

        if (!memcmp(command, "ucinewgame", 10)) {
            origScore = 0;
            TransTable.Clear();
            PVHashTable.Clear();
            ThreadsMgr.ClearPawnHash();
            ThreadsMgr.ClearEvalHash();
            SearchManager.info.lastDepthSearched = MAXPLY;
        } else if (!memcmp(command, "uci", 3)) {
            uciStart();
        } else if (!memcmp(command, "debug", 5)) {
            /* dummy */
        } else if (!memcmp(command, "isready", 7)) {
            Print(3, "readyok\n");
        } else if (!memcmp(command, "position", 8)) {
            uciSetPosition(&pos, command + 9);
        } else if (!memcmp(command, "go", 2)) {
            uciGo(&pos, command + 3);
        } else if (!memcmp(command, "setoption", 9)) {
            uciSetOption(command + 10);
        } else if (!memcmp(command, "testloop", 8)) {
#ifdef DEBUG
            nonUCI(&pos);
#endif
        } else if (!memcmp(command, "stop", 4)) {
            SearchManager.stopSearch();
            Print(2, "info string Aborting search: stop\n");
        } else if (!memcmp(command, "ponderhit", 9)) {
            SearchManager.ponderHit();
        } else if (!memcmp(command, "quit", 4)) {
            break;
        } else if (!memcmp(command, "speedup", 7)) {
            SearchManager.checkSpeedUp(&pos, command+8);
        } else if (!memcmp(command, "split", 5)) {
            SearchManager.benchMinSplitDepth(&pos, command+6);
        } else if (!memcmp(command, "sthreads", 8)) {
            SearchManager.benchThreadsperSplit(&pos, command+9);
        } else if (!memcmp(command, "active", 6)) {
            SearchManager.benchActiveSplits(&pos, command+7);
        }
#ifdef OPTIMIZE
        else if (!memcmp(command, "optimize1",9)) optimize(&pos, 1);
        else if (!memcmp(command, "optimize2",9)) optimize(&pos, 2);
        else if (!memcmp(command, "optimize4",9)) optimize(&pos, 4);
        else if (!memcmp(command, "optimize8",9)) optimize(&pos, 8);
#endif
        else Print(3, "info string Unknown UCI command.\n");
    }
    quit();
    exit(EXIT_SUCCESS);
}
