/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include <iostream>
#include <string>
using namespace std;

#ifndef TCEC
#define MOVE_BYTES 4

const int Polyglot_Entry_Size = 16;
const int Puck_Entry_Size = 12; //REDUCE with only 1 score (not white & black)
struct PolyglotBookEntry {
    uint64 key;
    basic_move_t move;
    uint16 weight;
    uint32 learn;
};

struct PuckBookEntry {
    uint64 key;
    int score;
};

#define NO_MOVE 0
const PolyglotBookEntry PolyglotBookEntryNone = {
    0, NO_MOVE, 0, 0
};

const PuckBookEntry PuckBookEntryNone = {
    0, 0
};

const int MAX_MOVES = 256;


basic_move_t polyglot_move_to_move(uint16 move, position_t *p) {
    int from, to;
    from= ((move>>6)&077);
    to = (move&077);
	int capturedPiece = getPiece(p, to);
	int movedPiece = getPiece(p, from);
    int promotePiece = ((move>>12)&0x7);

    if (promotePiece) {
		if (capturedPiece) {
			return GenPromote(from,to,promotePiece,capturedPiece);
		}
		else {
			return GenPromoteStraight(from,to,promotePiece);
		}
	}
    if (movedPiece == KING) {
        if (from==E1 && to==H1) return GenWhiteOO();
        if (from==E1 && to==A1) return GenWhiteOOO();
        if (from==E8 && to==H8) return GenBlackOO();
        if (from==E8 && to==A8) return  GenBlackOOO();
    }
    if (movedPiece == PAWN && to==p->posStore.epsq) return GenEnPassant(from, to);
    return GenBasicMove(from, to, movedPiece, capturedPiece);
}

int int_from_file(FILE *f, int l, uint64 *r) {
    if (f==NULL) {
        cout << "info string NULL file int_from_file\n";
        return 0;
    }
    int i,c;
    for (i=0;i<l;i++) {
        c=fgetc(f);
        if (c==EOF) {
            return 1;
        }
        (*r)=((*r)<<8)+c;
    }
    return 0;
}

void int_to_file(FILE *f, int l, uint64 r){
    if (f==NULL) {
        cout << "info string NULL file int_to_file\n";
        return;
    }
    int i,c;
    for(i=0;i<l;i++){
        c=(r>>8*(l-i-1))&0xff;
        fputc(c,f);
    }
}

void write_puck_entry(book_t *book, PuckBookEntry *entry){
    int_to_file(book->bookFile,8,entry->key);
    int_to_file(book->bookFile,4,entry->score);
}

int entry_from_polyglot_file(FILE *f, PolyglotBookEntry *entry, position_t *p) {
    uint64 r;
    if (int_from_file(f,8,&r)) return 1;
    entry->key=r;
    if (int_from_file(f,2,&r)) return 1;
    entry->move=polyglot_move_to_move((uint16) r, p);

    if (int_from_file(f,2,&r)) return 1;
    entry->weight=(uint16) r;
    if (int_from_file(f,4,&r)) return 1;
    entry->learn=(uint16) r;
    return 0;
}

