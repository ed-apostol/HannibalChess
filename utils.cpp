/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include <chrono>
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

int fr_square(int f, int r) {
    return ((r << 3) | f);
}
void PrintBitBoard(uint64 n) {
    PrintOutput() << "info string ";
    while (n) {
        int sq = popFirstBit(&n);
        PrintOutput() << (char)(SQFILE(sq) + 'a') << (char)('1' + SQRANK(sq)) << ' ';
    }
    PrintOutput() << "\n";
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
    static char promstr[] = "\0pnbrqk";
    static char str[6];

    /* ASSERT(moveIsOk(m)); */

    if (m == 0) sprintf_s(str, "%c%c%c%c%c", '0', '0', '0', '0', '\0');
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

/* a utility to get a certain piece from a position given a square */
int getPiece(const position_t& pos, uint32 sq) {
    return pos.pieces[sq];
}

/* a utility to get a certain color from a position given a square */
int getColor(const position_t& pos, uint32 sq) {
    uint64 mask = BitMask[sq];

    ASSERT(squareIsOk(sq));

    if (mask & pos.color[WHITE]) return WHITE;
    else if (mask & pos.color[BLACK]) return BLACK;
    else {
        ASSERT(mask & ~pos.occupied);
        return WHITE;
    }
}
int DiffColor(const position_t& pos, uint32 sq, int color) {
    ASSERT(squareIsOk(sq));

    return ((BitMask[sq] & pos.color[color]) == 0);
}
/* returns time in milli-seconds */
uint64 getTime(void) {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/* parse the move from string and returns a move from the
move list of generated moves if the move string matched
one of them */
uint32 parseMove(movelist_t& mvlist, const char *s) {
    uint32 m;
    uint32 from, to, p;

    from = (s[0] - 'a') + (8 * (s[1] - '1'));
    to = (s[2] - 'a') + (8 * (s[3] - '1'));
    m = (from) | (to << 6);
    for (mvlist.pos = 0; mvlist.pos < mvlist.size; mvlist.pos++) {
        if (m == (mvlist.list[mvlist.pos].m & 0xfff)) {
            p = EMPTY;
            if (movePromote(mvlist.list[mvlist.pos].m)) {
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
            if (p == movePromote(mvlist.list[mvlist.pos].m)) return mvlist.list[mvlist.pos].m;
        }
    }
    return 0;
}

bool anyRep(const position_t& pos) {//this is used for book repetition detection, but should not be used in search
    if (pos.posStore.fifty >= 100) return true;
    pos_store_t* psp;
    if (!pos.posStore.previous || !pos.posStore.previous->previous) return false;
    psp = pos.posStore.previous->previous;
    for (int plyForRep = 4, pliesToCheck = MIN(pos.posStore.pliesFromNull, pos.posStore.fifty); plyForRep <= pliesToCheck; plyForRep += 2) {
        if (!psp->previous || !psp->previous->previous) return false;
        psp = psp->previous->previous;
        if (psp->hash == pos.posStore.hash) return true;
    }
    return false;
}

bool anyRepNoMove(const position_t& pos, const int m) {
    if (moveCapture(m) || isCastle(m) || pos.posStore.fifty < 3 || pos.posStore.pliesFromNull < 3 || movePiece(m) == PAWN) return false;
    if (pos.posStore.fifty >= 99) return true;

    uint64 compareTo = pos.posStore.hash ^ ZobColor ^ ZobPiece[pos.side][movePiece(m)][moveFrom(m)] ^ ZobPiece[pos.side][movePiece(m)][moveTo(m)];
    pos_store_t* psp = pos.posStore.previous;

    for (int plyForRep = 3, pliesToCheck = MIN(pos.posStore.pliesFromNull, pos.posStore.fifty); psp && plyForRep <= pliesToCheck; plyForRep += 2) {
        if (!psp->previous) return false;
        else if (!psp->previous->previous) return false;
        psp = psp->previous->previous;
        if (psp && psp->hash == compareTo) return true;
    }
    return false;
}