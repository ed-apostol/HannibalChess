/**************************************************/
/*  Name: Twisted Logic Chess Engine              */
/*  Copyright: 2009                               */
/*  Author: Edsel Apostol                         */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

// patterns
int BlockedRookPenalty = 60;
int TrappedBishopPenalty = 60;
int BlockedCenterPawnsPenalty = 60;

// side to move bonus
int MidgameTempo = 20;
int EndgameTempo = 10;

// piece mobilities
int MidgameKnightMob = 6;
int EndgameKnightMob = 8;
int MidgameBishopMob = 5;
int EndgameBishopMob = 5;
int MidgameRookMob = 3;
int EndgameRookMob = 2;
int MidgameQueenMob = 2;
int EndgameQueenMob = 2;

// rook specific evaluation
int MidgameRookSemiOpenFile = 10;
int EndgameRookSemiOpenFile = 10;
int MidgameRookOpenFile = 20;
int EndgameRookOpenFile = 10;
int MidgameRookSemiKingFile = 10;
int MidgameRookKingFile = 20;
int MidgameRook7th = 20;
int EndgameRook7th = 40;

// queen specific evaluation
int MidgameQueen7th = 10;
int EndgameQueen7th = 20;

// passed pawn evaluation
int MidgamePassedMin = 20;
int MidgamePassedMax = 140;
int EndgamePassedMin = 10;
int EndgamePassedMax = 70;
int UnstoppablePassedPawn = 800;
int PathFreeFriendPasser = 10;
int DefendedByPawnPasser = 10;
int DuoPawnPasser = 15;
int PathFreeNotAttackedDefAllPasser = 80;
int PathFreeNotAttackedDefPasser = 70;
int PathFreeAttackedDefAllPasser = 60;
int PathFreeAttackedDefPasser = 40;
int PathNotFreeAttackedDefPasser = 10;
int PassedPawnAttackerDistance = 20;
int PassedPawnDefenderDistance = 40;

// pawn evaluation
int MidgameDoubledPawn = 8;
int EndgameDoubledPawn = 10;
int MidgameIsolatedPawn = 12;
int MidgameIsolatedPawnOpen = 16;
int EndgameIsolatedPawn = 10;
int MidgameBackwardPawn = 15;
int MidgameBackwardPawnOpen = 15;
int EndgameBackwardPawn = 10;
int MidgameCandidatePawnMin = 10;
int MidgameCandidatePawnMax = 100;
int EndgameCandidatePawnMin = 5;
int EndgameCandidatePawnMax = 50;

// king safety
int QueenAttackValue = 8; //5;
int RookAttackValue = 7; //3;
int BishopAttackValue = 3; //2;
int KnightAttackValue = 5; //2;
int QueenSafeContactCheckValue = 13; //3;
int QueenSafeCheckValue = 8; //2;
int RookSafeCheckValue = 12; //1;
int BishopSafeCheckValue = 9; //1;
int KnightSafeCheckValue = 6; //1;
int DiscoveredCheckValue = 15; //3;
int PawnShelterMultiplier = 6; //12;
int KingAttacksMultiplier = 3; //5;

// piece attacks
int QueenAttacked = 4;
int RookAttacked = 3;
int BishopAttacked = 2;
int KnightAttacked = 2;
int PawnAttacked = 1;
int QueenAttackPower = 1;
int RookAttackPower = 2;
int BishopAttackPower = 3;
int KnightAttackPower = 3;
int PawnAttackPower = 4;

int PieceAttacksMidgame = 5;
int PieceAttacksEndgame = 5;

int scale(int min, int max, int r) {
    int bonus[8] = {0, 0, 0, 13, 34, 77, 128, 0};

    ASSERT(rankIsOk(r));
    ASSERT(valueIsOk(min));
    ASSERT(valueIsOk(max));

    return (min + ((max-min)*bonus[r]) / bonus[6]);
}

