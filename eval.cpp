/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
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
#include "search.h"

// TODO

//add pawn is piece attacking king if queen is, and detect checkmate threats
// rewrite moves generated into move stack
// try reduced double pawn values?
// use ei->pawns
// consider multiplying all the positional values by 32, and then not dividing by phase

//some castling comments
static const int KSC[2] = {WCKS,BCKS};
static const int QSC[2] = {WCQS,BCQS};
static const int Castle[2] = {WCQS+WCKS,BCQS+BCKS};
#define KS(c)			(WCKS+c*3)
#define QS(c)			(WCQS+c*6)
// F1 | G1, F8 | G8
static const uint64 KCSQ[2] = {( 0x0000000000000020ULL | 0x0000000000000040ULL),(0x2000000000000000ULL | 0x4000000000000000ULL)};
// C1 | D1, C8 | D8
static const uint64 QCSQ[2] = {( 0x0000000000000004ULL | 0x0000000000000008ULL),(0x0400000000000000ULL | 0x0800000000000000ULL)};
static const int KScastleTo[2] = {g1,g8};
static const int QScastleTo[2] = {c1,c8} ;

//constants
#define NUM_GENOMES 3

#define STUCK_END 5 //5
#define STUCK_BISHOP 10 //10
#define STUCK_ROOK 10 //10
#define STUCK_QUEEN 10 //10

#define TEMPO_OPEN 20 //20
#define TEMPO_END 10 //10

#define TRAPPED1 64 //64
#define TRAPPED2 54 //54
#define TRAPPED3 25 //25
#define TRAPPED4 4 //4

