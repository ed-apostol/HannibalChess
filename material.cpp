/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "utils.h"
#include "material.h"

/////SAM ENDGAME TODO NOTES
////1. rook and piece against rook and lots of pawns, is drawish if pawns are winning
////2. rook and 2 pawns vs. rook and pawn drawish when blockaded
////3. rook against knight more drawish than listed

int SetPhase(int minors, int rooks, int queens) {
    //	return minors+rooks*3+queens*6; //090411

    int p = 32;
    int lm = 8 - minors;
    int lr = 4 - rooks;
    int lq = 2 - queens;
    lr += lq;
    lm += lr;

    if (lm) {
        p -= NP_M1*lm;
        if (lm > 2) p -= NP_M2 * (lm - 2);
        if (lm > 4) p -= NP_M3 * (lm - 4);
        if (lm > 6) p -= NP_M4 * (lm - 6);
        if (lm > 8) p -= NP_M5 * (lm - 8);
        if (lm > 10) p -= NP_M6 * (lm - 10);
    }
    if (lr) {
        p -= NP_R1*lr;
        if (lr > 2) p -= NP_R2 * (lr - 2);
    }
    if (lq) {
        p -= NP_Q*lq;
        if (lq > 1) {
            if (lm - 2) {
                p -= NP_QM1*lm;
                if (lm > 4) p -= NP_QM2 * (lm - 4);
                if (lm > 6) p -= NP_QM3 * (lm - 6);
                if (lm > 8) p -= NP_QM4 * (lm - 8);
                if (lm > 10) p -= NP_QM5 * (lm - 10);
            }
            if (lr - 2) {
                p -= NP_QR1*lr;
                if (lr > 4) p -= NP_QR2 * (lr - 4);
            }
        }
    }

    if (p < 0) return 0;
    if (p > 32) return 32;
    else return p;
}