void evalMotifs(const position_t *pos, eval_info_t *ei) {

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);

    if ((pos->kings & pos->color[WHITE] & (B1|C1)) && (pos->rooks & pos->color[WHITE] & (A1|A2|B1)))
        ei->mid_score[WHITE] -= BlockedRookPenalty;
    if ((pos->kings & pos->color[WHITE] & (F1|G1)) && (pos->rooks & pos->color[WHITE] & (H1|H2|G1)))
        ei->mid_score[WHITE] -= BlockedRookPenalty;
    if ((pos->kings & pos->color[BLACK] & (B8|C8)) && (pos->rooks & pos->color[BLACK] & (A8|A7|B8)))
        ei->mid_score[BLACK] -= BlockedRookPenalty;
    if ((pos->kings & pos->color[BLACK] & (F8|G8)) && (pos->rooks & pos->color[BLACK] & (H8|H7|G8)))
        ei->mid_score[BLACK] -= BlockedRookPenalty;

    if (((pos->bishops & pos->color[WHITE]) >> 7) & (pos->pawns & pos->color[BLACK] & (B5|B6)))
        ei->mid_score[WHITE] -= TrappedBishopPenalty;
    if (((pos->bishops & pos->color[WHITE]) >> 9) & (pos->pawns & pos->color[BLACK] & (G5|G6)))
        ei->mid_score[WHITE] -= TrappedBishopPenalty;

    if (((pos->bishops & pos->color[BLACK]) << 9) & (pos->pawns & pos->color[WHITE] & (B4|B3)))
        ei->mid_score[BLACK] -= TrappedBishopPenalty;
    if (((pos->bishops & pos->color[BLACK]) << 7) & (pos->pawns & pos->color[WHITE] & (G4|G3)))
        ei->mid_score[BLACK] -= TrappedBishopPenalty;

    if ((pos->pawns & pos->color[WHITE] & D2) && (pos->occupied & D3))
        ei->mid_score[WHITE] -= BlockedCenterPawnsPenalty;
    if ((pos->pawns & pos->color[WHITE] & E2) && (pos->occupied & E3))
        ei->mid_score[WHITE] -= BlockedCenterPawnsPenalty;
    if ((pos->pawns & pos->color[BLACK] & D7) && (pos->occupied & D6))
        ei->mid_score[BLACK] -= BlockedCenterPawnsPenalty;
    if ((pos->pawns & pos->color[BLACK] & E7) && (pos->occupied & E6))
        ei->mid_score[BLACK] -= BlockedCenterPawnsPenalty;
}

// this is only called when there are more than 1 queen for a side
int computeMaterial(const position_t *pos, eval_info_t *ei) {
    int wq = bitCnt(pos->queens & pos->color[WHITE]);
    int wr = bitCnt(pos->rooks & pos->color[WHITE]);
    int wb = bitCnt(pos->bishops & pos->color[WHITE]);
    int wn = bitCnt(pos->knights & pos->color[WHITE]);
    int wp = bitCnt(pos->pawns & pos->color[WHITE]);
    int bq = bitCnt(pos->queens & pos->color[BLACK]);
    int br = bitCnt(pos->rooks & pos->color[BLACK]);
    int bb = bitCnt(pos->bishops & pos->color[BLACK]);
    int bn = bitCnt(pos->knights & pos->color[BLACK]);
    int bp = bitCnt(pos->pawns & pos->color[BLACK]);

    int score = 0;
    ASSERT(pos != NULL);
    ASSERT(ei != NULL);

    ei->phase = (wb+bb+wn+bn+wq*6+bq*6+wr*3+br*3);
    if (ei->phase > 32) ei->phase = 32;
    ei->phase = ei->phase << 3;
    if (bq != 0 && (br + bb + bn) > 0) ei->phase |= KingAtkPhaseMaskByColor[WHITE];
    if (wq != 0 && (wr + wb + wn) > 0) ei->phase |= KingAtkPhaseMaskByColor[BLACK];

    score += (((wq - bq) * (QueenValueMid1+QueenValueMid2) / 2)
        + ((wr - br) * (RookValueMid1+RookValueMid2) / 2)
        + ((wb - bb) * (BishopValueMid1+BishopValueMid2) / 2)
        + ((wn - bn) * (KnightValueMid1+KnightValueMid2) / 2)
        + ((wp - bp) * (PawnValueMid1+PawnValueMid2) / 2)
        + ((wb >= 2) * (BishopPairBonusMid1+BishopPairBonusMid2) / 2)
        - ((bb >= 2) * (BishopPairBonusMid1+BishopPairBonusMid2) / 2));

    return ((pos->side==WHITE)?score:-score);
}

