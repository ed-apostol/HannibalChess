/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void Print(int vb, char *fmt, ...);
extern void displayBit(uint64 a, int x);
extern int fr_square(int f, int r);
extern void showBitboard(uint64 b, int x);
extern char *bit2Str(uint64 n);
extern char *move2Str(basic_move_t m);
extern char *sq2Str(int sq);

#ifdef TESTING_ON
extern string sqToStr(int sq);
extern string pieceToString(int pc);
extern string movenumToStr(int i);
extern string moveToStr(basic_move_t m);
extern string pv2Str(continuation_t *c);
extern void displayBoard(const position_t *pos, int x);
#endif

extern int getPiece(const position_t *pos, uint32 sq);
extern int getColor(const position_t *pos, uint32 sq);
extern int DiffColor(const position_t *pos, uint32 sq, int color);
extern uint64 getTime(void);
extern uint32 parseMove(movelist_t *mvlist, char *s);
extern int biosKey(void);
extern int anyRep(const position_t *pos);
extern int anyRepNoMove(const position_t *pos, const int m);
