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

EvalScore pawnSQ(int sq) {
	EvalScore central[8] = { COMP(2, 0), COMP(1, 0), COMP(0, 0), COMP(-1, 0), COMP(-1, 0), COMP(-1, 0), COMP(-1, 0), COMP(-1, 0) };
	EvalScore file[8] = { COMP(-16, -4), COMP(-7, -5), COMP(-1, -7), COMP(5, -8), COMP(5, -8), COMP(-1, -7), COMP(-7, -5), COMP(-16, -4) };
	EvalScore rank[8] = { COMP(0, 0), COMP(-3, -2), COMP(-2, -3), COMP(0, 1), COMP(1, 4), COMP(2, 7), COMP(0, 1), COMP(0, 0) };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}

EvalScore knightSQ(int sq) {
	EvalScore central[8] = { COMP(0, 3), COMP(0, 2), COMP(0, 1), COMP(0, 0), COMP(0, -2), COMP(0, -4), COMP(0, -6), COMP(0, -8) };
	EvalScore file[8] = { COMP(-26, -10), COMP(-9, -3), COMP(2, 0), COMP(5, 2), COMP(5, 2), COMP(2, 0), COMP(-9, -3), COMP(-26, -10) };
	EvalScore rank[8] = { COMP(-30, -10), COMP(-9, -4), COMP(6, -1), COMP(16, 2), COMP(20, 4), COMP(19, 6), COMP(11, 3), COMP(-11, -5) };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
	return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}
EvalScore bishopSQ(int sq) {
	EvalScore central[8] = { COMP(11, 5), COMP(6, 3), COMP(2, 1), COMP(-2, 1), COMP(-4, -2), COMP(-6, -2), COMP(-7, -3), COMP(-11, -5) };
	EvalScore rank[8] = { COMP(-7, 0), COMP(0, 0), COMP(0, 0), COMP(0, 0), COMP(0, 0), COMP(0, 0), COMP(0,0 ), COMP(-1, 0) };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + rank[r]);
}
EvalScore rookSQ(int sq) {
	EvalScore file[8] = { COMP(-2, 0), COMP(1, 0), COMP(4, 0), COMP(7, 0), COMP(7, 0), COMP(4, 0), COMP(1, 0), COMP(-2, 0) };
    int f = SQFILE(sq);
    return (file[f]);
}
EvalScore queenSQ(int sq) {
	EvalScore central[8] = { COMP(4, 3), COMP(2, 2), COMP(0, 1), COMP(-1, -1), COMP(-2, -3), COMP(-4, -5), COMP(-6, -7), COMP(-9, -11) };
	EvalScore file[8] = { COMP(-3, -3), COMP(0, 0), COMP(1, 1), COMP(3, 3), COMP(3, 3), COMP(1, 1), COMP(0, 0), COMP(-3, -3) };
	EvalScore rank[8] = { COMP(-7, -3), COMP(0, 0), COMP(0, 1), COMP(1, 3), COMP(1, 3), COMP(0, 1), COMP(0, 0), COMP(-1, -3) };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
    return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
} 
EvalScore kingSQ(int sq) {
	EvalScore central[8] = { COMP(0, 0), COMP(0, -2), COMP(0, -4), COMP(0, -7), COMP(0, -10), COMP(0, -14), COMP(0, -23), COMP(0, -32) };
	EvalScore file[8] = { COMP(26, -13), COMP(30, 1), COMP(0, 11), COMP(-20, 16), COMP(-20, 16), COMP(0, 11), COMP(30, 1), COMP(26, -13) };
	EvalScore rank[8] = { COMP(3, -29), COMP(0, -4), COMP(-5, 1), COMP(-9, 6), COMP(-13, 10), COMP(-17, 6), COMP(-17, 1), COMP(-17, -10) };
    int f = SQFILE(sq);
    int r = SQRANK(sq);
	return (central[abs(f - r)] + central[abs(f + r - 7)] + file[f] + rank[r]);
}

void initPST() {
    int i, j, k;

    for (i = 0; i < 64; i++) {
		PST(WHITE, PAWN, i) = pawnSQ(i);
		PST(WHITE, KNIGHT, i) = knightSQ(i);
		PST(WHITE, BISHOP, i) = bishopSQ(i);
		PST(WHITE, ROOK, i) = rookSQ(i);
		PST(WHITE, QUEEN, i) = queenSQ(i);
		PST(WHITE, KING, i) = kingSQ(i);
    }

	for (i = PAWN; i <= KING; i++) {
        for (j = 0; j < 64; j++) {
            k = ((7 - SQRANK(j)) * 8) + SQFILE(j);
            PST(BLACK, i, k) = PST(WHITE, i, j);
        }
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