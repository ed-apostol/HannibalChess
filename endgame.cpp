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
#include "utils.h"
#include "bitutils.h"
#include "trans.h"
#include "eval.h"
#include "material.h"

void RookvKnight(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw, int mover) {
    int nsq = GetOnlyBit(p->knights);
    int penalty = DISTANCE(nsq, p->kpos[attacker ^ 1]);
    penalty *= penalty;
    if (showEval) Print(3, "ndp %d", penalty);
    *score += penalty * sign[attacker];
    *draw -= penalty;
}

void SinglePawnEnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw, int mover) {
    int pawnSq = GetOnlyBit(p->pawns & p->color[attacker]);
    int targetSq = pawnSq + PAWN_MOVE_INC(attacker);
    int defender = attacker ^ 1;
    int myDist;
    int defenderDist;
    // a and h pawns are special cases but handled elsewhere
    // pawns on 7th is special case...assume search handles it
    if (PAWN_RANK(pawnSq, attacker) < Rank7) {
        int pastTarget = targetSq + PAWN_MOVE_INC(attacker);
        int supportAdvance = (p->kpos[attacker] == pastTarget) ||
            (KingMoves[p->kpos[attacker]] & BitMask[pastTarget]);
        // is you are 2 in front of pawn black has no draw
        if (IN_FRONT(SQRANK(p->kpos[attacker]), SQRANK(targetSq), attacker) && supportAdvance) {
            if (ei->MLindex[defender] == 0) *score += 400 * sign[attacker];
            if (showEval) Print(3, " wk in front ");
        }
        // if you control the square in front of pawn black must have
        // opposition
        else if (supportAdvance) {
            if (showEval) Print(3, " wk supporting ");
            // if you are on 6th rank you got it
            // relying on rook pawns dealt with first elsewhere
            if (PAWN_RANK(pawnSq, attacker) >= Rank5) {
                if (showEval) Print(3, " rank 6 ");
                if (ei->MLindex[defender] == 0) *score += 400 * sign[attacker];
                else *score += 100 * sign[attacker];//sam3.13 small bug fix
            } else {
                // otherwise if king has opposition he is safe
                int oppSq = p->kpos[attacker] + PAWN_MOVE_INC(attacker) * 2;
                if ((oppSq == p->kpos[defender]) && mover == attacker)
                    *draw = MAX_DRAW;
                else if ((KingMoves[p->kpos[defender]] & BitMask[oppSq])
                    && mover == defender)
                    *draw = MAX_DRAW;
                else if (ei->MLindex[defender] == 0) *score += 400 * sign[attacker];
            }
            return;
        }
        // if black can obviously get to the pawn first he wins
        // remember, cases where white king is in front of pawn
        // are already dealt with
        else {
            // if pawn is on 6th and king supporting
            if (PAWN_RANK(pawnSq, attacker) == Rank6 && (KingMoves[p->kpos[attacker]] & BitMask[targetSq])) {
                int oppSq = p->kpos[attacker] + PAWN_MOVE_INC(attacker) * 2;
                if (p->kpos[defender] == oppSq && mover == attacker) {
                    *draw = MAX_DRAW;
                    return;
                }
                if ((KingMoves[p->kpos[defender]] & BitMask[oppSq]) && mover == defender) {
                    *draw = MAX_DRAW;
                    return;
                }
                if (ei->MLindex[defender] == 0) *score += 400 * sign[attacker];
                return;
            }
            myDist = DISTANCE(p->kpos[attacker], targetSq);
            defenderDist = DISTANCE(p->kpos[defender], targetSq) - (mover == defender);
            if (defenderDist <= myDist) *draw = MAX_DRAW;
            else if (ei->MLindex[defender] == 0) *score += 100 * sign[attacker];
        }
    }
    // if king controls square 2 past pawn, black better have opposition
    // TODO this can be improved to deal with draws when white is in front
    // but has lost opposition
    else {
        // if we control queening square, we win
        if (KingMoves[p->kpos[attacker]] & BitMask[targetSq]) {
            *score += 500 * sign[attacker];
            return;
        }
        // if we could control it, we win
        if (p->kpos[defender] != targetSq && mover == attacker &&
            KingMoves[p->kpos[attacker]] & KingMoves[targetSq]) {
            *score += 450 * sign[attacker];
            return;
        }
        if (p->kpos[defender] == targetSq) {
            if (mover == defender && p->kpos[attacker] == targetSq - PAWN_MOVE_INC(attacker) * 2)
                *draw = MAX_DRAW;
            else if (mover == attacker && p->kpos[attacker] != targetSq - PAWN_MOVE_INC(attacker) * 2 &&
                (KingMoves[p->kpos[attacker]] & BitMask[pawnSq]))
                *draw = MAX_DRAW;
            else if (ei->MLindex[defender] == 0) *score += 400 * sign[attacker];
        } else if ((PawnCaps[p->kpos[defender]][attacker] & BitMask[targetSq]) &&
            mover == defender && (PawnCaps[p->kpos[attacker]][attacker] & BitMask[targetSq]) == 0)
            *draw = MAX_DRAW;
        else if (DISTANCE(p->kpos[defender], pawnSq) - (mover == defender) < DISTANCE(p->kpos[attacker], targetSq))
            *draw = MAX_DRAW;
        else if (ei->MLindex[defender] == 0) *score += 400 * sign[attacker];
    }
}
void DrawnRookPawn(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw, int32 mover) {
    int qs;
    uint64 pawns = p->pawns & p->color[attacker];
    if ((pawns& (~FileABB)) == 0) {
        int defender = attacker ^ 1;
        uint64 bishop = (p->bishops & p->color[attacker]);
        if (showEval) Print(3, " only a pawn draw? ");
        qs = PAWN_PROMOTE(FileA, attacker);
        if (DISTANCE(p->kpos[attacker], qs) > (DISTANCE(p->kpos[defender], qs) - (mover == defender)) && ((bishop == 0) ||
            ((bishop & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0))) {
            *draw = MAX_DRAW;
            if (showEval) Print(3, "YES ");
        }
        //TODO check if we can get to C8 without pawn getting to A7
        // do draw when king trapped in front of own pawn
        else if ((bishop == 0) && ((SQFILE(p->kpos[attacker]) == FileA &&
            (p->kpos[defender] == p->kpos[attacker] + 2 || (mover == defender && DISTANCE(p->kpos[attacker] + 2, p->kpos[defender]) == 1)
            || (SQFILE(p->kpos[defender]) == FileC && (DISTANCE(p->kpos[defender], qs + 2) < DISTANCE(p->kpos[attacker], qs)
            || DISTANCE(p->kpos[defender], PAWN_PROMOTE(SQFILE(p->kpos[defender]), attacker)) - (mover == defender) <= 1)))) ||
            (p->kpos[defender] == qs + 2 && (KingMoves[qs] & pawns) == 0))) {
            *draw = MAX_DRAW;
        }
    }

    if ((pawns & (~FileHBB)) == 0) {
        int defender = attacker ^ 1;
        uint64 bishop = (p->bishops & p->color[attacker]);
        if (showEval) Print(3, " only h pawn draw? ");
        qs = PAWN_PROMOTE(FileH, attacker);
        if (DISTANCE(p->kpos[attacker], qs) > DISTANCE(p->kpos[attacker ^ 1], qs) && (bishop == 0 ||
            ((bishop & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0))) {
            *draw = MAX_DRAW;
            if (showEval) Print(3, "YES ");
        } else if (bishop == 0 && ((SQFILE(p->kpos[attacker]) == FileH &&
            (p->kpos[defender] == p->kpos[attacker] - 2 || (mover == defender && DISTANCE(p->kpos[attacker] - 2, p->kpos[defender]) == 1)
            || (SQFILE(p->kpos[defender]) == FileF && (DISTANCE(p->kpos[defender], qs - 2) < DISTANCE(p->kpos[attacker], qs)
            || DISTANCE(p->kpos[defender], PAWN_PROMOTE(SQFILE(p->kpos[defender]), attacker)) - (mover == defender) <= 1)))) ||
            (p->kpos[defender] == qs - 2 && (KingMoves[qs] & pawns) == 0))) {
            *draw = MAX_DRAW;
        }
    }
}
void KnightBishopMate(int color, const position_t *p, int *score, int *draw) {
    int cornerDist;
    int defender = color ^ 1;
    if ((p->bishops & WhiteSquaresBB & p->color[color])) {
        cornerDist = MIN(DISTANCE(p->kpos[defender], h1), DISTANCE(p->kpos[defender], a8));
        if (showEval) Print(3, "forced h1/a8 ");
    } else {
        cornerDist = MIN(DISTANCE(p->kpos[defender], a1), DISTANCE(p->kpos[defender], h8));
        if (showEval) Print(3, "forced a1/h8 ");
    }
    *score += ((7 - cornerDist) * 50)*sign[color];
    *draw = 0;
}

static int mate_square[64];
// this is useful for combatting high draw scores, which may obfiscate progress in hard to mate situations
void InitMateBoost() {
    int row[8] = { 50, 30, 10, 0, 0, 10, 30, 50 };
    int i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            mate_square[i * 8 + j] = row[i] + row[j] + MAX(row[i], row[j]);
        }
    }
}
void MateNoPawn(int attacker, const position_t *p, int *score, int *draw) {
    int defender = attacker ^ 1;
    int boostKing = mate_square[p->kpos[defender]];
    int boostProx = 10 * (7 - DISTANCE(p->kpos[attacker], p->kpos[defender]));
    if (showEval) Print(3, " mate boost k %d p %d", boostKing, boostProx);
    *score += (boostKing + boostProx) * sign[attacker];
}

void RookBishopEnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw) {
    // opposite bishop endgames
    if (ei->flags & OPPOSITE_BISHOPS) {
        *draw += 5;
        if ((ei->pawn_entry->passedbits & p->color[attacker]) == 0) *draw += 10; // ones without passed are really drawish
        if (MaxOneBit(p->pawns & p->color[attacker])) *draw += 30; // can saq a bishop to get RB v. R endgame
    }
}
void BishopEnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw) {
    int defender;
    int qs;
    uint64 passed;
    if (showEval) Print(3, " Bend ");
    // opposite bishop endgames

    if (ei->flags & OPPOSITE_BISHOPS) {
        // all opposite bishop endgames are drawish
        if (showEval) Print(3, "opb e ");
        // opposite bishop can saq for last pawn its drawn
        // this is not perfect, but search can catch exceptions
        if (ei->MLindex[attacker] == MLB + MLP) {
            if (showEval) Print(3, " opb DRAW ");
            *draw = MAX_DRAW;
            return;
        }
        defender = attacker ^ 1;
        // if we can saq for a pawn and get a draw rook pawn endgame, go for it
        qs = PAWN_PROMOTE(FileA, attacker);

        if (MaxOneBit(p->pawns & p->color[attacker] & (~FileABB)) &&
            DISTANCE(p->kpos[defender], qs) < DISTANCE(p->kpos[attacker], qs) &&
            ((p->bishops & p->color[attacker] & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0)) {
            *draw = MAX_DRAW;
            if (showEval) Print(3, "a pawn black draw ");
            return;
        }
        qs = PAWN_PROMOTE(FileH, attacker);
        if (MaxOneBit(p->pawns & p->color[attacker] & (~FileHBB)) &&
            DISTANCE(p->kpos[defender], qs) < DISTANCE(p->kpos[attacker], qs) &&
            ((p->bishops & p->color[attacker] & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0)) {
            *draw = MAX_DRAW;
            if (showEval) Print(3, "h pawn black draw ");
            return;
        }


        passed = ei->pawn_entry->passedbits & p->color[attacker];
        if (passed == 0) *draw += 30;
        else if (MaxOneBit(passed)) *draw += 20;
        {
            int pUp = (ei->MLindex[attacker]) - (ei->MLindex[defender]);
            if (pUp == 1 || pUp == 2) {
                *score = (attacker == WHITE) ? MAX(*score / 2, *score - 50 * pUp) : MIN(*score / 2, *score + 50 * pUp);
            }

        }
        // if the king is blocking the passed pawns its quite drawish
        {
            int blockingAll = TRUE;
            while (passed) { // this is for both colors
                int sq = popFirstBit(&passed);
                if (abs(SQFILE(p->kpos[defender]) - SQFILE(sq)) > 1 || !IN_FRONT(SQRANK(p->kpos[defender]), SQRANK(sq), attacker)) {
                    blockingAll = FALSE;
                    break;
                }
            }
            if (blockingAll) *draw += 40; //SAM FIX probably should check potential passed pawns some too
            else
                *draw += 10; // all opposite bishop endings are somewhat drawish
        }

    }
    // same color bishop
    else {
        int pNum = ei->MLindex[attacker] - MLB;
        if (pNum == 1) {
            int sq = GetOnlyBit(p->pawns & p->color[attacker]);
            defender = attacker ^ 1;
            if (abs(SQFILE(p->kpos[defender]) - SQFILE(sq)) <= 1 && IN_FRONT(SQRANK(p->kpos[defender]), SQRANK(sq), attacker))
                *draw = SUPER_DRAWISH;
            return;
        }
    }
}
void DrawnNP(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw) {
    int defender = attacker ^ 1;
    uint64 pawn = (p->pawns & p->color[attacker]);
    if (((pawn ^ BitMask[(PAWN_PROMOTE(FileA, attacker) - PAWN_MOVE_INC(attacker))]) == 0
        && DISTANCE(p->kpos[defender], PAWN_PROMOTE(FileA, attacker)) <= 1) ||
        ((pawn ^ BitMask[(PAWN_PROMOTE(FileH, attacker) - PAWN_MOVE_INC(attacker))]) == 0
        && DISTANCE(p->kpos[defender], PAWN_PROMOTE(FileH, attacker)) <= 1))
        *draw = MAX_DRAW;
    // if opponent king is in front and they have a piece, darn drawn
    if (ei->MLindex[defender] >= MLN) {
        int sq = GetOnlyBit(pawn);
        if (abs(SQFILE(p->kpos[defender]) - SQFILE(sq)) <= 1 && IN_FRONT(SQRANK(p->kpos[defender]), SQRANK(sq), attacker))
            *draw = SUPER_DRAWISH;

    }
}
void PawnEnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw, int mover) {
    int defender = attacker ^ 1;
    uint64 pawns[2], nonApawns, nonHpawns;
    pawns[WHITE] = p->pawns & p->color[attacker];
    pawns[BLACK] = p->pawns & p->color[defender];
    // drawn races to win rook pawns
    nonApawns = pawns[attacker] & (~FileABB);
    if ((pawns[attacker] & FileABB) && (pawns[defender] & FileABB) && MaxOneBit(nonApawns)) {
        int dpSq = getFirstBit(pawns[defender] & FileABB);
        uint64 qSquares = (*FillPtr[attacker])(BitMask[dpSq]);
        // make sure rook pawn is stopping opposition
        // and pawn is not so advanced that attacker can shoulder defender
        if ((qSquares & pawns[attacker]) == 0 && Q_DIST(dpSq, attacker) > 2) {
            // how long does it take to capture defender pawn
            // add one, since we need to get out from in front of our pawn
            int aRace = DISTANCE(dpSq, p->kpos[attacker]) + 1;
            // how long to capture attacker pawn and then trap attacker in front of his pawn or get to the C sq
            int apSq = GetOnlyBit(nonApawns);
            int dRace1 = DISTANCE(apSq, p->kpos[defender]) - (mover == defender);
            if (showEval) Print(3, " Adraw? ");
            if (aRace >= dRace1 + DISTANCE(apSq, dpSq + 2)) {
                *draw = VERY_DRAWISH;
                return;
            }

            if (aRace >= dRace1 + DISTANCE(apSq, PAWN_PROMOTE(FileA, attacker) + 2)) {
                *draw = VERY_DRAWISH;
                return;
            }
        }
    }
    nonHpawns = pawns[attacker] & (~FileHBB);
    if ((pawns[attacker] & FileHBB) && (pawns[defender] & FileHBB) && MaxOneBit(nonHpawns)) {
        int dpSq = getFirstBit(pawns[defender] & FileHBB);
        uint64 qSquares = (*FillPtr[attacker])(BitMask[dpSq]);
        // make sure rook pawn is stopping opposition
        // and pawn is not so advanced that attacker can shoulder defender
        if ((qSquares & pawns[attacker]) == 0 && Q_DIST(dpSq, attacker) > 2) {
            // how long does it take to capture defender pawn
            // add one, since we need to get out from in front of our pawn
            int aRace = DISTANCE(dpSq, p->kpos[attacker]) + 1;
            // how long to capture attacker pawn and then trap attacker in front of his pawn or get to the C sq
            int apSq = GetOnlyBit(nonHpawns);
            int dRace1 = DISTANCE(apSq, p->kpos[defender]) - (mover == defender);
            if (showEval) Print(3, " Hdraw? ");
            if (aRace >= dRace1 + DISTANCE(apSq, dpSq - 2)) {
                *draw = SUPER_DRAWISH;
                return;
            }

            if (aRace >= dRace1 + DISTANCE(apSq, PAWN_PROMOTE(FileH, attacker) - 2)) {
                *draw = SUPER_DRAWISH;
                return;
            }
        }
    }
}


void RookEnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw) {
    // if we can get in front of a single pawn we are safe
    // TODO address h and a pawn situations
    int defender = attacker ^ 1;
    // if I have not pawns, and my opponent has passed pawns, and his king
    // is not cut off and is closer to queening square, its probably won
    uint64 passedA = (ei->pawn_entry->passedbits & p->color[attacker]);
    if (ei->MLindex[defender] == MLR && passedA != 0) {
        uint64 passed;
        int raSq = GetOnlyBit(p->rooks & p->color[attacker]);
        int raFile = SQFILE(raSq);
        int behindRook = raSq + PAWN_MOVE_INC(defender);
        if (ei->MLindex[attacker] == MLR + MLP) {
            int sq = GetOnlyBit(p->pawns & p->color[attacker]);
            int sqFile = SQFILE(sq);
            // if the defender is closer to attacking the queening square then the defender its probably drawn
            int promoteSquare = PAWN_PROMOTE(sq, attacker);
            if (showEval) Print(3, "promote square %s ", sq2Str(promoteSquare));
            // need to be closer, not just equal, so that shouldering techniques do not work
            if (DISTANCE(p->kpos[defender], promoteSquare) < DISTANCE(p->kpos[attacker], promoteSquare)) {
                *draw = (DRAWN + DRAWN1) / 2;
                return;
            }
            // if it is a rook pawn there are some special cases
            if (sqFile == FileA || sqFile == FileH) {
                if (Q_DIST(sq, attacker) == 1 &&
                    PAWN_PROMOTE(sq, attacker) == p->kpos[attacker] &&
                    Q_DIST(p->kpos[defender], attacker) <= 1 && DISTANCE(p->kpos[defender], sq) <= 4) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
                // = is ok since shouldering does not rook if opponent king is on rook file
                if (DISTANCE(p->kpos[defender], promoteSquare) <= Q_DIST(sq, attacker) &&
                    sqFile == SQFILE(p->kpos[attacker])) { //there are many more cases, but want to be a bit conservative

                    *draw = (DRAWN + DRAWN1) / 2;
                    return;

                }
                if (Q_DIST(sq, attacker) == 1 &&
                    PAWN_PROMOTE(sq, attacker) == p->kpos[attacker] &&
                    Q_DIST(p->kpos[defender], attacker) <= 1 && DISTANCE(p->kpos[defender], sq) <= 4) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }

            }
        }
        // if we have a rook in front of a pawn on the 7th
        if (raSq == PAWN_PROMOTE(raSq, attacker) && (raFile == FileA) &&
            (BitMask[behindRook] & ei->pawns[attacker])) {
            // if the king is in good defensive position
            if (ei->MLindex[attacker] == MLR + MLP) {
                int kdSq = p->kpos[defender];
                if (Q_DIST(kdSq, attacker) <= 1 &&
                    (DISTANCE(kdSq, behindRook) <= 2 || DISTANCE(kdSq, behindRook) >= 6)) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            } else if ((ei->pawns[attacker] & ~(FileABB | FileGBB)) == 0 ||
                (ei->pawns[attacker] & ~(FileABB | FileHBB)) == 0) {
                int kdSq = p->kpos[defender];
                if (showEval) Print(3, " RRP draw? ");
                if (Q_DIST(kdSq, attacker) <= 1 && DISTANCE(kdSq, behindRook) >= 6) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            }
        }
        if (raSq == PAWN_PROMOTE(raSq, attacker) && (raFile == FileH) &&
            (BitMask[behindRook] & ei->pawns[attacker])) {
            // if the king is in good defensive position
            if (ei->MLindex[attacker] == MLR + MLP) {
                int kdSq = p->kpos[defender];
                if (Q_DIST(kdSq, attacker) <= 1 &&
                    (DISTANCE(kdSq, behindRook) <= 2 || DISTANCE(kdSq, behindRook) >= 6)) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            } else if ((ei->pawns[attacker] & ~(FileHBB | FileBBB)) == 0 ||
                (ei->pawns[attacker] & ~(FileHBB | FileABB)) == 0) {
                int kdSq = p->kpos[defender];
                if (showEval) Print(3, " RRP draw? ");
                if (Q_DIST(kdSq, attacker) <= 1 && DISTANCE(kdSq, behindRook) >= 6) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            }
        }

        passed = passedA;
        while (passed) {
            int sq = popFirstBit(&passed);
            // if king is not cut off
            if (abs(SQFILE(p->kpos[attacker]) - SQFILE(sq)) <= 1) {
                int qs = PAWN_PROMOTE(SQFILE(sq), attacker);
                // if our king is closer the position is probably winning
                if (DISTANCE(p->kpos[attacker], qs) <= DISTANCE(p->kpos[defender], qs)) {
                    *draw /= 2;
                    *score += 100 * sign[attacker];
                    return;
                }
                // if closer to queening square than opponent
            }

        }
    }
    if (ei->MLindex[attacker] == MLR + MLP) {
        int sq = GetOnlyBit(p->pawns & p->color[attacker]);
        // lets deal with rook pawn situations
        int sqFile = SQFILE(sq);
        if (abs(SQFILE(p->kpos[defender]) - sqFile) <= 1 && IN_FRONT(SQRANK(p->kpos[defender]), SQRANK(sq), attacker))
            *draw = 190 - 30 * (PAWN_RANK(p->kpos[defender], attacker) == Rank8);
        return;
    }
    // if no more than one pawn down
    else if (ei->MLindex[attacker] - ei->MLindex[defender] <= MLP  && passedA == 0) {
        uint64 pawnking = p->color[defender] & ~p->rooks;
        // no passed pawn is drawish
        *draw += 10;
        // pawns on one side is more drawish assuming the defending king is on the same side
        if ((QUEENSIDE & pawnking) == 0 || (KINGSIDE & pawnking) == 0) {
            int pawnsTraded = MAX(4, 8 - (ei->MLindex[attacker] - MLR));
            // the less unguarded pawns, the more drawish it is

            int unguarded = bitCnt(ei->pawns[defender] & ~(ei->atkkings[defender] | ei->atkpawns[defender] | shiftLeft(ei->pawns[defender], 1) |
                shiftRight(ei->pawns[defender], 1)));
            if (showEval) Print(3, " unguarded %d ", unguarded);

            if (unguarded == 0) {
                *draw += pawnsTraded * 12;
            } else if (unguarded == 1) {
                *draw += pawnsTraded * 8;
            } else {
                *draw += pawnsTraded * 4;
            }
        }
    }
}