// white is the side trying to win
int Drawish(int wp, int bp, int wn, int bn, int wb, int bb, int wr, int br, int wq, int bq, int originalDrawn) {
    // in order of simplist first
    int wminors = wn + wb;
    int bminors = bn + bb;
    int drawn = 0;
    // now lets deal with no pawns left situations
    if (wp == 0) {
        // 1 or less pieces is drawn
        if (wq == 0 && wr == 0 && wminors <= 1) drawn += DRAWN;
        // 1 rook
        if (wq == 0 && wr == 1 && wminors == 0) {
            if (bq | br) drawn += DRAWN;
            if (bb) drawn += DRAWN1;
            if (bn) drawn += DRAWN1;
        }
        // 2 pieces
        else if (wq == 0 && wr == 0 && wminors == 2) {
            if (wn == 2) {
                if (bp == 0 || bminors || br || bq) drawn += DRAWN;
                else drawn += DRAWN1;
            }
            else if (wb == 2 && bq == 0 && br == 0 && bn == 1) drawn += DRAWN8;
            else if (bq || br || bminors) drawn += DRAWN1;
        }
        // R and N against R (RB vs. R already taken care of
        else if (wq == 0 && wr == 1 && wb == 0 && wn == 1 && wp == 0 && (bq || br)) drawn += (DRAWN + DRAWN1) / 2;
        // queen against 2 rooks
        else if (wq == 1 && wr == 0 && wminors == 0 && br == 2) drawn += DRAWN1;
        // 2 rooks against queen
        else if (wq == 0 && wr == 2 && wminors == 0 && bq == 1) drawn += DRAWN1;
        // queen against rook and minor
        else if (wq == 1 && wr == 0 && wminors == 0 && bq >= 0 && br >= 1 && bminors >= 1) drawn += DRAWN2;
        // rook and minor against queen
        else if (wq == 0 && wr == 1 && wminors == 1 && bq == 1) drawn += DRAWN3;
    }
    else {
        // queen endgames are a little drawish
        if (wq == 1 && wr == 0 && wminors == 0 && bq) {
            drawn += DRAWN9;
            if (wp == 1) drawn += DRAWN9;
        }
        // trade into queen endgames
        else if (wq == 1 && wr == 1 && wminors == 0 && bq && br) {
            drawn += DRAWN10;
            if (wp == 1) drawn += DRAWN10;
        }
        else if (wq == 1 && wr == 0 && wminors == 1 && bq && (br | bminors)) {
            drawn += DRAWN10;
            if (wp == 1) drawn += DRAWN10;
        }

        // rook endgames can also be drawish
        else if (wq == 0 && wr == 1 && wminors == 0 && br) {
            drawn += DRAWN9;
            if (wp == 1) drawn += DRAWN9;
            else if (wp == 2 && bp >= 1) drawn += DRAWN10;
            else if (wp == 3 && bp >= 2) drawn += DRAWN11;
        }

        // trade into rook endgame
        else if (wq == 0 && wr == 1 && wminors == 1 && br && (br == 2 || bminors)) {
            drawn += DRAWN10;
            if (wp == 1) drawn += DRAWN10;
        }
        else if (wq == 0 && wr == 2 && wminors == 0 && br == 2) {
            drawn += DRAWN10;
            if (wp == 1) drawn += DRAWN10;
        }
    }
    // lets imagine some possible trades
    // AFTER SIGH4
    // piece trades (no point if pawns > 1 since it can always be done late)

    // consider increasing penalty

    if (wp == 0) { //increase to 1 if 1 has special cases
        if (bq) {
            if (wq) {
                int pGone = Drawish(wp, bp, wn, bn, wb, bb, wr, br, wq - 1, bq - 1, 0) - 5;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wr) {
                int pGone = Drawish(wp, bp, wn, bn, wb, bb, wr - 1, br, wq, bq - 1, 0) - 5;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wb) {
                int pGone = Drawish(wp, bp, wn, bn, wb - 1, bb, wr, br, wq, bq - 1, 0) - 5;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wn) {
                int pGone = Drawish(wp, bp, wn - 1, bn, wb, bb, wr, br, wq, bq - 1, 0) - 5;
                if (pGone > drawn) drawn = pGone;
            }
        }
        if (br) {
            if (wr) {
                int pGone;
                if (wr > 1 || br > 1) pGone = Drawish(wp, bp, wn, bn, wb, bb, wr - 1, br - 1, wq, bq, 0) - 5;
                else pGone = Drawish(wp, bp, wn, bn, wb, bb, wr - 1, br - 1, wq, bq, 0) - 8;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wb) {
                int pGone = Drawish(wp, bp, wn, bn, wb - 1, bb, wr, br - 1, wq, bq, 0) - 10;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wn) {
                int pGone = Drawish(wp, bp, wn - 1, bn, wb, bb, wr, br - 1, wq, bq, 0) - 10;
                if (pGone > drawn) drawn = pGone;
            }
        }
        if (bb) {
            if (wb) {
                int pGone;
                if (wminors > 1 || bminors > 1) {
                    pGone = Drawish(wp, bp, wn, bn, wb - 1, bb - 1, wr, br, wq, bq, 0) - 8;
                }
                else pGone = Drawish(wp, bp, wn, bn, wb - 1, bb - 1, wr, br, wq, bq, 0) - 20;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wn) {
                int pGone;
                if (wminors > 1 || bminors > 1) {
                    pGone = Drawish(wp, bp, wn - 1, bn, wb, bb - 1, wr, br, wq, bq, 0) - 8;
                }
                else pGone = Drawish(wp, bp, wn - 1, bn, wb, bb - 1, wr, br, wq, bq, 0) - 20;
                if (pGone > drawn) drawn = pGone;
            }
        }
        if (bn) {
            if (wb) {
                int pGone;
                if (wminors > 1 || bminors > 1) {
                    pGone = Drawish(wp, bp, wn, bn - 1, wb - 1, bb, wr, br, wq, bq, 0) - 8;
                }
                else
                    pGone = Drawish(wp, bp, wn, bn - 1, wb - 1, bb, wr, br, wq, bq, 0) - 20;
                if (pGone > drawn) drawn = pGone;
            }
            else if (wn) {
                int pGone;
                if (wminors > 1 || bminors > 1) {
                    pGone = Drawish(wp, bp, wn - 1, bn - 1, wb, bb, wr, br, wq, bq, 0) - 8;
                }
                else

                    pGone = Drawish(wp, bp, wn - 1, bn - 1, wb, bb, wr, br, wq, bq, 0) - 20;
                if (pGone > drawn) drawn = pGone;
            }
        }
    }
    if (wp == 1) { // only saq for pawn if we can get to a special case
        //assuming we can saq a knight for a pawn and its a draw, we should assume its drawn
        if (bn) {
            int pGone = Drawish(wp - 1, bp, wn, bn - 1, wb, bb, wr, br, wq, bq, 0) - 20;
            if (pGone > drawn) drawn = pGone;
        }
        //assuming we can saq a bishop for a pawn and its a draw, we should assume its drawn
        else if (bb) {
            int pGone = Drawish(wp - 1, bp, wn, bn, wb, bb - 1, wr, br, wq, bq, 0) - 20;
            if (pGone > drawn) drawn = pGone;
        }
        //assuming we can saq a rook for a pawn and its a draw, we should assume its drawn
        if (br) {
            int pGone = Drawish(wp - 1, bp, wn, bn, wb, bb, wr, br - 1, wq, bq, 0) - 5;
            if (pGone > drawn) drawn = pGone;
        }
        //assuming we can saq a queen for a pawn and its a draw, we should assume its drawn
        if (bq) {
            int pGone = Drawish(wp - 1, bp, wn, bn, wb, bb, wr, br, wq, bq - 1, 0); //added
            if (pGone > drawn) drawn = pGone;
        }
    }
    // allow a pawn trade force to get to special case
    if (bp && (wp == 2 || wp == 3 || wp == 4)) {
        int pGone;
        pGone = Drawish(wp - 1, bp - 1, wn, bn, wb, bb, wr, br, wq, bq, 0) / 2;
        if (bp < wp) pGone -= 10;
        if (pGone > drawn) drawn = pGone;
    }

    return originalDrawn + drawn;
}

