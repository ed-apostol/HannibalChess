#pragma once

/* movepicker.c */
extern void sortInit(const position_t *pos, movelist_t *mvlist, uint64 pinned, uint32 hashmove, int scout, int eval, int depth, int type, Thread& sthread);
extern move_t* sortNext(SplitPoint* sp, position_t *pos, movelist_t *mvlist, int& phase, int thread_id);
extern bool moveIsPassedPawn(const position_t * pos, uint32 move);