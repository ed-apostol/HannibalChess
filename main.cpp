/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "macros.h"
#include "position.h"
#include "init.h"
#include "smp.h"
#include "data.h"
#include "book.h"
#include "trans.h"
#include "material.h"
#include "uci.h"
#include "utils.h"
#include "search.h"

void quit(void) {
    //    Print(2, "info string Hannibal is quitting.\n");
    fclose(logfile);
    fclose(errfile);
    fclose(dumpfile);
    stopThreads();
    exit(EXIT_SUCCESS);
}

int main(void) {
    position_t pos;
    char command[8192];
    char* ptr;

    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    logfile = fopen("logfile.txt", "w");
    errfile = fopen(ERROR_FILE, "a+");
    dumpfile = fopen("dumpfile.txt", "a+");

    Print(3, "Hannibal %s by Sam Hamilton & Edsel Apostol\n", VERSION);
    Print(3, "Use Universal Chess Interface(UCI) commands\n");

    //Print(1, "SplitPoint:%.2fkB\n", (float)sizeof(SplitPoint)/(float)1024);
    //Print(1, "position_t:%.2fkB\n", (float)sizeof(position_t)/(float)1024);
#ifndef TESTING_ON
    GpolyglotBook.bookFile = NULL;
    initBook(DEFAULT_POLYGLOT_BOOK, &GpolyglotBook, POLYGLOT_BOOK);
    SearchInfo(0).outOfBook = 0;
#endif
#ifdef LEARNING
    GhannibalBook.bookFile = NULL;
    Glearn.learnFile = NULL;
    for (int i=0; i < MaxNumOfThreads;i++) {	//the default is every thread is a normal search
        SearchInfoMap[i] = &global_search_info;
    }
    initBook(DEFAULT_HANNIBAL_BOOK, &GhannibalBook, PUCK_BOOK);
    initLearn("HannibalLearn.lrn", &Glearn);
#endif
    initTrans(INIT_HASH, 0);
    initPVHashTab(&PVHashTable, INIT_PVHASH);
    initPawnTab(&SearchInfo(0).pt, INIT_PAWN);
    initEvalTab(&SearchInfo(0).et, INIT_EVAL);

    initOption(&Guci_options); // this should be initialized first
    initArr();
    initPST();
    InitTrapped();

    initSmpVars();
    initThreads();

    initMaterial();
    InitMateBoost();

    setPosition(pos, STARTPOS);
    needReplyReady = FALSE;
    while (TRUE) {
        if (needReplyReady) {
            Print(3, "readyok\n");
            needReplyReady = FALSE;
        }
        if (fgets(command, 8192, stdin) == NULL)
            strcpy(command, "quit\n");

        ptr = strchr(command, '\n');
        if (ptr != NULL) *ptr = '\0';

        Print(2, "Command: %s\n", command);

        if (!memcmp(command, "ucinewgame", 10)) {
            origScore = 0;
            transClear(0);
            pvHashTableClear(&PVHashTable);
#ifndef TRAINING_ON
            SearchInfo(0).outOfBook = 0;
#else
#ifdef LEARNING_ON
            movesSoFar.length = 0;
#endif
#endif
            pawnTableClear(&SearchInfo(0).pt);
            evalTableClear(&SearchInfo(0).et);
        } else if (!memcmp(command, "uci", 3)) {
            uciStart();
        } else if (!memcmp(command, "debug", 5)) {
            /* dummy */
        } else if (!memcmp(command, "isready", 7)) {
            Print(3, "readyok\n");
        } else if (!memcmp(command, "position", 8)) {
            uciSetPosition(pos, command + 9);
        } else if (!memcmp(command, "go", 2)) {
            uciGo(pos, command + 3);
        } else if (!memcmp(command, "setoption", 9)) {
            uciSetOption(command + 10);
        } else if (!memcmp(command, "testloop", 8)) {
#ifdef DEBUG
            nonUCI(pos);
#endif
        } else if (!memcmp(command, "stop", 4)) {
            /* no op */
        } else if (!memcmp(command, "quit", 4)) {
            break;
        } else if (!memcmp(command, "speedup", 7)) {
            checkSpeedUp(pos, command + 8);
        } else if (!memcmp(command, "split", 5)) {
            benchSplitDepth(pos, command + 6);
        } else if (!memcmp(command, "sthreads", 8)) {
            benchSplitThreads(pos, command + 9);
        }
#ifdef OPTIMIZE
        else if (!memcmp(command, "optimize1",9)) optimize(pos, 1);
        else if (!memcmp(command, "optimize2",9)) optimize(pos, 2);
        else if (!memcmp(command, "optimize4",9)) optimize(pos, 4);
        else if (!memcmp(command, "optimize8",9)) optimize(pos, 8);
#endif
#ifdef LEARNING_ON
        else if (!memcmp(command,"ConsumeBook",11)) {
            if (GpolyglotBook.bookFile != NULL && GpolyglotBook.type!=POLYGLOT_BOOK) { //CONSIDER UPDATING
                book_t HannibalFormat;
                HannibalFormat.bookFile = NULL;
                open_write_book(DEFAULT_HANNIBAL_BOOK,&HannibalFormat,PUCK_BOOK);
                if (Glearn.learnFile==NULL) initLearn(DEFAULT_HANNIBAL_LEARN,&Glearn);
                continuation_t moves;
                MutexLock(LearningLock);
                MutexLock(BookLock);
                long num = polyglot_to_puck(&GpolyglotBook, &HannibalFormat, &Glearn, pos, &moves, 0);
                cout << "info string wrote " << num << " positions to book, size is now " << HannibalFormat.size << endl;
                closeBook(&HannibalFormat);  
                MutexUnlock(LearningLock);
                MutexUnlock(BookLock);

            }
            else cout << "info string no book to consume" << endl;
        }
#endif
        else Print(3, "info string Unknown UCI command.\n");
    }
    quit();
}
