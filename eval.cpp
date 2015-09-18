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
#include "attacks.h"
#include "utils.h"
#include "bitutils.h"
#include "trans.h"
#include "eval.h"
#include "material.h"
#include "threads.h"

const uint64 CenterRanks = (Rank3BB | Rank4BB | Rank5BB | Rank6BB); 
//some castling comments
const int KSC[2] = { WCKS, BCKS };
const int QSC[2] = { WCQS, BCQS };
const int Castle[2] = { WCQS + WCKS, BCQS + BCKS };
#define KS(c)			(WCKS+c*3)
#define QS(c)			(WCQS+c*6)
// F1 | G1, F8 | G8
const uint64 KCSQ[2] = { (0x0000000000000020ULL | 0x0000000000000040ULL), (0x2000000000000000ULL | 0x4000000000000000ULL) };
// C1 | D1, C8 | D8
const uint64 QCSQ[2] = { (0x0000000000000004ULL | 0x0000000000000008ULL), (0x0400000000000000ULL | 0x0800000000000000ULL) };
const int KScastleTo[2] = { g1, g8 };
const int QScastleTo[2] = { c1, c8 };

#define MINOR_SHIELD 12
#define TEMPO_OPEN 20 
#define TEMPO_END 10

class Personality {
public:

    static const int tempo_open = TEMPO_OPEN;
    static const int tempo_end = TEMPO_END;
};
static const Personality personality;
#define personality(thread) personality

//king safety stuff
#define PARTIAL_ATTACK 0 //0 means don't count, otherwise multiplies attack by 5
#define MATE_CODE 10 // mate threat
static const uint64 bewareTrapped[2] = { (Rank8BB | Rank7BB | Rank6BB ), (Rank1BB | Rank2BB | Rank3BB ) };

#define TRAPPED_PENALTY 125 
#define KING_ATT_W 4
#define SB1 9
#define SB2 (SB1+9)
#define SB3 (SB2+8)
#define SB4 (SB3+8)
#define SB5 (SB4+7)
#define SB6 (SB5+7)
#define SB7 (SB6+7)
#define SB8 (SB7+7)
#define SB9 (SB8+7)
#define SB10 (SB9+7)
#define SB11 (SB10+4)
#define SB12 (SB11+3)
#define SB13 (SB12+2)
#define SB14 (SB13+2)
const EvalScore ShelterBonus[] = { COMP(0,0), COMP(SB1,0), COMP(SB2,0), COMP(SB3,0), COMP(SB4,0), COMP(SB5,0), COMP(SB6,0), COMP(SB7,0), 
									COMP(SB8,0), COMP(SB9,0), COMP(SB10,0), COMP(SB11,0), COMP(SB12,0), COMP(SB13,0), COMP(SB14,0)};

EvalScore KnightMobArray[9];
EvalScore BishopMobArray[7 * 2 + 1];
EvalScore XrayBishopMobArray[7 * 2 + 1];
EvalScore RookMobArray[7 * 2 + 1];
EvalScore XrayRookMobArray[7 * 2 + 1];
EvalScore QueenMobArray[7 * 4 + 1];
EvalScore XrayQueenMobArray[7 * 4 + 1];

// queen specific evaluation
const int MidgameQueen7th = 10;
const int EndgameQueen7th = 20;

// rook specific evaluation
const int MidgameRookPawnPressure = 5;
const int EndgameRookPawnPressure = 9;
const int MidgameRook7th = 15;
const int EndgameRook7th = 30;
//minor specific evaluation
static const int MINOR_BLOCKER = 8;
//bishop specific
#define OB_WEIGHT 12
const int MidgameBishopPawnPressure = 5; //TODO make bishop pawn pressure count xrays when coming from behind
const int EndgameBishopPawnPressure = 9;

// knight specific evaluation
const int MidgameKnightPawnPressure = 5;
const int EndgameKnightPawnPressure = 9;
#define KNIGHT_PWIDTH 7

// king specific evaluation
const int kingAttackWeakness[8] = { 0, 22, 12, 5, 3, 1, 0, 0 };

//pawn specific evaluttion
static const int MSAFE_PPUSH = 4;
static const int ESAFE_PPUSH = 5;
static const int MPPUSH_THREAT = 8;
static const int EPPUSH_THREAT = 8;
#define DOUBLED_O 2
#define DOUBLED_E 4
#define DOUBLED_OPEN_O 2
#define DOUBLE_OPEN_E 4

#define ISOLATED_O 4
#define ISOLATED_E 4

#define TARGET_O 5 //in addition to being a target
#define TARGET_E 8

#define TARGET_OPEN_O 10
#define TARGET_OPEN_E 12

#define DOUBLE_TARGET_O 2
#define DOUBLE_TARGET_E 4
#define DOUBLE_TARGET_OPEN_O 4
#define DOUBLE_TARGET_OPEN_E 6

//passed pawns
const int UnstoppablePassedPawn = 700;
const int EndgamePassedMin = 10;
const int EndgamePassedMax = 60;
const int MidgamePassedMin = 20;
const int MidgamePassedMax = 120;
const int MidgameCandidatePawnMin = 10;
const int MidgameCandidatePawnMax = 90;
const int EndgameCandidatePawnMin = 5;
const int EndgameCandidatePawnMax = 45;
const int ProtectedPasser = 10;
const int DuoPawnPasser = 15;
const int kingDistImp[8] = { 0, 20, 40, 55, 65, 70, 70, 70 }; //best if this cannot get higher than EndgamePassedMax
const int PathFreeFriendPasser = 10;
const int DefendedByPawnPasser = 10;
const int PathFreeNotAttackedDefAllPasser = 80;
const int PathFreeNotAttackedDefPasser = 70;
const int PathFreeAttackedDefAllPasser = 60;
const int PathFreeAttackedDefPasser = 40;
const int PathNotFreeAttackedDefPasser = 10;
const int PassedPawnAttackerDistance = 20;
const int PassedPawnDefenderDistance = 40;

// king safety
const int QueenAttackValue = 5;
const int RookAttackValue = 3;
const int BishopAttackValue = 2;
const int KnightAttackValue = 2;

const int QueenSafeContactCheckValue = 4; 
const int QueenSafeCheckValue = 3; 
const int RookSafeCheckValue = 3; 
const int BishopSafeCheckValue = 1;
const int KnightSafeCheckValue = 2; 
const int DiscoveredCheckValue = 3;

// piece attacks
const int QueenAttacked = 4; 
const int RookAttacked = 3; 
const int BishopAttacked = 2; 
const int KnightAttacked = 2;

const int QueenAttackPower = 1; 
const int RookAttackPower = 2; 
const int BishopAttackPower = 3; 
const int KnightAttackPower = 3; 
const int PawnAttackPower = 5; 

const int PieceAttackMulMid = 4; 
const int PieceAttackMulEnd = 3; 

const EvalScore ThreatBonus[] = { COMP(8,8), COMP(88,88), COMP(108,108), COMP(128,128), COMP(138,138), COMP(148,148), COMP(178,178), COMP(188,188) };

inline int scale(int smax, int sr) {
    static const int sbonus[8] = { 0, 0, 0, 13, 34, 77, 128, 0 };
    return ((((smax))*sbonus[sr]) / sbonus[6]);
}

