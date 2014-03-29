/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

// error check shows could use minor debugging work

// rbp v rb opp bishop drawish
/*
TODO ED:
-implement Search/Thread/Protocol/BitBoard/ classes
-fix all Level 4 warnings in MSVC
-fix all Code Analysis warnings in MSVC
-implement one score for midgame/endgame in eval, more efficient
-add Chess 960 support
*/
/* TODO EVAL
rook vs pawn cut off on 4th rank
try transstore on ALL pruning
write situation specific transtores
can knight catch pawns code from LL
//68 at 3:29
*/
#define VERSION            "1.5beta5"
#define NUM_THREADS			    6
#define MIN_SPLIT_DEPTH			4 // best is 4
#define MAX_SPLIT_THREADS		4 // best is 4
//#define TESTING_ON true
//#define TCEC true
//#define LEARNING_ON true

//#define SPEED_TEST
//#define NEW_EASY true
//#define DEBUG_EASY true
//#define OPTIMIZE true
//#define DEBUG
//#define EVAL_DEBUG true
//#define DEBUG_ML true
//#define DEBUG_SEE true
//#define DEBUG_EVAL_TABLE true
//#define DEBUG_RAZOR true
//#define DEBUG_INDEPTH true

//#define SELF_TUNE2 1 //number of simultaneous training games
//#define TUNE_MAT TRUE

#define NP2 TRUE
#define USE_PHASH TRUE
#define MIN_TRANS_SIZE 16

#ifdef LEARNING
#define DEFAULT_LEARN_THREADS 0
#define DEFAULT_LEARN_TIME 3
#define LEARN_NODES 10000000
#define SHOW_LEARNING false
#define LOG_LEARNING true
#define LEARN_PAWN_HASH_SIZE 32
#define LEARN_EVAL_HASH_SIZE 32

#define DEFAULT_BOOK_EXPLORE 2
#define MAXLEARN_OUT_OF_BOOK 2
#define DEFAULT_HANNIBAL_BOOK "HannibalBook.han"
#define DEFAULT_HANNIBAL_LEARN "HannibalLearn.lrn"
#define MAX_CONVERT 20
#define HANNIBAL_BOOK_RANDOM (Guci_options.bookExplore*10)
#define MIN_RANDOM -20
#define DEFAULT_BOOK_SCORE INF
#endif

#ifdef TCEC
#define INIT_EVAL 64
#define INIT_PAWN 32
#define INIT_HASH 64
#define INIT_PVHASH 1

#else
#define INIT_EVAL 64
#define INIT_PAWN 32
#define INIT_HASH 64
#define INIT_PVHASH 1

#define DEFAULT_POLYGLOT_BOOK "HannibalPoly.bin"
#define MAX_BOOK 60 //could be MAXPLY
#endif

#define DEBUG_BOOK false
#define DEBUG_LEARN false

#define ERROR_FILE "errfile.txt"

#ifdef SPEED_TEST
#define SHOW_SEARCH FALSE
#define RETURN_MOVE FALSE
#else
#ifdef SELF_TUNE2
#define SHOW_SEARCH FALSE
#define RETURN_MOVE FALSE
#else
#define SHOW_SEARCH TRUE
#define RETURN_MOVE TRUE
#endif
#endif



#if defined(__x86_64) || defined(_WIN64)
#define VERSION64BIT
#endif

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <sys/timeb.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type

#include "macros.h"

#include <iostream>
using namespace std;
/* some basic definitions */
#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline
#elif defined(__GNUC__)
#define INLINE __inline__ __attribute__((always_inline))
#else
#define INLINE inline
#endif
#endif

#ifndef _MSC_VER
#include <inttypes.h>
typedef int8_t			int8;
typedef uint8_t			uint8;
typedef int16_t			int16;
typedef uint16_t		uint16;
typedef int32_t			int32;
typedef uint32_t		uint32;
typedef int64_t			int64;
typedef uint64_t		uint64;
typedef unsigned int	uint;
#else
typedef __int8				int8;
typedef unsigned __int8		uint8;
typedef __int16				int16;
typedef unsigned __int16	uint16;
typedef __int32				int32;
typedef unsigned __int32	uint32;
typedef __int64				int64;
typedef unsigned __int64	uint64;
typedef unsigned int		uint;
#endif

