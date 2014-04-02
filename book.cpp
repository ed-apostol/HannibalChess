/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "book.h"
#include "utils.h"
#include "movegen.h"

#include <iostream>
#include <string>
using namespace std;

#define NO_MOVE 0
const int MAX_MOVES = 256;
const int Polyglot_Entry_Size = 16;
struct PolyglotBookEntry {
    uint64 key;
    basic_move_t move;
    uint16 weight;
    uint32 learn;
};
const PolyglotBookEntry PolyglotBookEntryNone = {
    0, NO_MOVE, 0, 0
};
void closeBook(book_t *book) { //TODO consider mutex ramifications
    if (book->bookFile != NULL) fclose(book->bookFile);
}
int int_from_file(FILE *f, int l, uint64 *r) {
    if (f == NULL) {
        cout << "info string NULL file int_from_file\n";
        return 0;
    }
    int i, c;
    for (i = 0; i < l; i++) {
        c = fgetc(f);
        if (c == EOF) {
            return 1;
        }
        (*r) = ((*r) << 8) + c;
    }
    return 0;
}
basic_move_t polyglot_move_to_move(uint16 move, position_t& pos) {
    int from, to;
    from = ((move >> 6) & 077);
    to = (move & 077);
    int capturedPiece = getPiece(pos, to);
    int movedPiece = getPiece(pos, from);
    int promotePiece = ((move >> 12) & 0x7);

    if (promotePiece) {
        if (capturedPiece) {
            return GenPromote(from, to, promotePiece, capturedPiece);
        } else {
            return GenPromoteStraight(from, to, promotePiece);
        }
    }
    if (movedPiece == KING) {
        if (from == E1 && to == H1) return GenWhiteOO();
        if (from == E1 && to == A1) return GenWhiteOOO();
        if (from == E8 && to == H8) return GenBlackOO();
        if (from == E8 && to == A8) return  GenBlackOOO();
    }
    if (movedPiece == PAWN && to == pos.posStore.epsq) return GenEnPassant(from, to);
    return GenBasicMove(from, to, movedPiece, capturedPiece);
}
int entry_from_polyglot_file(FILE *f, PolyglotBookEntry *entry, position_t& pos) {
    uint64 r;
    if (int_from_file(f, 8, &r)) return 1;
    entry->key = r;
    if (int_from_file(f, 2, &r)) return 1;
    entry->move = polyglot_move_to_move((uint16)r, pos);

    if (int_from_file(f, 2, &r)) return 1;
    entry->weight = (uint16)r;
    if (int_from_file(f, 4, &r)) return 1;
    entry->learn = (uint16)r;
    return 0;
}
long find_polyglot_key(FILE *f, uint64 key, PolyglotBookEntry *entry, position_t& pos) {
    long first, last, middle;
    PolyglotBookEntry first_entry = PolyglotBookEntryNone, last_entry, middle_entry;
    if (f == NULL) {
        *entry = PolyglotBookEntryNone;
        entry->key = key + 1; //hack, should not be necessary if no entry can be 0
        cout << "info string NULL file find_polyglot_key" << endl;
        return -1;
    }
    first = -1;
    if (fseek(f, -Polyglot_Entry_Size, SEEK_END)) {
        *entry = PolyglotBookEntryNone;
        entry->key = key + 1; //hack, should not be necessary if no entry can be 0
        return -1;
    }
    last = ftell(f) / Polyglot_Entry_Size;
    entry_from_polyglot_file(f, &last_entry, pos);
    while (true) {
        if (last - first == 1) {
            *entry = last_entry;
            return last;
        }
        middle = (first + last) / 2;
        fseek(f, Polyglot_Entry_Size*middle, SEEK_SET);
        entry_from_polyglot_file(f, &middle_entry, pos);
        if (key <= middle_entry.key) {
            last = middle;
            last_entry = middle_entry;
        } else {
            first = middle;
            first_entry = middle_entry;
        }
    }
}
bool equalMove(const basic_move_t m1, const basic_move_t m2) {
    return (moveTo(m1) == moveTo(m2) && moveFrom(m1) == moveFrom(m2));
}
bool move_in_list(const basic_move_t m, const movelist_t *ml) {
    int on;
    for (on = 0; on < ml->size; on++) {
        if (equalMove(ml->list[on].m, m)) return true;
    }
    return false;
}

void initBook(char* book_name, book_t *book, BookType type) {
    if (type != POLYGLOT_BOOK) cout << "info string book type not supported" << endl;
    else {
        if (book->bookFile != NULL) fclose(book->bookFile);
        book->bookFile = fopen(book_name, "rb");
        book->name = string(book_name);

        if (book->bookFile != NULL) {
            book->type = type;
            fseek(book->bookFile, 0, SEEK_END);
            book->size = ftell(book->bookFile) / Polyglot_Entry_Size;
        }
        if (DEBUG_BOOK) {
            cout << "info string init polyglot book " << book_name << endl;
        }
    }
}
basic_move_t getBookMove(position_t& pos, book_t *book/*, movelist_t *ml, bool verbose*/, int randomness) {
    uint64 key;
    int numMoves = 0;
    uint64 totalWeight = 0;
    long offset;
    FILE *f = book->bookFile;

    if (f == NULL || book->size == 0) {
        return NO_MOVE;
    }
    PolyglotBookEntry entry;
    PolyglotBookEntry entries[MAX_MOVES];
    key = pos.posStore.hash;
    offset = find_polyglot_key(f, key, &entry, pos);
    if (entry.key != key) {
        return NO_MOVE;
    }
    movelist_t moves;
    genLegal(pos, &moves, true);
    entries[numMoves] = entry;
    if (move_in_list(entry.move, &moves)) {
        totalWeight += entry.weight;
        numMoves++;
    }
    fseek(f, Polyglot_Entry_Size*(offset + 1), SEEK_SET);

    while (true) {
        if (numMoves >= MAX_MOVES - 1) {
            break;
        }
        if (entry_from_polyglot_file(f, &entry, pos)) break;
        if (entry.key != key) break;
        entries[numMoves] = entry;
        if (move_in_list(entry.move, &moves)) {
            totalWeight += entry.weight;
            numMoves++;
        }
    }
    if (totalWeight <= 0) {
        return  NO_MOVE;
    }
    // chose here the move from the array and verify if it exists in the movelist
    uint64 bookRandom = rand() % totalWeight; //TODO do a real randomization
    uint64 bookIndex = 0;
    //    if (DEBUG_BOOK && verbose) cout << "info string random " << bookRandom << " out of " << totalWeight << endl;
    for (int i = 0; i<numMoves; i++) {
        bookIndex += entries[i].weight;
        if (bookIndex > bookRandom) {
            return entries[i].move;
        }
    }
    return NO_MOVE;
}