#ifndef SELF_TUNE2
#define TM1 64 
#define TM2 54 
#define TM3 25 
#define TM4 4 
class Personality {
public:
    static const int stuck_end=STUCK_END;
    static const int stuck_bishop=STUCK_BISHOP;
    static const int stuck_rook=STUCK_ROOK;
    static const int stuck_queen=STUCK_QUEEN;
    static const int TrappedMoves[];
    static const int tempo_open = TEMPO_OPEN;
    static const int tempo_end = TEMPO_END; 
};
const int Personality::TrappedMoves[] = {TRAPPED1+TRAPPED2+TRAPPED3+TRAPPED4,TRAPPED2+TRAPPED3+TRAPPED4,TRAPPED3+TRAPPED4,TRAPPED4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const Personality personality;
#define personality(thread) personality
#endif

//king safety stuff
#define PARTIAL_ATTACK 0 //0 means don't count, otherwise multiplies attack by 5
#define MATE_CODE 10 // mate threat
#define KING_ATT_W 4
#define K_SHELT_W 17
#define LOW_SHELTER 5
#define EXP_PENALTY 1

// side to move bonus
//static const int MidgameTempo = 20;
//static const int EndgameTempo = 10;

static const int MidgameKnightMob = 6; 
static const int EndgameKnightMob = 8; 

static const int MidgameBishopMob = 3;
static const int MidgameXrayBishopMob = 2; 

static const int EndgameBishopMob = 3; 
static const int EndgameXrayBishopMob = 2; 

static const int MidgameRookMob = 1;
static const int MidgameXrayRookMob = 3;

static const int EndgameRookMob = 2;
static const int EndgameXrayRookMob = 3; 

static const int MidgameQueenMob = 1;
static const int MidgameXrayQueenMob = 1; 

static const int EndgameQueenMob = 2;
static const int EndgameXrayQueenMob = 2; 

// rook specific evaluation
static const int MidgameRookPawnPressure = 4; 
static const int EndgameRookPawnPressure = 8; 
static const int MidgameRook7th = 20 - MidgameRookPawnPressure*2; 
static const int EndgameRook7th = 40 - EndgameRookPawnPressure*2; 

// queen specific evaluation
static const int MidgameQueen7th = 10;
static const int EndgameQueen7th = 20;
//bishop specific
#define OB_WEIGHT 12 //opposite color bishops drawishness

// knight specific evaluation
#define N_ONE_SIDE 20

//pawn specific
#define KAW1 10 // 8
#define KAW2 7 // 4
#define KAW3 2 // 2
#define KAW4 2 // 1
#define KAW5 1 // 1
#define KAW6 0 // 0
#define KAW7 0 // 0
static const int kingAttackWeakness[8] = {	0,
    KAW1+KAW2+KAW3+KAW4+KAW5+KAW6+KAW7,
    KAW2+KAW3+KAW4+KAW5+KAW6+KAW7,
    KAW3+KAW4+KAW5+KAW6+KAW7,
    KAW4+KAW5+KAW6+KAW7,
    KAW5+KAW6+KAW7,
    KAW6+KAW7,
    KAW7 };

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
static const int UnstoppablePassedPawn = 700;
#define EndgamePassedMin 10 // maybe needs to be higher
static const int EndgamePassedMax = 70 - EndgamePassedMin;
#define MidgamePassedMin 20
static const int MidgamePassedMax = 140 - MidgamePassedMin;
#define MidgameCandidatePawnMin 10
static const int MidgameCandidatePawnMax = 100 - MidgameCandidatePawnMin;
#define EndgameCandidatePawnMin 5
static const int EndgameCandidatePawnMax = 50-EndgameCandidatePawnMin; //50 WEIRD to have this less than middle, perhaps takes up space
static const int ProtectedPasser = 10;
static const int DuoPawnPasser = 15;
static const int kingDistImp[8] = {0, 20, 40, 55, 65, 70, 70, 70}; //best if this cannot get higher than EndgamePassedMax
static const int PathFreeFriendPasser = 10;
static const int DefendedByPawnPasser = 10;
static const int PathFreeNotAttackedDefAllPasser = 80;
static const int PathFreeNotAttackedDefPasser = 70;
static const int PathFreeAttackedDefAllPasser = 60;
static const int PathFreeAttackedDefPasser = 40;
static const int PathNotFreeAttackedDefPasser = 10;
static const int PassedPawnAttackerDistance = 20;
static const int PassedPawnDefenderDistance = 40;

// king safety
static const int QueenAttackValue = 5;
static const int RookAttackValue = 3;
static const int BishopAttackValue = 2;
static const int KnightAttackValue = 2;
static const int QueenSafeContactCheckValue = 3;
static const int QueenSafeCheckValue = 2;
static const int RookSafeCheckValue = 1;
static const int BishopSafeCheckValue = 1;
static const int KnightSafeCheckValue = 1;
static const int DiscoveredCheckValue = 3;

// piece attacks
static const int QueenAttacked = 4; //4
static const int RookAttacked = 3; //3
static const int BishopAttacked = 2; //2
static const int KnightAttacked = 2; //2

static const int QueenAttackPower = 1; //1
static const int RookAttackPower = 2; //2
static const int BishopAttackPower = 3; //3
static const int KnightAttackPower = 3; //3
static const int PawnAttackPower = 4; //4

static const int PieceAttackMulMid = 4; //5
static const int PieceAttackMulEnd = 3; //3

#define TB1 8 
#define TB2 80 

static const int sbonus[8] = {0, 0, 0, 13, 34, 77, 128, 0};
#define scale(smax,sr) ((((smax))*sbonus[sr]) / sbonus[6])


void judgeTrapped(const position_t *pos, eval_info_t *ei,const int color, const int thread/*, int *upside, int *downside*/) {

    uint64 targets,safeMoves;
    int targetSq;
    int penalty;
    const int enemy = color^1;
    uint64 safe = ~(ei->atkpawns[enemy] | pos->color[enemy]); // occupied assumes we can capture our way out of trouble in search
    uint64 beware = pos->color[color] & bewareTrapped[color] & ~(pos->pawns | pos->kings); // added if not guarded 1a21
    uint64 safer = (Rank4BB | Rank5BB | ei->atkall[color]);

    if (!beware) return;
    targets = pos->knights & beware;
    while (targets)
    {
        targetSq = popFirstBit(&targets);
        safeMoves = (KnightMoves[targetSq]) &

            (safe & (~ei->atkall[enemy] | ei->atkpawns[color] | ei->atkbishops[color] | ei->atkrooks[color] | ei->atkqueens[color] | ei->atkkings[color]));
        if (showEval) Print(3," judging knight %d ",targetSq);

        if (!(escapeTrapped[color] & safeMoves)) {
            penalty = (BitMask[targetSq] & safer)!=0 ? personality(thread).TrappedMoves[bitCnt(safeMoves)]/2 : personality(thread).TrappedMoves[bitCnt(safeMoves)];
            ei->mid_score[color] -= penalty;
            //			*upside += penalty/2;
            //			*downside += penalty/2;
            if (showEval) {
                Print(3," trapped knight %d",bitCnt(safeMoves));
            }
        }
    }
    targets = pos->bishops & beware;
    while (targets)
    {
        targetSq = popFirstBit(&targets);
        safeMoves = (bishopAttacksBB(targetSq, pos->occupied)) &
            (safe & (~ei->atkall[enemy] | ei->atkpawns[color] | ei->atkknights[color] | ei->atkrooks[color] | ei->atkqueens[color] | ei->atkkings[color]));
        if (showEval) Print(3," judging bishop %d ",targetSq);

        if (!(escapeTrapped[color] & safeMoves)) {
            penalty = (BitMask[targetSq] & safer)!=0 ? personality(thread).TrappedMoves[bitCnt(safeMoves)]/2 : personality(thread).TrappedMoves[bitCnt(safeMoves)];
            ei->mid_score[color] -= penalty;
            //			*upside += penalty/2;
            //			*downside += penalty/2;
            if (showEval) {
                Print(3," trapped bishop %d",bitCnt(safeMoves));
            }
        }
    }
    safe &= ~(ei->atkknights[enemy] | ei->atkbishops[enemy]);
    targets = pos->rooks & beware;
    while (targets)
    {
        targetSq = popFirstBit(&targets);
        safeMoves = (rookAttacksBB(targetSq, pos->occupied)) &

            (safe & (~ei->atkall[enemy] | ei->atkpawns[color] | ei->atkbishops[color] | ei->atkknights[color] | ei->atkqueens[color] | ei->atkkings[color]));
        if (showEval) Print(3," judging rook %d ",targetSq);

        if (!(escapeTrapped[color] & safeMoves)) {
            penalty =  personality(thread).TrappedMoves[bitCnt(safeMoves)]/2; //trapped rooks often get the exchange anyway
            ei->mid_score[color] -= penalty;
            //			*upside += penalty/2;
            //			*downside += penalty/2;
            if (showEval) {
                Print(3," trapped rook %d",bitCnt(safeMoves));
            }
        }
    }

    safe &= ~(ei->atkrooks[enemy]);
    targets = pos->queens & beware;
    while (targets)
    {
        targetSq = popFirstBit(&targets);
        if (DISTANCE(targetSq,pos->kpos[enemy]) > 2) {

            safeMoves = (queenAttacksBB(targetSq, pos->occupied)) &

                (safe & (~ei->atkall[enemy] | ei->atkpawns[color] | ei->atkbishops[color] | ei->atkrooks[color] | ei->atkknights[color] | ei->atkkings[color]));
            if (showEval) Print(3," judging queen %d ",targetSq);

            if (!(escapeTrapped[color] & safeMoves)) {
                penalty = personality(thread).TrappedMoves[bitCnt(safeMoves)]/2; //trapped queens are still dangerous
                ei->mid_score[color] -= penalty;
                //				*upside += penalty/2;
                //				*downside += penalty/2;
                if (showEval) {
                    Print(3," trapped queen %d",bitCnt(safeMoves));
                }
            }
        }
    }
}

// this is only called when there are more than 1 queen for a side
int computeMaterial(const position_t *pos, eval_info_t *ei) {

    int whiteM = ei->MLindex[WHITE];
    int blackM = ei->MLindex[BLACK];
    int wq,bq,wr,br,wb,bb,wn,bn;
    int score = 0;
    ei->phase = 0;

    wq = whiteM / MLQ;
    whiteM -= wq*MLQ;
    bq = blackM / MLQ;
    blackM -= bq*MLQ;
    score += (wq - bq) * ((QueenValueMid1+QueenValueMid2) / 2);
    ei->phase += (wq+bq)*6;

    wr = whiteM / MLR;
    whiteM -= wr*MLR;
    br = blackM / MLR;
    blackM -= br*MLR;
    score += (wr - br) * ((RookValueMid1+RookValueMid2) / 2);
    ei->phase += (wr+br)*3;

    wb = whiteM / MLB;
    whiteM -= wb*MLB;
    bb = blackM / MLB;
    blackM -= bb*MLB;
    score += (wb - bb) * ((BishopValueMid1+BishopValueMid2) / 2);
    ei->phase += (wb+bb);

    wn = whiteM / MLN;
    whiteM -= wn*MLN;
    bn = blackM / MLN;
    blackM -= bn*MLN;
    score += (wn - bn) * ((KnightValueMid1+KnightValueMid2) / 2);
    ei->phase += (wn+bn);

    score += (whiteM - blackM) * ((PawnValueMid1+PawnValueMid2) / 2);


    if (ei->phase > 32) ei->phase = 32;
    ei->draw[WHITE] = 0;
    ei->draw[BLACK] = 0;
    ei->endFlags[WHITE] = 0;
    ei->endFlags[BLACK] = 0;
    return (score);
}

void initPawnEvalByColor(const position_t *pos, eval_info_t *ei, const int color) {
    static const int Shift[] = {9, 7};
    uint64 temp64;

    ei->pawns[color] = (pos->pawns & pos->color[color]);
    temp64 = (((*ShiftPtr[color])(ei->pawns[color], Shift[color]) & ~FileABB) | ((*ShiftPtr[color])(ei->pawns[color], Shift[color^1]) & ~FileHBB)); 
    ei->atkpawns[color] = temp64;
    ei->atkcntpcs[color^1] += bitCnt(temp64 & ei->kingadj[color^1]);
    ei->potentialPawnAttack[color] = (*FillPtr2[color])(temp64);
    /*	if (showEval) {
    uint64 pAtt = temp64;
    while (pAtt)
    {
    int sq = popFirstBit(&pAtt);
    Print(3,"a %c%i ",sq%8+'a',sq/8+1);
    }
    }
    */
}

void evalShelter(const int color, eval_info_t *ei, const position_t *pos) {
    uint64 shelter,indirectShelter,doubledShelter;
    uint64 pawns = ei->pawns[color];

    // king shelter in your current position
    shelter = kingShelter[color][pos->kpos[color]] & pawns;
    indirectShelter = kingIndirectShelter[color][pos->kpos[color]] & pawns;
    // doubled pawns are a bit redundant as shelter
    doubledShelter = indirectShelter & (*FillPtr[color])(indirectShelter);
    indirectShelter ^= doubledShelter;
    // remember, shelter is also indirect shelter so it is counted twice
    ei->pawn_entry->shelter[color]  = bitCnt(shelter)+bitCnt(indirectShelter);
    // now take into account possible castling, and the associated safety SAM122109
    if (pos->posStore.castle & KS(color)) {
        shelter = kingShelter[color][KScastleTo[color]] & pawns;
        indirectShelter = kingIndirectShelter[color][KScastleTo[color]] & pawns;
        // doubled pawns are a bit redundant as shelter
        doubledShelter = indirectShelter & (*FillPtr[color])(indirectShelter);
        indirectShelter ^= doubledShelter;
        // remember, shelter is also indirect shelter so it is counted twice
        ei->pawn_entry->kshelter[color] = bitCnt(shelter)+bitCnt(indirectShelter);
    } else ei->pawn_entry->kshelter[color] = 0;
    if (pos->posStore.castle&QS(color)) {
        shelter = kingShelter[color][QScastleTo[color]] & pawns;
        indirectShelter = kingIndirectShelter[color][QScastleTo[color]] & pawns;
        // doubled pawns are a bit redundant as shelter
        doubledShelter = indirectShelter & (*FillPtr[color])(indirectShelter);
        indirectShelter ^= doubledShelter;
        // remember, shelter is also indirect shelter so it is counted twice
        ei->pawn_entry->qshelter[color] = bitCnt(shelter)+bitCnt(indirectShelter);
    } else ei->pawn_entry->qshelter[color] = 0;
}

void evalPawnsByColor(const position_t *pos, eval_info_t *ei, int mid_score[], int end_score[], const int color) {
    static const int Shift[2][3][4] = {{{23, 18, 13, 8}, {26, 21, 16, 11}, {28, 23, 18, 13}}, {{15, 26, 37, 48}, {18, 29, 40, 51}, {20, 31, 42, 53}}};
    static const int weakFile[8] = {0,2,3,3,3,3,2,0};
    static const int weakRank[8] = {0,0,2,4,6,8,10,0};
    uint64 openBitMap, isolatedBitMap, doubledBitMap, passedBitMap, backwardBitMap,targetBitMap, temp64;

    int count, sq, rank;
    const int enemy = color ^ 1;

    openBitMap = ei->pawns[color] & ~((*FillPtr2[enemy])((*ShiftPtr[enemy])((ei->pawns[color]|ei->pawns[enemy]), 8)));
    doubledBitMap = ei->pawns[color] & (*FillPtr[enemy])(ei->pawns[color]);
    isolatedBitMap = ei->pawns[color] & ~((*FillPtr2[color^1])(ei->potentialPawnAttack[color]));
    backwardBitMap = (*ShiftPtr[enemy])(((*ShiftPtr[color])(ei->pawns[color], 8) & (ei->potentialPawnAttack[enemy]|ei->pawns[enemy])
        & ~ei->potentialPawnAttack[color]), 8) & ~isolatedBitMap; //this includes isolated
    passedBitMap = openBitMap & ~ei->potentialPawnAttack[enemy];
    targetBitMap = isolatedBitMap;
    //any backward pawn not easily protect by its own pawns is a target
    //TODO consider calculating exactly how many moves to protect and defend including double moves
    //TODO consider increasing distance of defender by 1
    while (backwardBitMap) { //by definition this is not on the 7th rank
        sq = popFirstBit(&backwardBitMap);
        bool permWeak = 
            (BitMask[sq + PAWN_MOVE_INC(color)] & (ei->atkpawns[enemy])) || //enemy adjacent pawn within 1
            ((BitMask[sq + PAWN_MOVE_INC(color)*2] & ei->atkpawns[enemy]) && //enemy adjacent pawn within 2
            (BitMask[sq + PAWN_MOVE_INC(color)] & ei->atkpawns[color])==0); //support pawn within 1
        if (permWeak) {
            targetBitMap |= BitMask[sq];
            if (showEval) Print(3," pwb %s ",sq2Str(sq));
        }
    }

    //lets look through all the passed pawns and give them their static bonuses 
    //TODO consider moving some king distance stuff in here
    temp64 = ei->pawns[color] & openBitMap & ~passedBitMap;
    while (temp64) {
        sq = popFirstBit(&temp64);
        if (bitCnt((*FillPtr2[color])(PawnCaps[sq][color]) & ei->pawns[enemy])
            <= bitCnt((*FillPtr2[enemy])(PawnCaps[sq + PAWN_MOVE_INC(color)][enemy]) & ei->pawns[color]) &&
            bitCnt(PawnCaps[sq][color] & ei->pawns[enemy])
            <= bitCnt(PawnCaps[sq][enemy] & ei->pawns[color])) {
                rank = PAWN_RANK(sq, color);

                mid_score[color] += MidgameCandidatePawnMin + scale(MidgameCandidatePawnMax, rank);
                end_score[color] += EndgameCandidatePawnMin + scale(EndgameCandidatePawnMax, rank);

        }
    }
    if (doubledBitMap) {
        count = bitCnt(doubledBitMap);
        mid_score[color] -= count * DOUBLED_O;
        end_score[color] -= count * DOUBLED_E;
        if (doubledBitMap & openBitMap) {
            count = bitCnt(doubledBitMap & openBitMap);
            mid_score[color] -= count * DOUBLED_OPEN_O;
            end_score[color] -= count * DOUBLE_OPEN_E;
        }
    }

    if (targetBitMap) { //isolated or vulnerable backward pawns
        //target pawns that are not A or H file pawns are especially heinus
        count = bitCnt(targetBitMap);
        mid_score[color] -= count * TARGET_O;
        end_score[color] -= count * TARGET_E;
        uint64 RFileTargetBitMap = targetBitMap & (FileABB | FileHBB);
        if (RFileTargetBitMap) {
            count = bitCnt(RFileTargetBitMap);
            mid_score[color] += count * TARGET_O/2;
            end_score[color] += count * TARGET_E/2;
            if (RFileTargetBitMap & openBitMap) {
                count = bitCnt(RFileTargetBitMap & openBitMap);
                mid_score[color] += count * TARGET_OPEN_O/3;
                end_score[color] += count * TARGET_OPEN_E/3;
            }
        }
        if (targetBitMap & openBitMap) {
            count = bitCnt(targetBitMap & openBitMap);
            mid_score[color] -= count * TARGET_OPEN_O;
            end_score[color] -= count * TARGET_OPEN_E;
        }
        if (targetBitMap & doubledBitMap) {
            count = bitCnt(doubledBitMap & targetBitMap);
            mid_score[color] -= count * DOUBLE_TARGET_O;
            end_score[color] -= count * DOUBLE_TARGET_E;
            if (targetBitMap & doubledBitMap & openBitMap) {
                count = bitCnt(doubledBitMap & targetBitMap & openBitMap);//all isolated are also backward
                mid_score[color] -= count * DOUBLE_TARGET_OPEN_O;
                end_score[color] -= count * DOUBLE_TARGET_OPEN_E;
            }
        }
    }

    //backward pawns are addressed here as are all other pawn weaknesses to some extent
    //these are pawns that are not guarded, and are not adjacent to other pawns
    temp64 = ei->pawns[color] & ~(ei->atkpawns[color] | shiftLeft(ei->pawns[color],1) |
        shiftRight(ei->pawns[color],1));
    {
        int enemyKing = pos->kpos[color^1];
        while (temp64) {
            uint64 openPawn, doublePawn, sqBitMask;
            int sqPenalty;
            int weakness = 0;
            sq = popFirstBit(&temp64);
            sqBitMask = BitMask[sq];

            openPawn = openBitMap & sqBitMask;
            doublePawn = doubledBitMap & sqBitMask;
            //fixedPawn = (BitMask[sq + PAWN_MOVE_INC(color)] & (ei->atkpawns[enemy] | ei->pawns[enemy]));
            sqPenalty = weakFile[SQFILE(sq)] + weakRank[PAWN_RANK(sq,color)]; //TODO precale 64 entries
            mid_score[color] -= sqPenalty;	//penalty for being weak in middlegame
            end_score[color] -= kingAttackWeakness[DISTANCE(sq,enemyKing)]; //end endgame, king attacks is best measure of vulnerability
            if (showEval) Print(3,"w %s ",sq2Str(sq));
            //if (showEval && (backwardBitMap & sqBitMask)) Print(3," back %s ",sq2Str(sq));
            if (openPawn) {
                mid_score[color] -= sqPenalty;
            }
            /*
            if (fixedPawn) { //if we cannot move it
            if (showEval) Print(3,"f %s ",sq2Str(sq));
            mid_score[color] -= WEAK_FIXED_O;
            end_score[color] -= WEAK_FIXED_E;
            }*/
        }
    }
    evalShelter(color,ei,pos);


    ei->pawn_entry->passedbits |= passedBitMap;

    ei->mid_score[color] += mid_score[color];
    ei->end_score[color] += end_score[color];
}
inline int outpost(const position_t *pos, eval_info_t *ei, const int color, const int enemy, const int sq) {
    int outpostValue = OutpostValue[color][sq];
    if (outpostValue>0) {
        if (BitMask[sq] & ei->atkpawns[color]) {
            if (!(pos->knights & pos->color[enemy]) && !((BitMask[sq] & WhiteSquaresBB) ?
                (pos->bishops & pos->color[enemy] & WhiteSquaresBB) : (pos->bishops & pos->color[enemy] & BlackSquaresBB)))
                outpostValue += outpostValue + outpostValue/2;
            else outpostValue += outpostValue/2;
        }
    }
    return outpostValue;
}
void evalPieces(const position_t *pos, eval_info_t *ei, const int color, const int thread) {

    uint64 pc_bits, temp64, katemp64,xtemp64, weak_sq_mask;
    int from, temp1;
    const int enemy = color ^ 1;
    uint64 mobileSquare;
    uint64 notOwnColor =  ~pos->color[color];
    uint64 notOwnSkeleton;
    int threatScore = 0;
    uint64 boardSkeleton = pos->pawns;

    if (0==(pos->posStore.castle & Castle[color])) boardSkeleton |= (pos->kings & pos->color[color]);
    notOwnSkeleton = ~(boardSkeleton & pos->color[color]);

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);
    ASSERT(colorIsOk(color));
    mobileSquare = ~ei->atkpawns[enemy] & notOwnColor;
    weak_sq_mask = WeakSqEnemyHalfBB[color] & ~ei->potentialPawnAttack[color];

    pc_bits = pos->knights & pos->color[color];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = KnightMoves[from];
        ei->atkknights[color] |= temp64;
        if (ei->flags & ONE_SIDED_PAWNS) ei->end_score[color] +=  N_ONE_SIDE;
        if (temp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(temp64 & ei->kingadj[enemy]) + (KnightAttackValue<<10);
        temp1 = bitCnt(temp64 & mobileSquare);
        ei->mid_score[color] += temp1 * MidgameKnightMob;
        ei->end_score[color] += temp1 * EndgameKnightMob;
        if (BitMask[from] & weak_sq_mask) {
            temp1 = outpost(pos,ei,color,enemy,from);
            ei->mid_score[color] += temp1;
            ei->end_score[color] += temp1;
        }
    }
    pc_bits = pos->bishops & pos->color[color] ;
    ei->kingatkbishops[color] = 0;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);

        temp64 = bishopAttacksBB(from, pos->occupied);
        katemp64 = bishopAttacksBB(from, pos->occupied & ~pos->queens);
        ei->kingatkbishops[color] |= katemp64;
        ei->atkbishops[color] |= temp64;
        if (katemp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(katemp64 & ei->kingadj[enemy])*2 + (BishopAttackValue<<10);

        temp1 = bitCnt(temp64 & mobileSquare);
        ei->mid_score[color] += (temp1) * MidgameBishopMob;
        ei->end_score[color] += temp1 * EndgameBishopMob;
        xtemp64 = bishopAttacksBB(from, boardSkeleton) & notOwnSkeleton;

        if (xtemp64 & notOwnColor & (pos->kings | pos->queens)) {
            threatScore += (BishopAttackPower * QueenAttacked)/2;
        }
        if (xtemp64 & notOwnColor & pos->rooks) {
            threatScore += (BishopAttackPower * RookAttacked)/2;
        }
        temp1 = bitCnt(xtemp64);
        ei->mid_score[color] += (temp1) * MidgameXrayBishopMob;
        ei->end_score[color] += temp1 * EndgameXrayBishopMob;
        //TODO consider being a bit more accurate with king
        if (temp1 < 3) {
            ei->mid_score[color] -= personality(thread).stuck_bishop;
            ei->end_score[color] -= personality(thread).stuck_bishop*personality(thread).stuck_end;
            if (showEval) Print(3," stuck bishop %s %d %d",sq2Str(from),personality(thread).stuck_bishop,personality(thread).stuck_bishop*personality(thread).stuck_end);
        }
        if (BitMask[from] & weak_sq_mask) {
            temp1 = outpost(pos,ei,color,enemy,from)/2;
            ei->mid_score[color] += temp1;
            ei->end_score[color] += temp1;

        }

    }
    pc_bits = pos->rooks & pos->color[color];
    ei->kingatkrooks[color] = 0;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = rookAttacksBB(from, pos->occupied);
        katemp64 = rookAttacksBB(from, pos->occupied & ~(pos->queens|pos->rooks));
        ei->kingatkrooks[color] |= katemp64;
        ei->atkrooks[color] |= temp64;

        if (katemp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(katemp64 & ei->kingadj[enemy])*2 + (RookAttackValue<<10);
        temp1 = bitCnt(temp64 & mobileSquare);
        ei->mid_score[color] += temp1 * MidgameRookMob;
        ei->end_score[color] += temp1 * EndgameRookMob;

        xtemp64 = rookAttacksBB(from, boardSkeleton) & notOwnSkeleton;
        temp1 = bitCnt(xtemp64);
        if (xtemp64 & notOwnColor & (pos->kings | pos->queens)) {
            threatScore += (RookAttackPower * QueenAttacked)/2;
        }
        //TODO consider being a bit more accurate with king
        if (temp1 < 3) {
            ei->mid_score[color] -= personality(thread).stuck_rook;
            ei->end_score[color] -= personality(thread).stuck_rook*personality(thread).stuck_end;
            if (showEval) Print(3," stuck rook %s ",sq2Str(from));
        }
        ei->mid_score[color] += temp1 * MidgameXrayRookMob;
        ei->end_score[color] += temp1 * EndgameXrayRookMob;

        //its good to be lined up with a lot of enemy pawns (7th rank most common example)
        uint64 pressured = rookAttacksBB(from, ((pos->pawns | pos->kings) & pos->color[color]));
        uint64 pawnsPressured = pressured & ei->pawns[enemy] & ~ei->atkpawns[enemy]; 
        if (pawnsPressured) {
            int numPawnsPressured = bitCnt(pawnsPressured);
            if (showEval) Print(3,"pp %d ",numPawnsPressured);
            ei->mid_score[color] += MidgameRookPawnPressure * numPawnsPressured;
            ei->mid_score[color] += EndgameRookPawnPressure * numPawnsPressured;
        }

        if (BitMask[from] & Rank7ByColorBB[color]) { //7th rank is also good for other reasons
            if ((pos->pawns & pos->color[enemy] & Rank7ByColorBB[color]) || (BitMask[pos->kpos[enemy]] & Rank8ByColorBB[color])) {
                ei->mid_score[color] += MidgameRook7th;
                ei->end_score[color] += EndgameRook7th;
            }
        }
        
    }
    pc_bits = pos->queens & pos->color[color];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = queenAttacksBB(from, pos->occupied);
        ei->atkqueens[color] |= temp64;


        if (temp64 & ei->kingzone[enemy]) {
            ei->atkcntpcs[enemy] += (1<<20) + bitCnt(temp64 & ei->kingadj[enemy])*2 + (QueenAttackValue<<10);
            if (ei->kingadj[enemy] & ei->atkpawns[color]) ei->atkcntpcs[enemy] += (1<<20);
        }

        temp1 = bitCnt(temp64 & mobileSquare);
        ei->mid_score[color] += temp1 * MidgameQueenMob;
        ei->end_score[color] += temp1 * EndgameQueenMob;

        xtemp64 = queenAttacksBB(from, boardSkeleton) & notOwnSkeleton;
        temp1 = bitCnt(xtemp64);
        //TODO consider being a bit more accurate with king
        if (temp1 < 3) {
            ei->mid_score[color] -= personality(thread).stuck_queen;
            ei->end_score[color] -= personality(thread).stuck_queen*personality(thread).stuck_end;
            if (showEval) Print(3," stuck queen %s ",sq2Str(from));
        }
        ei->mid_score[color] += temp1 * MidgameXrayQueenMob;
        ei->end_score[color] += temp1 * EndgameXrayQueenMob;

        if (xtemp64 & notOwnColor & pos->kings) {
            threatScore += (QueenAttackPower * QueenAttacked)/2;
        }

        if (BitMask[from] & Rank7ByColorBB[color]) {
            if ((pos->pawns & pos->color[enemy] & Rank7ByColorBB[color]) || (BitMask[pos->kpos[enemy]] & Rank8ByColorBB[color])) {
                ei->mid_score[color] += MidgameQueen7th;
                ei->end_score[color] += EndgameQueen7th;
            }
        }
    }
    ei->atkall[color] = ei->atkpawns[color] | ei->atkknights[color] | ei->atkbishops[color] | ei->atkrooks[color] | ei->atkqueens[color] ;
    ei->atkkings[color] = KingMoves[pos->kpos[color]];
    ei->atkall[color] |= ei->atkkings[color];
    ei->mid_score[color] += threatScore * PieceAttackMulMid;
    ei->end_score[color] += threatScore * PieceAttackMulEnd;
}