// this is only called when there are more than 1 queen for a side
int computeMaterial(eval_info_t& ei) {
    int whiteM = ei.MLindex[WHITE];
    int blackM = ei.MLindex[BLACK];
    int wq, bq, wr, br, wb, bb, wn, bn;
    int score = 0;
    ei.phase = 0;

    wq = whiteM / MLQ;
    whiteM -= wq*MLQ;
    bq = blackM / MLQ;
    blackM -= bq*MLQ;
    score += (wq - bq) * ((QueenValueMid1 + QueenValueMid2) / 2);
    ei.phase += (wq + bq) * 6;

    wr = whiteM / MLR;
    whiteM -= wr*MLR;
    br = blackM / MLR;
    blackM -= br*MLR;
    score += (wr - br) * ((RookValueMid1 + RookValueMid2) / 2);
    ei.phase += (wr + br) * 3;

    wb = whiteM / MLB;
    whiteM -= wb*MLB;
    bb = blackM / MLB;
    blackM -= bb*MLB;
    score += (wb - bb) * ((BishopValueMid1 + BishopValueMid2) / 2);
    ei.phase += (wb + bb);

    wn = whiteM / MLN;
    whiteM -= wn*MLN;
    bn = blackM / MLN;
    blackM -= bn*MLN;
    score += (wn - bn) * ((KnightValueMid1 + KnightValueMid2) / 2);
    ei.phase += (wn + bn);

    score += (whiteM - blackM) * ((PawnValueMid1 + PawnValueMid2) / 2);

    if (ei.phase > 32) ei.phase = 32;
    ei.draw[WHITE] = 0;
    ei.draw[BLACK] = 0;
	ei.endFlags[WHITE] = NoEnd;
	ei.endFlags[BLACK] = NoEnd;
    return (score);
}

void initPawnEvalByColor(const position_t& pos, eval_info_t& ei, const int color) {
    uint64 temp64;

    ei.pawns[color] = (pos.pawns & pos.color[color]);
	temp64 = pawnAttackBB(ei.pawns[color], color);
    ei.atkpawns[color] = temp64;
    ei.atkcntpcs[color ^ 1] += bitCnt(temp64 & ei.kingadj[color ^ 1]);
    ei.potentialPawnAttack[color] = (*FillPtr2[color])(temp64);
}

void evalShelter(const int color, eval_info_t& ei, const position_t& pos) {
    uint64 shelter, indirectShelter, doubledShelter;
    uint64 pawns = ei.pawns[color];

    // king shelter in your current position
    shelter = kingShelter[color][pos.kpos[color]] & (pawns | (pos.pawns & FileBB[SQFILE(pos.kpos[color])]  )); //opponent pawn in front of king can act as a shelter
    indirectShelter = kingIndirectShelter[color][pos.kpos[color]] & pawns;
    // doubled pawns are a bit redundant as shelter
    doubledShelter = indirectShelter & (*FillPtr[color])(indirectShelter);
    indirectShelter ^= doubledShelter;
    shelter ^= doubledShelter; 
    // remember, shelter is also indirect shelter so it is counted twice
    ei.pawn_entry->shelter[color] = bitCnt(shelter) + bitCnt(indirectShelter);
    // now take into account possible castling, and the associated safety SAM122109
    if (pos.posStore.castle & KS(color)) {
        shelter = kingShelter[color][KScastleTo[color]] & pawns;
        indirectShelter = kingIndirectShelter[color][KScastleTo[color]] & pawns;
        // doubled pawns are a bit redundant as shelter
        doubledShelter = indirectShelter & (*FillPtr[color])(indirectShelter);
        indirectShelter ^= doubledShelter;
        shelter ^= doubledShelter; 
        // remember, shelter is also indirect shelter so it is counted twice
        ei.pawn_entry->kshelter[color] = bitCnt(shelter) + bitCnt(indirectShelter);
    }
    else ei.pawn_entry->kshelter[color] = 0;
    if (pos.posStore.castle&QS(color)) {
        shelter = kingShelter[color][QScastleTo[color]] & pawns;
        indirectShelter = kingIndirectShelter[color][QScastleTo[color]] & pawns;
        // doubled pawns are a bit redundant as shelter
        doubledShelter = indirectShelter & (*FillPtr[color])(indirectShelter);
        indirectShelter ^= doubledShelter;
        shelter ^= doubledShelter; 
        // remember, shelter is also indirect shelter so it is counted twice
        ei.pawn_entry->qshelter[color] = bitCnt(shelter) + bitCnt(indirectShelter);
    }
    else ei.pawn_entry->qshelter[color] = 0;
}

int BBwidth(const uint64 BB) {
    if (BB == 0) return 0;
    int left = FileA;
    int right = FileH;
    while ((BB & FileMask[left]) == 0) left++;
    while ((BB & FileMask[right]) == 0) right--;
    return right - left;
}

