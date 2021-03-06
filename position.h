/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void unmakeNullMove(position_t& pos, pos_store_t& undo);
extern void makeNullMove(position_t& pos, pos_store_t& undo);
extern void unmakeMove(position_t& pos, pos_store_t& undo);
extern void makeMove(position_t& pos, pos_store_t& undo, basic_move_t m);
extern void setPosition(position_t& pos, const char *fen);
extern char *positionToFEN(const position_t& pos);
