/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/
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
    int open[2];
    int end[2];
    int mat_summ[2];
    uint64 phash;
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
    uint64 hash;
    uint64 stack[MAX_HASH_STORE];
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

    // DEBUG
    uint64 cutnodes;
    uint64 allnodes;
    uint64 cutfail;
    uint64 allfail;

    bool try_easy;
    int rbestscore1;
    int rbestscore2;

    int lastDepthSearched;

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