const int ThreatBonus[9] = {0,TB1, TB1+TB2, TB1+TB2+20, TB1+TB2+40, TB1+TB2+50, TB1+TB2+60, TB1+TB2+20+70, TB1+TB2+20+80}; //021713 because of upside
void evalThreats(const position_t *pos, eval_info_t *ei, const int color, int *upside) {
    uint64 temp64, not_guarded, enemy_pcs;
    int temp1;

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);
    ASSERT(colorIsOk(color));

    temp1 = 0;
    enemy_pcs = pos->color[color^1] & ~(pos->pawns|pos->kings);

    not_guarded = ~ei->atkpawns[color^1];
    temp64 = ei->atkpawns[color] & enemy_pcs;
    if (temp64) {
        temp1 += PawnAttackPower * QueenAttacked * ((temp64 & pos->queens)!=0);
        temp1 += PawnAttackPower * RookAttacked* ((temp64 & pos->rooks)!=0);
        temp1 += PawnAttackPower * BishopAttacked* ((temp64 & pos->bishops)!=0);
        temp1 += PawnAttackPower * KnightAttacked* ((temp64 & pos->knights)!=0);
    }

    temp64 = ei->atkknights[color] & enemy_pcs;
    if (temp64) {

        temp1 += KnightAttackPower * QueenAttacked *((temp64 & pos->queens)!=0);
        temp1 += KnightAttackPower * QueenAttacked *((temp64 & pos->queens)!=0);
        temp1 += KnightAttackPower * RookAttacked *((temp64 & pos->rooks)!=0);
        temp1 += KnightAttackPower * BishopAttacked *((temp64 & pos->bishops & not_guarded)!=0);
    }

    temp64 = ei->atkbishops[color] & enemy_pcs;
    if (temp64) {
        temp1 += BishopAttackPower * QueenAttacked * ((temp64 & pos->queens)!=0);
        temp1 += BishopAttackPower * RookAttacked * ((temp64 & pos->rooks)!=0);
        temp1 += BishopAttackPower * KnightAttacked * ((temp64 & pos->knights & not_guarded)!=0);
    }
    temp64 = ei->atkrooks[color] & enemy_pcs;
    if (temp64) {
        temp1 += RookAttackPower * QueenAttacked * ((temp64 & pos->queens)!=0);
        temp1 += RookAttackPower * BishopAttacked * ((temp64 & pos->bishops & not_guarded)!=0);
        temp1 += RookAttackPower * KnightAttacked * ((temp64 & pos->knights & not_guarded)!=0);
    }

    temp64 = ei->atkqueens[color] & enemy_pcs;
    if (temp64) {
        temp1 += QueenAttackPower * RookAttacked * ((temp64 & pos->rooks & not_guarded)!=0);
        temp1 += QueenAttackPower * BishopAttacked * ((temp64 & pos->bishops & not_guarded)!=0);
        temp1 += QueenAttackPower * KnightAttacked * ((temp64 & pos->knights & not_guarded)!=0);
    }

    ei->mid_score[color] += temp1 * PieceAttackMulMid;
    ei->end_score[color] += temp1 * PieceAttackMulEnd;
    ///now some serious threats
    {
        uint64 threatB = enemy_pcs & ei->atkall[color] & ~ei->atkall[color^1];
        uint64 target = enemy_pcs & pos->queens;

        threatB |= (ei->atkrooks[color]) & target;
        target |= enemy_pcs & pos->rooks;
        threatB |= (ei->atkbishops[color] | ei->atkknights[color]) & target;
        target |= enemy_pcs & (pos->knights | pos->bishops);
        threatB |= ei->atkpawns[color] & target;

        if (threatB) {
            int numThreats = bitCnt(threatB);
            if (showEval) {
                if (color==WHITE) Print(3,"WTHR %d ",numThreats);
                else Print(3,"BTHR %d ",numThreats);
            }
            if (pos->side != color) *upside += ThreatBonus[numThreats]; 
            //only really takes double threats for the opponent seriously, since its not handled well by qsearch and such
            numThreats += (pos->side==color);
            ei->mid_score[color] += ThreatBonus[numThreats];
            ei->end_score[color] += ThreatBonus[numThreats];
        }
    }
}