void evalPawnsByColor(const position_t& pos, eval_info_t& ei, int& mid_score, int& end_score, const int color) {
    static const int weakFile[8] = { 0, 2, 3, 3, 3, 3, 2, 0 };
    static const int weakRank[8] = { 0, 0, 2, 4, 6, 8, 10, 0 };
    uint64 openBitMap, isolatedBitMap, doubledBitMap, passedBitMap, backwardBitMap, targetBitMap, temp64;

    int count, sq, rank;
    const int enemy = color ^ 1;
	const uint64 advanceSquares = (*ShiftPtr[color])(ei.pawns[color], 8);
    openBitMap = ei.pawns[color] & ~((*FillPtr2[enemy])((*ShiftPtr[enemy])(pos.pawns, 8)));
    doubledBitMap = ei.pawns[color] & (*FillPtr[enemy])(ei.pawns[color]); //the least advanced ones are considered doubled
    isolatedBitMap = ei.pawns[color] & ~((*FillPtr2[color ^ 1])(ei.potentialPawnAttack[color]));
	backwardBitMap = (*ShiftPtr[enemy])((advanceSquares & (ei.potentialPawnAttack[enemy] | ei.pawns[enemy])
		& ~ei.potentialPawnAttack[color]), 8) & ~(isolatedBitMap | ei.atkpawns[enemy]);
	passedBitMap = openBitMap & ~ei.potentialPawnAttack[enemy];
	const uint64 adjacentPawns = ei.pawns[color] & adjacent(ei.pawns[color]);
	const uint64 guarded = ei.pawns[color] & ei.atkpawns[color];
	const uint64 safePawns = adjacentPawns | guarded;
	const uint64 unsafePawns = ei.pawns[color] & ~safePawns;
//	const uint64 squashSquares = (doublePawnAttackBB(ei.pawns[enemy], enemy) & ~ei.atkpawns[color]) | (advanceSquares & pos.pawns);
	uint64 connected = safePawns & pastThirdRank[color];
	const uint64 blockedBB = advanceSquares & (pos.pawns | (doublePawnAttackBB(ei.pawns[enemy], enemy) & ~ei.atkpawns[color]));
	ei.pawn_entry->locked += bitCnt(blockedBB); //TODO consider pawns that are blocked after 1 move

	while (connected) {
		const int ConnectBonus[] = { 0, 0, 0, 4, 10, 30, 60, 90 };

		sq = popFirstBit(&connected);
//		bool blocked = (BitMask[sq + PAWN_MOVE_INC(color)] & squashSquares) != 0;
		bool blocked = (BitMask[sq + PAWN_MOVE_INC(color)] & blockedBB) != 0;
		int advanced = PAWN_RANK(sq, color);
		int score = ((adjacentPawns & BitMask[sq]) != 0) ? ConnectBonus[advanced] : (ConnectBonus[advanced + 1] - ConnectBonus[advanced]) / 2;
		if (blocked) score /= 2;
		mid_score += score;
		end_score += (score * 2) / 3;
		if (SHOW_EVAL) PrintOutput() << "info string connected " << (char)(SQFILE(sq) + 'a') << (char)('1' + SQRANK(sq)) << " score " << score << "\n";
		if (SHOW_EVAL && blocked) PrintOutput() << "info string blocked\n";
	}
    int enemyKing = pos.kpos[color ^ 1];
    //backward pawns are addressed here as are all other pawn weaknesses to some extent
    //these are pawns that are not guarded, and are not adjacent to other pawns
	temp64 = unsafePawns;
    while (temp64) {
        uint64 openPawn, sqBitMask;
        int sqPenalty;
        sq = popFirstBit(&temp64);
        sqBitMask = BitMask[sq];
        openPawn = openBitMap & sqBitMask;
        sqPenalty = weakFile[SQFILE(sq)] + weakRank[PAWN_RANK(sq, color)]; //TODO precale 64 entries
        mid_score -= sqPenalty;	//penalty for being weak in middlegame
        end_score -= kingAttackWeakness[DISTANCE(sq, enemyKing)]; //end endgame, king attacks is best measure of vulnerability
        if (openPawn) mid_score -= sqPenalty;
    }
    targetBitMap = isolatedBitMap;
    //any backward pawn not easily protect by its own pawns is a target
    //TODO consider calculating exactly how many moves to protect and defend including double moves
    //TODO consider increasing distance of defender by 1
    while (backwardBitMap) { //by definition this is not on the 7th rank
        sq = popFirstBit(&backwardBitMap);
        bool permWeak =
            (BitMask[sq + PAWN_MOVE_INC(color)] & (ei.atkpawns[enemy])) || //enemy adjacent pawn within 1
            ((BitMask[sq + PAWN_MOVE_INC(color) * 2] & ei.atkpawns[enemy]) && //enemy adjacent pawn within 2
            (BitMask[sq + PAWN_MOVE_INC(color)] & ei.atkpawns[color]) == 0); //support pawn within 1
        if (permWeak) targetBitMap |= BitMask[sq];
    }
    //lets look through all the passed pawns and give them their static bonuses
    //TODO consider moving some king distance stuff in here
    temp64 = ei.pawns[color] & openBitMap & ~passedBitMap;

    while (temp64) { //SAM TODO examine for speed improvements
        sq = popFirstBit(&temp64);
        if (bitCnt((*FillPtr2[color])(PawnCaps[sq][color]) & ei.pawns[enemy])
            <= bitCnt((*FillPtr2[enemy])(PawnCaps[sq + PAWN_MOVE_INC(color)][enemy]) & ei.pawns[color]) &&
            bitCnt(PawnCaps[sq][color] & ei.pawns[enemy])
            <= bitCnt(PawnCaps[sq][enemy] & ei.pawns[color])) {
            rank = PAWN_RANK(sq, color);

            mid_score += MidgameCandidatePawnMin + scale(MidgameCandidatePawnMax, rank);
            end_score += EndgameCandidatePawnMin + scale(EndgameCandidatePawnMax, rank);
        }
    }
    if (doubledBitMap) {
        count = bitCnt(doubledBitMap);
        mid_score -= count * DOUBLED_O;
        end_score -= count * DOUBLED_E;
        if (doubledBitMap & openBitMap) {
            count = bitCnt(doubledBitMap & openBitMap);
            mid_score -= count * DOUBLED_OPEN_O;
            end_score -= count * DOUBLE_OPEN_E;
        }
    }

    if (targetBitMap) { //isolated or vulnerable backward pawns
        count = bitCnt(targetBitMap);
        mid_score -= count * TARGET_O;
        end_score -= count * TARGET_E;
        uint64 RFileTargetBitMap = targetBitMap & (FileABB | FileHBB);
        if (RFileTargetBitMap) {  //target pawns on A or H file pawns are not as bad
            count = bitCnt(RFileTargetBitMap);
            mid_score += count * TARGET_O / 2;
            end_score += count * TARGET_E / 2;
            if (RFileTargetBitMap & openBitMap) {
                count = bitCnt(RFileTargetBitMap & openBitMap);
                mid_score += count * TARGET_OPEN_O / 3;
                end_score += count * TARGET_OPEN_E / 3;
            }
        }
		if (targetBitMap & openBitMap) {
            count = bitCnt(targetBitMap & openBitMap);
            mid_score -= count * TARGET_OPEN_O;
            end_score -= count * TARGET_OPEN_E;
        }
        if (targetBitMap & doubledBitMap) {
            count = bitCnt(doubledBitMap & targetBitMap);
            mid_score -= count * DOUBLE_TARGET_O;
            end_score -= count * DOUBLE_TARGET_E;
            if (targetBitMap & doubledBitMap & openBitMap) {
                count = bitCnt(doubledBitMap & targetBitMap & openBitMap);//all isolated are also backward
                mid_score -= count * DOUBLE_TARGET_OPEN_O;
                end_score -= count * DOUBLE_TARGET_OPEN_E;
            }
        }
    }
    evalShelter(color, ei, pos);
    ei.pawn_entry->passedbits |= passedBitMap;
}
inline int outpost(const position_t& pos, eval_info_t& ei, const int color, const int enemy, const int sq) {
    int outpostValue = OutpostValue[color][sq];
    if (BitMask[sq] & ei.atkpawns[color]) {
        if (!(pos.knights & pos.color[enemy]) && !((BitMask[sq] & WhiteSquaresBB) ?
            (pos.bishops & pos.color[enemy] & WhiteSquaresBB) : (pos.bishops & pos.color[enemy] & BlackSquaresBB)))
            outpostValue += outpostValue + outpostValue / 2;
        else outpostValue += outpostValue / 2;
    }
    return outpostValue;
}