void InitMaterial(void) {
    int win, openscore, midscore1, midscore2, endscore, windex, bindex, phase;
    int bp, wp, bn, wn, bb, wb, br, wr, bq, wq;
    int wdraw, bdraw;

    for (wq = 0; wq <= 1; wq++)
        for (bq = 0; bq <= 1; bq++)
            for (wr = 0; wr <= 2; wr++)
                for (br = 0; br <= 2; br++)
                    for (wb = 0; wb <= 2; wb++)
                        for (bb = 0; bb <= 2; bb++)
                            for (wn = 0; wn <= 2; wn++)
                                for (bn = 0; bn <= 2; bn++)
                                    for (wp = 0; wp <= 8; wp++)
                                        for (bp = 0; bp <= 8; bp++) {
                                            int wminors = wn + wb;
                                            int bminors = bn + bb;
                                            int wnonQ = wminors + wr;
                                            int bnonQ = bminors + br;

                                            mflag_t wflag = 0;
                                            mflag_t bflag = 0;

                                            windex = wp * MatSummValue[PAWN] + wn * MatSummValue[KNIGHT] + wb * MatSummValue[BISHOP] +
                                                wr * MatSummValue[ROOK] + wq * MatSummValue[QUEEN];
                                            bindex = bp * MatSummValue[PAWN] + bn * MatSummValue[KNIGHT] + bb * MatSummValue[BISHOP] +
                                                br * MatSummValue[ROOK] + bq * MatSummValue[QUEEN];
                                            if (wp*MLP + wn*MLN + wb*MLN + wr*MLR + wq*MLQ >
                                                bp + bn*MLN + bb*MLN + br*MLR + bq*MLQ ||
                                                (wp + wn*MLN + wb*MLN + wr*MLR + wq*MLQ ==
                                                wp + wn*MLN + wb*MLN + wr*MLR + wq*MLQ && windex >= bindex)) {
                                                phase = SetPhase(wn + wb + bn + bb, wr + br, wq + bq);
//                                                if (wq && bq) PrintOutput() << wn + wb + bn + bb << " minors " << wr + br << " rooks " << wq + bq << " queens: " << phase << "\n";
                                                openscore =
                                                    ((wq - bq) * QueenValueOpen)
                                                    + ((wr - br) * RookValueOpen)
                                                    + ((wb - bb) * BishopValueOpen)
                                                    + ((wn - bn) * KnightValueOpen)
                                                    + ((wp - bp) * PawnValueOpen)
                                                    + ((wb >= 2) * BishopPairBonusOpen)
                                                    - ((bb >= 2) * BishopPairBonusOpen)
                                                    + ((wp - 5) * wn * 0) - ((bp - 5) * bn * 0)
                                                    - ((wp - 5) * wr * 5) + ((bp - 5) * br * 5)
                                                    //            - ((wr==2) * (16-3)) + ((br==2) * (16-3))
                                                    //            - ((wq+wr>2) * 8) + ((bq+br>2) * 8)
                                                    ;

                                                midscore1 =
                                                    ((wq - bq) * QueenValueMid1)
                                                    + ((wr - br) * RookValueMid1)
                                                    + ((wb - bb) * BishopValueMid1)
                                                    + ((wn - bn) * KnightValueMid1)
                                                    + ((wp - bp) * PawnValueMid1)
                                                    + ((wb >= 2) * BishopPairBonusMid1)
                                                    - ((bb >= 2) * BishopPairBonusMid1)
                                                    + ((wp - 5) * wn * 2) - ((bp - 5) * bn * 2)
                                                    - ((wp - 5) * wr * 4) + ((bp - 5) * br * 4)
                                                    //            - ((wr==2) * (20-6)) + ((br==2) * (20-6))
                                                    //            - ((wq+wr>2) * 10) + ((bq+br>2) * 10)
                                                    ;

                                                midscore2 =
                                                    ((wq - bq) * QueenValueMid2)
                                                    + ((wr - br) * RookValueMid2)
                                                    + ((wb - bb) * BishopValueMid2)
                                                    + ((wn - bn) * KnightValueMid2)
                                                    + ((wp - bp) * (PawnValueMid2

                                                    )) + ((wb >= 2) * BishopPairBonusMid2)
                                                    - ((bb >= 2) * BishopPairBonusMid2)
                                                    + ((wp - 5) * wn * 4) - ((bp - 5) * bn * 4)
                                                    - ((wp - 5) * wr * 2) + ((bp - 5) * br * 2)
                                                    - ((wr == 2) * (32 - 10)) + ((br == 2) * (32 - 10))
                                                    - ((wq + wr > 2) * 32) + ((bq + br > 2) * 32)
                                                    ;

                                                endscore =
                                                    ((wq - bq) * QueenValueEnd)
                                                    + ((wr - br) * RookValueEnd)
                                                    + ((wb - bb) * BishopValueEnd)
                                                    + ((wn - bn) * KnightValueEnd)
                                                    + ((wp - bp) * (PawnValueEnd

                                                    )) + ((wb >= 2) * BishopPairBonusEnd)
                                                    - ((bb >= 2) * BishopPairBonusEnd)
                                                    + ((wp - 5) * wn * 5) - ((bp - 5) * bn * 5)
                                                    - ((wp - 5) * wr * 0) + ((bp - 5) * br * 0)
                                                    - ((wr == 2) * (48 - 12)) + ((br == 2) * (48 - 12))
                                                    - ((wq + wr > 2) * 48) + ((bq + br > 2) * 48)
                                                    ;

                                                // SAM ADJUSTMENTS
                                                // it is good to have a rook when your opponent does not
                                                // though 2 bishops counteracts this a bit
                                                /* SM (simple material edit this out */
                                                if (br && !wr && !wq && wb < 2) {
                                                    openscore -= 3;
                                                    midscore1 -= 6;
                                                    midscore2 -= 10;
                                                    endscore -= 12;
                                                }
                                                if (wr && !br && !bq && bb < 2) {
                                                    openscore += 3;
                                                    midscore1 += 6;
                                                    midscore2 += 10;
                                                    endscore += 12;
                                                }
                                                //2 pieces work well against a rook even in endgame if one is a bishop
                                                if (wr == br + 1 && bq >= wq && wminors <= bminors - 2 && bb) {
                                                    midscore2 -= 20;
                                                    endscore -= 20;
                                                }
                                                if (br == wr + 1 && wq >= bq && bminors <= wminors - 2 && wb) {
                                                    midscore2 += 20;
                                                    endscore += 20;
                                                }
                                                //N2BN 2 bishops works particularly well against 2 knights
                                                if (wb == 2 && wn == 0 && bb == 0 && bn == 2 && wr == br && wq == bq) {
                                                    openscore += 10;
                                                    midscore1 += 10;
                                                    midscore2 += 10;
                                                    endscore += 10;
                                                }
                                                if (wb == 0 && wn == 2 && bb == 2 && bn == 0 && wr == br && wq == bq) {
                                                    openscore -= 10;
                                                    midscore1 -= 10;
                                                    midscore2 -= 10;
                                                    endscore -= 10;
                                                }

                                                // 3 pieces are great against a queen because it can grab pawns and escort own pawn to victory
                                                if (wq == 1 && bq == 0 && bnonQ >= wnonQ + 3 && br >= wr && wp && bp) {
                                                    if (br > wr) {
                                                        openscore -= 25;
                                                        midscore1 -= 25;
                                                        midscore2 -= 25;
                                                        endscore -= 25;
                                                    }
                                                    else {
                                                        openscore -= 19;
                                                        midscore1 -= 16;
                                                        midscore2 -= 13;
                                                        endscore -= 10;
                                                    }
                                                }

                                                // a single rook can often wipe up non-advanced pawns
                                                // if advanced this is countered by high passed pawn scores
                                                if ((wr || wq) && bq == 0 && bnonQ == 0) {
                                                    openscore += 50;
                                                    midscore1 += 50;
                                                    midscore2 += 50;
                                                    endscore += 50;
                                                }
                                                //8_11_10 moved
                                                // its really nice to be up a piece or a rook
                                                if (wminors >= bminors && wq >= bq && wr > br) {
                                                    openscore += 20;
                                                    midscore1 += 20;
                                                    midscore2 += 20;
                                                    endscore += 20;
                                                }
                                                if (wminors > bminors && wq >= bq && wr >= br) {
                                                    openscore += 20;
                                                    midscore1 += 15;
                                                    midscore2 += 10;
                                                    endscore += 5;
                                                }
                                                if (phase < 8) {
                                                    endscore *= 8 - phase;
                                                    midscore2 *= phase;
                                                    win = (midscore2 + endscore) / 8;
                                                }
                                                else if (phase < 24) {
                                                    midscore2 *= 24 - phase;
                                                    midscore1 *= phase - 8;
                                                    win = (midscore1 + midscore2) / 16;
                                                }
                                                else {
                                                    midscore1 *= 32 - phase;
                                                    openscore *= phase - 24;
                                                    win = (openscore + midscore1) / 8;
                                                }
                                                // now adjust draw

                                                wdraw = 0;
                                                bdraw = 0;

                                                // material less important in queen or rook endgames
                                                if (wq == 1 && wr == 0 && wminors == 0 && bq == 1 && br == 0 && bminors == 0) win = (win * 9) / 10; //Q
                                                if (wq == 0 && wr == 1 && wminors == 0 && bq == 0 && br == 1 && bminors == 0) win = (win * 9) / 10; //R
                                                // if you do not have winning material, devalue your material

                                                // reduce piece value if we are winning but mating is tough or impossible
                                                if (wq == 0 && wr == 0 && wminors == 1 && wp == 0 && bq == 0 && br == 0 && bminors == 0) win -= KnightValueEnd / 2; //only 1 piece
                                                if (wq == 0 && wr == 0 && wb == 0 && wn == 2 && wp == 0 && bq == 0 && br == 0 && win > 0) win = MAX(0, (win - KnightValueEnd) / 2); // 2 knights
                                                if (wp == 0 && win > 0 && ((wq - bq) * 9 + (wr - br) * 5 + (wminors - bminors) * 3) <= 3) win /= 2; //max of a piece up
                                                if (bp == 0 && win < 0 && ((bq - wq) * 9 + (br - wr) * 5 + (bminors - wminors) * 3) <= 3) win /= 2; //max of a piece up

                                                // reverse order, least material first
                                                wdraw = Drawish(wp, bp, wn, bn, wb, bb, wr, br, wq, bq, wdraw);
                                                bdraw = Drawish(bp, wp, bn, wn, bb, wb, br, wr, bq, wq, bdraw);

                                                if (wdraw < 0) wdraw = 0;
                                                if (wdraw > MAX_DRAW) wdraw = MAX_DRAW;
                                                if (bdraw < 0) bdraw = 0;
                                                if (bdraw > MAX_DRAW) bdraw = MAX_DRAW;

                                                if (wdraw == MAX_DRAW && bdraw < MAX_DRAW && win > -10) win = -10;
                                                if (bdraw == MAX_DRAW && wdraw < MAX_DRAW && win < 10) win = 10;

                                                MaterialTable[windex][bindex].value = win; //SCALE CHANGES
                                                MaterialTable[windex][bindex].phase = phase;
                                                MaterialTable[windex][bindex].draw[WHITE] = wdraw;
                                                MaterialTable[windex][bindex].draw[BLACK] = bdraw;
                                                // reverse things to make sure everything is nice and symmetrical
                                                MaterialTable[bindex][windex].value = -win;
                                                MaterialTable[bindex][windex].phase = phase;
                                                MaterialTable[bindex][windex].draw[WHITE] = bdraw;
                                                MaterialTable[bindex][windex].draw[BLACK] = wdraw;

                                                //FLAGS are set from the attackers perspective (so the other side is trying to draw)
                                                if (windex == MLP) wflag = 1;
                                                if (bindex == MLP) bflag = 1;
                                                bool wlock = (wp >= 4 && bp >= 3);
                                                bool block = (bp >= 4 && wp >= 3);

                                                if (wr == 1 && wq == 0 && wminors == 0 && wp && br == 1 && bq == 0 && bminors == 0) {
                                                    wflag = 2;
                                                }
                                                if (wr == 1 && wq == 0 && wminors == 0 && bp && br == 1 && bq == 0 && bminors == 0) {
                                                    bflag = 2;
                                                }
                                                if (wb == 1 && bb == 1 && wr == 0 && br == 0 && wq == 0 && bq == 0 && wn == 0 && bn == 0) {
                                                    wflag = wlock ? 19 : 3;
                                                    bflag = block ? 19 : 3;
                                                    wflag = bflag = 3;
                                                }
                                                if (wr == 1 && wq == 0 && wminors == 0 && wp == 0 && bq == 0 && bp && bnonQ == 0) {
                                                    wflag = 4;
                                                }

                                                if (br == 1 && bq == 0 && bminors == 0 && bp == 0 && wq == 0 && wp && wnonQ == 0) {
                                                    bflag = 4;
                                                }
                                                if (wp == 1 && wn == 1 && wb == 0 && wr == 0 && wq == 0) {
                                                    wflag = 5;
                                                }
                                                if (bp == 1 && bn == 1 && bb == 0 && br == 0 && bq == 0) {
                                                    bflag = 5;
                                                }
                                                if (wq == 1 && wnonQ == 0 && wp == 0 && br == 1 && bq == 0 && bminors == 0 && bp) {
                                                    wflag = 6;
                                                }
                                                if (bq == 1 && bnonQ == 0 && bp == 0 && wr == 1 && wq == 0 && wminors == 0 && wp) {
                                                    bflag = 6;
                                                }
                                                if (windex == MLN + MLB && bindex == 0) wflag = 7;
                                                if (wq == 0 && bq == 0 && wn == 0 && bn == 0 && wr == br && wb == 1 && bb == 1 && wr) {
                                                    wflag = 8;
                                                    bflag = 8;
                                                }
                                                if (wflag == 0 && wp && wq == 0) {
                                                    if (wnonQ == 1 && wnonQ == wb) { //pawn endgames and bishop endgames
                                                        wflag = 9;
                                                    }
                                                    if (wnonQ == 0 && wp > 1) { //pawn endgames (not single pawn)
                                                        wflag = wlock ? 18 : 10;
                                                        wflag = 10;
                                                    }
                                                }
                                                if (bflag == 0 && bp && bq == 0) {
                                                    if (bnonQ == 1 && bnonQ == bb) {
                                                        bflag = 9;
                                                    }
                                                    if (bnonQ == 0 && bp > 1) {
                                                        bflag = block ? 18 : 10;
                                                        bflag = 10;
                                                    }
                                                }
                                                if (wq == 0 && wminors == 0 && wr == 1 && bq == 0 && br == 0 && bb == 0 && bn == 1 && wp == 0 && bp == 0) {
                                                    wflag = 11;
                                                }
                                                if (wq == 1 && bq == 0 && wnonQ == 0 && bnonQ == 0 && wp == 0 && bp == 1) {
                                                    wflag = 12;
                                                }
                                                if (wr == 1 && wq == 0 && wp == 1 && wminors == 0 && bb == 1 && bn == 0 && bq == 0 && br == 0 && bp == 0) {
                                                    wflag = 14;
                                                }
                                                if (wq == 0 && wr == 0 && wb == 1 && wn == 0 && wp == 1 && bq == 0 && br == 0 && bb == 0 && bn == 1) {
                                                    wflag = 15;
                                                }
                                                if (wq == 0 && wr == 0 && wb == 0 && wn == 1 && wp == 1 && bq == 0 && br == 0 && bb == 0 && bn == 1) {
                                                    wflag = 16;
                                                }
                                                if (bq == 0 && br == 0 && bb == 0 && bn == 1 && bp == 1 && wq == 0 && wr == 0 && wb == 1 && wn == 0) {
                                                    bflag = 16;
                                                }
                                                if (wq == 0 && wr == 0 && wb == 1 && wn == 0 && wp == 2 && bq == 0 && br == 0 && bb == 0 && bn == 1 && bp== 1) {
                                                    wflag = 17;
                                                }
                                                if (wq == 0 && wr == 0 && wb == 0 && wn == 1 && wp == 2 && bq == 0 && br == 0 && bb == 0 && bn == 1 && bp == 1) {
                                                    wflag = 17;
                                                }
                                                if (bq == 0 && br == 0 && bb == 0 && bn == 1 && bp == 2 && wq == 0 && wr == 0 && wb == 1 && wn == 0 && wp == 1) {
                                                    bflag = 17;
                                                }
                                                if (wq == 0 && wr == 0 && wb == 1 && wn == 0 && (bq+ br + bn) >= 1 && wlock) {
                                                    wflag = 20; //attacker has bishop and lots of pawns, defender has non queen and lots of pawns
                                                }
                                                if (wq == 0 && wr == 0 && wb == 0 && wn == 1 && (bq + br + bb + bn) >= 1 && wlock) {
                                                    wflag = 20; //attacker has knight and lots of pawns, defender has non queen and lots of pawns
                                                }
                                                if (bq == 0 && br == 0 && bb == 1 && bn == 0 && (wq + wr + wn) >= 1 && block) {
                                                    bflag = 20; //attacker has bishop and lots of pawns, defender has non queen and lots of pawns
                                                }
                                                if (bq == 0 && br == 0 && bb == 0 && bn == 1 && (wq + wr + wb + wn) >= 1 && block) {
                                                    bflag = 20; //attacker has knight and lots of pawns, defender has non queen and lots of pawns
                                                }
                                                //TODO consider expanding to include more pieces (rook, queen, more minors, etc.
                                                if (wflag == 0 && win > 40 && wp == 0 && wdraw < MAX_DRAW) {
                                                    wflag = 13; // try to mate with no pawns (do not supercede other things like NB v. King)
                                                }
                                                if (bflag == 0 && win < -40 && bp == 0 && bdraw < MAX_DRAW) {
                                                    bflag = 13; // try to mate with no pawns (do not supercede other things like NB v. King)
                                                }
                                                MaterialTable[windex][bindex].flags[WHITE] = wflag;
                                                MaterialTable[windex][bindex].flags[BLACK] = bflag;
                                                MaterialTable[bindex][windex].flags[WHITE] = bflag;
                                                MaterialTable[bindex][windex].flags[BLACK] = wflag;
                                            }
                                        }
}