int entry_from_puck_file(FILE *f, PuckBookEntry *entry) {
    uint64 r;
    if (int_from_file(f,8,&r)) return 1;
    entry->key=r;
    
    if (int_from_file(f,4,&r)) return 1;
    entry->score=(int) r; //adjusting because of no negatives
    return 0;
}
long find_puck_key(FILE *f, uint64 key, PuckBookEntry *entry) {
    long first, last, middle;
    PuckBookEntry first_entry=PuckBookEntryNone, last_entry,middle_entry;//TODO why is first entry setting needed?
    
    if (f==NULL) {
        cout << "info string find_puck_key book is oddly null\n";
    }
    first=-1;
    if (fseek(f,-Puck_Entry_Size,SEEK_END)) {
        *entry=PuckBookEntryNone;
        entry->key=key+1; //hack, should not be necessary if no entry can be 0
        return -1;
    }
    last=ftell(f)/Puck_Entry_Size;
    entry_from_puck_file(f,&last_entry);
    while (true) {
        if (last-first==1) {
            *entry=last_entry;
            return last;
        }
        middle=(first+last)/2;
        fseek(f,Puck_Entry_Size*middle,SEEK_SET);
        entry_from_puck_file(f,&middle_entry);
        if (key<=middle_entry.key) {//TODO can just return on an = if we want to
            last=middle;
            last_entry=middle_entry;
        } else {
            first=middle;
            first_entry=middle_entry;
        }
    }
}
void insert_entry_to_puck_file(book_t *book, PuckBookEntry *entry, long position){

	if (DEBUG_BOOK && book->type != PUCK_BOOK) cout << "info string trying to write to a non-puck book\n";
    else {
        if (book->bookFile==NULL) {
            cout << "info string NULL file insert_entry_to_puck_file\n";
            return;
        }
		if (position < 0) {
			write_puck_entry(book, entry);
			book->size=1;
		}
		else  {
			long positionOn = (long) book->size;
			if (DEBUG_BOOK) cout << "info string  book size now is " << positionOn << " writing to spot " << position << endl;
			while (positionOn > position) {
				PuckBookEntry tempEntry;
				fseek(book->bookFile,Puck_Entry_Size*(positionOn-1),SEEK_SET);
				entry_from_puck_file(book->bookFile, &tempEntry);
				fseek(book->bookFile,Puck_Entry_Size*(positionOn),SEEK_SET); //unnecessary I think
				write_puck_entry(book, &tempEntry);
				positionOn--;
			}
			fseek(book->bookFile,Puck_Entry_Size*position,SEEK_SET);
			write_puck_entry(book, entry);   
			fflush(book->bookFile);
			fseek(book->bookFile, 0, SEEK_END);
//			book->size++;
			book->size = ftell(book->bookFile) / Puck_Entry_Size;
		}
	}
}
void replace_entry_to_puck_file(book_t *book, PuckBookEntry *entry, long position){
	fseek(book->bookFile,Puck_Entry_Size*position,SEEK_SET);
	write_puck_entry(book, entry);   
	fflush(book->bookFile);
	fseek(book->bookFile, 0, SEEK_END);
	book->size = ftell(book->bookFile) / Puck_Entry_Size; //should have no effect
}

void insert_score_to_puck_file(book_t *book, uint64 key, int score){
	MutexLock(BookLock);
    if (DEBUG_BOOK && book->type != PUCK_BOOK) cout << "info string trying to write to a non-puck book\n";
    else {
        PuckBookEntry entry, tempEntry;
        entry.key = key;
        entry.score = score;
		long position = find_puck_key(book->bookFile, key, &tempEntry);
		if (key==tempEntry.key) {
			replace_entry_to_puck_file(book, &entry, position);
		}
		else
	        insert_entry_to_puck_file(book, &entry, position);
    }
	MutexUnlock(BookLock);

}

long find_polyglot_key(FILE *f, uint64 key, PolyglotBookEntry *entry, position_t *p) {
    long first, last, middle;
    PolyglotBookEntry first_entry=PolyglotBookEntryNone, last_entry,middle_entry;
    if (f==NULL) {
        *entry=PolyglotBookEntryNone;
        entry->key=key+1; //hack, should not be necessary if no entry can be 0
		cout << "info string NULL file find_polyglot_key" << endl;
        return -1;
    }    
    first=-1;
    if (fseek(f,-Polyglot_Entry_Size,SEEK_END)) {
        *entry=PolyglotBookEntryNone;
        entry->key=key+1; //hack, should not be necessary if no entry can be 0
        return -1;
    }
    last=ftell(f)/Polyglot_Entry_Size;
    entry_from_polyglot_file(f,&last_entry, p);
    while (true) {
        if (last-first==1) {
            *entry=last_entry;
            return last;
        }
        middle=(first+last)/2;
        fseek(f,Polyglot_Entry_Size*middle,SEEK_SET);
        entry_from_polyglot_file(f,&middle_entry, p);
        if (key<=middle_entry.key) {
            last=middle;
            last_entry=middle_entry;
        } else {
            first=middle;
            first_entry=middle_entry;
        }
    }
}

//TODO consider appending this, and having things get fromm back (and learned polyglot to reverse order of thing