void evalPawnPushes(const position_t& pos, eval_info_t& ei, const int color) {
	static const uint64 Rank4[] = { Rank4BB, Rank5BB };
	uint64 frontSquares = (*ShiftPtr[color])(ei.pawns[color], 8) & ~(pos.occupied)  & ~ei.atkpawns[color ^ 1];
	frontSquares |= (*ShiftPtr[color])(frontSquares, 8) & ~pos.occupied & Rank4[color] & ~ei.atkpawns[color ^ 1];
	uint64 safePawnMove = frontSquares & (ei.atkall[color] | ~(ei.atkall[color ^ 1]));
	const int numSafe = bitCnt(safePawnMove);
	ei.posScore[color] += ComposeEvalScore(MSAFE_PPUSH, ESAFE_PPUSH) * numSafe;
	uint64 attackMoves = pawnAttackBB(safePawnMove, color) & (pos.color[color ^ 1] & ~pos.pawns);
	if (attackMoves) {
		const int numAttacks = bitCnt(attackMoves);
		ei.posScore[color] += ComposeEvalScore(MPPUSH_THREAT, EPPUSH_THREAT) * numAttacks;
	}
}
void evalPieces(const position_t& pos, eval_info_t& ei, const int color) {
	uint64 pc_bits, temp64, katemp64, xtemp64, weak_sq_mask;
	int from, temp1;
	const int enemy = color ^ 1;
	uint64 mobileSquare;
	const uint64 notOwnColor = ~pos.color[color];
	uint64 notOwnSkeleton;
	int threatScore = 0;
	uint64 boardSkeleton = pos.pawns;
	const uint64 maybeTrapped = bewareTrapped[color] & ~ei.atkpawns[color];
	uint64 pawnTargets = ei.pawns[enemy] & ~ei.atkpawns[enemy];
	uint64 attackBlockers = ((pos.pawns | pos.kings) & pos.color[color]) | (ei.pawns[enemy] & ei.atkpawns[enemy]);

	if (0 == (pos.posStore.castle & Castle[color])) boardSkeleton |= (pos.kings & pos.color[color]);
	notOwnSkeleton = ~(boardSkeleton & pos.color[color]);

	ASSERT(colorIsOk(color));

	mobileSquare = ~ei.atkpawns[enemy] & notOwnColor;
	weak_sq_mask = WeakSqEnemyHalfBB[color] & ~ei.potentialPawnAttack[color];

	pc_bits = pos.knights & pos.color[color];
	while (pc_bits) {
		from = popFirstBit(&pc_bits);
		temp64 = KnightMoves[from];

		ei.atkknights[color] |= temp64;
		int lockedScore = ei.pawn_entry->locked * 2;
		ei.posScore[color] += ComposeEvalScore(lockedScore, lockedScore - ei.pawn_entry->pawnWidthBonus * KNIGHT_PWIDTH);
		if (temp64 & ei.kingzone[enemy]) ei.atkcntpcs[enemy] += (1 << 20) + bitCnt(temp64 & ei.kingzone[enemy]) + (KnightAttackValue << 10);
		uint64 pawnsPressured = temp64 & pawnTargets;
		if (pawnsPressured) {
			int numPawnsPressured = bitCnt(pawnsPressured);
			ei.posScore[color] += ComposeEvalScore(MidgameKnightPawnPressure, EndgameKnightPawnPressure) * numPawnsPressured;
		}
		uint64 safeMoves = temp64 & mobileSquare;
		temp1 = bitCnt(safeMoves);
		ei.posScore[color] += KnightMobArray[temp1];
		const uint64 fromMask = BitMask[from];
		if (fromMask & weak_sq_mask) {
			temp1 = outpost(pos, ei, color, enemy, from); //TODO convert to evalScore
			ei.posScore[color] += ComposeEvalScore(temp1, temp1);
		}
		if ((fromMask & boardSide[color]) && (BitMask[from + PAWN_MOVE_INC(color)] & pos.pawns)) ei.posScore[color] += ComposeEvalScore(MINOR_BLOCKER,0);
		if ((maybeTrapped & fromMask) && temp1 < 2) //trapped piece if you are in opponent area and you have very few safe moves
			ei.posScore[color] -= ComposeEvalScore(TRAPPED_PENALTY,0);
	}
	ei.kingatkbishops[color] = 0;
	pc_bits = pos.bishops & pos.color[color];
	if (pc_bits) {
		int SameColBishop = ((pc_bits & WhiteSquaresBB) == 0) ? 0 : bitCnt(WhiteSquaresBB & ei.pawns[color]);
		SameColBishop += ((pc_bits & ~WhiteSquaresBB) == 0) ? 0 : bitCnt(~WhiteSquaresBB & ei.pawns[color]);
		ei.posScore[color] -= SameColBishop * ComposeEvalScore(2,3);
		int bishopMult = MinTwoBits(pc_bits) ? 4 : 1;
		int lockedScore = ei.pawn_entry->locked * bishopMult;
		ei.posScore[color] -= ComposeEvalScore(lockedScore, lockedScore);
		do {
			from = popFirstBit(&pc_bits);
			temp64 = bishopAttacksBB(from, pos.occupied);
			katemp64 = bishopAttacksBB(from, pos.occupied & ~pos.queens);

			ei.kingatkbishops[color] |= katemp64;
			ei.atkbishops[color] |= temp64;
			if (katemp64 & ei.kingzone[enemy]) ei.atkcntpcs[enemy] += (1 << 20) + bitCnt(katemp64 & ei.kingadj[enemy]) * 2 + (BishopAttackValue << 10);
			uint64 safeMoves = temp64 & mobileSquare;
			temp1 = bitCnt(safeMoves);
			ei.posScore[color] += BishopMobArray[temp1];

			const uint64 fromMask = BitMask[from];
			if ((maybeTrapped & fromMask) && temp1 < 2) //trapped piece if you are in opponent area and you have very few safe moves
				ei.posScore[color] -= ComposeEvalScore(TRAPPED_PENALTY, 0);
			xtemp64 = bishopAttacksBB(from, boardSkeleton) & notOwnSkeleton;
			uint64 pawnsPressured = xtemp64 & pawnTargets;
			if (pawnsPressured) {
				int numPawnsPressured = bitCnt(pawnsPressured);
				ei.posScore[color] += ComposeEvalScore(MidgameBishopPawnPressure, EndgameBishopPawnPressure) * numPawnsPressured;
			}
			if (xtemp64 & notOwnColor & (pos.kings | pos.queens)) {
				threatScore += (BishopAttackPower * QueenAttacked) / 2;
			}
			if (xtemp64 & notOwnColor & pos.rooks) {
				threatScore += (BishopAttackPower * RookAttacked) / 2;
			}
			temp1 = bitCnt(xtemp64);
			ei.posScore[color] += XrayBishopMobArray[temp1];

			if (fromMask & weak_sq_mask) {
				temp1 = outpost(pos, ei, color, enemy, from) / 2;
				ei.posScore[color] += ComposeEvalScore(temp1,temp1);
			}
			if ((fromMask & boardSide[color]) && (BitMask[from + PAWN_MOVE_INC(color)] & pos.pawns)) ei.posScore[color] += ComposeEvalScore(MINOR_BLOCKER, 0);
		} while (pc_bits);
	}
    pc_bits = pos.rooks & pos.color[color];
    ei.kingatkrooks[color] = 0;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = rookAttacksBB(from, pos.occupied);
        katemp64 = rookAttacksBB(from, pos.occupied & ~(pos.queens | pos.rooks));
        ei.kingatkrooks[color] |= katemp64;
        ei.atkrooks[color] |= temp64;
        if (katemp64 & ei.kingzone[enemy]) ei.atkcntpcs[enemy] += (1 << 20) + bitCnt(katemp64 & ei.kingadj[enemy]) * 2 + (RookAttackValue << 10);
        uint64 safeMoves = temp64 & mobileSquare;
        temp1 = bitCnt(safeMoves);
        ei.posScore[color] += RookMobArray[temp1];

        xtemp64 = rookAttacksBB(from, boardSkeleton) & notOwnSkeleton;
        temp1 = bitCnt(xtemp64);
        if (xtemp64 & notOwnColor & (pos.kings | pos.queens)) {
            threatScore += (RookAttackPower * QueenAttacked) / 2;
        }
		ei.posScore[color] += XrayRookMobArray[temp1];
        //its good to be lined up with a lot of enemy pawns (7th rank most common example)
        uint64 pressured = rookAttacksBB(from, attackBlockers);
        uint64 pawnsPressured = pressured & pawnTargets;
        if (pawnsPressured) {
            int numPawnsPressured = bitCnt(pawnsPressured);
			ei.posScore[color] += ComposeEvalScore(MidgameRookPawnPressure, EndgameRookPawnPressure) * numPawnsPressured;
        }
        // restricting king on 8th
        if ((BitMask[from] & Rank7ByColorBB[color]) && (BitMask[pos.kpos[enemy]] & Rank8ByColorBB[color])) {
			ei.posScore[color] += ComposeEvalScore(MidgameRook7th, EndgameRook7th);
        }
    }
    pc_bits = pos.queens & pos.color[color];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = queenAttacksBB(from, pos.occupied);
        ei.atkqueens[color] |= temp64;
        if (temp64 & ei.kingzone[enemy]) {
            ei.atkcntpcs[enemy] += (1 << 20) + bitCnt(temp64 & ei.kingadj[enemy]) * 2 + (QueenAttackValue << 10);
            if (ei.kingadj[enemy] & ei.atkpawns[color]) ei.atkcntpcs[enemy] += (1 << 20);
        }

        temp1 = bitCnt(temp64 & mobileSquare);
        ei.posScore[color] += QueenMobArray[temp1];

        xtemp64 = queenAttacksBB(from, boardSkeleton) & notOwnSkeleton;
        temp1 = bitCnt(xtemp64);
        ei.posScore[color] += XrayQueenMobArray[temp1];

        if (xtemp64 & notOwnColor & pos.kings) {
            threatScore += (QueenAttackPower * QueenAttacked) / 2;
        }
        if ((BitMask[from] & Rank7ByColorBB[color]) && (BitMask[pos.kpos[enemy]] & Rank8ByColorBB[color])) {
			ei.posScore[color] += ComposeEvalScore(MidgameQueen7th, EndgameQueen7th);
        }
    }
    ei.atkall[color] = ei.atkpawns[color] | ei.atkknights[color] | ei.atkbishops[color] | ei.atkrooks[color] | ei.atkqueens[color];
    ei.atkkings[color] = KingMoves[pos.kpos[color]];
    ei.atkall[color] |= ei.atkkings[color];
	ei.posScore[color] += threatScore * ComposeEvalScore(PieceAttackMulMid, PieceAttackMulEnd);
}