void KingShelter(const int color, eval_info_t *ei, const position_t *pos,int danger) {
    int currentKing = ei->pawn_entry->shelter[color];

    int kingSide = ((pos->posStore.castle & KSC[color])==0 || (ei->atkall[color^1] & KCSQ[color])) ?  0 : ei->pawn_entry->kshelter[color];
    int queenSide =  ((pos->posStore.castle & QSC[color])==0 || (ei->atkall[color^1] & QCSQ[color])) ? 0 : ei->pawn_entry->qshelter[color];


    kingSide = (kingSide > queenSide) ? kingSide : queenSide;
    currentKing = (currentKing >= kingSide) ? currentKing : (currentKing/2 + kingSide/2);

    danger += K_SHELT_W;
    currentKing *= danger;
    if (currentKing > (LOW_SHELTER*danger)) currentKing -= (currentKing-(LOW_SHELTER*danger))/2;
    currentKing = (currentKing-(9*danger))/2;

    if (showEval) {
        if (color==WHITE) Print(3," ws %d ",currentKing);
        else Print(3," bs %d ",currentKing);
    }
    ei->mid_score[color] += currentKing;
}

void evalKingAttacks(const position_t *pos, eval_info_t *ei, const int color, int *upside) {

    int danger;
    uint64 pc_atkrs_mask, pc_atkhelpersmask, king_atkmask, pc_defenders_mask;
    int penalty, tot_atkrs, pc_weights, kzone_atkcnt;
    ASSERT(pos != NULL);
    ASSERT(ei != NULL);
    ASSERT(colorIsOk(color));

    tot_atkrs = ei->atkcntpcs[color] >> 20;
    kzone_atkcnt = ei->atkcntpcs[color] & ((1 << 10) - 1);

    danger = (tot_atkrs>=2 && kzone_atkcnt>=1);
    KingShelter(color, ei, pos,kzone_atkcnt+tot_atkrs);
    if (danger) {

        king_atkmask = KingMoves[pos->kpos[color]];
        pc_weights = (ei->atkcntpcs[color] & ((1 << 20) - 1)) >> 10;
        penalty = KingPosPenalty[color][pos->kpos[color]] + ((pc_weights * tot_atkrs) / 2)
            + kzone_atkcnt;
        pc_defenders_mask = ei->atkqueens[color] | ei->atkrooks[color] | ei->atkbishops[color] | ei->atkknights[color] | ei->atkpawns[color];
        penalty += bitCnt(king_atkmask & ei->atkall[color^1] & ~pc_defenders_mask);
        pc_atkrs_mask = king_atkmask & ei->atkqueens[color^1] & (~pos->color[color^1]);
        if (pc_atkrs_mask) {
            uint64 queenContact;
            pc_atkhelpersmask = (ei->atkkings[color^1] | ei->kingatkrooks[color^1] | ei->kingatkbishops[color^1] | ei->atkknights[color^1] | ei->atkpawns[color^1]);
            pc_atkrs_mask &= pc_atkhelpersmask;
            queenContact = pc_atkrs_mask & ~pc_defenders_mask;

            if (queenContact) {
                int bonus = ((pos->side == (color^1)) ? 2 : 1) * QueenSafeContactCheckValue;
                uint64 kingEscape = king_atkmask & ~(pc_atkhelpersmask | pos->color[color]);
                do {
                    int sq = popFirstBit(&queenContact);
                    uint64 queenCovers = queenAttacksBB(sq, pos->occupied);
                    /*
                    if (showEval) {
                    Print(3," qcontact %s \n",sq2Str(sq));
                    showBitboard(queenCovers,3);

                    Print(3,"\n");
                    showBitboard(kingEscape,3);
                    }*/
                    penalty += bonus;
                    if (0==(kingEscape & ~queenCovers)) { //checkmate
                        //MAKE SURE IT IS REALLY MATE
                        // first sliders
                        uint64 tempOcc = pos->occupied & ~(pos->queens & pos->color[color^1]); // a little conservative when there are two queens but that is OK

                        if (bishopAttacksBB(sq, tempOcc) & (pos->bishops|pos->queens) & pos->color[color]) continue;
                        if (rookAttacksBB(sq, tempOcc) & (pos->rooks|pos->queens) & pos->color[color]) continue;
                        //FOR NOW IGNORE QUEEN IS PINNED ISSUES
                        if (pos->side == (color^1)) {
                            ei->mid_score[color] -= 10000;
                            ei->end_score[color] -= 10000; //CHECKMATE
                            if (showEval) Print(3," checkmate %s ",sq2Str(sq));
                        }
                        else {
                            penalty += MATE_CODE;
                            *upside += 10000; // maybe checkmate!!
                            if (showEval) Print(3, " MateThreat %s ",sq2Str(sq));
                        }
                    }
                    /*
                    else if (showEval) {
                    Print(3,"\n");
                    showBitboard((kingEscape & ~queenCovers),3);
                    }*/

                } while (queenContact);
            }
        }

        pc_atkrs_mask = rookAttacksBB(pos->kpos[color], pos->occupied) & ~ei->atkall[color] & ~pos->color[color^1];
        if (pc_atkrs_mask & ei->atkqueens[color^1]) {
            int numQueenChecks = bitCnt(pc_atkrs_mask & ei->atkqueens[color^1]);
            penalty += numQueenChecks * QueenSafeCheckValue;
        }
        if (pc_atkrs_mask & ei->kingatkrooks[color^1]) penalty += bitCnt(pc_atkrs_mask & ei->kingatkrooks[color^1]) * RookSafeCheckValue;
        pc_atkrs_mask = bishopAttacksBB(pos->kpos[color], pos->occupied) & ~ei->atkall[color] & ~pos->color[color^1];
        if (pc_atkrs_mask & ei->atkqueens[color^1]) penalty += bitCnt(pc_atkrs_mask & ei->atkqueens[color^1]) * QueenSafeCheckValue;
        if (pc_atkrs_mask & ei->kingatkbishops[color^1]) penalty += bitCnt(pc_atkrs_mask & ei->kingatkbishops[color^1]) * BishopSafeCheckValue;
        pc_atkrs_mask = KnightMoves[pos->kpos[color]] & ~ei->atkall[color] & ~pos->color[color^1];
        if (pc_atkrs_mask & ei->atkknights[color^1]) penalty += bitCnt(pc_atkrs_mask & ei->atkknights[color^1]) * KnightSafeCheckValue;
        pc_atkrs_mask = discoveredCheckCandidates(pos, color^1) & ~pos->pawns;
        if (pc_atkrs_mask) penalty += bitCnt(pc_atkrs_mask) * DiscoveredCheckValue;
        ei->mid_score[color] -= (penalty * (penalty-EXP_PENALTY) * KING_ATT_W) / 20; 
        *upside += (penalty * penalty*KING_ATT_W)/20; 

        if (showEval) {
            if (color==WHITE) Print(3, " wa %d",penalty * (penalty-EXP_PENALTY)); 
            else Print(3, " ba %d",penalty * (penalty-EXP_PENALTY) * KING_ATT_W/ 20);
        }
    }
    else if (PARTIAL_ATTACK) { //if there is a maximum of 1 active attacker
        if (showEval) Print(3, " pa %d", kzone_atkcnt*PARTIAL_ATTACK);
        ei->mid_score[color] -= kzone_atkcnt*PARTIAL_ATTACK;
    }
}