void initPawnEvalByColor(const position_t *pos, eval_info_t *ei, int allied) {
    int Shift[] = {9, 7};
    uint64 temp64;
    int enemy = allied^1;

    ei->pawns[allied] = (pos->pawns & pos->color[allied]);
    temp64 = (((*ShiftPtr[allied])(ei->pawns[allied], Shift[allied]) & ~FileABB) | ((*ShiftPtr[allied])(ei->pawns[allied], Shift[enemy]) & ~FileHBB));
    ei->atkpawns[allied] = temp64;
    ei->atkcntpcs[enemy] += bitCnt(temp64 & ei->kingadj[enemy]);
    ei->potentialPawnAttack[allied] = (*FillPtr2[allied])(temp64);
}

void evalPawnsByColor(const position_t *pos, eval_info_t *ei, int mid_score[], int end_score[], int allied) {
    uint64 open, isolated, backward, doubled, halfpassed, passed, temp64;
    int count, sq, rank, enemy = allied^1;

    open = ei->pawns[allied] & ~((*FillPtr2[enemy])((*ShiftPtr[enemy])((ei->pawns[allied]|ei->pawns[enemy]), 8)));
    doubled = ei->pawns[allied] & (*FillPtr[enemy])(ei->pawns[allied]);
    isolated = ei->pawns[allied] & ~((*FillPtr2[enemy])(ei->potentialPawnAttack[allied]));
    backward = ((*ShiftPtr[enemy])(((*ShiftPtr[allied])(ei->pawns[allied], 8) & (ei->potentialPawnAttack[enemy]|ei->pawns[enemy])
        & ~ei->potentialPawnAttack[allied]), 8)) & ~isolated;
    passed = open & ~ei->potentialPawnAttack[enemy];
    temp64 = ei->pawns[allied] & open & ~passed;
    halfpassed = 0;
    while (temp64) {
        sq = popFirstBit(&temp64);
        if (bitCnt((*FillPtr2[allied])(PawnCaps[sq][allied]) & ei->pawns[enemy])
                <= bitCnt((*FillPtr2[enemy])(PawnCaps[sq + PAWN_MOVE_INC(allied)][enemy]) & ei->pawns[allied]) &&
                bitCnt(PawnCaps[sq][allied] & ei->pawns[enemy])
                <= bitCnt(PawnCaps[sq][enemy] & ei->pawns[allied]))
            halfpassed |= BitMask[sq];
    }

    count = bitCnt(doubled);
    mid_score[allied] -= count * MidgameDoubledPawn;
    end_score[allied] -= count * EndgameDoubledPawn;
    count = bitCnt(backward);
    mid_score[allied] -= count * MidgameBackwardPawn;
    end_score[allied] -= count * EndgameBackwardPawn;
    count = bitCnt(backward&open);
    mid_score[allied] -= count * MidgameBackwardPawnOpen;
    count = bitCnt(isolated);
    mid_score[allied] -= count * MidgameIsolatedPawn;
    end_score[allied] -= count * EndgameIsolatedPawn;
    count = bitCnt(isolated&open);
    mid_score[allied] -= count * MidgameIsolatedPawnOpen;

    temp64 = halfpassed;
    while (temp64) {
        sq = popFirstBit(&temp64);
        rank = PAWN_RANK(sq, allied);
        mid_score[allied] += scale(MidgameCandidatePawnMin, MidgameCandidatePawnMax, rank);
        end_score[allied] += scale(EndgameCandidatePawnMin, EndgameCandidatePawnMax, rank);
    }

    ei->pawn_entry->shield[allied][0] = bitCnt(PawnShelterMask1[allied][0] & ei->pawns[allied]) * 2;
    ei->pawn_entry->shield[allied][0] += bitCnt(PawnShelterMask2[allied][0] & ei->pawns[allied]);
    ei->pawn_entry->shield[allied][1] = bitCnt(PawnShelterMask1[allied][1] & ei->pawns[allied]) * 2;
    ei->pawn_entry->shield[allied][1] += bitCnt(PawnShelterMask2[allied][1] & ei->pawns[allied]);
    ei->pawn_entry->shield[allied][2] = bitCnt(PawnShelterMask1[allied][2] & ei->pawns[allied]) * 2;
    ei->pawn_entry->shield[allied][2] += bitCnt(PawnShelterMask2[allied][2] & ei->pawns[allied]);
    ei->pawn_entry->storm[allied][0] = bitCnt(PawnShelterMask2[allied][0] & ei->pawns[enemy]) * 2;
    ei->pawn_entry->storm[allied][0] += bitCnt(PawnShelterMask3[allied][0] & ei->pawns[enemy]);
    ei->pawn_entry->storm[allied][1] = bitCnt(PawnShelterMask2[allied][1] & ei->pawns[enemy]) * 2;
    ei->pawn_entry->storm[allied][1] += bitCnt(PawnShelterMask3[allied][1] & ei->pawns[enemy]);
    ei->pawn_entry->storm[allied][2] = bitCnt(PawnShelterMask2[allied][2] & ei->pawns[enemy]) * 2;
    ei->pawn_entry->storm[allied][2] += bitCnt(PawnShelterMask3[allied][2] & ei->pawns[enemy]);

    ei->pawn_entry->passedbits |= passed;
    ei->mid_score[allied] += mid_score[allied];
    ei->end_score[allied] += end_score[allied];
}

