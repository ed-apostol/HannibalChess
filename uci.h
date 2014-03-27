/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void initOption(uci_option_t* opt);
extern void uciStart(void);
extern void uciSetOption(char string[]);
extern void uciParseSearchmoves(movelist_t *ml, char *str, uint32 moves[]);
extern void uciGo(position_t& pos, char *options);
extern void uciSetPosition(position_t& pos, char *str);