void evalThreats(const position_t& pos, eval_info_t& ei, const int color) {
    uint64 temp64, not_guarded, enemy_pcs;
    int temp1;

    ASSERT(colorIsOk(color));

    temp1 = 0;
    enemy_pcs = pos.color[color ^ 1] & ~(pos.pawns | pos.kings);

    not_guarded = ~ei.atkpawns[color ^ 1];
    temp64 = ei.atkpawns[color] & enemy_pcs;
    if (temp64) {
        temp1 += PawnAttackPower * QueenAttacked * ((temp64 & pos.queens) != 0);
        temp1 += PawnAttackPower * RookAttacked* ((temp64 & pos.rooks) != 0);
        temp1 += PawnAttackPower * BishopAttacked* ((temp64 & pos.bishops) != 0);
        temp1 += PawnAttackPower * KnightAttacked* ((temp64 & pos.knights) != 0);
    }

    temp64 = ei.atkknights[color] & enemy_pcs;
    if (temp64) {
        temp1 += KnightAttackPower * QueenAttacked *((temp64 & pos.queens) != 0);
        temp1 += KnightAttackPower * RookAttacked *((temp64 & pos.rooks) != 0);
        temp1 += KnightAttackPower * BishopAttacked *((temp64 & pos.bishops & not_guarded) != 0);
    }

    temp64 = ei.atkbishops[color] & enemy_pcs;
    if (temp64) {
        temp1 += BishopAttackPower * QueenAttacked * ((temp64 & pos.queens) != 0);
        temp1 += BishopAttackPower * RookAttacked * ((temp64 & pos.rooks) != 0);
        temp1 += BishopAttackPower * KnightAttacked * ((temp64 & pos.knights & not_guarded) != 0);
    }
    temp64 = ei.atkrooks[color] & enemy_pcs;
    if (temp64) {
        temp1 += RookAttackPower * QueenAttacked * ((temp64 & pos.queens) != 0);
        temp1 += RookAttackPower * BishopAttacked * ((temp64 & pos.bishops & not_guarded) != 0);
        temp1 += RookAttackPower * KnightAttacked * ((temp64 & pos.knights & not_guarded) != 0);
    }

    temp64 = ei.atkqueens[color] & enemy_pcs;
    if (temp64) {
        temp1 += QueenAttackPower * RookAttacked * ((temp64 & pos.rooks & not_guarded) != 0);
        temp1 += QueenAttackPower * BishopAttacked * ((temp64 & pos.bishops & not_guarded) != 0);
        temp1 += QueenAttackPower * KnightAttacked * ((temp64 & pos.knights & not_guarded) != 0);
    }

	ei.posScore[color] += temp1 * ComposeEvalScore(PieceAttackMulMid, PieceAttackMulEnd);
    ///now some serious threats
    uint64 unprotected = enemy_pcs & ~ei.atkall[color ^ 1];
    //undefended minors get penalty
    uint64 unprotectedMinors = unprotected & (pos.bishops | pos.knights);
    if (unprotectedMinors) {
        int umScore = MinTwoBits(unprotectedMinors) ? 2 : 1;
		ei.posScore[color] += ComposeEvalScore(13,5) * umScore;
    }
    uint64 threatB = unprotected & ei.atkall[color];
    uint64 target = enemy_pcs & pos.queens;

    threatB |= (ei.atkrooks[color]) & target;
    target |= enemy_pcs & pos.rooks;
    threatB |= (ei.atkbishops[color] | ei.atkknights[color]) & target;
    target |= enemy_pcs & (pos.knights | pos.bishops);
    threatB |= ei.atkpawns[color] & target;

    if (threatB) {
		int numThreats = MIN(7,bitCnt(threatB) - (pos.side != color));
        //only really takes double threats for the opponent seriously, since its not handled well by qsearch and such
        ei.posScore[color] += ThreatBonus[numThreats];
    }

}
static const int kingCastleBonus = GetOpening(PST(WHITE, KING, g1) - PST(WHITE, KING, e1))/2;
static const int queenCastleBonus = GetOpening(PST(WHITE, KING, c1) - PST(WHITE, KING, e1))/2;

int KingShelter(const int color, eval_info_t& ei, const position_t& pos) { //returns penalty for being under attack
    int currentKing = ei.pawn_entry->shelter[color];
	int kingSide = ((pos.posStore.castle & KSC[color]) == 0 || (ei.atkall[color ^ 1] & KCSQ[color])) ? 0 : (ei.pawn_entry->kshelter[color] + kingCastleBonus);
	int queenSide = ((pos.posStore.castle & QSC[color]) == 0 || (ei.atkall[color ^ 1] & QCSQ[color])) ? 0 : (ei.pawn_entry->qshelter[color] + queenCastleBonus);

    kingSide = (kingSide > queenSide) ? kingSide : queenSide;
    currentKing += MAX(currentKing, kingSide);
    ei.posScore[color] += ShelterBonus[currentKing]; 
    return currentKing;
}

