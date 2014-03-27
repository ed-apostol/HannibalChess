/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#ifndef TESTING_ON
extern void initBook(char* book_name, book_t *book, BookType type);
#ifdef LEARNING_ON
extern int puck_book_score(position_t& pos, book_t *book);
extern basic_move_t getBookMove(position_t& pos, book_t *book, movelist_t *ml, bool verbose, int randomness);

extern void add_to_learn_begin(learn_t *learn, continuation_t *toLearn);
extern int current_puck_book_score(position_t& pos, book_t *book);
extern bool get_continuation_to_learn(learn_t *learn, continuation_t *toLearn);
extern void insert_score_to_puck_file(book_t *book, uint64 key, int score);
extern bool learn_continuation(int thread_id, continuation_t *toLearn);
extern void generateContinuation(continuation_t *variation);
#else
extern basic_move_t getBookMove(position_t& pos, book_t *book/*, movelist_t *ml, bool verbose*/, int randomness);
#endif
#endif