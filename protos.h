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



/* material.h */
extern  void initMaterial(void);



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



/* main.c */
extern void quit(void);
extern int main(void);


// move
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


extern void evalEndgame(int attacker,const position_t *pos, eval_info_t *ei,int *score, int *draw, int mover);



extern void InitTrapped();
extern void InitMateBoost();



extern void checkSpeedUp(position_t* pos, char string[]);
extern void benchSplitDepth(position_t* pos, char string[]);
extern void benchSplitThreads(position_t* pos, char string[]);




inline uint32 getFirstBit(uint64 b);
inline uint32 popFirstBit(uint64 *b);
inline uint32 bitCnt(uint64 b);

inline uint64 fillUp(uint64 b);
inline uint64 fillDown(uint64 b);
inline uint64 fillUp2(uint64 b);
inline uint64 fillDown2(uint64 b);
inline uint64 shiftLeft(uint64 b, uint32 i);
inline uint64 shiftRight(uint64 b, uint32 i);