void evalPieces(const position_t *pos, eval_info_t *ei, int allied) {
    uint64 pc_bits, temp64, weak_sq_mask, enemy_pawn_attacks, enemy_knights, enemy_bishops, enemy_rooks, enemy_queens;
    int from, temp1, enemy = allied^1;

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);
    ASSERT(colorIsOk(allied));

    enemy_knights = pos->knights & pos->color[enemy];
    enemy_bishops = pos->bishops & pos->color[enemy];
    enemy_rooks = pos->rooks & pos->color[enemy];
    enemy_queens = pos->queens & pos->color[enemy];
    enemy_pawn_attacks = ei->atkpawns[enemy];
    weak_sq_mask = WeakSqEnemyHalfBB[allied] & ~ei->potentialPawnAttack[enemy];

    pc_bits = pos->knights & pos->color[allied];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = KnightMoves[from];
        ei->atkknights[allied] |= temp64;
        temp1 = bitCnt(temp64 & ~pos->color[allied] & ~enemy_pawn_attacks);
        ei->mid_score[allied] += temp1 * MidgameKnightMob;
        ei->end_score[allied] += temp1 * EndgameKnightMob;
        if (temp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(temp64 & ei->kingadj[enemy]) + (KnightAttackValue<<10);
        temp1 = 0;
        if (temp64 & enemy_queens) temp1 += KnightAttackPower * QueenAttacked;
        if (temp64 & enemy_rooks) temp1 += KnightAttackPower * RookAttacked;
        if (temp64 & enemy_bishops & ~enemy_pawn_attacks) temp1 += KnightAttackPower * BishopAttacked;
        if (BitMask[from] & enemy_pawn_attacks) temp1 -= PawnAttackPower * KnightAttacked;
        ei->mid_score[allied] += temp1 * PieceAttacksMidgame;
        ei->end_score[allied] += temp1 * PieceAttacksEndgame;
        if (BitMask[from] & weak_sq_mask) {
            temp1 = PST(allied, KNIGHT, from, MIDGAME) / 2;
            if (BitMask[from] & ei->atkpawns[allied]) {
                if (!(pos->knights & pos->color[enemy]) && !((BitMask[from] & WhiteSquaresBB) ?
                (pos->bishops & pos->color[enemy] & WhiteSquaresBB) : (pos->bishops & pos->color[enemy] & BlackSquaresBB)))
                    temp1 *= 3;
                else temp1 *= 2;
            }
            ei->mid_score[allied] += temp1;
        }
    }
    pc_bits = pos->bishops & pos->color[allied] ;
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = bishopAttacksBB(from, pos->occupied);
        ei->atkbishops[allied] |= temp64;
        temp1 = bitCnt(temp64 & ~pos->color[allied] & ~enemy_pawn_attacks);
        ei->mid_score[allied] += temp1 * MidgameBishopMob;
        ei->end_score[allied] += temp1 * EndgameBishopMob;
        if (temp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(temp64 & ei->kingadj[enemy]) + (BishopAttackValue<<10);
        temp1 = 0;
        if (temp64 & enemy_queens) temp1 += BishopAttackPower * QueenAttacked;
        if (temp64 & enemy_rooks) temp1 += BishopAttackPower * RookAttacked;
        if (temp64 & enemy_knights & ~enemy_pawn_attacks) temp1 += BishopAttackPower * KnightAttacked;
        if (BitMask[from] & enemy_pawn_attacks) temp1 -= PawnAttackPower * BishopAttacked;
        ei->mid_score[allied] += temp1 * PieceAttacksMidgame;
        ei->end_score[allied] += temp1 * PieceAttacksEndgame;
        if (BitMask[from] & weak_sq_mask) {
            temp1 = PST(allied, KNIGHT, from, MIDGAME) / 3;
            if (BitMask[from] & ei->atkpawns[allied]) {
                if (!(pos->knights & pos->color[enemy]) && !((BitMask[from] & WhiteSquaresBB) ?
                (pos->bishops & pos->color[enemy] & WhiteSquaresBB) : (pos->bishops & pos->color[enemy] & BlackSquaresBB)))
                    temp1 *= 3;
                else temp1 *= 2;
            }
            ei->mid_score[allied] += temp1;
        }
    }
    pc_bits = pos->rooks & pos->color[allied];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = rookAttacksBB(from, pos->occupied);
        ei->atkrooks[allied] |= temp64;
        temp1 = bitCnt(temp64 & ~pos->color[allied] & ~enemy_pawn_attacks);
        ei->mid_score[allied] += temp1 * MidgameRookMob;
        ei->end_score[allied] += temp1 * EndgameRookMob;
        if (temp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(temp64 & ei->kingadj[enemy]) + (RookAttackValue<<10);
        temp1 = 0;
        if (temp64 & enemy_queens) temp1 += RookAttackPower * QueenAttacked;
        if (temp64 & enemy_bishops & ~enemy_pawn_attacks) temp1 += RookAttackPower * BishopAttacked;
        if (temp64 & enemy_knights & ~enemy_pawn_attacks) temp1 += RookAttackPower * KnightAttacked;
        if (BitMask[from] & enemy_pawn_attacks) temp1 -= PawnAttackPower * RookAttacked;
        ei->mid_score[allied] += temp1 * PieceAttacksMidgame;
        ei->end_score[allied] += temp1 * PieceAttacksEndgame;
        if (!(pos->pawns & pos->color[allied] & FileMask[from])) {
            ei->mid_score[allied] += MidgameRookSemiOpenFile;
            ei->end_score[allied] += EndgameRookSemiOpenFile;
            if (!(pos->pawns & pos->color[enemy] & FileMask[from])) {
                ei->mid_score[allied] += MidgameRookOpenFile;
                ei->end_score[allied] += EndgameRookOpenFile;
            }
            if (ei->phase & KingAtkPhaseMaskByColor[enemy]) {
                temp1 = abs(SQFILE(pos->kpos[enemy]) - SQFILE(from));
                if (temp1 <= 1) {
                    if (temp1 == 0) ei->mid_score[allied] += MidgameRookKingFile;
                    ei->mid_score[allied] += MidgameRookSemiKingFile;
                }
            }
        }
        if (BitMask[from] & Rank7ByColorBB[allied]) {
            if ((pos->pawns & pos->color[enemy] & Rank7ByColorBB[allied]) || (BitMask[pos->kpos[enemy]] & Rank8ByColorBB[allied])) {
                ei->mid_score[allied] += MidgameRook7th;
                ei->end_score[allied] += EndgameRook7th;
            }
        }
    }
    pc_bits = pos->queens & pos->color[allied];
    while (pc_bits) {
        from = popFirstBit(&pc_bits);
        temp64 = queenAttacksBB(from, pos->occupied);
        ei->atkqueens[allied] |= temp64;
        temp1 = bitCnt(temp64 & ~pos->color[allied] & ~enemy_pawn_attacks);
        ei->mid_score[allied] += temp1 * MidgameQueenMob;
        ei->end_score[allied] += temp1 * EndgameQueenMob;
        if (temp64 & ei->kingzone[enemy]) ei->atkcntpcs[enemy] += (1<<20) + bitCnt(temp64 & ei->kingadj[enemy]) + (QueenAttackValue<<10);
        temp1 = 0;
        if (BitMask[from] & enemy_pawn_attacks) temp1 -= PawnAttackPower * QueenAttacked;
        ei->mid_score[allied] += temp1 * PieceAttacksMidgame;
        ei->end_score[allied] += temp1 * PieceAttacksEndgame;
        if (BitMask[from] & Rank7ByColorBB[allied]) {
            if ((pos->pawns & pos->color[enemy] & Rank7ByColorBB[allied]) || (BitMask[pos->kpos[enemy]] & Rank8ByColorBB[allied])) {
                ei->mid_score[allied] += MidgameQueen7th;
                ei->end_score[allied] += EndgameQueen7th;
            }
        }
    }
}