void evalKingAttacked(const position_t& pos, eval_info_t& ei, const int color) {
    int danger;
    uint64 pc_atkrs_mask, pc_atkhelpersmask, king_atkmask, pc_defenders_mask;
    int shelter, tot_atkrs, pc_weights, kzone_atkcnt;
    ASSERT(colorIsOk(color));

    tot_atkrs = ei.atkcntpcs[color] >> 20;
    kzone_atkcnt = ei.atkcntpcs[color] & ((1 << 10) - 1);

    danger = (tot_atkrs >= 2 && kzone_atkcnt >= 1);
    shelter = KingShelter(color, ei, pos);
    if (danger) {
        int penalty = MAX(0, 8 - shelter);
        king_atkmask = KingMoves[pos.kpos[color]];
        uint64 kingEscapes = king_atkmask & ~ei.atkall[color ^ 1];
        if (MaxOneBit(kingEscapes)) penalty += (kingEscapes == 0) ? 4 : 2;
        pc_weights = (ei.atkcntpcs[color] & ((1 << 20) - 1)) >> 10;
        penalty += KingPosPenalty[color][pos.kpos[color]] + ((pc_weights * tot_atkrs) / 2) + kzone_atkcnt;
        pc_defenders_mask = ei.atkqueens[color] | ei.atkrooks[color] | ei.atkbishops[color] | ei.atkknights[color] | ei.atkpawns[color];
        penalty += bitCnt(king_atkmask & ei.atkall[color ^ 1] & ~pc_defenders_mask);
        pc_atkrs_mask = king_atkmask & ei.atkqueens[color ^ 1] & (~pos.color[color ^ 1]);
        if (pc_atkrs_mask) { //TODO rook contact checks
            uint64 queenContact;
            pc_atkhelpersmask = (ei.atkkings[color ^ 1] | ei.kingatkrooks[color ^ 1] | ei.kingatkbishops[color ^ 1] | ei.atkknights[color ^ 1] | ei.atkpawns[color ^ 1]);
            pc_atkrs_mask &= pc_atkhelpersmask;
            queenContact = pc_atkrs_mask & ~pc_defenders_mask;
            if (queenContact) {
                int bonus = ((pos.side == (color ^ 1)) ? 2 : 1) * QueenSafeContactCheckValue;
                uint64 kingEscape = king_atkmask & ~(pc_atkhelpersmask | pos.color[color]);
                do {
                    int sq = popFirstBit(&queenContact);
                    uint64 queenCovers = queenAttacksBB(sq, pos.occupied);

                    penalty += bonus;
                    if (0 == (kingEscape & ~queenCovers)) { //checkmate
                        //MAKE SURE IT IS REALLY MATE
                        // first sliders
                        uint64 tempOcc = pos.occupied & ~(pos.queens & pos.color[color ^ 1]); // a little conservative when there are two queens but that is OK

                        if (bishopAttacksBB(sq, tempOcc) & (pos.bishops | pos.queens) & pos.color[color]) continue;
                        if (rookAttacksBB(sq, tempOcc) & (pos.rooks | pos.queens) & pos.color[color]) continue;
                        //FOR NOW IGNORE QUEEN IS PINNED ISSUES
                        if (pos.side == (color ^ 1)) {
                            ei.posScore[color] -= ComposeEvalScore(10000,10000);
                        }
                        else {
                            penalty += MATE_CODE;
                        }
                    }
                } while (queenContact);
            }
        }
        //SAM TODO probably faster to do queen separate rather than evoke bitCnt multiple time
        pc_atkrs_mask = rookAttacksBB(pos.kpos[color], pos.occupied) & ~ei.atkall[color] & ~pos.color[color ^ 1];
        if (pc_atkrs_mask & ei.atkqueens[color ^ 1]) {
            int numQueenChecks = bitCnt(pc_atkrs_mask & ei.atkqueens[color ^ 1]);
            penalty += numQueenChecks * QueenSafeCheckValue;
        }
        if (pc_atkrs_mask & ei.kingatkrooks[color ^ 1]) penalty += bitCnt(pc_atkrs_mask & ei.kingatkrooks[color ^ 1]) * RookSafeCheckValue;
        pc_atkrs_mask = bishopAttacksBB(pos.kpos[color], pos.occupied) & ~ei.atkall[color] & ~pos.color[color ^ 1];
        if (pc_atkrs_mask & ei.atkqueens[color ^ 1]) penalty += bitCnt(pc_atkrs_mask & ei.atkqueens[color ^ 1]) * QueenSafeCheckValue;
        if (pc_atkrs_mask & ei.kingatkbishops[color ^ 1]) penalty += bitCnt(pc_atkrs_mask & ei.kingatkbishops[color ^ 1]) * BishopSafeCheckValue;
        pc_atkrs_mask = KnightMoves[pos.kpos[color]] & ~ei.atkall[color] & ~pos.color[color ^ 1];
        if (pc_atkrs_mask & ei.atkknights[color ^ 1]) penalty += bitCnt(pc_atkrs_mask & ei.atkknights[color ^ 1]) * KnightSafeCheckValue;

        pc_atkrs_mask = discoveredCheckCandidates(pos, color ^ 1) & ~pos.pawns;
        if (pc_atkrs_mask) penalty += bitCnt(pc_atkrs_mask) * DiscoveredCheckValue;
        ei.posScore[color] -= ComposeEvalScore((penalty * penalty * KING_ATT_W) / 20,0);
        ei.draw[color ^ 1] += penalty/2;
    }
}