// detect queen vs. rook pawn or bishop pawn draws
// for now, just rely on search to find exceptions

void QueenvPawn(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw) {
    //find pawn
    int defender = attacker ^ 1;
    // if we are on the 7th rank and rook or bishop pawn
    if ((Rank7ByColorBB[defender] & (FileABB | FileCBB | FileFBB | FileHBB)) & p->pawns) {
        int pSq = GetOnlyBit(p->pawns);
        int dDist = DISTANCE(p->kpos[defender], pSq);
        // king should be somewhat near pawn I guess
        // there is one position where this is wrong
        if ((dDist == 1 && DISTANCE(pSq, (int)GetOnlyBit(p->queens)) > 1) ||
            ((ei->atkkings[defender] & Rank8ByColorBB[defender]) &&
            dDist <= 2)) {
            int eDist = DISTANCE(p->kpos[attacker], pSq);
            *score /= 5;
            if (eDist < 3) *draw = DRAWISH + 5;
            else {

                *draw = SUPER_DRAWISH - 1 + eDist;
            }
            if (showEval) Print(3, "qvPdraw?");
        }
    }
}

void RookvPawnsEnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw, int mover) {
    int defender = attacker ^ 1;
    int defenderMove = (mover != attacker);
    uint64 pawns = p->pawns & p->color[defender];
    if (showEval) Print(3, "RvPdraw?");
    while (pawns) {
        int pSq = popFirstBit(&pawns);
        int dkpDist = DISTANCE(p->kpos[defender], pSq);
        int akDist, pDist, dkDist;
        int dDist;
        int qSq;
        int rDist;
        int rSq;
        // if we are too distant from our pawn its not drawish
        if (dkpDist - defenderMove > 2) continue;
        // find out how we stand in the race
        qSq = PAWN_PROMOTE(SQFILE(pSq), defender);
        akDist = DISTANCE(qSq, p->kpos[attacker]);
        pDist = DISTANCE(pSq, qSq);
        dkDist = DISTANCE(p->kpos[defender], qSq);
        dDist = pDist + dkDist - defenderMove;
        rSq = GetOnlyBit(p->rooks & p->color[attacker]);
        rDist = !(SQRANK(rSq) == SQRANK(qSq)) || (SQFILE(rSq) == SQFILE(qSq));
        if (showEval) Print(3, "RvPdraw? dd %d dm %d akd %d akr %d ", dDist, defenderMove, akDist, rDist);
        // hey, we may have a draw here
        if (dDist < akDist + rDist) {
            int newDraw = PRETTY_DRAWISH + 2 * ((akDist + rDist) - dDist);
            *score = *score / 4; // each pawn effects score, so multiple pawns are good

            if (newDraw > *draw) *draw = newDraw;

        }
        // this is drawish, but many times lost, so lets be conservative
        else if (dkDist - defenderMove <  akDist) {
            int newDraw = DRAWISH - 10 + 2 * ((akDist + rDist) - dDist);
            *score = *score / 2;
            if (newDraw > *draw) *draw = newDraw;
        }
    }
}

