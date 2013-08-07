/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
#define MIN_TIME 2
#define TIME_DIVIDER 30 

void initOption(uci_option_t* opt) {
	//TODO add hash size option
	opt->contempt = 0;
    opt->time_buffer = 1000;
    opt->threads = NUM_THREADS;
	opt->min_split_depth = MIN_SPLIT_DEPTH;
	opt->evalcachesize = INIT_EVAL;
	opt->pawnhashsize = INIT_PAWN;
#ifndef TCEC
    opt->try_book = FALSE;
    opt->book_limit = 128;
 	opt->learnThreads = DEFAULT_LEARN_THREADS;
	opt->usehannibalbook = TRUE;//TODO fix
	opt->learnTime = DEFAULT_LEARN_TIME;
	opt->bookExplore = DEFAULT_BOOK_EXPLORE;
#endif
}

void uciStart(void) {
    Print(3, "id name Hannibal %s\n", VERSION);
    Print(3, "id author Sam Hamilton & Edsel Apostol\n");
    Print(3, "option name Hash type spin min 1 max 16384 default %d\n", INIT_HASH);
    Print(3, "option name Pawn Hash type spin min 1 max 128 default %d\n", INIT_PAWN);
	Print(3, "option name Eval Cache type spin min 1 max 1024 default %d\n", INIT_EVAL);
    Print(3, "option name Clear Hash type button\n");
    Print(3, "option name Ponder type check default false\n");
#ifndef TCEC
    Print(3, "option name OwnBook type check default false\n");
    Print(3, "option name Book File type string default %s\n",DEFAULT_POLYGLOT_BOOK);
    Print(3, "option name HannibalBook File type string default %s\n",DEFAULT_HANNIBAL_BOOK);
	Print(3, "option name UseHannibalBook type check default true\n"); //TODO fix
	Print(3, "option name LearnTime type spin min 0 max 20 default %d\n",DEFAULT_LEARN_TIME);
	Print(3, "option name BookExplore type spin min 0 max 4 default %d\n",DEFAULT_BOOK_EXPLORE);
    Print(3, "option name Book Move Limit type spin min 0 max 256 default 128\n");
#endif
    Print(3, "option name Time Buffer type spin min 0 max 10000 default 1000\n");
    Print(3, "option name Threads type spin min 1 max %d default %d\n", MaxNumOfThreads, NUM_THREADS);
#ifndef TCEC
    Print(3, "option name LearnThreads type spin min 0 max %d default %d\n", MaxNumOfThreads, DEFAULT_LEARN_THREADS);
#endif
	Print(3, "option name Min Split Depth type spin min 1 max 16 default %d\n", MIN_SPLIT_DEPTH);
    Print(3, "option name Contempt type spin min -100 max 100 default 0\n");
    Print(3, "uciok\n"); //this makes sure there are no issues I hope?
}