void add_to_learn_begin(learn_t *learn, continuation_t *toLearn) {//we add it to the front
	MutexLock(LearningLock);
	if (learn->learnFile==NULL) {
        cout << "info string trying to learn with NULL learn file\n";
		MutexUnlock(LearningLock);
        return;
    }
    FILE * tempFile;
    uint64 r;
    tempFile = tmpfile();
    if (tempFile == NULL) {
        cout << "info string error opening a temp file\n";
    }
    else {

		if (SHOW_LEARNING) Print(3,"info string want to learn %s\n", pv2Str(toLearn).c_str());
		if (LOG_LEARNING) Print(2,"learning: want to learn %s\n", pv2Str(toLearn).c_str());
        rewind(learn->learnFile);
        while (int_from_file(learn->learnFile,MOVE_BYTES,&r)==0) {
            int_to_file(tempFile, MOVE_BYTES, r);
        }
        fclose(learn->learnFile);
        rewind(tempFile);
        //ok now we create a new learn file, stick our stuff in front, and then write the temp stuff in
        learn->learnFile = fopen(learn->name.c_str(), "w+b"); // going to write over everything with new file stuff
        if (learn->learnFile==NULL) cout << "info string uhoh...trouble reopening learn file '" << learn->name << "'\n";
        for (int i = 0; i < toLearn->length; i++) {
            int_to_file(learn->learnFile, MOVE_BYTES, toLearn->moves[i]);
        }
        int_to_file(learn->learnFile, MOVE_BYTES, NO_MOVE);
        while (int_from_file(tempFile,MOVE_BYTES,&r)==0) {
            int_to_file(learn->learnFile, MOVE_BYTES, r);
        }   
        fclose(tempFile);
		MutexUnlock(LearningLock);
    }
}

void closeBook(book_t *book) { //TODO consider mutex ramifications
	if (book->bookFile != NULL) fclose(book->bookFile);
}

void closeLearn(learn_t *learn) { //TODO consider mutex ramifications
	if (learn->learnFile != NULL) fclose(learn->learnFile);
}


void open_write_book(string book_name, book_t *book, BookType type) {
    if (type != PUCK_BOOK && type != POLYGLOT_BOOK) cout << "info string trying to open(write) unsupported book type\n";
    else {
        book->type = type;
        if (book->bookFile != NULL) fclose(book->bookFile);
        book->bookFile = fopen(book_name.c_str(), "r+b"); 
        if (book->bookFile == NULL) {
            book->bookFile = fopen(book_name.c_str(), "w+b"); 
            Print(3,"info string creating new file '%s'\n",book_name);
        }
        if (book->bookFile != NULL) {
            fseek(book->bookFile, 0, SEEK_END);
            if (type==POLYGLOT_BOOK) 
                book->size = ftell(book->bookFile) / Polyglot_Entry_Size;
            else if (type==PUCK_BOOK) {
                book->size = ftell(book->bookFile) / Puck_Entry_Size;
            }
            if (DEBUG_BOOK) cout << "info string opened(write) book to " << book_name << endl;
        }
        else  if (DEBUG_BOOK) cout << "info string failed to open(write) book " << book_name << endl;
    }
}
bool equalMove(const basic_move_t m1, const basic_move_t m2) {
	return (moveTo(m1)==moveTo(m2) && moveFrom(m1)==moveFrom(m2));
}
bool move_in_list(const basic_move_t m, const movelist_t *ml) {
	int on;
	for (on = 0; on < ml->size; on++) {
		if (equalMove(ml->list[on].m, m)) return true;
	}
    return false;
}

