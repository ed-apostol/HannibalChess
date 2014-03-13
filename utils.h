/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "typedefs.h"
#include <fstream>
#include <iostream>
#include <sstream>

/* utils.c */
extern void Print(int vb, char *fmt, ...);
extern void displayBit(uint64 a, int x);
extern char *bit2Str(uint64 n);
extern char *move2Str(basic_move_t m);
extern char *sq2Str(int sq);
extern void displayBoard(const position_t *pos, int x);
extern int getPiece(const position_t *pos, uint32 sq);
extern int getColor(const position_t *pos, uint32 sq);
extern int DiffColor(const position_t *pos, uint32 sq,int color);
extern uint64 getTime(void);
extern uint32 parseMove(movelist_t *mvlist, const char *s);
extern int biosKey(void);
extern int getDirIndex(int d);
extern int anyRep(const position_t *pos);
extern int anyRepNoMove(const position_t *pos, const int m);

enum LogLevel { cNONE = 0, cOUT = 1, cERROR = 2, cWARNING = 3, cINFO = 4, cDEBUG = 5 };

struct Logger : public std::ofstream {
    Logger(const std::string& f = "log.txt") : std::ofstream(f.c_str(), std::ios::out | std::ios::app) {}
    ~Logger() { if (is_open()) close(); }
};

template <LogLevel m_Level, bool logtofile = false>
class Log {
public:
    static const LogLevel ClearanceLevel = cINFO;
    Log() {}
    template <typename T>
    Log& operator << (const T& object) {
        _buffer << object;
        return *this;
    }
    ~Log() {
        _buffer << std::endl;
        if (m_Level <= ClearanceLevel) std::cout << _buffer.str();
        if (logtofile) Logger() << _buffer.str();
    }
private:
    std::ostringstream _buffer;
};