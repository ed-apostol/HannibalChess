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
#include "attacks.h"
#include "position.h"
#include "utils.h"
#include "eval.h"



/* a utility to print output into different files:
bit 0: stdout
bit 1: logfile
bit 2: errfile
bit 3: dumpfile
*/
void Print(int vb, char *fmt, ...) {
    va_list ap;

    ASSERT( vb >= 1 && vb <= 15);

    va_start(ap, fmt);
    if (vb&1) {
        vprintf(fmt, ap);
        fflush(stdout);
    }
    va_start(ap, fmt);
    if (logfile&&((vb>>1)&1)) {
        vfprintf(logfile, fmt, ap);
        fflush(logfile);
    }
    va_start(ap, fmt);
    if (errfile&&((vb>>2)&1)) {
        vfprintf(errfile, fmt, ap);
        fflush(errfile);
    }
    va_start(ap, fmt);
    if (dumpfile&&((vb>>3)&1)) {
        vfprintf(dumpfile, fmt, ap);
        fflush(dumpfile);
    }
    va_end(ap);
}

/* a utility to print the bits in a position-like format */
void displayBit(uint64 a, int x) {
    int i, j;
    for (i = 56; i >= 0; i -= 8) {
        Print(x, "\n%d  ",(i / 8) + 1);
        for (j = 0; j < 8; ++j) {
            Print(x, "%c ", ((a & ((uint64)1 << (i + j))) ? '1' : '_'));
        }
    }
    Print(x, "\n\n");
    Print(x, "   a b c d e f g h \n\n");
}
int fr_square(int f, int r) {
    return ((r << 3) | f);
}
void showBitboard(uint64 b, int x) {
    int i,j;
    for (j = 7; j >= 0; j=j-1) {
        for (i=0; i <= 7; i=i+1) {
            int sq = fr_square(i, j);
            uint64 sqBM = BitMask[sq];
            if (sqBM & b) {
                Print(x,"X");
            }
            else Print(x,".");
            Print(x," ");
        }
        Print(x,"\n");
    }
}
/* a utility to convert large int to hex string*/
char *bit2Str(uint64 n) {
    static char s[19];
    int i;
    s[0] = '0';
    s[1] = 'x';
    for (i = 0; i < 16; i++) {
        if ((n & 15) < 10) s[17 - i] = '0' + (n & 15);
        else s[17 - i] = 'A' + (n & 15) - 10;
        n >>= 4;
    }
    s[18] = 0;
    return s;
}

/* a utility to convert int move to string */
char *move2Str(basic_move_t m) {
    static char promstr[]="\0pnbrqk";
    static char str[6];

    /* ASSERT(moveIsOk(m)); */

    if (m == 0) sprintf_s(str, "%c%c%c%c%c", '0','0','0','0','\0');
    else sprintf_s(str, "%c%c%c%c%c",
        SQFILE(moveFrom(m)) + 'a',
        '1' + SQRANK(moveFrom(m)),
        SQFILE(moveTo(m)) + 'a',
        '1' + SQRANK(moveTo(m)),
        promstr[movePromote(m)]
    );
    return str;
}

/* a utility to convert int square to string */
char *sq2Str(int sq) {
    static char str[3];

    /* ASSERT(moveIsOk(m)); */

    sprintf_s(str, "%c%c%c",
        SQFILE(sq) + 'a',
        '1' + SQRANK(sq),
        '\0'
        );
    return str;
}

/* a utility to print the position */
void displayBoard(const position_t *pos, int x) {
    static char pcstr[] = ".PNBRQK.pnbrqk";
    int i, j, c, p;
    int pes;
    for (i = 56; i >= 0; i -= 8) {
        Print(x, "\n%d  ",(i / 8) + 1);
        for (j = 0; j < 8; j++) {
            c = getColor(pos, i + j);
            p = getPiece(pos, i + j);
            Print(x, "%c ", pcstr[p + (c ? 7 : 0)]);
        }
    }
    Print(x, "\n\n");
    Print(x, "   a b c d e f g h \n\n");
    Print(x, "FEN %s\n", positionToFEN(pos));
    Print(x, "%d.%s%s ", (pos->sp)/2
        +(pos->side?1:0), pos->side?" ":" ..",
        move2Str(pos->posStore.lastmove));
    Print(x, "%s, ", pos->side == WHITE ? "WHITE" : "BLACK");
    Print(x, "Castle = %d, ", pos->posStore.castle);
    Print(x, "Ep = %d, ", pos->posStore.epsq);
    Print(x, "Fifty = %d, ", pos->posStore.fifty);
    Print(x, "Ev = %d, ", eval(pos, 0, &pes));
    Print(x, "Ch = %s,\n",
        isAtt(pos, pos->side^1, pos->kings&pos->color[pos->side])
        ? "T" : "F");
    Print(x, "H = %s, ", bit2Str(pos->posStore.hash));
    Print(x, "PH = %s\n", bit2Str(pos->posStore.phash));
}