bool learn_continuation(int thread_id, continuation_t *toLearn) {
    position_t pos;
//    FILE *f = Glearn.learnFile;
	pos_store_t continuationUndo[MAX_BOOK];

	MutexLock(BookLock);
	if (GhannibalBook.bookFile==NULL || GhannibalBook.type!=PUCK_BOOK) {
        cout << "info string Hannibal Book (.han) must be selected\n";
		MutexUnlock(BookLock);
        return false;
    }
	//make sure book is open for writing
	fclose(GhannibalBook.bookFile);
	GhannibalBook.bookFile = fopen(GhannibalBook.name.c_str(), "r+b"); 
	MutexUnlock(BookLock);

	if ( toLearn->length > MAX_BOOK) toLearn->length = MAX_BOOK;
	if (SHOW_LEARNING) cout << "info string considering " << pv2Str(toLearn) << endl;
	if (LOG_LEARNING) Print(2,"learning: considering %s\n",pv2Str(toLearn).c_str());
	//first we will create the position we will learn from
	setPosition(&pos, STARTPOS);	
    for (int moveOn = 0; moveOn < toLearn->length; moveOn++) {
        bool goodMove = false;
		basic_move_t learnMove = toLearn->moves[moveOn];
		uint64 pinned = pinnedPieces(&pos, pos.side);
        //if (genMoveIfLegal(&pos, learnMove, pinned)) { 
			movelist_t moves;
			genLegal(&pos, &moves, true); 
			for (int on = 0; on < moves.size; on++) { 
				basic_move_t move = moves.list[on].m;
				if (!anyRepNoMove(&pos,learnMove)) {
//					Print(3,"%d move %s ",moveOn,move2Str(move));
					if (equalMove(move,learnMove)) {
						learnMove = move;//make sure we have the generatd one to be sure its legal and well formated
						toLearn->moves[moveOn] = move;
						goodMove = true;
						break;
					}
				}
			}
		//}
//		Print(3,"\n");
        if (goodMove) makeMove(&pos, &continuationUndo[moveOn], learnMove);
		else {
			if (SHOW_LEARNING) cout << "info string bad move to learn " << move2Str(toLearn->moves[moveOn]) << " in " << pv2Str(toLearn) << endl;
			if (LOG_LEARNING)  Print(2,"learning: bad move to learn %s in %s\n", move2Str(toLearn->moves[moveOn]), pv2Str(toLearn).c_str());
            toLearn->length = moveOn;
			if (moveOn == 0) {
				return false;
			}
			else break;
		}
		
    }
    bool searchIt;
    PuckBookEntry currentEntry;
    for (; toLearn->length>=0; toLearn->length--) { //now we learn
		MutexLock(BookLock);
		long position = find_puck_key(GhannibalBook.bookFile, pos.hash, &currentEntry);
        if (currentEntry.key != pos.hash || currentEntry.score == DEFAULT_BOOK_SCORE) searchIt = true;
        else searchIt = false;
        if (!searchIt) {
            if (DEBUG_LEARN) Print(3,"info string position known %d\n",currentEntry.score);
            int curScore = currentEntry.score;
            int bookScore = puck_book_score(&pos, &GhannibalBook);
            if (DEBUG_LEARN) cout << "info string current " << curScore << " book " <<bookScore << endl;
            if (bookScore > curScore) {
                currentEntry.score = bookScore;
                replace_entry_to_puck_file(&GhannibalBook, &currentEntry, position); //currentEntry == key so replace
                searchIt = false;
				if (SHOW_LEARNING) Print(3,"info string %s rescored from %d to %d\n",pv2Str(toLearn).c_str(),curScore,bookScore);
				if (LOG_LEARNING) Print(2,"learning: %s rescored from %d to %d\n",pv2Str(toLearn).c_str(),curScore,bookScore);
            }
            else if (bookScore == curScore) {
				searchIt = false;
			}
            else {
				if (DEBUG_LEARN) Print(3,"info string downgrading %d with %d???\n",curScore,bookScore);
				searchIt = true; //TODO consider searching the move we just did deeper if it is a move change
			}
        }
		MutexUnlock(BookLock);
        if (searchIt) {
            if (DEBUG_LEARN) cout << "info string studying " << toLearn->length << " " << pv2Str(toLearn) << endl;
            if (learn_position(&pos,thread_id,toLearn)==false) return true; //returning true just so we don't sleep
        }
        else if (DEBUG_LEARN) cout << "info string skipit\n";
        if (0 != toLearn->length) unmakeMove(&pos, &continuationUndo[toLearn->length-1]);
    }
	if (DEBUG_LEARN) cout << "info string DONE considering " << pv2Str(toLearn) << endl;
	return true;
}
//void remove_learning

