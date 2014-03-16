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
extern void displayBoard(const position_t& pos, int x);
extern int getPiece(const position_t& pos, uint32 sq);
extern int getColor(const position_t& pos, uint32 sq);
extern int DiffColor(const position_t& pos, uint32 sq,int color);
extern uint64 getTime(void);
extern uint32 parseMove(movelist_t *mvlist, const char *s);
extern int biosKey(void);
extern int getDirIndex(int d);
extern int anyRep(const position_t& pos);
extern int anyRepNoMove(const position_t& pos, const int m);

enum LogLevel { logNONE = 0, logOUT = 1, logERROR = 2, logWARNING = 3, logINFO = 4, logDEBUG = 5 }; // TOD: to be improved

struct LogToFile : public std::ofstream {
    LogToFile(const std::string& f = "log.txt") : std::ofstream(f.c_str(), std::ios::out | std::ios::app) {}
    ~LogToFile() { if (is_open()) close(); }
};

template <LogLevel level, bool logtofile = false>
class Log {
public:
    static const LogLevel ClearanceLevel = logINFO;
    Log() { }
    template <typename T>
    Log& operator << (const T& object) {
        if (level > logNONE && level <= ClearanceLevel) _buffer << object;
        return *this;
    }
    ~Log() {
        if (level > logNONE && level <= ClearanceLevel) {
            static const std::string LevelText[6] = {"NONE", "OUT", "ERROR", "WARNING", "INFO", "DEBUG"};
            _buffer << std::endl;
            if (level == logOUT) std::cout << _buffer.str();
            if (logtofile) LogToFile() << LevelText[level] << ": " << _buffer.str();
            else if (level > logOUT) std::cout << LevelText[level] << ": " << _buffer.str();
        }
    }
private:
    std::ostringstream _buffer;
};

typedef Log<logERROR, true> LogError;
typedef Log<logWARNING, true> LogWarning;
typedef Log<logINFO, true> LogInfo;
typedef Log<logDEBUG, true> LogDebug;
typedef Log<logOUT, true> LogAndPrintOutput;
typedef Log<logERROR> PrintError;
typedef Log<logWARNING> PrintWarning;
typedef Log<logINFO> PrintInfo;
typedef Log<logDEBUG> PrintDebug;
typedef Log<logOUT> PrintOutput;

