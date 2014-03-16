/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "threads.h"

/* movegen.c */
extern void genLegal(const position_t& pos, movelist_t *mvlist,int promoteAll);
extern void genNonCaptures(const position_t& pos, movelist_t *mvlist);
extern void genCaptures(const position_t& pos, movelist_t *mvlist);
extern void genEvasions(const position_t& pos, movelist_t *mvlist);
extern void genQChecks(const position_t& pos, movelist_t *mvlist);
extern uint32 genMoveIfLegal(const position_t& pos, uint32 move, uint64 pinned);
extern void genGainingMoves(const position_t& pos, movelist_t *mvlist, int delta, Thread& sthread);

/* utilities for move generation */
inline basic_move_t GenOneForward(uint f, uint t) {return ((f) | ((t)<<6) | (PAWN<<12));}
inline basic_move_t GenTwoForward(uint f, uint t) {return ((f) | ((t)<<6) | (PAWN<<12) | (1<<16));}
inline basic_move_t GenPromote(uint f, uint t, uint r, uint c) {return ((f) | ((t)<<6) | (PAWN<<12) | ((c)<<18) | ((r)<<22) | (1<<17));}
inline basic_move_t GenPromoteStraight(uint f, uint t, uint r) {return ((f) | ((t)<<6) | (PAWN<<12) | ((r)<<22)  | (1<<17));}
inline basic_move_t GenEnPassant(uint f, uint t) {return ((f) | ((t)<<6) | (PAWN<<12) | (PAWN<<18) | (1<<21));}
inline basic_move_t GenPawnMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (PAWN<<12) | ((c)<<18));}
inline basic_move_t GenKnightMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (KNIGHT<<12) | ((c)<<18));}
inline basic_move_t GenBishopMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (BISHOP<<12) | ((c)<<18));}
inline basic_move_t GenRookMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (ROOK<<12) | ((c)<<18));}
inline basic_move_t GenQueenMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (QUEEN<<12) | ((c)<<18));}
inline basic_move_t GenKingMove(uint f, uint t, uint c) {return ((f) | ((t)<<6) | (KING<<12) | ((c)<<18));}
inline basic_move_t GenWhiteOO(void) {return (e1 | (g1<<6) | (KING<<12) | (1<<15));}
inline basic_move_t GenWhiteOOO(void) {return (e1 | (c1<<6) | (KING<<12) | (1<<15));}
inline basic_move_t GenBlackOO(void) {return (e8 | (g8<<6) | (KING<<12) | (1<<15));}
inline basic_move_t GenBlackOOO(void) {return (e8 | (c8<<6) | (KING<<12) | (1<<15));}