enum NodeType { CutNode = -1, PVNode = 0, AllNode };
enum BookType { POLYGLOT_BOOK, PUCK_BOOK };
enum HashType { HTLower, HTUpper, HTCutUpper, HTAllLower, HTExact, HTNoMoves };
enum HashMask { MLower = 1, MUpper = 2, MCutUpper = 4, MAllLower = 8, MExact = 16, MNoMoves = 32, MClear = 255 };

typedef uint32 basic_move_t;

struct continuation_t
{
    basic_move_t moves[MAXPLY + 1];
    int length;
};

///* the move structure */
struct move_t {
    basic_move_t m;
    int32 s;
};

/* the structure used in generating moves */
struct movelist_t {
    volatile uint32 phase;
    basic_move_t transmove;
    basic_move_t killer1;
    basic_move_t killer2;
    int32 evalvalue;
    int32 scout;
    int32 ply;
    int32 depth;
    int32 side;
    volatile int32 pos;
    volatile int32 size;
    volatile int32 startBad;
    uint64 pinned;
    move_t list[MAXMOVES];
};
#ifndef TCEC
#ifdef LEARNING_ON
struct learn_t {
    learn_t() : learnFile(NULL) {}
    ~learn_t() { if (learnFile) fclose(learnFile); }
    FILE *learnFile;
    string name;
};
#endif

struct book_t {
    BookType type;
    book_t() : bookFile(NULL) {}
    ~book_t() { if (bookFile) fclose(bookFile); }
    FILE *bookFile;
    int64 size;
    string name;
};
#endif
/* the pawn hash table entry type */
struct pawn_entry_t{
    uint32 hashlock;
    uint64 passedbits;
    int16 opn;
    int16 end;
    int8 shelter[2];
    int8 kshelter[2];
    int8 qshelter[2];
    //	int8 halfPassed[2]; //currently unused
};

/* the pawn hash table type */
struct pawntable_t{
    pawntable_t() : table(NULL) {}
    ~pawntable_t() { if (table) free(table); }
    pawn_entry_t *table;
    uint64 size;
    uint64 mask;
};

struct eval_entry_t{
    uint32 hashlock;
    int16 value;
    int16 pessimism;
};

struct evaltable_t{
    evaltable_t() : table(NULL) {}
    ~evaltable_t() { if (table) free(table); }
    eval_entry_t *table;
    uint64 size;
    uint64 mask;
};

/* the trans table entry type */
struct trans_entry_t{
    uint32 hashlock;
    uint32 move;
    int16 uppervalue;
    int16 lowervalue;
    uint8 mask;
    uint8 age;
    uint8 upperdepth;
    uint8 lowerdepth;
};

/* the trans table type */
struct transtable_t{
    transtable_t() : table(NULL) {}
    ~transtable_t() { if (table) free(table); }
    trans_entry_t *table;
    uint64 size;
    uint64 mask;
    int32 date;
    uint64 used;
    int32 age[DATESIZE];
};

struct pvhash_entry_t {
    uint32 hashlock;
    basic_move_t move;
    int16 score;
    uint8 depth;
    uint8 age;
};

struct pvhashtable_t {
    pvhashtable_t() : table(NULL) {}
    ~pvhashtable_t() { if (table) free(table); }
    pvhash_entry_t *table;
    uint64 size;
    uint64 mask;
};

struct uci_option_t{
    int time_buffer;
    int contempt;
    int threads;
#ifndef TESTING_ON
    int try_book;
    int book_limit;
    int learnThreads;
    int usehannibalbook;
    int learnTime;
    int bookExplore;
#endif
    int min_split_depth;
    int max_split_threads;
    int evalcachesize;
    int pawnhashsize;
};

/* the eval info structure */
struct eval_info_t{
    uint64 atkall[2];
    uint64 atkpawns[2];
    uint64 atkknights[2];
    uint64 atkbishops[2];
    uint64 kingatkbishops[2];
    uint64 atkrooks[2];
    uint64 kingatkrooks[2];
    uint64 atkqueens[2];
    uint64 atkkings[2];
    uint64 kingzone[2];
    uint64 kingadj[2];
    uint64 potentialPawnAttack[2];
    uint64 pawns[2];
    int mid_score[2];
    int end_score[2];
    int draw[2];
    int atkcntpcs[2];
    int phase;
    int MLindex[2];
    int flags;
    int queening;
    uint8 endFlags[2];
    pawn_entry_t *pawn_entry;
};