void evalKingAttacks(const position_t *pos, eval_info_t *ei, int allied) {
    int KingSideCastleMask[2] = {WCKS, BCKS};
    int QueenSideCastleMask[2] = {WCQS, BCQS};
    uint64 pc_atkrs_mask, pc_atkhelpersmask, king_atkmask, pc_defenders_mask;
    int penalty, best_shieldval, curr_shieldval, tot_atkrs, pc_weights, kzone_atkcnt, enemy = allied^1;

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);
    ASSERT(colorIsOk(allied));

    if (ei->phase & KingAtkPhaseMaskByColor[allied]) {
        best_shieldval = curr_shieldval = ei->pawn_entry->shield[allied][FileWing[pos->kpos[allied]]]
            - ei->pawn_entry->storm[allied][FileWing[pos->kpos[allied]]];
        if (pos->posStore.castle & QueenSideCastleMask[allied])
            best_shieldval = MAX((ei->pawn_entry->shield[allied][0] - ei->pawn_entry->storm[allied][0]), best_shieldval);
        if (pos->posStore.castle & KingSideCastleMask[allied])
            best_shieldval = MAX((ei->pawn_entry->shield[allied][2] - ei->pawn_entry->storm[allied][2]), best_shieldval);

        tot_atkrs = ei->atkcntpcs[allied] >> 20;
        kzone_atkcnt = ei->atkcntpcs[allied] & ((1 << 10) - 1);
        pc_weights = (ei->atkcntpcs[allied] & ((1 << 20) - 1)) >> 10;
        penalty = ((pc_weights * tot_atkrs) / 2) + kzone_atkcnt;

        best_shieldval = (best_shieldval + curr_shieldval) / 2;
        ei->mid_score[allied] += best_shieldval * (PawnShelterMultiplier + penalty);

        if (tot_atkrs >= 2 && kzone_atkcnt >= 1) {
            king_atkmask = KingMoves[pos->kpos[allied]];
            penalty += KingPosPenalty[allied][pos->kpos[allied]];
            pc_defenders_mask = ei->atkqueens[allied] | ei->atkrooks[allied] | ei->atkbishops[allied] | ei->atkknights[allied] | ei->atkpawns[allied];
            penalty += bitCnt(king_atkmask & ei->atkall[enemy] & ~pc_defenders_mask);
            pc_atkrs_mask = king_atkmask & ei->atkqueens[enemy];
            if (pc_atkrs_mask) {
                pc_atkhelpersmask = ei->atkkings[enemy] | ei->atkrooks[enemy] | ei->atkbishops[enemy] | ei->atkknights[enemy] | ei->atkpawns[enemy];
                pc_atkrs_mask &= pc_atkhelpersmask;
                if (pc_atkrs_mask) {
                    penalty += bitCnt(pc_atkrs_mask & ~pc_defenders_mask) * ((pos->side == (enemy)) ? 2 : 1) * QueenSafeContactCheckValue;
                }
            }
            pc_atkrs_mask = rookAttacksBB(pos->kpos[allied], pos->occupied) & ~ei->atkall[allied] & ~pos->color[enemy];
            if (pc_atkrs_mask & ei->atkqueens[enemy]) penalty += bitCnt(pc_atkrs_mask & ei->atkqueens[enemy]) * QueenSafeCheckValue;
            if (pc_atkrs_mask & ei->atkrooks[enemy]) penalty += bitCnt(pc_atkrs_mask & ei->atkrooks[enemy]) * RookSafeCheckValue;
            pc_atkrs_mask = bishopAttacksBB(pos->kpos[allied], pos->occupied) & ~ei->atkall[allied] & ~pos->color[enemy];
            if (pc_atkrs_mask & ei->atkqueens[enemy]) penalty += bitCnt(pc_atkrs_mask & ei->atkqueens[enemy]) * QueenSafeCheckValue;
            if (pc_atkrs_mask & ei->atkbishops[enemy]) penalty += bitCnt(pc_atkrs_mask & ei->atkbishops[enemy]) * BishopSafeCheckValue;
            pc_atkrs_mask = KnightMoves[pos->kpos[allied]] & ~ei->atkall[allied] & ~pos->color[enemy];
            if (pc_atkrs_mask & ei->atkknights[enemy]) penalty += bitCnt(pc_atkrs_mask & ei->atkknights[enemy]) * KnightSafeCheckValue;
            pc_atkrs_mask = discoveredCheckCandidates(pos, enemy) & ~pos->pawns;
            if (pc_atkrs_mask) penalty += bitCnt(pc_atkrs_mask) * DiscoveredCheckValue;
            ei->mid_score[allied] -= penalty * penalty * KingAttacksMultiplier / 20;
        }
    }
}