void evalPassedvsKing(const position_t& pos, eval_info_t& ei, const int allied, uint64 passed) {
    const int enemy = allied ^ 1;

    int leftBestFile = FileH;
    int leftBestToQueen = 8;
    int rightBestFile = FileA;
    int rightBestToQueen = 8;
    int fileDist;

    const int myMove = (pos.side == allied);
    uint64 passedpawn_mask = passed;
    do {
        int from = popFirstBit(&passedpawn_mask);
        int score = 0;
        int rank = PAWN_RANK(from, allied);
        uint64 prom_path = (*FillPtr[allied])(BitMask[from]);
        uint64 path_attkd = prom_path & ei.atkall[enemy];
        uint64 path_dfndd = prom_path & ei.atkall[allied];

        if (!(prom_path & pos.color[enemy])) {
            if (!path_attkd) score += (prom_path == path_dfndd) ? PathFreeNotAttackedDefAllPasser : PathFreeNotAttackedDefPasser;
            else score += ((path_attkd & path_dfndd) == path_attkd) ? PathFreeAttackedDefAllPasser : PathFreeAttackedDefPasser;
        }
        else if (((path_attkd | (prom_path & pos.color[enemy])) & ~path_dfndd) == EmptyBoardBB) score += PathNotFreeAttackedDefPasser;
        if ((prom_path & pos.color[allied]) == EmptyBoardBB) score += PathFreeFriendPasser;
        if (ei.atkpawns[allied] & BitMask[from]) score += DefendedByPawnPasser;
        if (ei.atkpawns[allied] & BitMask[from + PAWN_MOVE_INC(allied)]) {
            score += DuoPawnPasser;
        }
        score += EndgamePassedMax;
        {
            int frontSq = from + PAWN_MOVE_INC(allied);
            score += DISTANCE(frontSq, pos.kpos[enemy]) * PassedPawnDefenderDistance;
            score -= DISTANCE(frontSq, pos.kpos[allied]) * PassedPawnAttackerDistance;
        }
        {
            int pFile = SQFILE(from);
            int qDist = 7 - rank - myMove;
            if (pFile + qDist < leftBestFile + leftBestToQueen) {
                leftBestFile = pFile;
                leftBestToQueen = qDist;
            }
            if ((7 - pFile) + qDist < (7 - rightBestFile) + rightBestToQueen) {
                rightBestFile = pFile;
                rightBestToQueen = qDist;
            }
            // later we can analyze moving king out of way but for now lets let search do it

            if (!(prom_path & pos.occupied)) {
                int oppking = pos.kpos[enemy];
                int promotion = PAWN_PROMOTE(from, allied);
                int eDist = DISTANCE(oppking, promotion);

                if (eDist - 1 > qDist) {
                    score += UnstoppablePassedPawn;
                    ei.queening = true;
                }
                // or king can escort pawn in
                else if (qDist <= 2) {
                    int mDist = DISTANCE(pos.kpos[allied], promotion) - myMove;
                    if (pFile == FileA || pFile == FileH) mDist++; // need more margin for error for rook pawns NOTE: may need to check for king in from of pawn if we allow this later
                    if (mDist < eDist || (mDist == eDist && myMove)) { //if this race is a tie we can still win it, but must spend move on king first not pawn
                        score += UnstoppablePassedPawn;
                        ei.queening = true;
                    }
                }
            }
        }
		ei.posScore[allied] += ComposeEvalScore(MidgamePassedMin + scale(MidgamePassedMax, rank), EndgamePassedMin + scale(score, rank));
    } while (passedpawn_mask);

    fileDist = rightBestFile - leftBestFile;
    if (leftBestToQueen > rightBestToQueen) {
        if (fileDist > leftBestToQueen) {
			ei.posScore[allied] += ComposeEvalScore(0,scale(UnstoppablePassedPawn, 7 - leftBestToQueen));
            ei.queening = true;
        }
    }
    else if (fileDist > rightBestToQueen) {
		ei.posScore[allied] += ComposeEvalScore(0, scale(UnstoppablePassedPawn, 7 - rightBestToQueen));
        ei.queening = true;
    }
}
//TODO a fair amount of this could be accounted for in hashed pawn values
void evalPassed(const position_t& pos, eval_info_t& ei, const int allied, uint64 passed) {
    uint64 passedpawn_mask = passed;
    int myMove = (pos.side == allied);
    const int behind[] = { S, N };
    const int enemy = allied ^ 1;
    do {
        int from = popFirstBit(&passedpawn_mask);
        int score = 0;
        int rank = PAWN_RANK(from, allied);
        uint64 prom_path = (*FillPtr[allied])(BitMask[from]);
        uint64 path_attkd = prom_path & ei.atkall[enemy];
        uint64 path_dfndd = prom_path & ei.atkall[allied];
        uint64 rooksBehind = rookAttacksBB(from, pos.pawns) & pos.rooks & DirBitmap[behind[allied]][from]; 
        if (SHOW_EVAL) {
            if (rooksBehind) PrintOutput() << "info string behind\n";
        }
        int frontSq = from + PAWN_MOVE_INC(allied);

        score += EndgamePassedMax;
        score += DISTANCE(frontSq, pos.kpos[enemy]) * PassedPawnDefenderDistance;
        score -= DISTANCE(frontSq, pos.kpos[allied]) * PassedPawnAttackerDistance;

        if (rooksBehind & pos.color[enemy]) path_attkd |= prom_path;
        if (rooksBehind & pos.color[allied]) path_dfndd |= prom_path;
        if ((prom_path & pos.color[enemy]) == EmptyBoardBB) {
            if (path_attkd == EmptyBoardBB) score += (prom_path == path_dfndd) ? PathFreeNotAttackedDefAllPasser : PathFreeNotAttackedDefPasser;
            else score += ((path_attkd & path_dfndd) == path_attkd) ? PathFreeAttackedDefAllPasser : PathFreeAttackedDefPasser;
        }
        else if (((path_attkd | (prom_path & pos.color[enemy])) & ~path_dfndd) == EmptyBoardBB) score += PathNotFreeAttackedDefPasser;
        if ((prom_path & pos.color[allied]) == EmptyBoardBB) score += PathFreeFriendPasser;
        if (ei.atkpawns[allied] & BitMask[from]) score += DefendedByPawnPasser;
        if (ei.atkpawns[allied] & BitMask[frontSq]) {
            score += DuoPawnPasser;
        }

        if (passed & (PawnCaps[from][allied] | PawnCaps[frontSq][enemy])
            && ((pos.bishops | pos.queens) & pos.color[enemy]) == 0 && ei.queening == false && rank >= Rank6 && MaxOneBit(pos.color[enemy] & ~(pos.kings | pos.pawns)))	
        {
            int promotion = PAWN_PROMOTE(from, allied);
            int kDist = DISTANCE(pos.kpos[enemy], promotion) - (!myMove);
            if (kDist > (7 - rank - myMove)) {
                score += UnstoppablePassedPawn + RookValueMid2 - PawnValueMid2;
                ei.queening = true;
            }
        }
		ei.posScore[allied] += ComposeEvalScore(MidgamePassedMin + scale(MidgamePassedMax, rank), EndgamePassedMin + scale(score, rank));
    } while (passedpawn_mask);
}

