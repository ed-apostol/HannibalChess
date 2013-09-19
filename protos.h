/**************************************************/
/*  Name: Twisted Logic Chess Engine              */
/*  Copyright: 2009                               */
/*  Author: Edsel Apostol                         */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/
#pragma once


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
extern int anyRep(const position_t *pos);
extern int anyRepNoMove(const position_t *pos, const int m);

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
extern bool kingIsInCheck(const position_t *pos);
extern uint64 pinnedPieces(const position_t *pos, uint32 c);
extern uint64 discoveredCheckCandidates(const position_t *pos, uint32 c);
extern uint32 moveIsLegal(const position_t *pos, uint32 move, uint64 pinned, uint32 incheck);
extern bool moveIsCheck(const position_t *pos, uint32 m, uint64 dcc);
extern uint64 attackingPiecesAll(const position_t *pos, uint64 occ, uint32 sq);
extern uint64 attackingPiecesSide(const position_t *pos, uint32 sq, uint32 side);
extern int moveAttacksSquare(const position_t *pos, uint32 move, uint32 sq);
extern int swap(const position_t *pos, uint32 m);
extern bool isSqAtt(const position_t *pos, uint64 occ, int sq,int color);
extern uint64 pieceAttacksFromBB(const position_t* pos, const int pc, const int sq, const uint64 occ);

/* material.h */
extern  void initMaterial(void);

/* movegen.c */
extern void genLegal(const position_t *pos, movelist_t *mvlist,int promoteAll);
extern void genNonCaptures(const position_t *pos, movelist_t *mvlist);
extern void genCaptures(const position_t *pos, movelist_t *mvlist);
extern void genEvasions(const position_t *pos, movelist_t *mvlist);
extern void genQChecks(const position_t *pos, movelist_t *mvlist);
extern uint32 genMoveIfLegal(const position_t *pos, uint32 move, uint64 pinned);
extern void genGainingMoves(const position_t *pos, movelist_t *mvlist, int delta, int thread_id);

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
extern int eval(const position_t *pos, int thread_id, int *pessimism);

/* trans.c */
extern void initTrans(uint64 target, int thread);
extern void transClear(int thread);
extern pvhash_entry_t *pvHashProbe(const uint64 hash);
extern void transNewDate(int date, int thread);
extern void transStore(HashType ht, const uint64 hash, basic_move_t move, const int depth, const int value, const int thread);

/* movepicker.c */
extern void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, int thread_id);
extern move_t* getMove(movelist_t *mvlist);
extern move_t* sortNext(SplitPoint* sp, position_t *pos, movelist_t *mvlist, int& phase, int thread_id);
extern uint32 captureIsGood(const position_t *pos, uint32 m);
extern void scoreCaptures(movelist_t *mvlist);
extern void scoreCapturesPure(movelist_t *mvlist);
extern void scoreNonCaptures(const position_t *pos,movelist_t *mvlist, int thread_id);
extern void scoreAll(const position_t *pos, movelist_t *mvlist, int thread_id);
extern void scoreAllQ(movelist_t *mvlist, int thread_id);
extern void scoreRoot(movelist_t *mvlist);
extern bool moveIsPassedPawn(const position_t * pos, uint32 move);

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

/* utilities for move generation */
inline basic_move_t GenOneForward(uint f, uint t) {return ((f) | ((t)<<6) | (PAWN<<12));}
inline basic_move_t GenTwoForward(uint f, uint t) {return ((f) | ((t)<<6) | (PAWN<<12) | (1<<16));}
inline basic_move_t GenPromote(uint f, uint t, uint r, uint c) {return ((f) | ((t)<<6) | (PAWN<<12) | ((c)<<18) | ((r)<<22) | (1<<17));}
inline basic_move_t GenPromoteStraight(uint f, uint t, uint r) {return ((f) | ((t)<<6) | (PAWN<<12) | ((r)<<22)  | (1<<17));}
inline basic_move_t GenEnPassant(uint f, uint t) {return ((f) | ((t)<<6) | (PAWN<<12) | (PAWN<<18) | (1<<21));}
inline basic_move_t GenPawnMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (PAWN<<12) | ((c)<<18));}
inline basic_move_t GenKnightMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (KNIGHT<<12) | ((c)<<18));}
inline basic_move_t GenBishopMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (BISHOP<<12) | ((c)<<18));}
inline basic_move_t GenRookMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (ROOK<<12) | ((c)<<18));}
inline basic_move_t GenQueenMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (QUEEN<<12) | ((c)<<18));}
inline basic_move_t GenKingMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (KING<<12) | ((c)<<18));}
inline basic_move_t GenWhiteOO(void) {return (e1 | (g1<<6) | (KING<<12) | (1<<15));}
inline basic_move_t GenWhiteOOO(void) {return (e1 | (c1<<6) | (KING<<12) | (1<<15));}
inline basic_move_t GenBlackOO(void) {return (e8 | (g8<<6) | (KING<<12) | (1<<15));}
inline basic_move_t GenBlackOOO(void) {return (e8 | (c8<<6) | (KING<<12) | (1<<15));}