int kingPasser(const position_t *pos, int square, int allied) {
    int king, file, promotion;

    ASSERT(colorIsOk(allied));
    ASSERT(squareIsOk(square));

    king = pos->kpos[allied];
    file = SQFILE(square);
    promotion = PAWN_PROMOTE(square, allied);
    if (DISTANCE(king, promotion) <= 1 && DISTANCE(king, square) == 1
    && ((SQFILE(king) != file) || (file != FileA && file != FileH))) return TRUE;
    return FALSE;
}

int unstoppablePasser(const position_t *pos, int square, int allied) {
    int oppking, dist, promotion, enemy = allied^1;

    ASSERT(colorIsOk(allied));
    ASSERT(squareIsOk(square));

    if ((*FillPtr[allied])(BitMask[square]) & pos->occupied) return FALSE;
    if (PAWN_RANK(square, allied) == Rank2) square += PAWN_MOVE_INC(allied);
    oppking = pos->kpos[enemy];
    promotion = PAWN_PROMOTE(square, allied);
    dist = DISTANCE(square, promotion);
    if (allied == (pos->side^1)) dist++;
    if (DISTANCE(oppking, promotion) > dist) return TRUE;
    return FALSE;
}

void evalPassedPawns(const position_t *pos, eval_info_t *ei, int allied) {
    uint64 passedpawn_mask;
    int from, rank, score, enemy = allied^1;

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);
    ASSERT(colorIsOk(allied));

    passedpawn_mask = ei->pawn_entry->passedbits & pos->color[allied];
    while (passedpawn_mask) {
        from = popFirstBit(&passedpawn_mask);
        rank = PAWN_RANK(from, allied);
        ei->mid_score[allied] += scale(MidgamePassedMin, MidgamePassedMax, rank);
        ei->end_score[allied] += scale(EndgamePassedMin, EndgamePassedMax, rank);
        score = 0;
        score += DISTANCE((from + PAWN_MOVE_INC(allied)), pos->kpos[enemy]) * PassedPawnDefenderDistance;
        score -= DISTANCE((from + PAWN_MOVE_INC(allied)), pos->kpos[allied]) * PassedPawnAttackerDistance;
        if (!(pos->color[enemy] & ~pos->pawns & ~pos->kings) && (unstoppablePasser(pos, from, allied) || kingPasser(pos, from, allied))) {
            score += UnstoppablePassedPawn;
        } else {
            uint64 prom_path = (*FillPtr[allied])(BitMask[from]);
            uint64 path_attkd = prom_path & ei->atkall[enemy];
            uint64 path_dfndd = prom_path & ei->atkall[allied];
            if ((prom_path & pos->color[enemy]) == EmptyBoardBB) {
                if (path_attkd == EmptyBoardBB) score += (prom_path == path_dfndd) ? PathFreeNotAttackedDefAllPasser : PathFreeNotAttackedDefPasser;
                else score += ((path_attkd & path_dfndd) == path_attkd) ? PathFreeAttackedDefAllPasser : PathFreeAttackedDefPasser;
            } else if (((path_attkd | (prom_path & pos->color[enemy])) & ~path_dfndd) == EmptyBoardBB) score += PathNotFreeAttackedDefPasser;
            if ((prom_path & pos->color[allied]) == EmptyBoardBB) score += PathFreeFriendPasser;
        }
        if (ei->atkpawns[allied] & BitMask[from]) score += DefendedByPawnPasser;
        if (ei->atkpawns[allied] & BitMask[from + PAWN_MOVE_INC(allied)]) score += DuoPawnPasser;
        ei->end_score[allied] += scale(0, score, rank);
    }
}