void evalPawns(const position_t& pos, eval_info_t& ei, Thread& sthread) {
    initPawnEvalByColor(pos, ei, WHITE);
    initPawnEvalByColor(pos, ei, BLACK);
    ei.pawn_entry = sthread.pt.Entry(pos.posStore.phash);
    if (ei.pawn_entry->hashlock != LOCK(pos.posStore.phash)) {
        int midpawnscore = 0; 
        int endpawnscore = 0;
        ei.pawn_entry->passedbits = 0;
		ei.pawn_entry->locked = 0;
        evalPawnsByColor(pos, ei, midpawnscore, endpawnscore, BLACK);
        midpawnscore *= -1;
        endpawnscore *= -1;
        evalPawnsByColor(pos, ei, midpawnscore, endpawnscore, WHITE);
		ei.pawn_entry->score = ComposeEvalScore(midpawnscore, endpawnscore);
        ei.pawn_entry->pawnWidthBonus = BBwidth(pos.pawns);
        ei.pawn_entry->hashlock = LOCK(pos.posStore.phash);
    }
    ei.posScore[WHITE] += ei.pawn_entry->score;
}
bool QuickStalemate(const position_t& pos, eval_info_t& ei, const int color) {
    if ((ei.MLindex[color] > 8 * MLP) ||
        ((KingMoves[pos.kpos[color]] & ~(pos.color[color] | ei.atkall[color^1])) != 0) ||
        (ei.atkpawns[color] & pos.color[color ^ 1]) != 0) return false;
    uint64 pawnMoves = (*ShiftPtr[color])(ei.pawns[color], 8) & ~pos.occupied;
    return (pawnMoves==0);
}
int eval(const position_t& pos, Thread& sthread) {
    eval_info_t ei;
    material_info_t *mat;
    int open, end, score;
    EvalEntry *entry;
    uint64 whitePassed, blackPassed;

    entry = sthread.et.Entry(pos.posStore.hash);
    if (entry->hashlock == LOCK(pos.posStore.hash)) {
        return entry->value;
    }

    ei.MLindex[WHITE] = pos.posStore.mat_summ[WHITE];
    ei.MLindex[BLACK] = pos.posStore.mat_summ[BLACK];

    ASSERT(ei.MLindex[WHITE] >= 0);
    ASSERT(ei.MLindex[BLACK] >= 0);
    if (ei.MLindex[WHITE] < MAX_MATERIAL && ei.MLindex[BLACK] < MAX_MATERIAL) {
        mat = &MaterialTable[ei.MLindex[WHITE]][ei.MLindex[BLACK]];

        score = mat->value;
        ei.phase = mat->phase;
        ei.draw[WHITE] = mat->draw[WHITE];
        ei.draw[BLACK] = mat->draw[BLACK];
        ei.endFlags[WHITE] = mat->flags[WHITE];
        ei.endFlags[BLACK] = mat->flags[BLACK];
    }
    else {
        score = computeMaterial(ei);
    }
	ei.posScore[WHITE] = pos.posStore.posScore[WHITE];
	ei.posScore[BLACK] = pos.posStore.posScore[BLACK];

    ei.atkpawns[WHITE] = ei.atkpawns[BLACK] = 0;
    ei.atkknights[WHITE] = ei.atkknights[BLACK] = 0;
    ei.atkbishops[WHITE] = ei.atkbishops[BLACK] = 0;
    ei.atkrooks[WHITE] = ei.atkrooks[BLACK] = 0;
    ei.atkqueens[WHITE] = ei.atkqueens[BLACK] = 0;
    ei.atkkings[WHITE] = ei.atkkings[BLACK] = 0;
    ei.atkcntpcs[WHITE] = ei.atkcntpcs[BLACK] = 0;

    ei.kingadj[WHITE] = KingMoves[pos.kpos[WHITE]];
    ei.kingadj[BLACK] = KingMoves[pos.kpos[BLACK]];
    ei.kingzone[WHITE] = ei.kingadj[WHITE] | (ei.kingadj[WHITE] << 8) | (ei.kingadj[WHITE] >> 8);
    ei.kingzone[BLACK] = ei.kingadj[BLACK] | (ei.kingadj[BLACK] >> 8) | (ei.kingadj[BLACK] << 8);

	ei.posScore[pos.side] += ComposeEvalScore(personality(thread_id).tempo_open, personality(thread_id).tempo_end);
    evalPawns(pos, ei, sthread);

    blackPassed = ei.pawn_entry->passedbits & pos.color[BLACK];
    whitePassed = ei.pawn_entry->passedbits & pos.color[WHITE];
    ei.oppBishops = (pos.bishops & pos.color[WHITE]) && (pos.bishops & pos.color[BLACK]) && //TODO could do this only once side with edge determined
        MaxOneBit((pos.bishops & pos.color[WHITE])) &&
        MaxOneBit((pos.bishops & pos.color[BLACK])) &&
        (((pos.bishops & pos.color[WHITE] & WhiteSquaresBB) == 0) !=
        ((pos.bishops & pos.color[BLACK] & WhiteSquaresBB) == 0));
    if (ei.oppBishops) {
        ei.draw[WHITE] += OB_WEIGHT;
        ei.draw[BLACK] += OB_WEIGHT;
    }
    evalPieces(pos, ei, WHITE);
    evalPieces(pos, ei, BLACK);
    if (QuickStalemate(pos, ei, pos.side)) {
        score = DrawValue[pos.side];
        if (SHOW_EVAL) PrintOutput() << "info string stalemate detected";
    }
    else {
        evalPawnPushes(pos, ei, WHITE);
        evalPawnPushes(pos, ei, BLACK);

        ei.queening = false;

        if (pos.color[WHITE] & ~pos.pawns & ~pos.kings) {//if white has a piece
            if (blackPassed) evalPassed(pos, ei, BLACK, blackPassed);
            evalThreats(pos, ei, WHITE);
            if (ei.MLindex[WHITE] >= MLQ + MLN) {
                evalKingAttacked(pos, ei, BLACK);
            }
        }
        else {
            if (blackPassed) evalPassedvsKing(pos, ei, BLACK, blackPassed);
        }
        if (pos.color[BLACK] & ~pos.pawns & ~pos.kings) { //if black has a piece
            if (whitePassed) evalPassed(pos, ei, WHITE, whitePassed);
            evalThreats(pos, ei, BLACK);
            if (ei.MLindex[BLACK] >= MLQ + MLN) { // attacking the white king
                evalKingAttacked(pos, ei, WHITE);
            }
        }
        else {
            if (whitePassed) evalPassedvsKing(pos, ei, WHITE, whitePassed);
        }
		EvalScore scoreDif = ei.posScore[WHITE] - ei.posScore[BLACK];
		open = GetOpening(scoreDif);
		end = GetEndgame(scoreDif);
        {
            int posVal = ((open * ei.phase) + (end * (32 - ei.phase))) / (32); //was 32, /16 means doubling the values
            score += posVal;
        }
        // if there is an unstoppable pawn on the board, we cannot trust any of our draw estimates or endgame rules of thumb
        if (!ei.queening) {
            int edge = score < DrawValue[WHITE];
            int draw = ei.draw[edge]; //this is who is trying to draw
            if (SHOW_EVAL) {
                PrintOutput() << "info string drawish was = " << draw << "\n";
            }
            evalEndgame(edge, pos, ei, &score, &draw);
			int lockedDraw = MAX(0, (ei.pawn_entry->locked - 8)) * 2; //hhh53
			draw += ((100 - draw) * lockedDraw) / 100;
            draw = MIN(MAX_DRAW, draw); // max of 200 draw
            if (SHOW_EVAL) {
                PrintOutput() << "info string drawish is = " << draw << "\n";
            }
            score = ((score * (MAX_DRAW - draw)) + (DrawValue[WHITE] * draw)) / MAX_DRAW;
        }
        score = score*sign[pos.side];
        if (score < -MAXEVAL) score = -MAXEVAL;
        else if (score > MAXEVAL) score = MAXEVAL;
    }
    entry->hashlock = LOCK(pos.posStore.hash);
    entry->value = score;
    return score;
}

void FillMobilityArray(double oWeight, double eWeight, int minGood, double oPenalty, double ePenalty, int numMoves, int a[]) {
    double mobilityArray[7 * 4 + 1];
    mobilityArray[0] = 0;
    for (int i = 1; i < numMoves; i++) {
        mobilityArray[i] = mobilityArray[i - 1] + ComposeEvalScore(oWeight,eWeight); //weight is what to play with if you want diminishing returns on mobility
    }
    for (int i = 0; i < numMoves; i++) {
        if (i < minGood)  mobilityArray[i] -= ComposeEvalScore(oPenalty,ePenalty);
        a[i] = round(mobilityArray[i]);
    }
}

void InitMobility() {
    const double MxrayLow = 10;
    const double ExrayLow = 50;
    const double Mlow = 0; 
    const double Elow = 0; 

    const double MidgameKnightMob = 6;
    const double EndgameKnightMob = 8;

    const double MidgameBishopMob = 3;
    const double MidgameXrayBishopMob = 2;
    const double EndgameBishopMob = 3;
    const double EndgameXrayBishopMob = 2;

    const double MidgameRookMob = 1;
    const double MidgameXrayRookMob = 3;
    const double EndgameRookMob = 2;
    const double EndgameXrayRookMob = 3;

    const double MidgameQueenMob = 1; 
    const double MidgameXrayQueenMob = 1;
    const double EndgameQueenMob = 2;
    const double EndgameXrayQueenMob = 2;

	FillMobilityArray(MidgameKnightMob, EndgameKnightMob, 2, Mlow, Elow, 9, KnightMobArray);
	FillMobilityArray(MidgameBishopMob, EndgameBishopMob, 2, Mlow, Elow, 7 * 2 + 1, BishopMobArray);
	FillMobilityArray(MidgameXrayBishopMob, EndgameXrayBishopMob, 3, MxrayLow, ExrayLow, 7 * 2 + 1, XrayBishopMobArray);
	FillMobilityArray(MidgameRookMob, EndgameRookMob, 2, Mlow, Elow, 7 * 2 + 1, RookMobArray);
	FillMobilityArray(MidgameXrayRookMob, EndgameXrayRookMob, 3, MxrayLow, ExrayLow, 7 * 2 + 1, XrayRookMobArray);
	FillMobilityArray(MidgameQueenMob, EndgameQueenMob, 2, Mlow, Elow, 7 * 4 + 1, QueenMobArray);
	FillMobilityArray(MidgameXrayQueenMob, EndgameXrayQueenMob, 3, MxrayLow, ExrayLow, 7 * 4 + 1, XrayQueenMobArray);
}