void evalPassedvsKing(const position_t *pos, eval_info_t *ei, const int allied,uint64 passed) {
    const int enemy = allied ^ 1;

    int leftBestFile = FileH;
    int leftBestToQueen = 8;
    int rightBestFile = FileA;
    int rightBestToQueen = 8;
    int fileDist;

    const int myMove = (pos->side == allied);
    uint64 passedpawn_mask = passed;
    do {
        int from = popFirstBit(&passedpawn_mask);
        int score = 0;
        int rank = PAWN_RANK(from, allied);
        uint64 prom_path = (*FillPtr[allied])(BitMask[from]);
        uint64 path_attkd = prom_path & ei->atkall[enemy];
        uint64 path_dfndd = prom_path & ei->atkall[allied];

        if (!(prom_path & pos->color[enemy])) {
            if (!path_attkd) score += (prom_path == path_dfndd) ? PathFreeNotAttackedDefAllPasser : PathFreeNotAttackedDefPasser;
            else score += ((path_attkd & path_dfndd) == path_attkd) ? PathFreeAttackedDefAllPasser : PathFreeAttackedDefPasser;
        } else if (((path_attkd | (prom_path & pos->color[enemy])) & ~path_dfndd) == EmptyBoardBB) score += PathNotFreeAttackedDefPasser;
        if ((prom_path & pos->color[allied]) == EmptyBoardBB) score += PathFreeFriendPasser;
        if (ei->atkpawns[allied] & BitMask[from]) score += DefendedByPawnPasser;
        if (ei->atkpawns[allied] & BitMask[from + PAWN_MOVE_INC(allied)]) {
            score += DuoPawnPasser; 
            if (showEval) Print(3," duo passer %s ",sq2Str(from));
        }
        ei->mid_score[allied] += MidgamePassedMin + scale(MidgamePassedMax, rank);
        score += EndgamePassedMax;
        {
            int frontSq = from + PAWN_MOVE_INC(allied);
            score += DISTANCE(frontSq, pos->kpos[enemy]) * PassedPawnDefenderDistance;
            score -= DISTANCE(frontSq, pos->kpos[allied]) * PassedPawnAttackerDistance;
        }
        {
            int pFile = SQFILE(from);
            int qDist = 7-rank - myMove;
            if (pFile+qDist<leftBestFile + leftBestToQueen) {
                leftBestFile = pFile;
                leftBestToQueen = qDist;
            }
            if ((7-pFile)+qDist<(7-rightBestFile) + rightBestToQueen) {
                rightBestFile = pFile;
                rightBestToQueen = qDist;
            }
            // later we can analyze moving king out of way but for now lets let search do it

            if (!(prom_path & pos->occupied)) {
                int oppking = pos->kpos[enemy];
                int promotion = PAWN_PROMOTE(from, allied);
                int eDist = DISTANCE(oppking, promotion);

                if (showEval) Print(3," passed rank %d qdist %d ",rank,qDist);

                if (eDist-1 > qDist) {
                    score += UnstoppablePassedPawn; 
                    ei->queening = true;
                    if (showEval) Print(3," unstoppable ");
                }
                // or king can escort pawn in
                else if (qDist <=2) {
                    int mDist = DISTANCE(pos->kpos[allied], promotion)-myMove;
                    // SAMNOTE TRY EDITING OUT ROOKPAWN FILES
                    if (pFile == FileA || pFile == FileH) mDist++; // need more margin for error for rook pawns NOTE: may need to check for king in from of pawn if we allow this later
                    if (mDist < eDist || (mDist==eDist && myMove)) { //if this race is a tie we can still win it, but must spend move on king first not pawn
                        score += UnstoppablePassedPawn;
                        ei->queening = true;
                        if (showEval) Print(3," unstopable king escort ");
                    }
                }

            }

        }
        ei->end_score[allied] += EndgamePassedMin+scale(score, rank);

    } while (passedpawn_mask);

    fileDist = rightBestFile-leftBestFile;
    if (showEval) Print(3," file dist %d ",fileDist);
    if (leftBestToQueen > rightBestToQueen) {
        if (fileDist > leftBestToQueen) { 
            ei->end_score[allied] += scale(UnstoppablePassedPawn, 7-leftBestToQueen);
            if (showEval) Print(3," unstoppable separated %d ",leftBestToQueen);
            ei->queening = true;
        }
    }
    else if (fileDist > rightBestToQueen) { 
        ei->end_score[allied] += scale(UnstoppablePassedPawn, 7-rightBestToQueen);
        if (showEval) Print(3," unstoppable separated %d ",rightBestToQueen);
        ei->queening = true;
    }
}