/* the undo structure */
struct pos_store_t{
    uint32 lastmove;
    int castle;
    int fifty;
    int epsq;
    int pliesFromNull;
    int open[2];
    int end[2];
    int mat_summ[2];
    uint64 phash;
    uint64 hash;
    pos_store_t* previous;
};

/* the position structure */
struct position_t{
    uint64 pawns;
    uint64 knights;
    uint64 bishops;
    uint64 rooks;
    uint64 queens;
    uint64 kings;
    uint64 color[2];
    uint64 occupied;
    int pieces[64];
    int kpos[2];
    pos_store_t posStore;

    int side;
    int ply;
    int sp;
};

typedef uint8 mflag_t;

/* the material info structure */
struct material_info_t{
    int16 value;
    uint8 phase;
    uint8 draw[2];
    mflag_t flags[2];
};

/* the search data structure */
struct search_info_t{
    int thinking_status;
#ifndef TCEC
    int outOfBook;
#endif
    int depth_is_limited;
    int depth_limit;
    int moves_is_limited;
    int time_is_limited;

    int64 time_limit_max;
    int64 time_limit_abs;
    int node_is_limited;
    int64 node_limit;

    int64 start_time;
    int64 last_time;
    int64 alloc_time;

    int last_last_value;
    int last_value;
    int best_value;

    int mate_found;
    int currmovenumber;
    int change;
    int research;
    int iteration;

    int legalmoves;
    basic_move_t bestmove;
    basic_move_t pondermove;

    basic_move_t moves[MAXMOVES];
    bool mvlist_initialized;
    continuation_t rootPV;
    int32 evalgains[1024];
    int32 history[1024];
    evaltable_t et;
    pawntable_t pt;
    transtable_t tt;
};


struct ThreadStack {
    void Init() {
        killer1 = EMPTY;
        killer2 = EMPTY;
    }
    basic_move_t killer1;
    basic_move_t killer2;
};

struct SearchStack {
    SearchStack() :
    firstExtend(false),
    reducedMove(false),
    moveGivesCheck(false),
    playedMoves(0),
    hisCnt(0),
    evalvalue(-INF),
    bestvalue(-INF),
    bestmove(EMPTY),
    dcc(0),
    counterMove(EMPTY),
    threatMove(EMPTY),
    bannedMove(EMPTY),
    hashMove(EMPTY),
    hashDepth(0),
    mvlist(&movelist),
    mvlist_phase(0)
    { }
    int playedMoves;
    int hisCnt;
    basic_move_t hisMoves[64];
    int bestvalue;
    basic_move_t bestmove;

    bool firstExtend;
    bool reducedMove;
    bool moveGivesCheck;
    int evalvalue;
    uint64 dcc;
    basic_move_t counterMove;
    basic_move_t threatMove;
    basic_move_t bannedMove;
    basic_move_t hashMove;
    int hashDepth;

    movelist_t movelist;
    movelist_t* mvlist;
    int mvlist_phase;
};

typedef CRITICAL_SECTION mutex_t;

struct SplitPoint {
    position_t pos[MaxNumOfThreads];
    SplitPoint* parent;
    SearchStack* sscurr;
    SearchStack* ssprev;
    int depth;
    bool inCheck;
    bool inRoot;
    NodeType nodeType;
    volatile int alpha;
    volatile int beta;
    volatile int bestvalue;
    volatile int played;
    volatile basic_move_t bestmove;
    volatile uint64 workersBitMask;
    volatile bool cutoff;
    mutex_t movelistlock[1];
    mutex_t updatelock[1];
};

struct thread_t {
    SplitPoint *split_point;
    volatile bool stop;
    volatile bool running;
    volatile bool searching;
    volatile bool exit_flag;
    HANDLE idle_event;
    uint64 nodes;
    uint64 nodes_since_poll;
    uint64 nodes_between_polls;
    uint64 started; // DEBUG
    uint64 ended; // DEBUG
    int64 numsplits; // DEBUG
    int num_sp;
    ThreadStack ts[MAXPLY];
    SplitPoint sptable[MaxNumSplitPointsPerThread];
#ifdef SELF_TUNE2
    bool playingGame;
#endif
};


enum directions { SW, W, NW, N, NE, E, SE, S, NO_DIR };//{-9, -1, 7, 8, 9, 1, -7, -8};