bool get_continuation_to_learn(learn_t *learn, continuation_t *toLearn) {
    uint64 r;
    toLearn->length = 0;
    FILE *f = learn->learnFile;
	MutexLock(LearningLock);
    if (f==NULL) {
        cout << "info string NULL learn file\n";
		MutexUnlock(LearningLock);

        return false;
    }
	rewind(f);
    if (DEBUG_LEARN) cout << "info string looking for something to learn\n";
    bool learnIt = false;
    while (true) {
        if (toLearn->length >= MAXPLY-1) { 
			//keep getting stuff, but throw away anything too long
			do {
				if (int_from_file(f,MOVE_BYTES,&r)) {
					break;
		        }
			} while ((basic_move_t) ((uint16) r) != NO_MOVE);
			learnIt = true;
			break;
            //return true;
        }
        if (int_from_file(f,MOVE_BYTES,&r)) {
            break;
        }
        toLearn->moves[toLearn->length] = (basic_move_t) ((uint16) r);
        if (toLearn->moves[toLearn->length]==NO_MOVE) {
            learnIt = true;
            break;
        }
        toLearn->length++;
        learnIt = true;
    }
    if (learnIt == false) {
		MutexUnlock(LearningLock);
		return false;
    }
    //remove this by writing everything to a temp file, then writing the remainder back
    FILE * tempFile;
    tempFile = tmpfile();
    
    while (int_from_file(f,MOVE_BYTES,&r)==0) {
        int_to_file(tempFile, MOVE_BYTES, r);
    }
    fclose(f);
    rewind(tempFile);
    learn->learnFile = fopen(learn->name.c_str(), "w+b"); // going to write over everything with new file stuff
    if (learn->learnFile==NULL) cout << "info string uhoh...trouble reopening learn file '" << learn->name << "'\n";
    while (int_from_file(tempFile,MOVE_BYTES,&r)==0) {
        int_to_file(learn->learnFile, MOVE_BYTES, r);
    }   
    
    fclose (tempFile); 
	MutexUnlock(LearningLock);
   	return true;
}


void new_to_learn_end(learn_t *learn, continuation_t *soFar) {
	MutexLock(LearningLock);
	if (learn->learnFile == NULL) cout << "info string info string trying to learn but learnfile is null!\n";

    for (int on = 0; on < soFar->length; on++) {
        int_to_file(learn->learnFile,MOVE_BYTES,soFar->moves[on]); //TODO make learn move more compact?
    }
    int_to_file(learn->learnFile,MOVE_BYTES,NO_MOVE); // terminate with no_move
	MutexUnlock(LearningLock);
}

