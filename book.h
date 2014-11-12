/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#include<string>

struct PolyglotBookEntry {
    uint64 key;
    basic_move_t move;
    uint16 weight;
    uint32 learn;
};

class Book {
public:
    Book() : bookFile(NULL) {}
    ~Book() {
        if (bookFile) fclose(bookFile);
    }

    void initBook(std::string book_name);
    int Book::entry_from_polyglot_file(PolyglotBookEntry *entry, position_t& pos);
    long Book::find_polyglot_key(uint64 key, PolyglotBookEntry *entry, position_t& pos);
    basic_move_t getBookMove(position_t& pos);
private:
    FILE *bookFile;
    int64 size;
    std::string name;
};
