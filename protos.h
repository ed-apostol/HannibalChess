/**************************************************/
/*  Name: Twisted Logic Chess Engine              */
/*  Copyright: 2009                               */
/*  Author: Edsel Apostol                         */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/
/* endgame.h */

/* init.c */
extern void initPST(uci_option_t *opt);
extern void initArr(void);

/* utils.c */
extern void Print(int vb, char *fmt, ...);
extern void displayBit(uint64 a, int x);
extern char *bit2Str(uint64 n);
extern char *move2Str(basic_move_t m);
extern char *sq2Str(int sq);
extern void displayBoard(const position_t *pos, int x);
extern int getPiece(const position_t *pos, uint32 sq);
extern int getColor(const position_t *pos, uint32 sq);
extern int DiffColor(const position_t *pos, uint32 sq,int color);
extern uint64 getTime(void);
extern uint32 parseMove(movelist_t *mvlist, char *s);
extern int biosKey(void);
extern int getDirIndex(int d);

/* bitutils.c */
extern uint64 rand64(void);
extern __inline uint32 bitCnt(uint64 x);
extern __inline uint32 getFirstBit(uint64 b);
extern __inline uint32 popFirstBit(uint64 *b);
extern uint64 fillUp(uint64 b);
extern uint64 fillDown(uint64 b);
extern uint64 fillUp2(uint64 b);
extern uint64 fillDown2(uint64 b);
extern uint64 shiftLeft(uint64 b, uint32 i);
extern uint64 shiftRight(uint64 b, uint32 i);

/* attacks.c */
extern int key000(uint64 b, int f);
extern int key090(uint64 b, int f);
extern int keyDiag(uint64 _b);
extern int key045(uint64 b, int f);
extern int key135(uint64 b, int f);
extern uint64 getLowestBit(uint64 bb);
extern uint64 _occ_free_board(int bc, int del, uint64 free);
extern void _init_rays(uint64* rays, uint64 (*rayFunc) (int, uint64, int), int (*key)(uint64, int));
extern void setBit(int f, uint64 *b);
extern uint64 _rook0(int f, uint64 board, int t);
extern uint64 _rook90(int f, uint64 board, int t);
extern uint64 _bishop45(int f, uint64 board, int t);
extern uint64 _bishop135(int f, uint64 board, int t);
extern uint64 rankAttacks(uint64 occ, int sq);
extern uint64 fileAttacks(uint64 occ, int sq);
extern uint64 diagonalAttacks(uint64 occ, int sq);
extern uint64 antiDiagAttacks(uint64 occ, int sq);
extern uint64 rankAttacksX(uint64 occ, int sq);
extern uint64 fileAttacksX(uint64 occ, int sq);
extern uint64 diagonalAttacksX(uint64 occ, int sq);
extern uint64 antiDiagAttacksX(uint64 occ, int sq);
extern uint64 bishopAttacksBBX(uint32 from, uint64 occ);
extern uint64 rookAttacksBBX(uint32 from, uint64 occ);
extern uint64 bishopAttacksBB(uint32 from, uint64 occ);
extern uint64 rookAttacksBB(uint32 from, uint64 occ);
extern uint64 queenAttacksBB(uint32 from, uint64 occ);
extern uint32 isAtt(const position_t *pos, uint32 color, uint64 target);
extern uint32 kingIsInCheck(const position_t *pos);
extern uint64 pinnedPieces(const position_t *pos, uint32 c);
extern uint64 discoveredCheckCandidates(const position_t *pos, uint32 c);
extern uint32 moveIsLegal(const position_t *pos, uint32 move, uint64 pinned, uint32 incheck);
extern bool moveIsCheck(const position_t *pos, uint32 m, uint64 dcc);
extern uint64 attackingPiecesAll(const position_t *pos, uint64 occ, uint32 sq);
extern uint64 attackingPiecesSide(const position_t *pos, uint32 sq, uint32 side);
extern int moveAttacksSquare(const position_t *pos, uint32 move, uint32 sq);
extern int swap(const position_t *pos, uint32 m);

/* material.h */
extern  void initMaterial(void);

/* movegen.c */
extern void genLegal(const position_t *pos, movelist_t *mvlist,int promoteAll);
extern void genNonCaptures(const position_t *pos, movelist_t *mvlist);
extern void genCaptures(const position_t *pos, movelist_t *mvlist);
extern void genEvasions(const position_t *pos, movelist_t *mvlist);
extern void genQChecks(const position_t *pos, movelist_t *mvlist);
extern uint32 genMoveIfLegal(const position_t *pos, uint32 move, uint64 pinned);

