#pragma once

/* tune.h */
#ifdef OPTIMIZE
extern void optimize(position_t *pos, int threads);
#endif
#ifdef SELF_TUNE2
extern void NewTuneGame(const int player1);
#endif