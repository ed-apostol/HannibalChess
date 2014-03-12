/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include <map>
#include <sstream>

/* uci.c */
extern void uciSetOption(char string[]);
extern void uciParseSearchmoves(movelist_t *ml, char *str, uint32 moves[]);
extern void uciGo(position_t *pos, char *options);
extern void uciStart(void);
extern void uciSetPosition(position_t *pos, char *str);

struct Options {
    typedef void (*ActionFunc)(const Options&);
    Options() {}
    Options(const char* v, ActionFunc f) :              m_Type("string"),   m_Min(0),     m_Max(0),     OnChange(f) { m_DefVal = m_CurVal = v; }
    Options(bool v, ActionFunc f) :                     m_Type("check"),    m_Min(0),     m_Max(0),     OnChange(f) { m_DefVal = m_CurVal = (v ? "true" : "false"); }
    Options(ActionFunc f) :                             m_Type("button"),   m_Min(0),     m_Max(0),     OnChange(f) {}
    Options(int v, int minv, int maxv, ActionFunc f) :  m_Type("spin"),     m_Min(minv),  m_Max(maxv),  OnChange(f) { std::ostringstream ss; ss << v; m_DefVal = m_CurVal = ss.str(); }
    int GetInt() const {return ( m_Type == "spin" ? atoi(m_CurVal.c_str()) : m_CurVal == "true"); }
    std::string GetStr() { return m_CurVal; }
    Options& operator=(const std::string& val) {
        if (m_Type != "button") m_CurVal = val;
        if (OnChange) OnChange(*this);
        Print(3, "Gone here\n");
    }
    std::string m_DefVal, m_CurVal, m_Type;
    int m_Min, m_Max;
    ActionFunc OnChange;
};

typedef std::map<std::string, Options> UCIOptions;

extern void PrintUCIOptions(const UCIOptions& uci_opt);
extern void InitUCIOptions(UCIOptions& uci_opt);

extern UCIOptions UCIOptionsMap;