long polyglot_to_puck(const book_t *polyBook, book_t *puckBook, learn_t *learn, position_t *p, continuation_t *soFar, const int depth) { //position is the start of the learning position
	if (polyBook == NULL || polyBook->type!= POLYGLOT_BOOK) {
        cout << "info string trying to learn non-existent polyglot book\n";
        return 0;
    }
    if (puckBook == NULL || puckBook->type!= PUCK_BOOK) {
        cout << "info string trying to learn non-existent puck book\n";
        return 0;
    }
    if (depth >= MAX_CONVERT) {
        cout << "info string book too long " << depth << endl;  
		return 0;
    }
    int moveOn = 0;
    PolyglotBookEntry polyEntry[MAX_MOVES];
    PuckBookEntry puckEntry, tempPuckEntry;
    find_polyglot_key(polyBook->bookFile,p->hash,&polyEntry[0],p); //getting first move entry at this key
    if (p->hash==polyEntry[0].key) {
        moveOn++;
        while (true) { //getting all the rest of the entries with this key
            if (moveOn>=MAX_MOVES-1){
                if (DEBUG_BOOK) Print(3,"info string too many book moves?\n");
                break;
            }
            if (entry_from_polyglot_file(polyBook->bookFile,&polyEntry[moveOn],p)){
                if (DEBUG_BOOK) Print(3, "info string bad book entry?\n");
                break;
            }
            if (polyEntry[moveOn].key!=p->hash) {
                if (DEBUG_BOOK) Print(3, "info string last key entry\n");
                break;
            }
            if (DEBUG_BOOK) cout << "info string bookmove " << move2Str(polyEntry[moveOn].move) << " weight " << polyEntry[moveOn].weight << " learn " << polyEntry[moveOn].learn << endl;
            moveOn++;
        }
        movelist_t moves;
		genLegal(p, &moves, true); 
		for (int on = 0; on < moves.size; on++) { //getting all the moves that don't have entries with this key, but transpose to entries in the book (such as if book onlyhas d4 for white, but has replies to e4 as black)
			basic_move_t move = moves.list[on].m;
			pos_store_t undo;

			if (!anyRepNoMove(p,move)) { 
				
				makeMove(p, &undo, move);
                find_polyglot_key(polyBook->bookFile,p->hash,&polyEntry[moveOn],p); 
                if (p->hash==polyEntry[moveOn].key) {
                    bool newMove = true;
                    polyEntry[moveOn].move = move;
                    for (int on= moveOn-1; on >= 0; on--) { //ok, lets add all these moves now
                        if (p->hash==polyEntry[on].key) {
                            newMove=false;
                            break;
                        }
                    }
                    if (newMove) {
						ASSERT(depth <= soFar->length);
                        soFar->length = depth;

                        if (SHOW_LEARNING) Print(3,"info string transposition found: %s -- %s\n",pv2Str(soFar), moveToStr(polyEntry[moveOn].move).c_str());
						if (LOG_LEARNING) Print(2,"learning: transposition found: %s -- %s\n",pv2Str(soFar), moveToStr(polyEntry[moveOn].move).c_str());
                        moveOn++;
                    }
                }
                unmakeMove(p, &undo);
            }
        }

        //lets get all the moves
        long numAdded = 0;
		uint64 pinned = pinnedPieces(p, p->side);
        for (moveOn= moveOn-1; moveOn >= 0; moveOn--) { //ok, lets add all these moves now
            if (genMoveIfLegal(p, polyEntry[moveOn].move, pinned) && !anyRepNoMove(p, polyEntry[moveOn].move)) { //TODO do real move generation and ensure its a real move
                if (DEBUG_BOOK) cout << "info string found poly " << polyEntry[moveOn].key << ": " << move2Str(polyEntry[moveOn].move) << endl;
                long puckOffset;
				pos_store_t undo;
				makeMove(p, &undo, polyEntry[moveOn].move);
                soFar->moves[depth] = polyEntry[moveOn].move;
                puckEntry.key = p->hash;
                puckOffset=find_puck_key(puckBook->bookFile,puckEntry.key,&tempPuckEntry); //Don't iterate if we already have this position (NOTE: this may mess with things if .pck already exists
                if (tempPuckEntry.key != puckEntry.key) {
                    numAdded += polyglot_to_puck(polyBook,puckBook,learn, p, soFar, depth+1);
                    puckOffset=find_puck_key(puckBook->bookFile,puckEntry.key,&tempPuckEntry); //Need to find new position index, note we MAY be writing over an old one if                     there was a collision later in the tree
                    if (tempPuckEntry.key != puckEntry.key) {
                        puckEntry.score = DEFAULT_BOOK_SCORE;
                        if (DEBUG_BOOK) cout << "info string trying to write to position " << puckOffset << endl;
                        insert_entry_to_puck_file(puckBook, &puckEntry, puckOffset); //make sure we have done find_puck_key first so the keys are properly sorted
                        if (DEBUG_BOOK) {
                            find_puck_key(puckBook->bookFile,puckEntry.key,&tempPuckEntry); //TODO remove this is unneeded
                            if (tempPuckEntry.key != puckEntry.key) {
                                if (SHOW_LEARNING) cout << "info string not finding the entry(" << puckEntry.key <<") I just put in there?  Instead: << " << tempPuckEntry.key << "!\n";
							}
                        }
                        numAdded++;
                    }
                    else {
                        write_puck_entry(puckBook, &puckEntry);
                        if (SHOW_LEARNING) cout << "info string overwriting collision " << puckEntry.key << " with " << depth << ". " <<  move2Str(polyEntry[moveOn].move) << endl;
                    }
                }
                else if (DEBUG_BOOK) cout << "info string move already in there: " << move2Str(polyEntry[moveOn].move) << endl;
                unmakeMove(p, &undo);
 
            }
        }
        if (numAdded==1) { //most likely a leaf or transposition, so lets learn it
            soFar->length = depth+1;
            new_to_learn_end(learn, soFar);
            if (SHOW_LEARNING) Print(3, "info string tolearn: %s\n",pv2Str(soFar).c_str()); 
        }
        return numAdded;
    }
    return 0;
    
}
int current_puck_book_score(position_t *p, book_t *book) {
	long offset;
	PuckBookEntry entry;
	if (book->bookFile==NULL || book->type!=PUCK_BOOK) {
		cout << "info string no hannibal book in puck_book_score?\n";
		return -DEFAULT_BOOK_SCORE;
	}
	offset=find_puck_key(book->bookFile,p->hash,&entry);
	if (p->hash==entry.key && entry.score != DEFAULT_BOOK_SCORE) return entry.score;
	return -DEFAULT_BOOK_SCORE;
}
int puck_book_score(position_t *p, book_t *book) {
	    PuckBookEntry entry;
        uint64 key;
		pos_store_t undo;
		int numMoves = 0;
		long offset;

		if (book->bookFile==NULL || book->type!=PUCK_BOOK) {
			cout << "info string no hannibal book in puck_book_score?\n";
			return -DEFAULT_BOOK_SCORE;
		}
		movelist_t ml;
		genLegal(p, &ml, true); 
		int bestScore = -DEFAULT_BOOK_SCORE;
        for (int on = 0;on < ml.size;on++) {
			makeMove(p, &undo, ml.list[on].m);
            key = p->hash;
            unmakeMove(p, &undo);
            offset=find_puck_key(book->bookFile,key,&entry);
			if (key==entry.key && entry.score != DEFAULT_BOOK_SCORE) {
				if (-entry.score > bestScore) {
					bestScore = -entry.score; //it should be from the opposite players perspective
				}
            }
        }
		return bestScore;
}
void generateContinuation(continuation_t *variation) {
	position_t pos;
	pos_store_t undo;
	basic_move_t move;
	movelist_t moves;
	setPosition(&pos, STARTPOS);
	variation->length = 0;
	int randomness = Guci_options->bookExplore*5 + 10 + Guci_options->learnThreads;
	do {
		genLegal(&pos, &moves, true); 
		move = getBookMove(&pos,&GhannibalBook,&moves,false,randomness);
		if (!move) break;
		variation->moves[variation->length] = move;
		variation->length++;
		makeMove(&pos, &undo, move);
	} while (true);
}
basic_move_t getBookMove(position_t *p, book_t *book, movelist_t *ml, bool verbose, int randomness) {
    uint64 key; 
    int numMoves=0;
    uint64 totalWeight=0;
    long offset;
    FILE *f = book->bookFile;
    

    if (book->type==POLYGLOT_BOOK) {
		if (f == NULL || book->size == 0) {
			if (DEBUG_BOOK && verbose) cout << "info string no book\n";
			return NO_MOVE;
		}
		PolyglotBookEntry entry;
        PolyglotBookEntry entries[MAX_MOVES];
		key = p->hash;
        if (DEBUG_BOOK && verbose) cout << "info string looking in polyglot book\n";
        offset=find_polyglot_key(f,key,&entry,p);
        if (entry.key!=key) {
			return NO_MOVE;
        }
        if (DEBUG_BOOK && verbose) cout << "info string key is there\n";
        entries[numMoves]=entry;
        if (verbose && SHOW_LEARNING) cout << "info string bookmove " << move2Str(entries[numMoves].move) << " weight " << entries[numMoves].weight << " learn " << entries[numMoves].learn << endl;
        if ( move_in_list(entry.move, ml)) {
            totalWeight += entry.weight;
            numMoves++;
        }
        else if (verbose) cout << "info string bookmove not legal?\n";
        fseek(f,Polyglot_Entry_Size*(offset+1),SEEK_SET);
        while (true) {
            if (numMoves>=MAX_MOVES-1){
                if (DEBUG_BOOK && verbose) cout << "info string too many book moves?\n";
                break;
            }
            if (entry_from_polyglot_file(f,&entry,p)){
                if (DEBUG_BOOK && verbose) cout << "info string bad book entry?\n";
                break;
            }
            if (entry.key!=key) {
                if (DEBUG_BOOK && verbose) cout << "info string last key entry\n";
                break;
            }
            entries[numMoves]=entry;
            if (verbose && SHOW_LEARNING) cout << "info string bookmove " << move2Str(entries[numMoves].move) << " weight " << entries[numMoves].weight << " learn " << entries[numMoves].learn << endl;
            if ( move_in_list(entry.move, ml)) {
                totalWeight += entry.weight;
                numMoves++;
            }
            else if (verbose) cout << "info string bookmove not legal?\n";
        }
        if (totalWeight <= 0) {
            if (verbose) cout << "info string book weights are 0\n";
            return  NO_MOVE;
        }
        // chose here the move from the array and verify if it exists in the movelist
        uint64 bookRandom = rand() % totalWeight; //TODO do a real randomization
        uint64 bookIndex = 0;
        if (DEBUG_BOOK && verbose) cout << "info string random " << bookRandom << " out of " << totalWeight << endl;
        for (int i=0;i<numMoves;i++) {
            bookIndex += entries[i].weight;
            if (bookIndex > bookRandom) {
				return entries[i].move;
            }
        }
        if (verbose) cout << "info string not picking move?\n";
    }
    else if (book->type == PUCK_BOOK) {
        PuckBookEntry entry;
        move_t entries[MAX_MOVES];
        uint64 key;
		pos_store_t undo;
		MutexLock(BookLock); //i need to not have something writing while i am reading
		if (f == NULL || book->size == 0) {
			if (DEBUG_BOOK && verbose) cout << "info string no book\n";
			MutexUnlock(BookLock);
			return NO_MOVE;
		}
		if (DEBUG_BOOK && verbose) Print(3,"info string looking in puck book %d\n",book->size);
        for (int on = 0;on < ml->size;on++) {
			makeMove(p, &undo, ml->list[on].m);
            key = p->hash;
            unmakeMove(p, &undo);
            offset=find_puck_key(f,key,&entry);
            if (key==entry.key) {
                entries[numMoves].m = ml->list[on].m;
				entries[numMoves].s = -entry.score; //opposite since it is for the other person to move
                if (verbose && SHOW_LEARNING) cout << "info string bookmove " << move2Str(ml->list[on].m) << " score " << entries[numMoves].s << endl;
				if (entries[numMoves].s == -DEFAULT_BOOK_SCORE && randomness>=10) //always try an untried book move if you are exploring
					entries[numMoves].s = DEFAULT_BOOK_SCORE;
				if (Guci_options->bookExplore) {
					int colorAdjustedScore = entries[numMoves].s - TEMPO_OPEN * sign[p->side];
					int random = rand()%randomness - rand()%randomness;
					if (colorAdjustedScore > -randomness*2)
						entries[numMoves].s += random; //random book move
					else if (colorAdjustedScore > -randomness*4)
						entries[numMoves].s += random/2; //random book move
				}
                numMoves++;
            }
        }
		MutexUnlock(BookLock);
        if (numMoves==0) {
            if (DEBUG_BOOK && verbose) cout << "info string no moves in book\n"; 
        }
        else { //for now we are just going to pick the best move
            int bestScore = entries[0].s;
            basic_move_t bestMove = entries[0].m;
            for (int i = 1; i < numMoves; i++) {
                if (entries[i].s > bestScore) {
                    bestMove = entries[i].m;
					bestScore = entries[i].s;
                }
            }
			return bestMove;
        }
    }
    else if (verbose) cout << "info string unknown book type!?\n";
   return NO_MOVE;
}

