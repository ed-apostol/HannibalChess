#pragma once

/* uci.c */
extern void uciSetOption(char string[]);
extern void initOption(uci_option_t* opt);
extern void uciParseSearchmoves(movelist_t *ml, char *str, uint32 moves[]);
extern void uciGo(position_t *pos, char *options);
extern void uciStart(void);
extern void uciSetPosition(position_t *pos, char *str);