// TODO: debug pawn hash value
void evalPawns(const position_t *pos, eval_info_t *ei) {

    ASSERT(pos != NULL);
    ASSERT(ei != NULL);

    initPawnEvalByColor(pos, ei, WHITE);
    initPawnEvalByColor(pos, ei, BLACK);

    ei->pawn_entry = SearchInfo(0).pt.table + (KEY(pos->posStore.phash) &SearchInfo(0).pt.mask);
#ifndef DEBUG
    if (ei->pawn_entry->hashlock == LOCK(pos->posStore.phash)) {
        ei->mid_score[WHITE] += ei->pawn_entry->opn;
        ei->end_score[WHITE] += ei->pawn_entry->end;
    }
    else
#endif
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

int eval(const position_t *pos) {
    eval_info_t ei;
    material_info_t *mat;
    int open, end, score, mat_idx_white, mat_idx_black;

    ASSERT(pos != NULL);

    ei.atkall[WHITE] = ei.atkall[BLACK] = 0;
    ei.atkpawns[WHITE] = ei.atkpawns[BLACK] = 0;
    ei.atkknights[WHITE] = ei.atkknights[BLACK] = 0;
    ei.atkbishops[WHITE] = ei.atkbishops[BLACK] = 0;
    ei.atkrooks[WHITE] = ei.atkrooks[BLACK] = 0;
    ei.atkqueens[WHITE] = ei.atkqueens[BLACK] = 0;
    ei.atkkings[WHITE] = ei.atkkings[BLACK] = 0;
    ei.atkcntpcs[WHITE] = ei.atkcntpcs[BLACK] = 0;

    ei.kingadj[WHITE] = KingMoves[pos->kpos[WHITE]];
    ei.kingadj[BLACK] = KingMoves[pos->kpos[BLACK]];
    ei.kingzone[WHITE] = ei.kingadj[WHITE] | (ei.kingadj[WHITE] << 8);
    ei.kingzone[BLACK] = ei.kingadj[BLACK] | (ei.kingadj[BLACK] >> 8);

    mat_idx_white = pos->posStore.mat_summ[WHITE];
    mat_idx_black = pos->posStore.mat_summ[BLACK];
    if (mat_idx_white < MAX_MATERIAL && mat_idx_black < MAX_MATERIAL) {
        mat = &MaterialTable[mat_idx_white][mat_idx_black];
        score = ((pos->side==WHITE)?mat->value:-mat->value);
        ei.phase = mat->flags;
    } else score = computeMaterial(pos, &ei);

    ei.mid_score[WHITE] = pos->posStore.open[WHITE];
    ei.mid_score[BLACK] = pos->posStore.open[BLACK];
    ei.end_score[WHITE] = pos->posStore.end[WHITE];
    ei.end_score[BLACK] = pos->posStore.end[BLACK];

    ei.mid_score[pos->side] += MidgameTempo;
    ei.end_score[pos->side] += EndgameTempo;

    evalMotifs(pos, &ei);

    evalPawns(pos, &ei);

    evalPieces(pos, &ei, WHITE);
    evalPieces(pos, &ei, BLACK);

    ei.atkkings[WHITE] = KingMoves[pos->kpos[WHITE]];
    ei.atkkings[BLACK] = KingMoves[pos->kpos[BLACK]];

    ei.atkall[WHITE] = ei.atkpawns[WHITE] | ei.atkknights[WHITE] | ei.atkbishops[WHITE] | ei.atkrooks[WHITE] | ei.atkqueens[WHITE] | ei.atkkings[WHITE];
    ei.atkall[BLACK] = ei.atkpawns[BLACK] | ei.atkknights[BLACK] | ei.atkbishops[BLACK] | ei.atkrooks[BLACK] | ei.atkqueens[BLACK] | ei.atkkings[BLACK];

    evalKingAttacks(pos, &ei, WHITE);
    evalKingAttacks(pos, &ei, BLACK);

    evalPassedPawns(pos, &ei, WHITE);
    evalPassedPawns(pos, &ei, BLACK);

    ASSERT(pos->side == WHITE || pos->side == BLACK);

    open = ei.mid_score[pos->side] - ei.mid_score[pos->side^1];
    end = ei.end_score[pos->side] - ei.end_score[pos->side^1];

    if (ei.phase & 1) {
        if ((pos->bishops & WhiteSquaresBB) && (pos->bishops & BlackSquaresBB)) open = open / 2;
        end = end / 2;
    }

    ei.phase = ei.phase >> 3;
    score += ((open * ei.phase) + (end * (32 - ei.phase))) / 32;

    ASSERT(valueIsOk(score));
    return score;
}