void initBook(char* book_name, book_t *book, BookType type) {
	if (type != POLYGLOT_BOOK && type != PUCK_BOOK) cout << "info string book type not supported" << endl;
	else {
		if (book->bookFile != NULL) fclose(book->bookFile);
		book->bookFile = fopen(book_name, "rb");
		book->name = string(book_name);
        if (book->bookFile == NULL && type == PUCK_BOOK) {
            book->bookFile = fopen(book_name, "w+b"); 
            if (SHOW_LEARNING) Print(3,"info string creating new file '%s'\n",book_name);
        }
		if (book->bookFile != NULL) {
			book->type = type;
			fseek(book->bookFile, 0, SEEK_END);
			if (type==POLYGLOT_BOOK) 
				book->size = ftell(book->bookFile) / Polyglot_Entry_Size;
			else if (type==PUCK_BOOK)
				book->size = ftell(book->bookFile) / Puck_Entry_Size;
		 }
		if (DEBUG_BOOK) {
			if (type==POLYGLOT_BOOK) cout << "info string init polyglot book " << book_name << endl;
			else if (type==PUCK_BOOK) cout << "info string init hannibal book " << book_name << endl;
		}
	}
}


void initLearn(string learn_name, learn_t *learn) {
	learn->name = string(learn_name);
    if (learn->learnFile!=NULL) fclose(learn->learnFile);
    learn->learnFile = fopen(learn_name.c_str(), "a+b");
    if (learn->learnFile==NULL) cout << "info string failed to open learn file\n";
    else {
        if (DEBUG_LEARN) cout << "info string opened learn file " << learn_name << endl;        
    }
}
#endif