void evalPassed(const position_t *pos, eval_info_t *ei, const int allied,uint64 passed) {
    uint64 passedpawn_mask = passed;
    int myMove = (pos->side == allied);
    const int enemy = allied ^ 1;
    do { 
        int from = popFirstBit(&passedpawn_mask);
        int score = 0;
        int rank = PAWN_RANK(from, allied);
        uint64 prom_path = (*FillPtr[allied])(BitMask[from]);
        uint64 path_attkd = prom_path & ei->atkall[enemy];
        uint64 path_dfndd = prom_path & ei->atkall[allied];
        uint64 rooksBehind = (*FillPtr[enemy])(BitMask[from]) & pos->rooks;
        int frontSq = from + PAWN_MOVE_INC(allied);

        ei->mid_score[allied] += MidgamePassedMin + scale(MidgamePassedMax, rank); 
        score += EndgamePassedMax;
        score += DISTANCE(frontSq, pos->kpos[enemy]) * PassedPawnDefenderDistance;
        score -= DISTANCE(frontSq, pos->kpos[allied]) * PassedPawnAttackerDistance;

        if (rooksBehind & pos->color[enemy]) path_attkd |= prom_path;
        if (rooksBehind & pos->color[allied]) path_dfndd |= prom_path;
        if ((prom_path & pos->color[enemy]) == EmptyBoardBB) {
            if (path_attkd == EmptyBoardBB) score += (prom_path == path_dfndd) ? PathFreeNotAttackedDefAllPasser : PathFreeNotAttackedDefPasser;
            else score += ((path_attkd & path_dfndd) == path_attkd) ? PathFreeAttackedDefAllPasser : PathFreeAttackedDefPasser;
        } else if (((path_attkd | (prom_path & pos->color[enemy])) & ~path_dfndd) == EmptyBoardBB) score += PathNotFreeAttackedDefPasser;
        if ((prom_path & pos->color[allied]) == EmptyBoardBB) score += PathFreeFriendPasser;
        if (ei->atkpawns[allied] & BitMask[from]) score += DefendedByPawnPasser;
        if (ei->atkpawns[allied] & BitMask[frontSq]) {
            score += DuoPawnPasser; 
            if (showEval) Print(3," duo passer %s ",sq2Str(from));
        }

        if (passed & (PawnCaps[from][allied] | PawnCaps[frontSq][enemy])
            && ((pos->bishops|pos->queens) & pos->color[enemy])==0 && ei->queening == false && rank >= Rank6 && MaxOneBit(pos->color[enemy] & ~(pos->kings | pos->pawns)))	//CON
        {
            int promotion = PAWN_PROMOTE(from, allied);
            int kDist = DISTANCE(pos->kpos[enemy],promotion) - (!myMove);
            if (kDist > (7-rank-myMove) )
            {
                score += UnstoppablePassedPawn+RookValueMid2 - PawnValueMid2;
                ei->queening = true;
                if (showEval) Print(3,"info string unstopable connected ");
            }
        }

        ei->end_score[allied] += EndgamePassedMin + scale(score, rank);

    } while (passedpawn_mask);
}