//SAMFIX does not do fortress with rook pawns correctly, perhaps can expand knight pawn fortresses
void DrawnQvREnding(int attacker, const position_t *p, eval_info_t *ei, int *score, int *draw, int mover) {
    // remember, only the defender has rooks or pawns
    int defender = attacker ^ 1;
    uint64 pawns = p->pawns;
    uint64 rRank = RankBB[SQRANK(GetOnlyBit(p->rooks & p->color[attacker]))];
    while (pawns) {
        int pSq = popFirstBit(&pawns);
        int qDist = Q_DIST(pSq, defender);
        // the easiest draw to detect is if the defender has a pawn on the 6th or 7th, its usually a draw
        if (qDist <= 2) {
            int drawish = VERY_DRAWISH - qDist * 5;
            if (drawish > *draw) *draw = drawish;
            *score /= 4;
        }
        // we also need to detect a fortress with the pawn on the 2nd
        else if (qDist == 6 && (ei->atkpawns[defender] & p->rooks)
            && (0 == (p->pawns & rRank))
            && DISTANCE(pSq, p->kpos[defender]) == 1 &&
            IN_FRONT(SQRANK(p->kpos[attacker]), SQRANK(pSq), defender)) {
            *draw = SUPER_DRAWISH;
            *score /= 4;
        }
    }
}