void uciSetOption(char string[]) {
	char *name, *value;

	name = strstr(string,"name ");
	value = strstr(string,"value ");

	name += 5;
	value += 6;

	if (!memcmp(name,"Hash",4)) {
		initTrans(atoi(value),0);
	} else if (!memcmp(name,"Pawn Hash",9)) {
		Guci_options->pawnhashsize = atoi(value);
		initPawnTab(&SearchInfo(0).pt, Guci_options->pawnhashsize);

	} else if (!memcmp(name,"Eval Cache",10)) {
		Guci_options->evalcachesize = atoi(value);
		initEvalTab(&SearchInfo(0).et, Guci_options->evalcachesize);
	} else if (!memcmp(name,"Clear Hash",10)) {
		transClear(0);
	} 
#ifndef TCEC
	else if (!memcmp(name,"OwnBook",7)) {
		if (value[0] == 't') Guci_options->try_book = TRUE;
		else Guci_options->try_book = FALSE;
	} else if (!memcmp(name,"Book File",9)) {
		initBook(value, &GpolyglotBook, POLYGLOT_BOOK);
	} else if (!memcmp(name,"HannibalBook File",17)) {
		initBook(value, &GhannibalBook, PUCK_BOOK);
	} else if (!memcmp(name,"Book Move Limit",15)) {
		Guci_options->book_limit = atoi(value);
	}
	else if (!memcmp(name,"LearnTime",9)) {
		Guci_options->learnTime = atoi(value);
	} 
	else if (!memcmp(name,"BookExplore",11)) {
		Guci_options->bookExplore = atoi(value);
	} 
	else if (!memcmp(name,"LearnThreads",12)) {
		int oldValue = Guci_options->learnThreads;
		int newValue  = atoi(value);
		if (Guci_options->threads + newValue > MaxNumOfThreads) newValue = MaxNumOfThreads - Guci_options->threads;
		if (newValue != oldValue) {
			 if (SHOW_LEARNING) Print(3,"info string changing learn threads from %d to %d\n",oldValue,newValue);
			Guci_options->learnThreads = newValue;
			
			if (newValue < oldValue) { // go kill all the learning threads
				for (int i = MaxNumOfThreads - oldValue; i < MaxNumOfThreads - newValue; i++) {
					 if (SHOW_LEARNING) Print(3,"info string about to stop thread %d\n",i);
					SearchInfo(i).thinking_status = STOPPED;
				}
			}
			else if (newValue > oldValue) {
				for (int i = MaxNumOfThreads-newValue; i < MaxNumOfThreads-oldValue; i++) { //wake up if it needs to
					 if (SHOW_LEARNING) Print(3,"info string about to wakup thread %d\n",i);
					SetEvent(Threads[i].idle_event);
					 if (SHOW_LEARNING) Print(3,"info string wokeup thread %d\n",i);
				}
			}
		}
	}
#endif
	else if (!memcmp(name,"Time Buffer",11)) {
		Guci_options->time_buffer = atoi(value);
	} else if (!memcmp(name,"Contempt",8)) {
		Guci_options->contempt = atoi(value);
	} else if (!memcmp(name,"Threads",7)) {
		int oldValue = Guci_options->threads;
		int newValue  = atoi(value);
#ifndef TCEC
		if (Guci_options->threads + Guci_options->learnThreads > MaxNumOfThreads) newValue = MaxNumOfThreads - Guci_options->learnThreads;
#endif
		if (newValue > oldValue) {
			for (int i = oldValue; i < newValue; i++) {
		       initSearchThread(i);
			}
		}
		Guci_options->threads = newValue;
	} else if (!memcmp(name,"Min Split Depth",15)) {
		Guci_options->min_split_depth = atoi(value);
	}
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
    int wtime=0, btime=0, winc=0, binc=0, movestogo=0, maxdepth=0, nodes=0, mate=0, movetime=0;
     movelist_t ml;

    ASSERT(pos != NULL);
    ASSERT(options != NULL);

    /* initialization */
    SearchInfo(0).depth_is_limited = FALSE;
    SearchInfo(0).depth_limit = MAXPLY;
    SearchInfo(0).moves_is_limited = FALSE;
    SearchInfo(0).time_is_limited = FALSE;
    SearchInfo(0).time_limit_max = 0;
    SearchInfo(0).time_limit_abs = 0;
    SearchInfo(0).node_is_limited = FALSE;
    SearchInfo(0).node_limit = 0;
    SearchInfo(0).start_time = SearchInfo(0).last_time = getTime();
    SearchInfo(0).alloc_time = 0;
    SearchInfo(0).best_value = -INF;
    SearchInfo(0).last_value = -INF;
    SearchInfo(0).last_last_value = -INF;
    SearchInfo(0).change = 0;
    SearchInfo(0).research = 0;
    SearchInfo(0).bestmove = 0;
    SearchInfo(0).pondermove = 0;
    SearchInfo(0).mate_found = 0;
    memset(SearchInfo(0).history, 0, sizeof(SearchInfo(0).history)); //TODO this is bad to share with learning
	memset(SearchInfo(0).evalgains, 0, sizeof(SearchInfo(0).evalgains)); //TODO this is bad to share with learning

	for (int i = 0; i < Guci_options->threads; i++) {
		initSearchThread(i);
    }

    memset(SearchInfo(0).moves, 0, sizeof(SearchInfo(0).moves));
   
	infinite = strstr(options, "infinite");
    ponder = strstr(options, "ponder");
    c = strstr(options, "wtime");
    if (c != NULL) sscanf(c+6, "%d", &wtime);
    c = strstr(options, "btime");
    if (c != NULL) sscanf(c+6, "%d", &btime);
    c = strstr(options, "winc");
    if (c != NULL) sscanf(c + 5, "%d", &winc);
    c = strstr(options, "binc");
    if (c != NULL) sscanf(c + 5, "%d", &binc);
    c = strstr(options, "movestogo");
    if (c != NULL) sscanf(c + 10, "%d", &movestogo);
    c = strstr(options, "depth");
    if (c != NULL) sscanf(c + 6, "%d", &maxdepth);
    c = strstr(options, "nodes");
    if (c != NULL) sscanf(c + 6, "%d", &nodes);
    c = strstr(options, "mate");
    if (c != NULL) sscanf(c + 5, "%d", &mate);
    c = strstr(options, "movetime");
    if (c != NULL) sscanf(c + 9, "%d", &movetime);
    c = strstr(options, "searchmoves");
    if (c != NULL) {
        genLegal(pos, &ml,TRUE);
        uciParseSearchmoves(&ml, c + 12, &(SearchInfo(0).moves[0]));
    }

    if (infinite) {
        SearchInfo(0).depth_is_limited = TRUE;
        SearchInfo(0).depth_limit = MAXPLY;
        Print(2, "info string Infinite\n");
    }
    if (maxdepth > 0) {
        SearchInfo(0).depth_is_limited = TRUE;
        SearchInfo(0).depth_limit = maxdepth;
        Print(2, "info string Depth is limited to %d half moves\n", SearchInfo(0).depth_limit);
    }
    if (mate > 0) {
        SearchInfo(0).depth_is_limited = TRUE;
        SearchInfo(0).depth_limit = mate * 2 - 1;
        Print(2, "info string Mate in %d half moves\n", SearchInfo(0).depth_limit);
    }
    if (nodes > 0) {
        SearchInfo(0).node_is_limited = TRUE;
        SearchInfo(0).node_limit = nodes;
        Print(2, "info string Nodes is limited to %d positions\n", SearchInfo(0).node_limit);
    }
    if (SearchInfo(0).moves[0]) {
        SearchInfo(0).moves_is_limited = TRUE;
        Print(2, "info string Moves is limited\n");
    }

    if (movetime > 0) {
        SearchInfo(0).time_is_limited = TRUE;
        SearchInfo(0).alloc_time = movetime;
        SearchInfo(0).time_limit_max = SearchInfo(0).start_time + movetime;
        SearchInfo(0).time_limit_abs = SearchInfo(0).start_time + movetime;
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
        SearchInfo(0).time_is_limited = TRUE;
        mytime = ((mytime * 95) / 100) - Guci_options->time_buffer;
        if (mytime  < 0) mytime = 0;
        if (movestogo <= 0 || movestogo > TIME_DIVIDER) movestogo = TIME_DIVIDER;
        SearchInfo(0).time_limit_max = (mytime / movestogo) + ((t_inc * 4) / 5);
        if (ponder) SearchInfo(0).time_limit_max += SearchInfo(0).time_limit_max / 4;

		if (SearchInfo(0).time_limit_max > mytime) SearchInfo(0).time_limit_max = mytime;
        SearchInfo(0).time_limit_abs = ((mytime * 2) / 5) + ((t_inc * 4) / 5);
        if (SearchInfo(0).time_limit_abs < SearchInfo(0).time_limit_max) SearchInfo(0).time_limit_abs = SearchInfo(0).time_limit_max;
        if (SearchInfo(0).time_limit_abs > mytime) SearchInfo(0).time_limit_abs = mytime;

		if (SearchInfo(0).time_limit_max < MIN_TIME) SearchInfo(0).time_limit_max = MIN_TIME; //SAM added to deail with issues when time < time_buffer
		if (SearchInfo(0).time_limit_abs < MIN_TIME) SearchInfo(0).time_limit_abs = MIN_TIME;

        Print(2, "info string Time is limited: ");
        Print(2, "max = %d, ", SearchInfo(0).time_limit_max);
        Print(2, "abs = %d\n", SearchInfo(0).time_limit_abs);
        SearchInfo(0).alloc_time = SearchInfo(0).time_limit_max;
        SearchInfo(0).time_limit_max += SearchInfo(0).start_time;
        SearchInfo(0).time_limit_abs += SearchInfo(0).start_time;
    }
    if (infinite) {
        SearchInfo(0).thinking_status = ANALYSING;
        Print(2, "info string Search status is ANALYSING\n");
    } else if (ponder) {
        SearchInfo(0).thinking_status = PONDERING;
        Print(2, "info string Search status is PONDERING\n");
    } else {
        SearchInfo(0).thinking_status = THINKING;
        Print(2, "info string Search status is THINKING\n");
    }
    /* initialize UCI parameters to be used in search */
	DrawValue[pos->side] = - Guci_options->contempt;
	DrawValue[pos->side^1] = Guci_options->contempt;
    getBestMove(pos,0);
    if (!SearchInfo(0).bestmove) {
		if (RETURN_MOVE)
	        Print(3, "info string No legal move found. Start a new game.\n\n");
        return;
    } else {
		if (RETURN_MOVE) {
	        Print(3, "bestmove %s", move2Str(SearchInfo(0).bestmove));
		    if (SearchInfo(0).pondermove) Print(3, " ponder %s", move2Str(SearchInfo(0).pondermove));
			 Print(3, "\n\n");
		}
		origScore = SearchInfo(0).last_value; // just to be safe
    }
}
void uciSetPosition(position_t *pos, char *str) {
    char *c = str, *m;
    char movestr[10];
    basic_move_t move;
    movelist_t ml;
    pos_store_t undo;
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
            genLegal(pos, &ml,TRUE);
            move = parseMove(&ml, movestr);
            if (!move) {
                Print(3, "info string Illegal move: %s\n", movestr);
                return;
            } else makeMove(pos, &undo, move);
#ifndef TCEC
			if (startPos && movesSoFar.length < MAXPLY-1) {
				movesSoFar.moves[movesSoFar.length] = move;
				movesSoFar.length++;
			}
#endif
			// this is to allow any number of moves in a game by only keeping the last relevant ones for rep detection
			if (pos->posStore.fifty==0) {
				pos->stack[0] = pos->hash;
				pos->sp = 0;
			}
            while (isspace(*c)) c++;
        }
    }
}
