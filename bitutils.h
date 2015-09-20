/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern uint32 getFirstBit(uint64 b);
extern uint32 popFirstBit(uint64 *b);
extern uint32 bitCnt(uint64 b);

extern uint64 fillUp(uint64 b);
extern uint64 fillDown(uint64 b);
extern uint64 fillUp2(uint64 b);
extern uint64 fillDown2(uint64 b);
extern uint64 shiftLeft(uint64 b, uint32 i);
extern uint64 shiftRight(uint64 b, uint32 i);
extern uint64 adjacent(const uint64 BB);
extern uint64 pawnAttackBB(const uint64 pawns, const int color);
extern uint64 doublePawnAttackBB(const uint64 pawns, const int color);

#define MaxOneBit(x) (((x) & ((x)-1))==0)
#define MinTwoBits(x) ((x) & ((x)-1))

extern EvalScore ComposeEvalScore(int16 s1, int16 s2);
extern int16 GetOpening(EvalScore s1);
extern int16 GetEndgame(EvalScore s1);
