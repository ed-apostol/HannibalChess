#pragma once

/* eval.c */
extern int kingPasser(const position_t *pos, int square, int color);
extern int unstoppablePasser(const position_t *pos, int square, int color);
//extern int eval(const position_t *pos, int thread_id);
extern int eval(const position_t *pos, int thread_id, int *pessimism);
extern void evalEndgame(int attacker,const position_t *pos, eval_info_t *ei,int *score, int *draw, int mover);

