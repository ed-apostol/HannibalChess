/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include "typedefs.h"
#include "threads.h"
#include <fstream>
#include <iostream>
#include <sstream>

extern void PrintBitBoard(uint64 n);
extern char *bit2Str(uint64 n);
extern char *move2Str(basic_move_t m);
extern char *sq2Str(int sq);
extern void displayBoard(const position_t& pos, int x);
extern int getPiece(const position_t& pos, uint32 sq);
extern int getColor(const position_t& pos, uint32 sq);
extern int DiffColor(const position_t& pos, uint32 sq, int color);
extern uint64 getTime(void);
extern uint32 parseMove(movelist_t& mvlist, const char *s);
extern int getDirIndex(int d);
extern bool anyRep(const position_t& pos);
extern bool anyRepNoMove(const position_t& pos, const int m);

enum LogLevel {
    logNONE = 0, logIN, logOUT, logERROR, logWARNING, logINFO, logDEBUG
};

struct LogToFile : public std::ofstream {
    LogToFile(const std::string& f = "log.txt") : std::ofstream(f.c_str(), std::ios::out | std::ios::app) {}
    ~LogToFile() {
        if (is_open()) close();
    }
};

template <LogLevel level, bool out = true, bool logtofile = false>
class Log {
public:
    static const LogLevel ClearanceLevel = logINFO;
    Log() {}
    template <typename T>
    Log& operator << (const T& object) {
        if (level > logNONE && level <= ClearanceLevel) _buffer << object;
        return *this;
    }
    ~Log() {
        if (level > logNONE && level <= ClearanceLevel) {
            static const std::string LevelText[7] = { "", "->", "<-", "!!", "??", "==", "||" };
            _buffer << std::endl;
            if (out) {
                if (level == logOUT) std::cout << _buffer.str();
                else std::cout << LevelText[level] << ": " << _buffer.str();
            }
            if (logtofile) {
                static Spinlock splck;
                std::lock_guard<Spinlock> lock(splck);
                LogToFile() << getTime() << " " << LevelText[level] << " " << _buffer.str();
            }
        }
    }
private:
    std::ostringstream _buffer;
};

typedef Log<logIN> PrinInput;
typedef Log<logOUT> PrintOutput;
typedef Log<logERROR> PrintError;
typedef Log<logWARNING> PrintWarning;
typedef Log<logINFO> PrintInfo;
typedef Log<logDEBUG> PrintDebug;
typedef Log<logIN, false, true> LogInput;
typedef Log<logOUT, false, true> LogOutput;
typedef Log<logERROR, false, true> LogError;
typedef Log<logWARNING, false, true> LogWarning;
typedef Log<logINFO, false, true> LogInfo;
typedef Log<logDEBUG, false, true> LogDebug;
typedef Log<logIN, true, true> LogAndPrintInput;
typedef Log<logOUT, true, true> LogAndPrintOutput;
typedef Log<logERROR, true, true> LogAndPrintError;
typedef Log<logWARNING, true, true> LogAndPrintWarning;
typedef Log<logINFO, true, true> LogAndPrintInfo;
typedef Log<logDEBUG, true, true> LogAndPrintDebug;
