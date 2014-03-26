/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern int eval(const position_t* pos, int thread_id, int *pessimism);
extern void evalEndgame(int attacker, const position_t* pos, eval_info_t *ei, int *score, int *draw, int mover);

