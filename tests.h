#pragma once

/* tests.c */
extern void perft(position_t *pos, int maxply, uint64 nodesx[]);
extern int perftDivide(position_t *pos, uint32 depth, uint32 maxply);
extern void runPerft(position_t *pos, int maxply);
extern void runPerftDivide(position_t *pos, uint32 maxply);
extern void nonUCI(position_t *pos);