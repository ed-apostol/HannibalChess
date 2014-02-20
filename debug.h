/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

/* debug.c */
extern int squareIsOk(int s);
extern int colorIsOk(int c);
extern int moveIsOk(basic_move_t m);
extern int valueIsOk(int v);
extern int rankIsOk(int r);
extern void flipPosition(const position_t *pos, position_t *clone);
extern int evalSymmetryIsOk(const position_t *pos);
extern uint64 pawnHashRecalc(const position_t *pos);
extern void positionIsOk(const position_t *pos);