/* a utility to get a certain piece from a position given a square */
int getPiece(const position_t *pos, uint32 sq) {

    ASSERT(pos != NULL);
    ASSERT(squareIsOk(sq));

    //uint64 mask = BitMask[sq];
    //if (mask & pos->pawns) return PAWN;
    //if (mask & pos->knights) return KNIGHT;
    //if (mask & pos->bishops) return BISHOP;
    //if (mask & pos->rooks) return ROOK;
    //if (mask & pos->queens) return QUEEN;
    //if (mask & pos->kings) return KING;
    //return EMPTY;
    return pos->pieces[sq];
}

/* a utility to get a certain color from a position given a square */
int getColor(const position_t *pos, uint32 sq) {
    uint64 mask = BitMask[sq];

    ASSERT(pos != NULL);
    ASSERT(squareIsOk(sq));

    if (mask & pos->color[WHITE]) return WHITE;
    else if (mask & pos->color[BLACK]) return BLACK;
    else {
        ASSERT(mask & ~pos->occupied);
        return WHITE;
    }
}
int DiffColor(const position_t *pos, uint32 sq,int color) {
    //    uint64 mask = BitMask[sq];

    ASSERT(pos != NULL);
    ASSERT(squareIsOk(sq));

    return ((BitMask[sq] & pos->color[color]) ==0);
}
/* returns time in milli-seconds */
uint64 getTime(void) {
#if defined(_WIN32) || defined(_WIN64)
    static struct _timeb tv;
    _ftime_s(&tv);
    return(tv.time * 1000 + tv.millitm);
#else
    static struct timeval tv;
    static struct timezone tz;
    gettimeofday (&tv, &tz);
    return(tv.tv_sec * 1000 + (tv.tv_usec / 1000));
#endif
}

/* parse the move from string and returns a move from the
move list of generated moves if the move string matched
one of them */
uint32 parseMove(movelist_t *mvlist, char *s) {
    uint32 m;
    uint32 from, to, p;

    ASSERT(mvlist != NULL);
    ASSERT(s != NULL);

    from = (s[0] - 'a') + (8 * (s[1] - '1'));
    to = (s[2] - 'a') + (8 * (s[3] - '1'));
    m = (from) | (to << 6);
    for (mvlist->pos = 0; mvlist->pos < mvlist->size; mvlist->pos++) {
        if (m == (mvlist->list[mvlist->pos].m & 0xfff)) {
            p = EMPTY;
            if (movePromote(mvlist->list[mvlist->pos].m)) {
                switch (s[4]) {
                case 'n':
                case 'N':
                    p = KNIGHT;
                    break;
                case 'b':
                case 'B':
                    p = BISHOP;
                    break;
                case 'r':
                case 'R':
                    p = ROOK;
                    break;
                default:
                    p = QUEEN;
                    break;
                }
            }
            if (p == movePromote(mvlist->list[mvlist->pos].m)) return mvlist->list[mvlist->pos].m;
        }
    }
    return 0;
}

int biosKey(void) {
#if defined(_WIN32) || defined(_WIN64)
    /* Windows-version */
    static int init = 0, pipe;
    static HANDLE inh;
    DWORD dw;
    if (stdin->_cnt > 0) return stdin->_cnt;
    if (!init) {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if (!pipe) {
            SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    if (pipe) {
        if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
        return dw;
    } else {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return dw <= 1 ? 0 : dw;
    }
#else
    /* Non-windows version */
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(fileno(stdin), &readfds);
    /* Set to timeout immediately */
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    select(16, &readfds, 0, 0, &timeout);
    return (FD_ISSET(fileno(stdin), &readfds));
#endif
}

int anyRep(const position_t *pos) {//this is used for book repetition detection, but should not be used in search
    if (pos->posStore.fifty >= 100) return true;
    ASSERT (pos->sp >= pos->posStore.fifty);
    int plyForRep = 4, pliesToCheck = MIN(pos->posStore.pliesFromNull, pos->posStore.fifty);
    if (plyForRep <= pliesToCheck) {
        pos_store_t* psp = pos->posStore.previous->previous;
        do {
            psp = psp->previous->previous;
            if (psp->hash == pos->posStore.hash) return true;
            plyForRep += 2;
        } while (plyForRep <= pliesToCheck);
    }
    return false;
}

int anyRepNoMove(const position_t *pos, const int m) {//assumes no castle and no capture
    int moved, fromSq, toSq;
    uint64 compareTo;

    if (moveCapture(m) || isCastle(m) || pos->posStore.fifty < 3) return false;
    moved = movePiece(m);
    if (moved==PAWN) return false;
    if (pos->posStore.fifty > 99) return true;
    //TODO consider  castle check in here
    fromSq = moveFrom(m);
    toSq = moveTo(m);
    compareTo = pos->posStore.hash ^ ZobColor ^ ZobPiece[pos->side][moved][fromSq] ^ ZobPiece[pos->side][moved][toSq];
    int plyForRep = 4, pliesToCheck = MIN(pos->posStore.pliesFromNull, pos->posStore.fifty);
    if (plyForRep <= pliesToCheck) {
        pos_store_t* psp = pos->posStore.previous;
        do {
            psp = psp->previous->previous;
            if (psp->hash == compareTo) return true;
            plyForRep += 2;
        } while (plyForRep <= pliesToCheck);
    }
    return false;
}



