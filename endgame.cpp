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
#include "bitutils.h"
#include "trans.h"
#include "eval.h"
#include "material.h"

void RookvKnight(int attacker, const position_t& pos, int *score, int *draw) {
    int nsq = GetOnlyBit(pos.knights);
    int penalty = DISTANCE(nsq, pos.kpos[attacker ^ 1]);
    penalty *= penalty;
    *score += penalty * sign[attacker];
    *draw -= penalty;
}

void SinglePawnEnding(int attacker, const position_t& pos, eval_info_t& ei, int *score, int *draw, int mover) {
    int pawnSq = GetOnlyBit(pos.pawns & pos.color[attacker]);
    int targetSq = pawnSq + PAWN_MOVE_INC(attacker);
    int defender = attacker ^ 1;
    int myDist;
    int defenderDist;
    // a and h pawns are special cases but handled elsewhere
    // pawns on 7th is special case...assume search handles it
    if (PAWN_RANK(pawnSq, attacker) < Rank7) {
        int pastTarget = targetSq + PAWN_MOVE_INC(attacker);
        int supportAdvance = (pos.kpos[attacker] == pastTarget) ||
            (KingMoves[pos.kpos[attacker]] & BitMask[pastTarget]);
        // is you are 2 in front of pawn black has no draw
        if (IN_FRONT(SQRANK(pos.kpos[attacker]), SQRANK(targetSq), attacker) && supportAdvance) {
            if (ei.MLindex[defender] == 0) *score += 400 * sign[attacker];
        }
        // if you control the square in front of pawn black must have
        // opposition
        else if (supportAdvance) {
            // if you are on 6th rank you got it
            // relying on rook pawns dealt with first elsewhere
            if (PAWN_RANK(pawnSq, attacker) >= Rank5) {
                if (ei.MLindex[defender] == 0) *score += 400 * sign[attacker];
                else *score += 100 * sign[attacker];//sam3.13 small bug fix
            }
            else {
                // otherwise if king has opposition he is safe
                int oppSq = pos.kpos[attacker] + PAWN_MOVE_INC(attacker) * 2;
                if ((oppSq == pos.kpos[defender]) && mover == attacker)
                    *draw = MAX_DRAW;
                else if ((KingMoves[pos.kpos[defender]] & BitMask[oppSq])
                    && mover == defender)
                    *draw = MAX_DRAW;
                else if (ei.MLindex[defender] == 0) *score += 400 * sign[attacker];
            }
            return;
        }
        // if black can obviously get to the pawn first he wins
        // remember, cases where white king is in front of pawn
        // are already dealt with
        else {
            // if pawn is on 6th and king supporting
            if (PAWN_RANK(pawnSq, attacker) == Rank6 && (KingMoves[pos.kpos[attacker]] & BitMask[targetSq])) {
                int oppSq = pos.kpos[attacker] + PAWN_MOVE_INC(attacker) * 2;
                if (pos.kpos[defender] == oppSq && mover == attacker) {
                    *draw = MAX_DRAW;
                    return;
                }
                if ((KingMoves[pos.kpos[defender]] & BitMask[oppSq]) && mover == defender) {
                    *draw = MAX_DRAW;
                    return;
                }
                if (ei.MLindex[defender] == 0) *score += 400 * sign[attacker];
                return;
            }
            myDist = DISTANCE(pos.kpos[attacker], targetSq);
            defenderDist = DISTANCE(pos.kpos[defender], targetSq) - (mover == defender);
            if (defenderDist <= myDist) *draw = MAX_DRAW;
            else if (ei.MLindex[defender] == 0) *score += 100 * sign[attacker];
        }
    }
    // if king controls square 2 past pawn, black better have opposition
    // TODO this can be improved to deal with draws when white is in front
    // but has lost opposition
    else {
        // if we control queening square, we win
        if (KingMoves[pos.kpos[attacker]] & BitMask[targetSq]) {
            *score += 500 * sign[attacker];
            return;
        }
        // if we could control it, we win
        if (pos.kpos[defender] != targetSq && mover == attacker &&
            KingMoves[pos.kpos[attacker]] & KingMoves[targetSq]) {
            *score += 450 * sign[attacker];
            return;
        }
        if (pos.kpos[defender] == targetSq) {
            if (mover == defender && pos.kpos[attacker] == targetSq - PAWN_MOVE_INC(attacker) * 2)
                *draw = MAX_DRAW;
            else if (mover == attacker && pos.kpos[attacker] != targetSq - PAWN_MOVE_INC(attacker) * 2 &&
                (KingMoves[pos.kpos[attacker]] & BitMask[pawnSq]))
                *draw = MAX_DRAW;
            else if (ei.MLindex[defender] == 0) *score += 400 * sign[attacker];
        }
        else if ((PawnCaps[pos.kpos[defender]][attacker] & BitMask[targetSq]) &&
            mover == defender && (PawnCaps[pos.kpos[attacker]][attacker] & BitMask[targetSq]) == 0)
            *draw = MAX_DRAW;
        else if (DISTANCE(pos.kpos[defender], pawnSq) - (mover == defender) < DISTANCE(pos.kpos[attacker], targetSq))
            *draw = MAX_DRAW;
        else if (ei.MLindex[defender] == 0) *score += 400 * sign[attacker];
    }
}
void DrawnRookPawn(int attacker, const position_t& pos, int *draw, int32 mover) {
    int qs;
    uint64 pawns = pos.pawns & pos.color[attacker];
    if ((pawns& (~FileABB)) == 0) {
        int defender = attacker ^ 1;
        uint64 bishop = (pos.bishops & pos.color[attacker]);
        qs = PAWN_PROMOTE(FileA, attacker);
        if (DISTANCE(pos.kpos[attacker], qs) > (DISTANCE(pos.kpos[defender], qs) - (mover == defender)) && ((bishop == 0) ||
            ((bishop & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0))) {
            *draw = MAX_DRAW;
        }
        //TODO check if we can get to C8 without pawn getting to A7
        // do draw when king trapped in front of own pawn
        else if ((bishop == 0) && ((SQFILE(pos.kpos[attacker]) == FileA &&
            (pos.kpos[defender] == pos.kpos[attacker] + 2 || (mover == defender && DISTANCE(pos.kpos[attacker] + 2, pos.kpos[defender]) == 1)
                || (SQFILE(pos.kpos[defender]) == FileC && (DISTANCE(pos.kpos[defender], qs + 2) < DISTANCE(pos.kpos[attacker], qs)
                    || DISTANCE(pos.kpos[defender], PAWN_PROMOTE(SQFILE(pos.kpos[defender]), attacker)) - (mover == defender) <= 1)))) ||
                    (pos.kpos[defender] == qs + 2 && (KingMoves[qs] & pawns) == 0))) {
            *draw = MAX_DRAW;
        }
    }

    if ((pawns & (~FileHBB)) == 0) {
        int defender = attacker ^ 1;
        uint64 bishop = (pos.bishops & pos.color[attacker]);
        qs = PAWN_PROMOTE(FileH, attacker);
        if (DISTANCE(pos.kpos[attacker], qs) > DISTANCE(pos.kpos[attacker ^ 1], qs) && (bishop == 0 ||
            ((bishop & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0))) {
            *draw = MAX_DRAW;
        }
        else if (bishop == 0 && ((SQFILE(pos.kpos[attacker]) == FileH &&
            (pos.kpos[defender] == pos.kpos[attacker] - 2 || (mover == defender && DISTANCE(pos.kpos[attacker] - 2, pos.kpos[defender]) == 1)
                || (SQFILE(pos.kpos[defender]) == FileF && (DISTANCE(pos.kpos[defender], qs - 2) < DISTANCE(pos.kpos[attacker], qs)
                    || DISTANCE(pos.kpos[defender], PAWN_PROMOTE(SQFILE(pos.kpos[defender]), attacker)) - (mover == defender) <= 1)))) ||
                    (pos.kpos[defender] == qs - 2 && (KingMoves[qs] & pawns) == 0))) {
            *draw = MAX_DRAW;
        }
    }
}
// THIS NEEDS TO BE RE-EXAMINED
void PawnEnding(int attacker, const position_t& pos, int *draw, int mover) {
    int defender = attacker ^ 1;
    uint64 pawns[2], nonApawns, nonHpawns;
    pawns[WHITE] = pos.pawns & pos.color[attacker];
    pawns[BLACK] = pos.pawns & pos.color[defender];
    // drawn races to win rook pawns
    nonApawns = pawns[attacker] & (~FileABB);
    if ((pawns[attacker] & FileABB) && (pawns[defender] & FileABB) && MaxOneBit(nonApawns) && (nonApawns & ~(FileBBB | FileCBB))) {
        if (SHOW_EVAL) PrintOutput() << "info string rook pawn race\n";
        int dpSq = getFirstBit(pawns[defender] & FileABB);
        uint64 qSquares = (*FillPtr[attacker])(BitMask[dpSq]);
        // make sure rook pawn is stopping opposition
        // and pawn is not so advanced that attacker can shoulder defender
        if ((qSquares & pawns[attacker]) == 0 && Q_DIST(dpSq, attacker) > 2) {
            // how long to capture attacker pawn and then trap attacker in front of his pawn or get to the C sq
            int aRace;
            int apSq = GetOnlyBit(nonApawns);
            int dRace1 = DISTANCE(apSq, pos.kpos[defender]);
            if ((KingMoves[pos.kpos[attacker]] & nonApawns) != 0) {
                aRace = DISTANCE(dpSq, apSq) - 2 + 1; // can scoot 1 over from pawn and force opponent 1 back
                dRace1 = MAX(dRace1, 2);
            }
            else {
                dRace1 -= (mover == defender);
                aRace = DISTANCE(dpSq, pos.kpos[attacker]) + 1; // add one, since we need to get out from in front of our pawn
            }
            // this lets defender trap attacker king in front of his pawn
            if (aRace >= dRace1 + DISTANCE(apSq, dpSq + 2)) {
                *draw = VERY_DRAWISH;
                if (SHOW_EVAL) PrintOutput() << "info string rook pawn race 1 att:" << aRace << " def " << dRace1 << " extra " << DISTANCE(apSq, dpSq + 2) << " \n";
                return;
            }
            if (SHOW_EVAL) PrintOutput() << "info string rook pawn race X att:" << aRace << " def " << dRace1 << " extra " << DISTANCE(apSq, dpSq + 2) << " \n";
            //see if defender can get to queen square area
            if (aRace + Q_DIST(dpSq, attacker) - 2 >= dRace1 + DISTANCE(apSq, PAWN_PROMOTE(FileA, attacker) + 2)) {
                *draw = VERY_DRAWISH;
                if (SHOW_EVAL) PrintOutput() << "info string rook pawn race 2 aR:" << aRace + Q_DIST(dpSq, attacker) - 2 << " def " << dRace1 << " extra " << DISTANCE(apSq, PAWN_PROMOTE(FileC, attacker) + 2) << " \n";
                return;
            }
            if (SHOW_EVAL) PrintOutput() << "info string rook pawn race X2 aR:" << aRace + Q_DIST(dpSq, attacker) - 2 << " def " << dRace1 << " extra " << DISTANCE(apSq, PAWN_PROMOTE(FileC, attacker) + 2) << " \n";
        }
    }
    nonHpawns = pawns[attacker] & (~FileHBB);
    if ((pawns[attacker] & FileHBB) && (pawns[defender] & FileHBB) && MaxOneBit(nonHpawns) && (nonHpawns & ~(FileGBB | FileFBB))) {
        if (SHOW_EVAL) PrintOutput() << "info string rook pawn race\n";
        int dpSq = getFirstBit(pawns[defender] & FileHBB);
        uint64 qSquares = (*FillPtr[attacker])(BitMask[dpSq]);
        // make sure rook pawn is stopping opposition
        // and pawn is not so advanced that attacker can shoulder defender
        if ((qSquares & pawns[attacker]) == 0 && Q_DIST(dpSq, attacker) > 2) {
            // how long to capture attacker pawn and then trap attacker in front of his pawn or get to the C sq
            int aRace;
            int apSq = GetOnlyBit(nonHpawns);
            int dRace1 = DISTANCE(apSq, pos.kpos[defender]);
            if ((KingMoves[pos.kpos[attacker]] & nonApawns) != 0) {
                aRace = DISTANCE(dpSq, apSq) - 2 + 1; // can scoot 1 over from pawn and force opponent 1 back
                dRace1 = MAX(dRace1, 2);
            }
            else {
                dRace1 -= (mover == defender);
                aRace = DISTANCE(dpSq, pos.kpos[attacker]) + 1; // add one, since we need to get out from in front of our pawn
            }
            // this lets defender trap attacker king in front of his pawn
            if (aRace >= dRace1 + DISTANCE(apSq, dpSq - 2)) {
                *draw = VERY_DRAWISH;
                if (SHOW_EVAL) PrintOutput() << "info string rook pawn race 1 att:" << aRace << " def " << dRace1 << " extra " << DISTANCE(apSq, dpSq - 2) << " \n";
                return;
            }
            if (SHOW_EVAL) PrintOutput() << "info string rook pawn race X att:" << aRace << " def " << dRace1 << " extra " << DISTANCE(apSq, dpSq - 2) << " \n";
            //see if defender can get to queen square area
            if (aRace + Q_DIST(dpSq, attacker) - 2 >= dRace1 + DISTANCE(apSq, PAWN_PROMOTE(FileH, attacker) - 2)) {
                *draw = VERY_DRAWISH;
                if (SHOW_EVAL) PrintOutput() << "info string rook pawn race 2 aR:" << aRace + Q_DIST(dpSq, attacker) - 2 << " def " << dRace1 << " extra " << DISTANCE(apSq, PAWN_PROMOTE(FileC, attacker) - 2) << " \n";
                return;
            }
            if (SHOW_EVAL) PrintOutput() << "info string rook pawn race X2 aR:" << aRace + Q_DIST(dpSq, attacker) - 2 << " def " << dRace1 << " extra " << DISTANCE(apSq, PAWN_PROMOTE(FileC, attacker) - 2) << " \n";
        }
    }
}
void KnightBishopMate(int color, const position_t& pos, int *score, int *draw) {
    int cornerDist;
    int defender = color ^ 1;
    if ((pos.bishops & WhiteSquaresBB & pos.color[color])) {
        cornerDist = MIN(DISTANCE(pos.kpos[defender], h1), DISTANCE(pos.kpos[defender], a8));
    }
    else {
        cornerDist = MIN(DISTANCE(pos.kpos[defender], a1), DISTANCE(pos.kpos[defender], h8));
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
//TODO consider proximity of defender knight to king (or other pieces?)
void MateNoPawn(int attacker, const position_t& pos, int *score) {
    int defender = attacker ^ 1;
    int boostKing = mate_square[pos.kpos[defender]];
    int boostProx = 10 * (7 - DISTANCE(pos.kpos[attacker], pos.kpos[defender]));
    *score += (boostKing + boostProx) * sign[attacker];
}

void RookBishopEnding(int attacker, const position_t& pos, eval_info_t& ei, int *draw) {
    // opposite bishop endgames
    if (ei.oppBishops) {
        *draw += 5;
        if ((ei.pawn_entry->passedbits & pos.color[attacker]) == 0) *draw += 10; // ones without passed are really drawish
        if (MaxOneBit(pos.pawns & pos.color[attacker])) *draw += 30; // can saq a bishop to get RB v. R endgame
    }
}
void BishopEnding(int attacker, const position_t& pos, eval_info_t& ei, int *score, int *draw) {
    // opposite bishop endgames
    if (ei.oppBishops) {
        int defender;
        int qs;
        // all opposite bishop endgames are drawish
        // opposite bishop can saq for last pawn its drawn
        // this is not perfect, but search can catch exceptions
        if (ei.MLindex[attacker] == MLB + MLP) {
            *draw = MAX_DRAW;
            return;
        }
        defender = attacker ^ 1;
        // if we can saq for a pawn and get a draw rook pawn endgame, go for it
        qs = PAWN_PROMOTE(FileA, attacker);

        if (MaxOneBit(pos.pawns & pos.color[attacker] & (~FileABB)) &&
            DISTANCE(pos.kpos[defender], qs) < DISTANCE(pos.kpos[attacker], qs) &&
            ((pos.bishops & pos.color[attacker] & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0)) {
            *draw = MAX_DRAW;
            return;
        }
        qs = PAWN_PROMOTE(FileH, attacker);
        if (MaxOneBit(pos.pawns & pos.color[attacker] & (~FileHBB)) &&
            DISTANCE(pos.kpos[defender], qs) < DISTANCE(pos.kpos[attacker], qs) &&
            ((pos.bishops & pos.color[attacker] & WhiteSquaresBB) == 0) != ((BitMask[qs] & WhiteSquaresBB) == 0)) {
            *draw = MAX_DRAW;
            return;
        }

        uint64 passed = ei.pawn_entry->passedbits & pos.color[attacker];
        if (passed == 0) *draw += 30;
        else if (MaxOneBit(passed)) *draw += 20;
        {
            int pUp = (ei.MLindex[attacker]) - (ei.MLindex[defender]);
            if (pUp == 1 || pUp == 2) {
                *score = (attacker == WHITE) ? MAX(*score / 2, *score - 50 * pUp) : MIN(*score / 2, *score + 50 * pUp);
            }
        }
        // if the king is blocking the passed pawns its quite drawish
        {
            bool blockingAll = true;
            while (passed) { // this is for both colors
                int sq = popFirstBit(&passed);
                if (abs(SQFILE(pos.kpos[defender]) - SQFILE(sq)) > 1 || !IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(sq), attacker)) {
                    blockingAll = false;
                    break;
                }
            }
            if (blockingAll) {
                *draw += 40; //SAM FIX probably should check potential passed pawns some too
                if (ei.MLindex[attacker] == MLB + MLP * 2) { //2 pawn is drawn if not on at least the 6th rank
                    passed = ei.pawn_entry->passedbits & pos.color[attacker];
                    int sq = popFirstBit(&passed);
                    if (Q_DIST(sq, attacker) > 2) {
                        *draw = SUPER_DRAWISH;
                        if (SHOW_EVAL) PrintOutput() << "info string oppo bishop 2 pawns < 6th rank king in front\n";
                    }
                    else {
                        sq = popFirstBit(&passed);
                        if (Q_DIST(sq, attacker) > 2) {
                            *draw = SUPER_DRAWISH;
                            if (SHOW_EVAL) PrintOutput() << "info string oppo bishop 2 pawns < 6th rank king in front\n";
                        }
                    }
                }
                if (SHOW_EVAL) PrintOutput() << "info string oppo bishop king stopping all\n";
            }
            else *draw += 10;
        }
    }
    // same color bishop
    else {
        //SAMNEW
        //if you are blocking all the pawns and they are on the same row as you, you are golden
        int defender = attacker ^ 1;
        uint64 blockedSquares = (*FillPtr[defender])(BitMask[pos.kpos[defender]]);
        if ((ei.pawns[attacker] & ~blockedSquares) == 0) {
            *draw = MAX_DRAW;
        }
        // otherwise if these is two or less pawns and no passed pawns
        else if (ei.pawn_entry->passedbits == 0 && ei.MLindex[attacker] - MLB <= 2) {
            const int pawnSq = GetOnlyBit(pos.pawns & pos.color[defender]);
            if (abs(SQFILE(pos.kpos[defender]) - SQFILE(pawnSq)) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(pawnSq), attacker)) {
                if (SHOW_EVAL) PrintOutput() << "info string drawish bishop endgame\n";
                *draw = DRAWISH;
            }
        }
        /*
    int pNum = ei.MLindex[attacker] - MLB;
    if (pNum == 1) {
        int sq = GetOnlyBit(pos.pawns & pos.color[attacker]);
        int defender = attacker ^ 1;
        if (abs(SQFILE(pos.kpos[defender]) - SQFILE(sq)) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(sq), attacker))
            *draw = VERY_DRAWISH;
        return;
    }
    else if (pNum == 2 && ei.pawn_entry->passedbits == 0) {
        const int defender = attacker ^ 1;
        const int pawnSq = GetOnlyBit(pos.pawns & pos.color[defender]);
        if (abs(SQFILE(pos.kpos[defender]) - SQFILE(pawnSq)) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(pawnSq), attacker)) {
            if (SHOW_EVAL) PrintOutput() << "info string drawish bishop endgame\n";
            *draw = DRAWISH;
        }
    }*/
    }
}
void DrawnNP(int attacker, const position_t& pos, eval_info_t& ei, int *draw) {
    int defender = attacker ^ 1;
    uint64 pawn = (pos.pawns & pos.color[attacker]);
    if (((pawn ^ BitMask[(PAWN_PROMOTE(FileA, attacker) - PAWN_MOVE_INC(attacker))]) == 0
        && DISTANCE(pos.kpos[defender], PAWN_PROMOTE(FileA, attacker)) <= 1) ||
        ((pawn ^ BitMask[(PAWN_PROMOTE(FileH, attacker) - PAWN_MOVE_INC(attacker))]) == 0
            && DISTANCE(pos.kpos[defender], PAWN_PROMOTE(FileH, attacker)) <= 1))
        *draw = MAX_DRAW;
    // if opponent king is in front and they have a piece, darn drawn
    if (ei.MLindex[defender] >= MLN) {
        int sq = GetOnlyBit(pawn);
        if (abs(SQFILE(pos.kpos[defender]) - SQFILE(sq)) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(sq), attacker))
            *draw = SUPER_DRAWISH;
    }
}

template<bool singlePawn>
void RookEnding(int attacker, const position_t& pos, eval_info_t& ei, int *score, int *draw) {
    // if we can get in front of a single pawn we are safe
    // TODO address h and a pawn situations
    int defender = attacker ^ 1;
    // if I have not pawns, and my opponent has passed pawns, and his king
    // is not cut off and is closer to queening square, its probably won
    uint64 passedA = (ei.pawn_entry->passedbits & pos.color[attacker]);
    if (ei.MLindex[defender] == MLR && passedA != 0) {
        uint64 passed;
        int raSq = GetOnlyBit(pos.rooks & pos.color[attacker]);
        int raFile = SQFILE(raSq);
        int behindRook = raSq + PAWN_MOVE_INC(defender);
        if (singlePawn) {
            int sq = GetOnlyBit(pos.pawns & pos.color[attacker]);
            int sqFile = SQFILE(sq);
            // if the defender is closer to attacking the queening square then the defender its probably drawn
            int promoteSquare = PAWN_PROMOTE(sq, attacker);
            // need to be closer, not just equal, so that shouldering techniques do not work
            if (DISTANCE(pos.kpos[defender], promoteSquare) < DISTANCE(pos.kpos[attacker], promoteSquare)) {
                *draw = (DRAWN + DRAWN1) / 2;
                return;
            }
            // if it is a rook pawn there are some special cases
            if (sqFile == FileA || sqFile == FileH) {
                if (Q_DIST(sq, attacker) == 1 &&
                    PAWN_PROMOTE(sq, attacker) == pos.kpos[attacker] &&
                    Q_DIST(pos.kpos[defender], attacker) <= 1 && DISTANCE(pos.kpos[defender], sq) <= 4) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
                // = is ok since shouldering does not rook if opponent king is on rook file
                if (DISTANCE(pos.kpos[defender], promoteSquare) <= Q_DIST(sq, attacker) &&
                    sqFile == SQFILE(pos.kpos[attacker])) { //there are many more cases, but want to be a bit conservative
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
                if (Q_DIST(sq, attacker) == 1 &&
                    PAWN_PROMOTE(sq, attacker) == pos.kpos[attacker] &&
                    Q_DIST(pos.kpos[defender], attacker) <= 1 && DISTANCE(pos.kpos[defender], sq) <= 4) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            }
        }
        // if we have a rook in front of a pawn on the 7th
        if (raSq == PAWN_PROMOTE(raSq, attacker) && (raFile == FileA) &&
            (BitMask[behindRook] & ei.pawns[attacker])) {
            // if the king is in good defensive position
            if (ei.MLindex[attacker] == MLR + MLP) {
                int kdSq = pos.kpos[defender];
                if (Q_DIST(kdSq, attacker) <= 1 &&
                    (DISTANCE(kdSq, behindRook) <= 2 || DISTANCE(kdSq, behindRook) >= 6)) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            }
            else if ((ei.pawns[attacker] & ~(FileABB | FileGBB)) == 0 ||
                (ei.pawns[attacker] & ~(FileABB | FileHBB)) == 0) {
                int kdSq = pos.kpos[defender];
                if (Q_DIST(kdSq, attacker) <= 1 && DISTANCE(kdSq, behindRook) >= 6) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            }
        }
        if (raSq == PAWN_PROMOTE(raSq, attacker) && (raFile == FileH) &&
            (BitMask[behindRook] & ei.pawns[attacker])) {
            // if the king is in good defensive position
            if (ei.MLindex[attacker] == MLR + MLP) {
                int kdSq = pos.kpos[defender];
                if (Q_DIST(kdSq, attacker) <= 1 &&
                    (DISTANCE(kdSq, behindRook) <= 2 || DISTANCE(kdSq, behindRook) >= 6)) {
                    *draw = (DRAWN + DRAWN1) / 2;
                    return;
                }
            }
            else if ((ei.pawns[attacker] & ~(FileHBB | FileBBB)) == 0 ||
                (ei.pawns[attacker] & ~(FileHBB | FileABB)) == 0) {
                int kdSq = pos.kpos[defender];
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
            if (abs(SQFILE(pos.kpos[attacker]) - SQFILE(sq)) <= 1) {
                int qs = PAWN_PROMOTE(SQFILE(sq), attacker);
                // if our king is closer the position is probably winning
                if (DISTANCE(pos.kpos[attacker], qs) <= DISTANCE(pos.kpos[defender], qs)) {
                    *draw /= 2;
                    *score += 100 * sign[attacker];
                    return;
                }
                // if closer to queening square than opponent
            }
        }
    }
    if (singlePawn) {
        int sq = GetOnlyBit(pos.pawns & pos.color[attacker]);
        // lets deal with rook pawn situations
        int sqFile = SQFILE(sq);
        if (abs(SQFILE(pos.kpos[defender]) - sqFile) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(sq), attacker))
            *draw = 190 - 30 * (PAWN_RANK(pos.kpos[defender], attacker) == Rank8);
    }
    else if (passedA && MaxOneBit(passedA) && ei.MLindex[attacker] - ei.MLindex[defender] < 2) { //if attacker has one passed pawn and is not up by more than 1 pawn
        int passedSq = getFirstBit(passedA);
        int passedFile = SQFILE(passedSq);
        if ((passedFile < FileD && ((ei.pawns[attacker] ^ passedA) & (FileABB | FileBBB | FileCBB | FileDBB | FileEBB)) == 0) ||
            (passedFile > FileE && ((ei.pawns[attacker] ^ passedA) & (FileHBB | FileGBB | FileFBB | FileEBB | FileFBB)) == 0)) {
            if (SHOW_EVAL) PrintOutput() << "info string before draw " << *draw << "\n";
            *draw += 2 * abs(passedFile - SQFILE(pos.kpos[attacker]));
            if (SHOW_EVAL) PrintOutput() << "info string king dist " << abs(passedFile - SQFILE(pos.kpos[attacker])) << "\n";

            *draw -= (ei.MLindex[attacker] - MLR) * 3;
            if (SHOW_EVAL) PrintOutput() << "info string num attacker pawns " << (ei.MLindex[attacker] - MLR) << "\n";
            const int behind[] = { S, N };
            uint64 behindPawn = DirBitmap[behind[attacker]][passedSq];
            uint64 frontPawn = DirBitmap[behind[defender]][passedSq];
            if (frontPawn & pos.rooks & pos.color[attacker]) {
                *draw += ((pos.rooks & pos.color[attacker] & (Rank1 | Rank8)) == 0) ? 10 : 20;
                if (SHOW_EVAL) PrintOutput() << "info string attacker in front\n";
            }
            if ((behindPawn & pos.rooks & pos.color[defender]) != 0) {
                *draw += 10;
                if (SHOW_EVAL) PrintOutput() << "info string defender behind\n";
            }
            bool attackerRookBehind = (behindPawn & pos.rooks & pos.color[attacker]) != 0;
            if (passedA & (FileABB | FileHBB)) {
                *draw += attackerRookBehind ? 5 : 20;
                if (SHOW_EVAL) PrintOutput() << "info string a file\n";
            }
            else if (passedA & (FileBBB | FileGBB)) {
                *draw += attackerRookBehind ? 0 : 10;
                if (SHOW_EVAL) PrintOutput() << "info string b file\n";
            }
            else {
                *draw += attackerRookBehind ? 0 : 15;
                if (SHOW_EVAL) PrintOutput() << "info string c file\n";
            }

            if (SHOW_EVAL) PrintOutput() << "info string after draw " << *draw << "\n";
            if (*draw < 0) *draw = 0;
        }
    }
    else if (ei.MLindex[attacker] - ei.MLindex[defender] <= MLP  && passedA == 0) {  // if no more than one pawn down
        uint64 pawnking = pos.color[defender] & ~pos.rooks;
        // no passed pawn is drawish
        *draw += 10;
        // pawns on one side is more drawish assuming the defending king is on the same side
        if ((QUEENSIDE & pawnking) == 0 || (KINGSIDE & pawnking) == 0) { //TODO consider using pawn width instead of king/queenside
            int pawnsTraded = MAX(4, 8 - (ei.MLindex[attacker] - MLR));
            // the less unguarded pawns, the more drawish it is
            int unguarded = bitCnt(ei.pawns[defender] & ~(ei.atkkings[defender] | ei.atkpawns[defender] | adjacent(ei.pawns[defender])));
            if (unguarded == 0) {
                *draw += pawnsTraded * 12;
            }
            else if (unguarded == 1) {
                *draw += pawnsTraded * 8;
            }
            else {
                *draw += pawnsTraded * 4;
            }
        }
    }
}

// detect queen vs. rook pawn or bishop pawn draws
// for now, just rely on search to find exceptions

void QueenvPawn(int attacker, const position_t& pos, eval_info_t& ei, int *score, int *draw) {
    //find pawn
    int defender = attacker ^ 1;
    // if we are on the 7th rank and rook or bishop pawn
    if ((Rank7ByColorBB[defender] & (FileABB | FileCBB | FileFBB | FileHBB)) & pos.pawns) {
        int pSq = GetOnlyBit(pos.pawns);
        int dDist = DISTANCE(pos.kpos[defender], pSq);
        // king should be somewhat near pawn I guess
        // there is one position where this is wrong
        if ((dDist == 1 && DISTANCE(pSq, (int)GetOnlyBit(pos.queens)) > 1) ||
            ((ei.atkkings[defender] & Rank8ByColorBB[defender]) &&
                dDist <= 2)) {
            int eDist = DISTANCE(pos.kpos[attacker], pSq);
            *score /= 5;
            if (eDist < 3) *draw = DRAWISH + 5;
            else *draw = SUPER_DRAWISH - 1 + eDist;
        }
    }
}

void RookvPawnsEnding(int attacker, const position_t& pos, int *score, int *draw, int mover) {
    int defender = attacker ^ 1;
    int defenderMove = (mover != attacker);
    uint64 pawns = pos.pawns & pos.color[defender];
    while (pawns) {
        int pSq = popFirstBit(&pawns);
        int dkpDist = DISTANCE(pos.kpos[defender], pSq);
        int akDist, pDist, dkDist;
        int dDist;
        int qSq;
        int rDist;
        int rSq;
        // if we are too distant from our pawn its not drawish
        if (dkpDist - defenderMove > 2) continue;
        // find out how we stand in the race
        qSq = PAWN_PROMOTE(SQFILE(pSq), defender);
        akDist = DISTANCE(qSq, pos.kpos[attacker]);
        pDist = DISTANCE(pSq, qSq);
        dkDist = DISTANCE(pos.kpos[defender], qSq);
        dDist = pDist + dkDist - defenderMove;
        rSq = GetOnlyBit(pos.rooks & pos.color[attacker]);
        rDist = !(SQRANK(rSq) == SQRANK(qSq)) || (SQFILE(rSq) == SQFILE(qSq));
        // hey, we may have a draw here
        if (dDist < akDist + rDist) {
            int newDraw = PRETTY_DRAWISH + 2 * ((akDist + rDist) - dDist);
            *score = *score / 4; // each pawn effects score, so multiple pawns are good

            if (newDraw > *draw) *draw = newDraw;
        }
        // this is drawish, but many times lost, so lets be conservative
        else if (dkDist - defenderMove < akDist) {
            int newDraw = DRAWISH - 10 + 2 * ((akDist + rDist) - dDist);
            *score = *score / 2;
            if (newDraw > *draw) *draw = newDraw;
        }
    }
}

//SAMFIX does not do fortress with rook pawns correctly, perhaps can expand knight pawn fortresses
void DrawnQvREnding(int attacker, const position_t& pos, eval_info_t& ei, int *score, int *draw) {
    // remember, only the defender has rooks or pawns
    int defender = attacker ^ 1;
    uint64 pawns = pos.pawns;
    uint64 rRank = RankBB[SQRANK(GetOnlyBit(pos.rooks & pos.color[attacker]))];
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
        else if (qDist == 6 && (ei.atkpawns[defender] & pos.rooks)
            && (0 == (pos.pawns & rRank))
            && DISTANCE(pSq, pos.kpos[defender]) == 1 &&
            IN_FRONT(SQRANK(pos.kpos[attacker]), SQRANK(pSq), defender)) {
            *draw = SUPER_DRAWISH;
            *score /= 4;
        }
    }
}
//does not inlude bishop vs. bishop
void MinorPawnvMinor(int attacker, const position_t& pos, eval_info_t& ei, int *draw) { //drawish if in front of pawn and knight safe
    const int pawnSq = GetOnlyBit(pos.pawns & pos.color[attacker]);
    const int defender = attacker ^ 1;
    if (abs(SQFILE(pos.kpos[defender]) - SQFILE(pawnSq)) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(pawnSq), attacker)) {
        const uint64 dKnight = (pos.knights & pos.color[defender]);
        if (dKnight == 0) {
            *draw = VERY_DRAWISH;
            if (SHOW_EVAL) PrintOutput() << "info string drawish knight pawn vs. minor endgame\n";
        }
        else if ((ei.atkall[attacker] & dKnight) == 0) {
            const int knightSq = GetOnlyBit(dKnight);
            const uint64 knightSquares = KnightMoves[knightSq];
            if (((~ei.atkall[attacker]) & knightSquares) != 0) {
                *draw = VERY_DRAWISH;
                if (SHOW_EVAL) PrintOutput() << "info string drawish minor + pawn vs. knight endgame\n";
            }
        }
    }
}
//does not inlude bishop vs. bishop
void MinorPawnPawnvMinorPawn(int attacker, const position_t& pos, eval_info_t& ei, int *draw) { //drawish if in front of pawn and knight safe
    const uint64 passed = ei.pawn_entry->passedbits & pos.color[attacker];
    const int defender = attacker ^ 1;
    if (passed == 0) {
        const int pawnSq = GetOnlyBit(pos.pawns & pos.color[defender]);
        if (abs(SQFILE(pos.kpos[defender]) - SQFILE(pawnSq)) <= 1 && IN_FRONT(SQRANK(pos.kpos[defender]), SQRANK(pawnSq), attacker)) {
            const uint64 dKnight = (pos.knights & pos.color[defender]);
            if (dKnight == 0) {
                *draw = DRAWISH;
                if (SHOW_EVAL) PrintOutput() << "info string drawish knight + 2 pawns vs. minor + pawn endgame\n";
            }
            else if ((ei.atkall[attacker] & dKnight) == 0) {
                const int knightSq = GetOnlyBit(dKnight);
                const uint64 knightSquares = KnightMoves[knightSq];
                if (((~ei.atkall[attacker]) & knightSquares) != 0) {
                    *draw = DRAWISH;
                    if (SHOW_EVAL) PrintOutput() << "info string drawish minor + 2 pawns vs. knight + pawn endgame\n";
                }
            }
        }
        else *draw += 10;
    }
}

void DrawnRookPawnvBishop(int attacker, const position_t& pos, int *draw) {
    //this is usually drawn if it is a rook pawn past the 4th row, and king is in front
    const uint64 pawn = pos.pawns;
    if (pawn & (FileABB | FileHBB)) { // if it is a rook pawn
        const int dpSq = getFirstBit(pawn);
        if (Q_DIST(dpSq, attacker) <= 3) { // if it is past the 4th row
            const int defender = attacker ^ 1;
            const int queenSquare = PAWN_PROMOTE(dpSq, attacker);
            if (DISTANCE(queenSquare, pos.kpos[defender]) <= 1) { // if king near queening square
                if (((pos.bishops & WhiteSquaresBB) == 0) != ((BitMask[queenSquare] & WhiteSquaresBB) == 0)) { // if right color bishop
                    if (SHOW_EVAL) PrintOutput() << "info string in RP v B endgame\n";
                    *draw = SUPER_DRAWISH;
                }
            }
        }
        else if (SHOW_EVAL) PrintOutput() << "info string dist: " << Q_DIST(dpSq, attacker) << "\n";
    }
}
void TwoKnightsvPawn(int attacker, const position_t& pos, int *draw) {
    uint64 pawnBB = pos.pawns;
    int TroitzkyLine[] = { 4, 6, 5, 4, 4, 5, 6, 4 };
    int drawish = 1;
    do {
        int pSq = popFirstBit(&pawnBB);
        int pRank = PAWN_RANK(pSq, attacker ^ 1);
        int pFile = SQFILE(pSq);
        drawish += MAX(0, TroitzkyLine[pFile] - pRank);
        if (SHOW_EVAL) PrintOutput() << "info string rank " << pRank + 1 << " tline " << TroitzkyLine[pFile] + 1 << "\n";
    } while (pawnBB);
    if (SHOW_EVAL) PrintOutput() << "info string drawish " << drawish << "\n";
    *draw = *draw / drawish;
}
void QueenPawnvQueen(int attacker, const position_t& pos, int *draw) {
    uint64 const pawnBB = pos.pawns & pos.color[attacker];
    if (pawnBB & (FileABB | FileBBB | FileGBB | FileHBB)) {
        int pawnSq = GetOnlyBit(pos.pawns & pos.color[attacker]);
        int promotionSq = PAWN_PROMOTE(pawnSq, attacker);
        int rank = PAWN_RANK(pawnSq, attacker);
        int qDist = 7 - rank;
        int oppking = pos.kpos[attacker ^ 1];
        *draw += (rank <= 6) ? 30 : 10;
        int eDist = DISTANCE(oppking, promotionSq);
        if (eDist - 1 <= qDist) {
            *draw += 35;
        }
    }
}
static const int LOCK_DRAW = 90;
static const int PARTIAL_LOCK_DRAW = 30;
static const int MIN_DRAW_EFFECT = 10;

inline uint64 shiftExpand(const uint64 pawns, const int color) {
    return (pawnAttackBB(pawns, color) | (ShiftPtr[color](pawns, 8)));
}
void BreakPawnReduction(const int color, const uint64 stopSquares, const position_t& pos, eval_info_t& ei, const bool fullLock, int *draw) {
    //    const uint64 passed = ei.pawn_entry->passedbits & pos.color[color];
    const uint64 targets = (ShiftPtr[color ^ 1](ei.potentialPawnAttack[color ^ 1], 8));
    const uint64 stoppers = (((stopSquares | pos.color[color ^ 1]) & ~ei.potentialPawnAttack[color]) | (pos.pawns & targets));
    const uint64 repressed = (*FillPtr[color ^ 1])(stoppers);
    const uint64 potBreakPawns = ~repressed;
    const uint64 pieces = pos.color[color] & ~(pos.pawns | pos.kings);
    const int complications = bitCnt((ei.pawn_entry->passedbits & pos.color[color]) | pieces);
    if (SHOW_EVAL) {
        PrintOutput() << "info string targets\n";
        PrintBitBoard(targets);
        PrintOutput() << "info string stoppers\n";
        PrintBitBoard(stoppers);
        PrintOutput() << "info string potbreak Pawns\n";
        PrintBitBoard(potBreakPawns);
    }
    int numBreaks = bitCnt(ei.pawns[color] & potBreakPawns & targets);
    if (numBreaks) numBreaks += complications;
    int score = fullLock ? MAX(PARTIAL_LOCK_DRAW, LOCK_DRAW * 2 / (numBreaks + 2))
        : MAX(MIN_DRAW_EFFECT, PARTIAL_LOCK_DRAW * 2 / (numBreaks + 2));
    //minor decrements
    score -= 10 * complications;
    *draw += MAX(0, score);
    if (SHOW_EVAL) {
        if (fullLock) PrintOutput() << "info string full lock";
        else PrintOutput() << "info string partial lock";

        PrintOutput() << "info string num breaks ";
        PrintOutput() << numBreaks;
        PrintOutput() << "info string score ";
        PrintOutput() << score;
    }
}

void LockedPawns(int attacker, const position_t& pos, eval_info_t& ei, int *draw) {
    const uint64 passed = ei.pawn_entry->passedbits & pos.color[attacker];
    if (MaxOneBit(passed) == true) {
        const int defender = attacker ^ 1;
        const uint64 kingStopper = (passed != 0) ? 0 : (ei.atkkings[defender] | (pos.kings & pos.color[defender]));
        const uint64 pieceStopper = kingStopper | ei.atkbishops[defender] | ei.atkknights[defender] | ei.atkrooks[defender] | ei.atkqueens[defender];
        uint64 squashSquares = doublePawnAttackBB(ei.pawns[defender], defender);
        squashSquares |= (ei.atkpawns[defender] & ~ei.atkall[attacker]);
        squashSquares &= boardSide[attacker];
        //        squashSquares |= (*ShiftPtr[defender])(squashSquares & ~ei.potentialPawnAttack[attacker], 8);
        const uint64 stopSquares = (squashSquares | (pieceStopper & ~ei.atkall[attacker]));
        if (SHOW_EVAL) {
            PrintOutput() << "info string squash Squares\n";
            PrintBitBoard(squashSquares);
            PrintOutput() << "info string stop Squares\n";
            PrintBitBoard(stopSquares);
        }
        const uint64 relevantPawns = ei.pawns[defender] & (ei.atkall[defender] | ~ei.atkall[attacker]);
        const uint64 pawnGuards = pawnAttackBB(relevantPawns, defender);
        const uint64 selfBlocked = ei.pawns[attacker] & (ShiftPtr[defender](relevantPawns | stopSquares, 8));
        const uint64 blocked = selfBlocked | pawnGuards | kingStopper;
        const  uint64 enemyKing = pos.kings & pos.color[attacker];
        if (defender == WHITE) {
            const uint64 wall3missing = Rank3BB & ~blocked;
            const uint64 wall4target = shiftExpand(wall3missing, defender);
            const uint64 wall4missing = wall4target & ~blocked;
            const uint64 wall5target = shiftExpand(wall4missing, defender);
            const uint64 wall5missing = wall5target & ~blocked;
            const uint64 wall6target = shiftExpand(wall5missing, defender);
            const uint64 wall6missing = wall6target & ~blocked;
            if (wall6missing == 0) {
                if ((enemyKing & (Rank1BB | Rank2BB | wall3missing | wall4missing | wall5missing | wall6missing)) == 0) {
                    if (SHOW_EVAL) PrintOutput() << "info string WHITE DRAW WALL\n";
                    BreakPawnReduction(attacker, stopSquares, pos, ei, true, draw);
                }
                else if (SHOW_EVAL) PrintOutput() << "info string WHITE DRAW WALL king penetrated\n";
            }
            else if ((MaxOneBit(wall4missing) && (enemyKing & (Rank1BB | Rank2BB | wall3missing | wall4missing)) == 0) ||
                (MaxOneBit(wall5missing) && (enemyKing & (Rank1BB | Rank2BB | wall3missing | wall4missing | wall5missing)) == 0) ||
                (MaxOneBit(wall6missing) && (enemyKing & (Rank1BB | Rank2BB | wall3missing | wall4missing | wall5missing | wall6missing)) == 0)) {
                if (SHOW_EVAL) PrintOutput() << "info string WHITE DRAW WALL partial wall\n";
                BreakPawnReduction(attacker, stopSquares, pos, ei, false, draw);
            }
        }
        else {
            const uint64 wall6missing = Rank6BB & ~blocked;
            if (SHOW_EVAL) {
                PrintOutput() << "info string wall6missing\n";
                PrintBitBoard(wall6missing);
            }
            const uint64 wall5target = shiftExpand(wall6missing, defender);
            const uint64 wall5missing = wall5target & ~blocked;
            if (SHOW_EVAL) {
                PrintOutput() << "info string wall5missing\n";
                PrintBitBoard(wall5missing);
            }
            const uint64 wall4target = shiftExpand(wall5missing, defender);
            const uint64 wall4missing = wall4target & ~blocked;
            if (SHOW_EVAL) {
                PrintOutput() << "info string wall4missing\n";
                PrintBitBoard(wall4missing);
            }
            const uint64 wall3target = shiftExpand(wall4missing, defender);
            const uint64 wall3missing = wall3target & ~blocked;
            if (SHOW_EVAL) {
                PrintOutput() << "info string wall3missing\n";
                PrintBitBoard(wall3missing);
            }
            if (wall3missing == 0) {
                if ((enemyKing & (Rank8BB | Rank7BB | wall3missing | wall4missing | wall5missing | wall6missing)) == 0) {
                    if (SHOW_EVAL) PrintOutput() << "info string BLACK DRAW WALL\n";
                    BreakPawnReduction(attacker, stopSquares, pos, ei, true, draw);
                }
                else if (SHOW_EVAL) PrintOutput() << "info string BLACK DRAW WALL king penetrated\n";
            }
            else if ((MaxOneBit(wall5missing) && (enemyKing & (Rank8BB | Rank7BB | wall6missing | wall5missing)) == 0) ||
                (MaxOneBit(wall4missing) && (enemyKing & (Rank8BB | Rank7BB | wall6missing | wall5missing | wall4missing)) == 0) ||
                (MaxOneBit(wall3missing) && (enemyKing & (Rank8BB | Rank7BB | wall6missing | wall5missing | wall4missing | wall6missing)) == 0)) {
                if (SHOW_EVAL) PrintOutput() << "info string BLACK DRAW WALL partial wall\n";
                BreakPawnReduction(attacker, stopSquares, pos, ei, false, draw);
            }
        }
    }
}

void evalEndgame(const int attacker, const position_t& pos, eval_info_t& ei, int *score, int *draw) {
    switch (ei.endFlags[attacker]) {
    case NoEnd:
        break;
    case SinglePawnEnd: // single pawn endgames
        SinglePawnEnding(attacker, pos, ei, score, draw, pos.side);
        DrawnRookPawn(attacker, pos, draw, pos.side);
        break;
    case RvREnd: // rook endings
        RookEnding<false>(attacker, pos, ei, score, draw);
        if (SHOW_EVAL) PrintOutput() << "info string each side has a rook endgame (multiple attaker pawns)\n";
        break;
    case BvBEnd: // bishop endings without enough pawns to lock things
        if (SHOW_EVAL) PrintOutput() << "info string each side has a bishop endgame (cannot be locked)\n";
        BishopEnding(attacker, pos, ei, score, draw);
        DrawnRookPawn(attacker, pos, draw, pos.side);
        break;
    case RvPEnd: // rook versus pawns endings
        RookvPawnsEnding(attacker, pos, score, draw, pos.side);
        break;
    case NPEnd: // knight + pawn
        DrawnNP(attacker, pos, ei, draw);
        break;
    case QvREnd: // Q vs. Rook + Pawn(s)
        DrawnQvREnding(attacker, pos, ei, score, draw);
        break;
    case BNEnd: // mate with knight and bishop
        KnightBishopMate(attacker, pos, score, draw);
        break;
    case RBvRB:
        RookBishopEnding(attacker, pos, ei, draw);
        break;
    case BPEnd: //pawn + bishop endgame
        DrawnRookPawn(attacker, pos, draw, pos.side);
        if (SHOW_EVAL) PrintOutput() << "info string bishop and pawn vs. endgame\n";
        break;
    case BEnd: //pawn endgames with more than 1 pawn
        DrawnRookPawn(attacker, pos, draw, pos.side);
        PawnEnding(attacker, pos, draw, pos.side);
        if (SHOW_EVAL) PrintOutput() << "info string more than one pawn endgame\n";
        break;
    case RvNEnd: //rook against knight
        RookvKnight(attacker, pos, score, draw);
        break;
    case QvPEnd: //queen against pawn
        QueenvPawn(attacker, pos, ei, score, draw);
        break;
    case NoPawnEnd:
        MateNoPawn(attacker, pos, score);
        break;
    case RPvBEnd: //rook against bishop
        DrawnRookPawnvBishop(attacker, pos, draw);
        break;
    case BPvNEnd: //bishop and pawn vs knight
        MinorPawnvMinor(attacker, pos, ei, draw);
        DrawnRookPawn(attacker, pos, draw, pos.side);
        if (SHOW_EVAL) PrintOutput() << "info string bishop and pawn vs. knight endgame\n";
        break;
    case BPvMEnd: //knight and pawn vs bishop or knight
        MinorPawnvMinor(attacker, pos, ei, draw);
        if (SHOW_EVAL) PrintOutput() << "info string knight and pawn vs. minor endgame\n";
        break;
    case MPPvMPEnd:
        MinorPawnPawnvMinorPawn(attacker, pos, ei, draw);
        break;
    case BLockEnd:
        PawnEnding(attacker, pos, draw, pos.side);
        DrawnRookPawn(attacker, pos, draw, pos.side);
        LockedPawns(attacker, pos, ei, draw);
        if (SHOW_EVAL) PrintOutput() << "info string more than one pawn endgame (might be locked)\n";
        break;
    case BvBLockEnd: // bishop endings WITH enough pawns to lock things
        if (SHOW_EVAL) PrintOutput() << "info string each side has a bishop endgame that might be locked\n";
        BishopEnding(attacker, pos, ei, score, draw);
        DrawnRookPawn(attacker, pos, draw, pos.side);
        LockedPawns(attacker, pos, ei, draw);
        break;
    case MinorLock: // bishop endings WITH enough pawns to lock things
        if (SHOW_EVAL) PrintOutput() << "info string stronger side has b/n weaker side has b/n/r (not bvb) and might be locked\n";
        LockedPawns(attacker, pos, ei, draw);
        break;
    case RPvREnd: // bishop endings WITH enough pawns to lock things
        RookEnding<true>(attacker, pos, ei, score, draw);
        if (SHOW_EVAL) PrintOutput() << "info string each side has a rook endgame (single attaker pawn)\n";
        break;
    case QPvQEnd: // queen and pawn vs. queen endgame
        QueenPawnvQueen(attacker, pos, draw);
        if (SHOW_EVAL) PrintOutput() << "info string queen and pawn vs. queen\n";
    case NNvPEnd: //two knights against pawn(s)
        TwoKnightsvPawn(attacker, pos, draw);
        MateNoPawn(attacker, pos, score);
        break;
    }
}