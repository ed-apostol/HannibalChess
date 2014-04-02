/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

extern void initBook(char* book_name, book_t *book, BookType type);
extern basic_move_t getBookMove(position_t& pos, book_t *book/*, movelist_t *ml, bool verbose*/, int randomness);