void evalPawns(const position_t *pos, eval_info_t *ei, int thread_id) {

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);

    initPawnEvalByColor(pos, ei, WHITE);
    initPawnEvalByColor(pos, ei, BLACK);

    ei->pawn_entry = SearchInfo(thread_id).pt.Entry(pos->posStore.phash);
    if (USE_PHASH && ei->pawn_entry->hashlock == LOCK(pos->posStore.phash)) {
        ei->mid_score[WHITE] += ei->pawn_entry->opn;
        ei->end_score[WHITE] += ei->pawn_entry->end;
    } else {

        {
            int midpawnscore[2] = {0, 0};
            int endpawnscore[2] = {0, 0};
            ei->pawn_entry->passedbits = 0;
            evalPawnsByColor(pos, ei, midpawnscore, endpawnscore, WHITE);
            evalPawnsByColor(pos, ei, midpawnscore, endpawnscore, BLACK);
            ei->pawn_entry->hashlock = LOCK(pos->posStore.phash);
            ei->pawn_entry->opn = midpawnscore[WHITE] - midpawnscore[BLACK];
            ei->pawn_entry->end = endpawnscore[WHITE] - endpawnscore[BLACK];
        }
    }
}


int eval(const position_t *pos, int thread_id, int *pessimism) {
    eval_info_t ei;
    material_info_t *mat;
    int open, end, score;
    int winning;
    EvalEntry *entry;
    uint64 whitePassed, blackPassed;
    int upside[2] = {0,0}; //this should never be negative

    entry = SearchInfo(thread_id).et.Entry(pos->posStore.hash);
    if (entry->hashlock == LOCK(pos->posStore.hash)) {
        *pessimism = entry->pessimism; //this was meant to be * 10
        if (showEval) Print(3," from hash ");
        return entry->value;

    }

    ASSERT(pos != NULL);
    ei.MLindex[WHITE] = pos->posStore.mat_summ[WHITE];
    ei.MLindex[BLACK] = pos->posStore.mat_summ[BLACK];

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
    } else {
        score = computeMaterial(pos, &ei);
    }
    if (showEval) {
        Print(3,"info string mat %d ",score);
    }

    ei.mid_score[WHITE] = pos->posStore.open[WHITE];
    ei.mid_score[BLACK] = pos->posStore.open[BLACK];
    ei.end_score[WHITE] = pos->posStore.end[WHITE];
    ei.end_score[BLACK] = pos->posStore.end[BLACK];

    ei.flags = 0;
    ei.atkpawns[WHITE] = ei.atkpawns[BLACK] = 0;
    ei.atkknights[WHITE] = ei.atkknights[BLACK] = 0;
    ei.atkbishops[WHITE] = ei.atkbishops[BLACK] = 0;
    ei.atkrooks[WHITE] = ei.atkrooks[BLACK] = 0;
    ei.atkqueens[WHITE] = ei.atkqueens[BLACK] = 0;
    ei.atkkings[WHITE] = ei.atkkings[BLACK] = 0;
    ei.atkcntpcs[WHITE] = ei.atkcntpcs[BLACK] = 0;

    ei.kingadj[WHITE] = KingMoves[pos->kpos[WHITE]];
    ei.kingadj[BLACK] = KingMoves[pos->kpos[BLACK]];
    ei.kingzone[WHITE] = ei.kingadj[WHITE] | (ei.kingadj[WHITE] << 8) | (ei.kingadj[WHITE] >> 8);
    ei.kingzone[BLACK] = ei.kingadj[BLACK] | (ei.kingadj[BLACK] >> 8) | (ei.kingadj[BLACK] << 8);

    ei.mid_score[pos->side] += personality(thread_id).tempo_open;
    ei.end_score[pos->side] += personality(thread_id).tempo_end;

    evalPawns(pos, &ei, thread_id);
    {
        int oneSided = ((QUEENSIDE & pos->pawns) == 0 ||
            (KINGSIDE & pos->pawns) == 0);
        if (oneSided) {
            ei.flags |= ONE_SIDED_PAWNS;
        }
    }
    {
        int oppBishops = (
            (pos->bishops & pos->color[WHITE]) && (pos->bishops & pos->color[BLACK]) &&
            MaxOneBit((pos->bishops & pos->color[WHITE])) &&
            MaxOneBit((pos->bishops & pos->color[BLACK])) &&
            (((pos->bishops & pos->color[WHITE] & WhiteSquaresBB)==0) !=
            ((pos->bishops & pos->color[BLACK] & WhiteSquaresBB)==0)));
        if (oppBishops) {
            ei.draw[WHITE] += OB_WEIGHT;
            ei.draw[BLACK] += OB_WEIGHT;

            ei.flags |= OPPOSITE_BISHOPS;			
        }
    }

    ei.flags |= ATTACK_KING[BLACK] * (ei.MLindex[WHITE] >= MLQ + MLN);
    ei.flags |= ATTACK_KING[WHITE] * (ei.MLindex[BLACK] >= MLQ + MLN);

    evalPieces(pos, &ei, WHITE,thread_id);
    evalPieces(pos, &ei, BLACK,thread_id);

    ei.queening = false;

    blackPassed = ei.pawn_entry->passedbits & pos->color[BLACK];

    if (pos->color[WHITE] & ~pos->pawns & ~pos->kings) {//if white has a piece
        if (blackPassed) evalPassed(pos,&ei,BLACK,blackPassed);
        evalThreats(pos, &ei, WHITE, &upside[WHITE]);
        // attacking the black king
        if (ei.flags & ATTACK_KING[BLACK]) {
            evalKingAttacks(pos, &ei, BLACK,&upside[WHITE]);
        }
        judgeTrapped(pos,&ei,WHITE,thread_id/*,&upside[WHITE],&upside[BLACK]*/);
    }
    else {
        if (blackPassed) evalPassedvsKing(pos,&ei,BLACK,blackPassed);
    }
    whitePassed = ei.pawn_entry->passedbits & pos->color[WHITE];
    if (pos->color[BLACK] & ~pos->pawns & ~pos->kings) { //if black has a piece
        if (whitePassed) evalPassed(pos,&ei,WHITE,whitePassed);
        evalThreats(pos, &ei, BLACK, &upside[BLACK]);
        if (ei.flags & ATTACK_KING[WHITE]) { // attacking the white king
            evalKingAttacks(pos, &ei, WHITE,&upside[BLACK]);
        }
        judgeTrapped(pos,&ei,BLACK,thread_id/*,&upside[WHITE],&upside[BLACK]*/);
    }
    else {
        if (whitePassed) evalPassedvsKing(pos,&ei,WHITE,whitePassed);
    }

    open = ei.mid_score[WHITE] - ei.mid_score[BLACK];
    end = ei.end_score[WHITE] - ei.end_score[BLACK];
    {
        int posVal = ((open * ei.phase) + (end * (32 - ei.phase))) / (32); //was 32, /16 means doubling the values
        score += posVal;
    }
    winning = (score < DrawValue[WHITE]);

    // if there is an unstoppable pawn on the board, we cannot trust any of our draw estimates or endgame rules of thumb
    if (!ei.queening) {
        int edge = score < DrawValue[WHITE];
        int draw = ei.draw[edge]; //this is who is trying to draw 
        if (showEval) Print(3," preenddraw %d ",draw );
        evalEndgame(edge, pos, &ei, &score, &draw, pos->side);
        draw = MIN(MAX_DRAW,draw); // max of 200 draw
        if (showEval) Print(3," draw %d ",draw );
        score = ((score * (MAX_DRAW-draw)) + (DrawValue[WHITE] * draw))/MAX_DRAW; 
    }
    score = score*sign[pos->side];
    *pessimism = upside[pos->side^1]; //make sure this can fit in 8 bits 

    if (score < -MAXEVAL) score = -MAXEVAL;
    else if (score > MAXEVAL) score = MAXEVAL;

    entry->hashlock = LOCK(pos->posStore.hash);
    entry->pessimism = *pessimism;
    entry->value = score;
    return score;
}

