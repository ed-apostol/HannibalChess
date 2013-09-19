#pragma once

/*book.h*/
extern int puck_book_score(position_t *p, book_t *book);
extern basic_move_t getBookMove(position_t *p, book_t *book, movelist_t *ml, bool verbose, int randomness);
extern void add_to_learn_begin(learn_t *learn, continuation_t *toLearn);
extern void initBook(char* book_name, book_t *book, BookType type);
extern int current_puck_book_score(position_t *p, book_t *book);
extern bool get_continuation_to_learn(learn_t *learn, continuation_t *toLearn);
extern void insert_score_to_puck_file(book_t *book, uint64 key, int score);
extern bool learn_continuation(int thread_id, continuation_t *toLearn);
extern void generateContinuation(continuation_t *variation);