/* position.c */
extern void unmakeNullMove(position_t *pos, pos_store_t *undo);
extern void makeNullMove(position_t *pos, pos_store_t *undo);
extern void unmakeMove(position_t *pos, pos_store_t *undo);
extern void makeMove(position_t *pos, pos_store_t *undo, uint32 m);
extern void setPosition(position_t *pos, const char *fen);
extern char *positionToFEN(const position_t *pos);

/* eval.c */
extern int kingPasser(const position_t *pos, int square, int color);
extern int unstoppablePasser(const position_t *pos, int square, int color);
//extern int eval(const position_t *pos, int thread_id);
extern int eval(const position_t *pos, int thread_id, int *optimism, int *pessimism);

/* trans.c */
extern void initTrans(uint64 target, int thread);
extern void transClear(int thread);

/* movepicker.c */
extern void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int depth, int type, int thread_id);
extern basic_move_t getMove(movelist_t *mvlist);
extern basic_move_t sortNext(split_point_t* sp, position_t *pos, movelist_t *mvlist, int *phase, int thread_id);
extern uint32 captureIsGood(const position_t *pos, uint32 m);
extern void scoreCaptures(movelist_t *mvlist);
extern void scoreCapturesPure(movelist_t *mvlist);
extern void scoreNonCaptures(const position_t *pos,movelist_t *mvlist, int thread_id);
extern void scoreAll(const position_t *pos, movelist_t *mvlist, int thread_id);
extern void scoreAllQ(movelist_t *mvlist, int thread_id);
extern void scoreRoot(movelist_t *mvlist);

/* search.c */
extern void ponderHit(int thread_id);
extern void check4Input(int thread_id);
extern void initNode(position_t *pos, int thread_id);
extern int moveIsTactical(uint32 m);
extern int simpleStalemate(const position_t *pos);
extern int historyIndex(uint32 side, uint32 move);
//extern int qSearch(position_t *pos, int alpha, int beta, int depth, const int pv, const int inCheck, int thread_id);
//template <bool inPv>
//int qSearch(position_t *pos, int alpha, int beta, int depth, int inCheck, const int thread_id);

template <bool inRoot, bool inSplitPoint, bool inSingular>
int searchNode(position_t *pos, int alpha, int beta, const int depth, const bool inCheck, const basic_move_t moveBanned, const int thread_id, NodeType nt);
extern void getBestMove(position_t *pos, int thread_id);

/* debug.c */
extern int squareIsOk(int s);
extern int colorIsOk(int c);
extern int moveIsOk(basic_move_t m);
extern int valueIsOk(int v);
extern int rankIsOk(int r);
extern void flipPosition(const position_t *pos, position_t *clone);
extern int evalSymmetryIsOk(const position_t *pos);
extern uint64 pawnHashRecalc(const position_t *pos);
extern void positionIsOk(const position_t *pos);

/* tests.c */
extern void perft(position_t *pos, int maxply, uint64 nodesx[]);
extern int perftDivide(position_t *pos, uint32 depth, uint32 maxply);
extern void runPerft(position_t *pos, int maxply);
extern void runPerftDivide(position_t *pos, uint32 maxply);
extern void nonUCI(position_t *pos);
/* tune.h */
#ifdef OPTIMIZE
extern void optimize(position_t *pos, int threads);
#endif
#ifdef SELF_TUNE2
extern void NewTuneGame(const int player1);
#endif
/* uci.c */
extern void uciSetOption(char string[]);
extern void initOption(uci_option_t* opt);
extern void uciParseSearchmoves(movelist_t *ml, char *str, uint32 moves[]);
extern void uciGo(position_t *pos, char *options);
extern void uciStart(void);
extern void uciSetPosition(position_t *pos, char *str);

/*book.h*/
extern int puck_book_score(position_t *p, book_t *book);
extern basic_move_t getBookMove(position_t *p, book_t *book, movelist_t *ml, bool verbose, int randomness);
extern void add_to_learn_begin(learn_t *learn, continuation_t *toLearn);
extern void initBook(char* book_name, book_t *book, BookType type);
extern int current_puck_book_score(position_t *p, book_t *book);
extern bool get_continuation_to_learn(learn_t *learn, continuation_t *toLearn);
extern void insert_score_to_puck_file(book_t *book, uint64 key, int score);
extern bool learn_continuation(int thread_id, continuation_t *toLearn);
extern void generateContinuation(continuation_t *variation);
/* main.c */
extern void quit(void);
extern int main(void);