inline uint moveFrom(basic_move_t m) {return (63&(m));}
inline uint moveTo(basic_move_t m) {return (63&((m)>>6));}
inline uint movePiece(basic_move_t m) {return (7&((m)>>12));}
inline uint moveAction(basic_move_t m) {return (63&((m)>>12));}
inline uint moveCapture(basic_move_t m) {return (7&((m)>>18));}
inline uint moveRemoval(basic_move_t m) {return (15&((m)>>18));}
inline uint movePromote(basic_move_t m) {return (7&((m)>>22));}
inline uint isCastle(basic_move_t m) {return (((m)>>15)&1);}
inline uint isPawn2Forward(basic_move_t m) {return (((m)>>16)&1);}
inline uint isPromote(basic_move_t m) {return (((m)>>17)&1);}
inline uint isEnPassant(basic_move_t m) {return  (((m)>>21)&1);}

inline int moveIsTactical(uint32 m) {
    ASSERT(moveIsOk(m));
    return (m & 0x01fe0000UL);
}

inline int historyIndex(uint32 side, uint32 move) {
    return ((((side) << 9) + ((movePiece(move)) << 6) + (moveTo(move))) & 0x3ff);
}

extern void setAllThreadsToStop(int thread);
extern void initSearchThread(int i);
extern bool smpCutoffOccurred(SplitPoint *sp);
extern void initSmpVars();
extern bool idleThreadExists(int master);
extern void initThreads(void);
extern void stopThreads(void);
extern bool splitRemainingMoves(const position_t* p, movelist_t* mvlist, SearchStack* ss, SearchStack* ssprev, int alpha, int beta, NodeType nt, int depth, bool inCheck, bool inRoot, const int master);

extern void evalEndgame(int attacker,const position_t *pos, eval_info_t *ei,int *score, int *draw, int mover);

extern void initPVHashTab(pvhashtable_t* pvt, uint64 target);
extern void initPawnTab(pawntable_t* pt, uint64 target);
extern void initEvalTab(evaltable_t* et, uint64 target);
extern void InitTrapped();
extern void InitMateBoost();

extern void pawnTableClear(pawntable_t* pt);
extern void evalTableClear(evaltable_t* et);

extern void checkSpeedUp(position_t* pos, char string[]);
extern void benchSplitDepth(position_t* pos, char string[]);
extern void benchSplitThreads(position_t* pos, char string[]);


inline uint32 pvGetHashLock (pvhash_entry_t * pve)      { return pve->hashlock; }
inline basic_move_t pvGetMove (pvhash_entry_t * pve)    { return pve->move; }
inline int pvGetAge (pvhash_entry_t * pve)              { return pve->age; }
inline int pvGetDepth (pvhash_entry_t * pve)            { return pve->depth; }
inline int pvGetValue (pvhash_entry_t * pve)            { return pve->score; }

extern pvhash_entry_t *getPvEntryFromMove(const uint64 hash, basic_move_t move);


inline const uint32 transHashLock(trans_entry_t * te) { return te->hashlock; }
inline const basic_move_t transMove(trans_entry_t * te) { return te->move; }
inline const int transAge(trans_entry_t * te) { return te->age; }
inline const int transMask(trans_entry_t * te) { return te->mask; }
inline const int transLowerDepth(trans_entry_t * te) { return te->lowerdepth; }
inline const int transUpperDepth(trans_entry_t * te) { return te->upperdepth; }
inline const int transLowerValue(trans_entry_t * te) { return te->lowervalue; }
inline const int transUpperValue(trans_entry_t * te) { return te->uppervalue; }

inline void transSetHashLock(trans_entry_t * te, const uint32 hashlock) { te->hashlock = hashlock; }
inline void transSetMove(trans_entry_t * te, const basic_move_t move) { te->move = move; }
inline void transSetAge(trans_entry_t * te, const uint8 date) { te->age = date; }
inline void transSetMask(trans_entry_t * te, const uint8 mask) { te->mask |= mask; }
inline void transRemMask(trans_entry_t * te, const uint8 mask) { te->mask &= ~mask; }
inline void transReplaceMask(trans_entry_t * te, const uint8 mask) { te->mask = mask; }
inline void transSetLowerDepth(trans_entry_t * te, const uint8 lowerdepth) { te->lowerdepth = lowerdepth; }
inline void transSetUpperDepth(trans_entry_t * te, const uint8 upperdepth) { te->upperdepth = upperdepth; }
inline void transSetLowerValue(trans_entry_t * te, const int16 lowervalue) { te->lowervalue = lowervalue; }
inline void transSetUpperValue(trans_entry_t * te, const int16 uppervalue) { te->uppervalue = uppervalue; }

inline int scoreFromTrans(int score, int ply) { return (score > MAXEVAL) ? (score-ply) : ((score < MAXEVAL) ? (score+ply) : score); }
inline int scoreToTrans(int score, int ply) { return (score > MAXEVAL) ? (score+ply) : ((score < MAXEVAL) ? (score-ply) : score); }


inline uint32 getFirstBit(uint64 b);
inline uint32 popFirstBit(uint64 *b);
inline uint32 bitCnt(uint64 b);

inline uint64 fillUp(uint64 b);
inline uint64 fillDown(uint64 b);
inline uint64 fillUp2(uint64 b);
inline uint64 fillDown2(uint64 b);
inline uint64 shiftLeft(uint64 b, uint32 i);
inline uint64 shiftRight(uint64 b, uint32 i);