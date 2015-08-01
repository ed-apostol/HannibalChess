/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include <cstring>
#include <cmath>
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "init.h"
#include "bitutils.h"

int mpawn(int sq) {
    int central[8] = { 2, 1, 0, -1, -1, -1, -1, -1 };
    int file[8] = { -16, -7, -1, 5, 5, -1, -7, -16 };
    int rank[8] = { 0, -3, -2, 0, 1, 2, 0, 0 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}
int epawn(int sq) {
    int file[8] = { -4, -5, -7, -8, -8, -7, -5, -4 };
    int rank[8] = { 0, -2, -3, 1, 4, 7, 1, 0 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (file[f] + rank[r]);
}

int outpost(int sq) {
    int central[8] = { 7, 6, 5, 4, 3, 2, 1, 0 };
    int file[8] = { -20, -8, -2, 0, 0, -2, -8, -20 };
    int rank[8] = { -20, -20, -5, 1, 5, 1, -5, -10 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    int value = (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
    if (value < 0) value = 0;
    return value;
}
int mknight(int sq) {
    //    int central[8] = {2,1,0,-1,-1,-1,-1,-1};
    int file[8] = { -26, -9, 2, 5, 5, 2, -9, -26 };
    int rank[8] = { -30, -9, 6, 16, 20, 19, 11, -11 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (file[f] + rank[r]);
    //    return (central[abs(f-r)] + central[abs(f+r-7)] + file[f] + rank[r]); //CEN
}
int eknight(int sq) {
    int central[8] = { 3, 2, 1, 0, -2, -4, -6, -8 };
    int file[8] = { -10, -3, 0, 2, 2, 0, -3, -10 };
    int rank[8] = { -10, -4, -1, 2, 4, 6, 3, -5 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}
int mbishop(int sq) {
    int central[8] = { 11, 6, 2, -2, -4, -6, -7, -11 };
    int rank[8] = { -7, 0, 0, 0, 0, 0, 0, -1 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + rank[r]);
}

int ebishop(int sq) {
    int central[8] = { 5, 3, 1, 1, -2, -2, -3, -5 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)]);
}
int mrook(int sq) {
    int file[8] = { -2, 1, 4, 7, 7, 4, 1, -2 };
    int f = SQFILE(sq);
    return (file[f]);
}
int mqueen(int sq) {
    int central[8] = { 4, 2, 0, -1, -2, -4, -6, -9 };
    int file[8] = { -3, 0, 1, 3, 3, 1, 0, -3 };
    int rank[8] = { -7, 0, 0, 1, 1, 0, 0, -1 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}
int equeen(int sq) {
    int central[8] = { 3, 2, 1, -1, -3, -5, -7, -11 };
    int file[8] = { -3, 0, 1, 3, 3, 1, 0, -3 };
    int rank[8] = { -3, 0, 1, 3, 3, 1, 0, -3 };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}

#define MKF1 04 //5
#define MKF2 30 //30
#define MKF3 20 //20

#define MKR1 3 //3
#define MKR2 5 //5
#define MKR3 4 //4
#define MKR4 4 //4
#define MKR5 4 //4
int mking(int sq) {
    int file[8] = { MKF2 - MKF1, MKF2, 0, -MKF3, -MKF3, 0, MKF2, MKF2 - MKF1 };
    int rank[8] = { MKR1, 0, -MKR2, -MKR2 - MKR3, -MKR2 - MKR3 - MKR4, -MKR2 - MKR3 - MKR4 - MKR5, -MKR2 - MKR3 - MKR4 - MKR5, -MKR2 - MKR3 - MKR4 - MKR5 };

    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (file[f] + rank[r]);
}

int eking(int sq) {
    int central[8] = { 0, -2, -4, -7, -10, -14, -23, -32 };
    int file[8] = { -13, 1, 11, 16, 16, 11, 1, -13 };
    int rank[8] = { -29, -4, 1, 6, 10, 6, 1, -10 };

    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}

void initPST() {
    const int KingFileMul[8] = {
        +3, +4, +2, +0, +0, +2, +4, +3,
    };
    const int KingRankMul[8] = {
        +1, +0, -2, -3, -4, -5, -6, -7,
    };

    int i, j, k;

    memset(PcSqTb, 0, sizeof(PcSqTb));
    for (i = 0; i < 64; i++) {
        // do pawns
        PST(WHITE, PAWN, i, MIDGAME) = mpawn(i);
        PST(WHITE, PAWN, i, ENDGAME) = epawn(i);
        // do knights
        PST(WHITE, KNIGHT, i, MIDGAME) = mknight(i);
        PST(WHITE, KNIGHT, i, ENDGAME) = eknight(i);
        // do bishops
        PST(WHITE, BISHOP, i, MIDGAME) = mbishop(i);
        PST(WHITE, BISHOP, i, ENDGAME) = ebishop(i);
        // do rooks
        PST(WHITE, ROOK, i, MIDGAME) = mrook(i); //no endgame values for rook
        // do queens
        PST(WHITE, QUEEN, i, MIDGAME) = mqueen(i);
        PST(WHITE, QUEEN, i, ENDGAME) = equeen(i);
        // do king
        PST(WHITE, KING, i, MIDGAME) = mking(i);
        PST(WHITE, KING, i, ENDGAME) = eking(i);

        //do outposts
        int oScore = outpost(i);
        OutpostValue[WHITE][i] = oScore;
        OutpostValue[BLACK][((7 - SQRANK(i)) * 8) + SQFILE(i)] = oScore;
    }

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 64; j++) {
            k = ((7 - SQRANK(j)) * 8) + SQFILE(j);
            PST(BLACK, i, k, MIDGAME) = PST(WHITE, i, j, MIDGAME);
            PST(BLACK, i, k, ENDGAME) = PST(WHITE, i, j, ENDGAME);
        }
    }
    for (j = 0; j < 64; j++) {
        k = ((7 - SQRANK(j)) * 8) + SQFILE(j);
        KingPosPenalty[WHITE][j] = (4 - KingFileMul[SQFILE(j)]) + (1 - KingRankMul[SQRANK(j)]);
        KingPosPenalty[BLACK][k] = KingPosPenalty[WHITE][j];
    }
}

void InitKingShelter() {
    // lets do king shelter and indirect shelter
    int i, j;
    int ri, fi, rj, fj;
    for (i = 0; i < 64; i++) {
        kingShelter[WHITE][i] = 0;
        kingIndirectShelter[WHITE][i] = 0;
        kingShelter[BLACK][i] = 0;
        kingIndirectShelter[BLACK][i] = 0;
        ri = i / 8;
        fi = i % 8;
        for (j = 0; j < 64; j++) {
            rj = j / 8;
            fj = j % 8;
            // WHITE
            if (ri <= rj) {
                //                if (abs(ri-rj) <= 1 && abs(fi - fj) <= 1)
                if (abs(ri - rj) <= 1 && abs(fi - fj) <= 1 && rj == Rank2) //only great protection on 2nd rate...example pawn on g2 protects Kg2/Kg1 equally
                {
                    kingShelter[WHITE][i] |= BitMask[j];
                }
                //               if (abs(ri-rj) <= 2 && abs(fi - fj) <= 1)
                if (abs(ri - rj) <= 2 && abs(fi - fj) <= 1 && (rj <= Rank3 || abs(ri - rj) <= 1)) {
                    kingIndirectShelter[WHITE][i] |= BitMask[j];
                }
            }
            // BLACK
            if (ri >= rj) {
                //                if (abs(ri-rj) <= 1 && abs(fi - fj) <= 1)
                if (abs(ri - rj) <= 1 && abs(fi - fj) <= 1 && rj == Rank7) {
                    kingShelter[BLACK][i] |= BitMask[j];
                }
                //                if (abs(ri-rj) <= 2 && abs(fi - fj) <= 1)
                if (abs(ri - rj) <= 2 && abs(fi - fj) <= 1 && (rj >= Rank6 || abs(ri - rj) <= 1)) {
                    kingIndirectShelter[BLACK][i] |= BitMask[j];
                }
            }
        }
    }

    // deal with exceptions for white
    for (i = 0; i < 7; i++) {
        // king on A file protected by pawn on C file
        kingIndirectShelter[WHITE][i * 8 + a1] |= BitMask[i * 8 + 8 + c1 - a1];
        kingIndirectShelter[WHITE][i * 8 + a1] |= BitMask[i * 8 + c1 - a1];
        // king on H file protected by pawn on F file
        kingIndirectShelter[WHITE][i * 8 + h1] |= BitMask[i * 8 + 8 + h1 - f1];
        kingIndirectShelter[WHITE][i * 8 + h1] |= BitMask[i * 8 + h1 - f1];
        // the E and D files do not protect as well
        kingShelter[WHITE][i * 8 + c1] &= ~BitMask[i * 8 + 8 + d1];
        kingShelter[WHITE][i * 8 + d1] &= ~BitMask[i * 8 + 8 + d1];
        kingShelter[WHITE][i * 8 + e1] &= ~BitMask[i * 8 + 8 + d1];
        kingShelter[WHITE][i * 8 + d1] &= ~BitMask[i * 8 + 8 + e1];
        kingShelter[WHITE][i * 8 + e1] &= ~BitMask[i * 8 + 8 + e1];
        kingShelter[WHITE][i * 8 + f1] &= ~BitMask[i * 8 + 8 + e1];
        // don't need to add back in indirect since its already there
    }

    // deal with exceptions for black
    for (i = 1; i < 8; i++) {
        // king on A file protected by pawn on C file
        kingIndirectShelter[BLACK][i * 8 + a1] |= BitMask[i * 8 - 8 + c1 - a1];
        kingIndirectShelter[BLACK][i * 8 + a1] |= BitMask[i * 8 + c1 - a1];
        // king on H file protected by pawn on F file
        kingIndirectShelter[BLACK][i * 8 + h1] |= BitMask[i * 8 - 8 + f1 - h1];
        kingIndirectShelter[BLACK][i * 8 + h1] |= BitMask[i * 8 + f1 - h1];
        // the E and D files do not protect as well
        kingShelter[BLACK][i * 8 + c1] &= ~BitMask[i * 8 - 8 + d1];
        kingShelter[BLACK][i * 8 + d1] &= ~BitMask[i * 8 - 8 + d1];
        kingShelter[BLACK][i * 8 + e1] &= ~BitMask[i * 8 - 8 + d1];
        kingShelter[BLACK][i * 8 + d1] &= ~BitMask[i * 8 - 8 + e1];
        kingShelter[BLACK][i * 8 + e1] &= ~BitMask[i * 8 - 8 + e1];
        kingShelter[BLACK][i * 8 + f1] &= ~BitMask[i * 8 - 8 + e1];
    }
}

uint64 BMagicHash(int i, uint64 occ) {
    uint64 free = 0;
    int j;
    if (SQFILE(i) < 7 && SQRANK(i) < 7) {
        occ |= (FileBB[7] | RankBB[7]);
        for (j = i + 9; ((j & 7) < 7) && ((occ & ((uint64)1 << j)) == 0); j += 9) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    if (SQFILE(i) > 0 && SQRANK(i) < 7) {
        occ |= (FileBB[0] | RankBB[7]);
        for (j = i + 7; ((j & 7) > 0) && ((occ & ((uint64)1 << j)) == 0); j += 7) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    if (SQFILE(i) > 0 && SQRANK(i) > 0) {
        occ |= (FileBB[0] | RankBB[0]);
        for (j = i - 9; ((j & 7) > 0) && ((occ & ((uint64)1 << j)) == 0); j -= 9) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    if (SQFILE(i) < 7 && SQRANK(i) > 0) {
        occ |= (FileBB[7] | RankBB[0]);
        for (j = i - 7; ((j & 7) < 7) && ((occ & ((uint64)1 << j)) == 0); j -= 7) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    return free;
}

uint64 RMagicHash(int i, uint64 occ) {
    uint64 free = 0;
    int j;
    if (SQFILE(i) < 7) {
        occ |= FileBB[7];
        for (j = i + 1; ((j & 7) < 7) && ((occ & ((uint64)1 << j)) == 0); j++) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    if (SQFILE(i) > 0) {
        occ |= FileBB[0];
        for (j = i - 1; ((j & 7) > 0) && ((occ & ((uint64)1 << j)) == 0); j--) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    if (SQRANK(i) < 7) {
        occ |= RankBB[7];
        for (j = i + 8; (j < 56) && ((occ & ((uint64)1 << j)) == 0); j += 8) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    if (SQRANK(i) > 0) {
        occ |= RankBB[0];
        for (j = i - 8; (j > 7) && ((occ & ((uint64)1 << j)) == 0); j -= 8) free |= (uint64)1 << j;
        free |= (uint64)1 << j;
    }
    return free;
}

/* this initializes the pseudo-constant variables used in the program */
void initArr(void) {
    int i, j, m, k, n;
    const int kingd[] = { -9, -1, 7, 8, 9, 1, -7, -8 };
    const int knightd[] = { -17, -10, 6, 15, 17, 10, -6, -15 };
    const int wpawnd[] = { 8 };
    const int bpawnd[] = { -8 };
    const int wpawnc[] = { 7, 9 };
    const int bpawnc[] = { -7, -9 };
    const int wpawn2mov[] = { 16 };
    const int bpawn2mov[] = { -16 };

    int bit_list[16];
    //memset(BMagicMask, 0, sizeof(BMagicMask));
    //memset(RMagicMask, 0, sizeof(RMagicMask));
    memset(MagicAttacks, 0, sizeof(MagicAttacks));
    memset(bit_list, 0, sizeof(bit_list));

    memset(KnightMoves, 0, sizeof(KnightMoves));
    memset(KingMoves, 0, sizeof(KingMoves));
    memset(PawnMoves, 0, sizeof(PawnMoves));
    memset(PawnCaps, 0, sizeof(PawnCaps));
    memset(PawnMoves2, 0, sizeof(PawnMoves2));
    memset(RankMask, 0, sizeof(RankMask));
    memset(FileMask, 0, sizeof(FileMask));
    memset(InBetween, 0, sizeof(InBetween));

    for (i = 0; i < 0x40; i++) CastleMask[i] = 0xF;

    CastleMask[e1] &= ~WCKS;
    CastleMask[h1] &= ~WCKS;
    CastleMask[e1] &= ~WCQS;
    CastleMask[a1] &= ~WCQS;
    CastleMask[e8] &= ~BCKS;
    CastleMask[h8] &= ~BCKS;
    CastleMask[e8] &= ~BCQS;
    CastleMask[a8] &= ~BCQS;

    for (i = 0; i < 0x40; i++) {
        for (j = 0; j < 0x40; j++) {
            if (SQRANK(i) == SQRANK(j)) RankMask[i] |= BitMask[j];
            if (SQFILE(i) == SQFILE(j)) FileMask[i] |= BitMask[j];
        }

        for (j = 0; j < 8; j++) {
            n = i + knightd[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                KnightMoves[i] |= BitMask[n];
        }
        for (j = 0; j < 8; j++) {
            n = i + kingd[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                KingMoves[i] |= BitMask[n];
        }
        for (j = 0; j < 1; j++) {
            n = i + wpawnd[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                PawnMoves[i][WHITE] |= BitMask[n];
        }
        for (j = 0; j < 1; j++) {
            n = i + bpawnd[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                PawnMoves[i][BLACK] |= BitMask[n];
        }
        for (j = 0; j < 2; j++) {
            n = i + wpawnc[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                PawnCaps[i][WHITE] |= BitMask[n];
        }
        for (j = 0; j < 2; j++) {
            n = i + bpawnc[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                PawnCaps[i][BLACK] |= BitMask[n];
        }
        for (j = 0; j < 1; j++) {
            n = i + wpawn2mov[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                PawnMoves2[i][WHITE] |= BitMask[n];
        }
        for (j = 0; j < 1; j++) {
            n = i + bpawn2mov[j];
            if (n < 64 && n >= 0 && abs(((n & 7) - (i & 7))) <= 2)
                PawnMoves2[i][BLACK] |= BitMask[n];
        }
    }

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) DirFromTo[i][j] = NO_DIR;
        for (k = 0; k < 8; k++) {
            DirBitmap[k][i] = 0;
            for (m = -1, j = i;;) {
                n = j + kingd[k];
                if (n < 0 || n > 63 || (j % 8 == 0 && n % 8 == 7)
                    || (j % 8 == 7 && n % 8 == 0))
                    break;
                DirFromTo[i][n] = k;
                DirBitmap[k][i] |= BitMask[n];
                m = j = n;
            }
        }
    }
    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) {
            k = DirFromTo[i][j];
            if (k != NO_DIR) {
                int k2 = DirFromTo[j][i];
                ASSERT(k2 != NO_DIR);
                InBetween[i][j] = DirBitmap[k][i] & DirBitmap[k2][j];
            }
        }
    }
    InitKingShelter();
//    InitMobility();

    for (i = 0; i <= MAX_FUT_MARGIN; i++) {
        for (j = 0; j < 64; j++) {
            FutilityMarginTable[i][j] = ((i*i*FUTILITY_SCALE) + FUTILITY_SCALE) - (((i*i*FUTILITY_SCALE) + FUTILITY_SCALE) * j / FUTILITY_MOVE);
            if (i >= MAX_FUT_MARGIN) FutilityMarginTable[i][j] = INF;
            // LogOutput() << "FutilityMarginTable[%d][%d] = %d\n", i, j, FutilityMarginTable[i][j]);
        }
    }

    for (j = 0; j < 64; j++) {
        for (k = 0; k < 64; k++) {
            m = (int)(PV_REDUCE_MIN + log((double)j) * log((double)k) / PV_REDUCE_SCALE);
            n = (int)(REDUCE_MIN + log((double)j) * log((double)k) / REDUCE_SCALE);
            ReductionTable[0][j][k] = ((m >= 1) ? m : 0);
            ReductionTable[1][j][k] = ((n >= 1) ? n : 0);
            // LogOutput() << "ReductionTable[PV][%d][%d] = %d\n", j, k, ReductionTable[0][j][k]);
            // LogOutput() << "ReductionTable[NonPV][%d][%d] = %d\n", j, k, ReductionTable[1][j][k]);
        }
    }
    for (i = 0; i < 64; i++) {
        int bits = 64 - BShift[i];
        uint64 u;
        for (u = BMagicMask[i], j = 0; u; j++) bit_list[j] = popFirstBit(&u);
        for (j = 0; j < (1 << bits); j++) {
            u = 0;
            for (k = 0; k < bits; k++) {
                if ((j >> k) & 1) u |= BitMask[bit_list[k]];
            }
            MagicAttacks[BOffset[i] + ((BMagic[i] * u) >> BShift[i])] = BMagicHash(i, u);
        }
        bits = 64 - RShift[i];
        for (u = RMagicMask[i], j = 0; u; j++) bit_list[j] = popFirstBit(&u);
        for (j = 0; j < (1 << bits); j++) {
            u = 0;
            for (k = 0; k < bits; k++) {
                if ((j >> k) & 1) u |= BitMask[bit_list[k]];
            }
            MagicAttacks[ROffset[i] + ((RMagic[i] * u) >> RShift[i])] = RMagicHash(i, u);
        }
    }
}