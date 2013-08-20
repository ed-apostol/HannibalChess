/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

void quit(void) {
    //    Print(2, "info string Hannibal is quitting.\n");
    fclose(logfile);
    fclose(errfile);
    fclose(dumpfile);

    free(TransTable(0).table);
    free(SearchInfo(0).pt.table);
    free(SearchInfo(0).et.table);
#ifndef TCEC
    closeBook(&GpolyglotBook);
    closeLearn(&Glearn);
    closeBook(&GhannibalBook);
#endif
    stopThreads();
    exit(EXIT_SUCCESS);
}

int main(void) {
    position_t pos;
    uci_option_t uci_option;
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

    //Print(1, "split_point_t:%.2fkB\n", (float)sizeof(split_point_t)/(float)1024);
    //Print(1, "position_t:%.2fkB\n", (float)sizeof(position_t)/(float)1024);
#ifndef TCEC
    for (int i=0; i < MaxNumOfThreads;i++) {	//the default is every thread is a normal search
        SearchInfoMap[i] = &global_search_info;
    }
    GhannibalBook.bookFile = NULL;
    GpolyglotBook.bookFile = NULL;
    Glearn.learnFile = NULL;
    initBook(DEFAULT_POLYGLOT_BOOK, &GpolyglotBook, POLYGLOT_BOOK);
    initBook(DEFAULT_HANNIBAL_BOOK, &GhannibalBook, PUCK_BOOK);
    initLearn("HannibalLearn.lrn", &Glearn);
    SearchInfo(0).outOfBook = 0;
#endif

    Guci_options = &uci_option;

    TransTable(0).table = NULL;
    initTrans(INIT_HASH,0);
    SearchInfo(0).pt.table = NULL;
    initPawnTab(&SearchInfo(0).pt, INIT_PAWN);
    SearchInfo(0).et.table = NULL;
    initEvalTab(&SearchInfo(0).et, INIT_EVAL);



    initOption(&uci_option); // this should be initialized first
    initArr();
    initPST(Guci_options);
    InitTrapped();

    initSmpVars();
    initThreads();

    initMaterial();
    InitMateBoost();

    setPosition(&pos,STARTPOS);
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
#ifndef TCEC
            SearchInfo(0).outOfBook = 0;
            movesSoFar.length = 0;
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
            uciSetPosition(&pos, command + 9);
        } else if (!memcmp(command, "go", 2)) {
            uciGo(&pos, command + 3);
        } else if (!memcmp(command, "setoption", 9)) {
            uciSetOption(command + 10);
#ifdef DEBUG
		} else if (!memcmp(command, "testloop", 8)) {

            nonUCI(&pos);
#endif
        } else if (!memcmp(command, "stop", 4)) {
            /* no op */
        } else if (!memcmp(command, "quit", 4)) {
            break;
#ifdef TESTING
        } else if (!memcmp(command, "speedup", 7)) {
            checkSpeedUp(&pos);
        } else if (!memcmp(command, "split", 5)) {
            benchSplitDepth(&pos);
        
#endif
		}
#ifdef OPTIMIZE
        else if (!memcmp(command, "optimize1",9)) optimize(&pos, 1);
        else if (!memcmp(command, "optimize2",9)) optimize(&pos, 2);
        else if (!memcmp(command, "optimize4",9)) optimize(&pos, 4);
        else if (!memcmp(command, "optimize8",9)) optimize(&pos, 8);
#endif
#ifndef TCEC
        else if (!memcmp(command,"ConsumeBook",11)) {
            if (GpolyglotBook.bookFile != NULL && GpolyglotBook.type!=POLYGLOT_BOOK) { //CONSIDER UPDATING
                book_t HannibalFormat;
                HannibalFormat.bookFile = NULL;
                open_write_book(DEFAULT_HANNIBAL_BOOK,&HannibalFormat,PUCK_BOOK);
                if (Glearn.learnFile==NULL) initLearn(DEFAULT_HANNIBAL_LEARN,&Glearn);
                continuation_t moves;
                MutexLock(LearningLock);
                MutexLock(BookLock);
                long num = polyglot_to_puck(&GpolyglotBook, &HannibalFormat, &Glearn, &pos, &moves, 0);
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