/* the squares */
enum squarenames {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

/* the phases used for lazy move generation */
enum movegen_phases {
    PH_NONE = 0,
    PH_ROOT,
    PH_EVASION,
    PH_TRANS,
    PH_ALL_CAPTURES,
    PH_ALL_CAPTURES_PURE,
    PH_GOOD_CAPTURES,
    PH_GOOD_CAPTURES_PURE,
    PH_BAD_CAPTURES,
    PH_KILLER_MOVES,
    PH_QUIET_MOVES,
    PH_NONTACTICAL_CHECKS,
    PH_NONTACTICAL_CHECKS_WIN,
    PH_NONTACTICAL_CHECKS_PURE,
    PH_GAINING,
    PH_END
};

#ifdef SELF_TUNE
struct player_t { //if you put an array in here you need to change the copy and compare functions

    /* MATERIAL
    int p1, p2, p3, p4;
    int n1, n2, n3, n4;
    int b1, b2, b3, b4;
    int r1, r2, r3, r4;
    int q1, q2, q3, q4;
    int bb1, bb2, bb3, bb4;
    */
    double red_min;
    double p_red_min;
    double red_scale;
    double p_red_scale;
    int lprune_min;
    int lprune_scale;
    int fut_scale;
    int qch;
    int p_qch;
    int ddepth;
    int dscore;
    int fut;
    int raz;
    int min_n;
    int n_scale;
    int n_bon;
    int n_res;
    int swap_prune;
    int ext_only;
    int asp;
    int lch;

    int games;
    int points; // win is 2, draw is 1.
    double rating;
};
#endif

INLINE uint moveFrom(basic_move_t m) { return (63 & (m)); }
INLINE uint moveTo(basic_move_t m) { return (63 & ((m) >> 6)); }
INLINE uint movePiece(basic_move_t m) { return (7 & ((m) >> 12)); }
INLINE uint moveAction(basic_move_t m) { return (63 & ((m) >> 12)); }
INLINE uint moveCapture(basic_move_t m) { return (7 & ((m) >> 18)); }
INLINE uint moveRemoval(basic_move_t m) { return (15 & ((m) >> 18)); }
INLINE uint movePromote(basic_move_t m) { return (7 & ((m) >> 22)); }
INLINE uint isCastle(basic_move_t m) { return (((m) >> 15) & 1); }
INLINE uint isPawn2Forward(basic_move_t m) { return (((m) >> 16) & 1); }
INLINE uint isPromote(basic_move_t m) { return (((m) >> 17) & 1); }
INLINE uint isEnPassant(basic_move_t m) { return  (((m) >> 21) & 1); }

INLINE basic_move_t GenOneForward(uint f, uint t) { return ((f) | ((t) << 6) | (PAWN << 12)); }
INLINE basic_move_t GenTwoForward(uint f, uint t) { return ((f) | ((t) << 6) | (PAWN << 12) | (1 << 16)); }
INLINE basic_move_t GenPromote(uint f, uint t, uint r, uint c) { return ((f) | ((t) << 6) | (PAWN << 12) | ((c) << 18) | ((r) << 22) | (1 << 17)); }
INLINE basic_move_t GenPromoteStraight(uint f, uint t, uint r) { return ((f) | ((t) << 6) | (PAWN << 12) | ((r) << 22) | (1 << 17)); }
INLINE basic_move_t GenEnPassant(uint f, uint t) { return ((f) | ((t) << 6) | (PAWN << 12) | (PAWN << 18) | (1 << 21)); }
INLINE basic_move_t GenPawnMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (PAWN << 12) | ((c) << 18)); }
INLINE basic_move_t GenKnightMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (KNIGHT << 12) | ((c) << 18)); }
INLINE basic_move_t GenBishopMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (BISHOP << 12) | ((c) << 18)); }
INLINE basic_move_t GenRookMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (ROOK << 12) | ((c) << 18)); }
INLINE basic_move_t GenQueenMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (QUEEN << 12) | ((c) << 18)); }
INLINE basic_move_t GenKingMove(uint f, uint t, uint c) { return ((f) | ((t) << 6) | (KING << 12) | ((c) << 18)); }
INLINE basic_move_t GenWhiteOO(void) { return (e1 | (g1 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenWhiteOOO(void) { return (e1 | (c1 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenBlackOO(void) { return (e8 | (g8 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenBlackOOO(void) { return (e8 | (c8 << 6) | (KING << 12) | (1 << 15)); }
INLINE basic_move_t GenBasicMove(uint f, uint t, int pieceType, uint c) { return ((f) | ((t) << 6) | (pieceType << 12) | ((c) << 18)); }