void evalEndgame(int attacker, const position_t *pos, eval_info_t *ei, int *score, int *draw, int mover) {
    mflag_t endIndex;
    endIndex = ei->endFlags[attacker];

    switch (endIndex) {
    case 1: // pawn endgames
        SinglePawnEnding(attacker, pos, ei, score, draw, pos->side);
        DrawnRookPawn(attacker, pos, ei, score, draw, pos->side);
        break;
    case 2: // rook endings
        RookEnding(attacker, pos, ei, score, draw);
        break;
    case 3: // bishop endings
        BishopEnding(attacker, pos, ei, score, draw);
        DrawnRookPawn(attacker, pos, ei, score, draw, pos->side);
        break;
    case 4: // rook versys pawns endings
        RookvPawnsEnding(attacker, pos, ei, score, draw, pos->side);
        break;
    case 5: // knight + pawn
        DrawnNP(attacker, pos, ei, score, draw);
        break;
    case 6: // Q vs. Rook + Pawn(s)
        DrawnQvREnding(attacker, pos, ei, score, draw, pos->side);
        break;
    case 7: // mate with knight and bishop
        KnightBishopMate(attacker, pos, score, draw);
        break;
    case 8:
        RookBishopEnding(attacker, pos, ei, score, draw);
        break;
    case 9:
        DrawnRookPawn(attacker, pos, ei, score, draw, pos->side);
        break;
    case 10:
        DrawnRookPawn(attacker, pos, ei, score, draw, pos->side);
        PawnEnding(attacker, pos, ei, score, draw, pos->side);
        break;
    case 11:
        RookvKnight(attacker, pos, ei, score, draw, pos->side);
        break;
    case 12:
        QueenvPawn(attacker, pos, ei, score, draw);
        break;
    case 13:
        MateNoPawn(attacker, pos, score, draw);
        break;
    }
}

