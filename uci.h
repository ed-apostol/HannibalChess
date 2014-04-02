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
#include "typedefs.h"

struct Options {
    typedef void(*ActionFunc)(const Options&);
    Options() {}
    Options(const char* v, ActionFunc f) : m_Type("string"), m_Min(0), m_Max(0), OnChange(f) { m_DefVal = m_CurVal = v; }
    Options(bool v, ActionFunc f) : m_Type("check"), m_Min(0), m_Max(0), OnChange(f) { m_DefVal = m_CurVal = (v ? "true" : "false"); }
    Options(ActionFunc f) : m_Type("button"), m_Min(0), m_Max(0), OnChange(f) {}
    Options(int v, int minv, int maxv, ActionFunc f) : m_Type("spin"), m_Min(minv), m_Max(maxv), OnChange(f) { std::ostringstream ss; ss << v; m_DefVal = m_CurVal = ss.str(); }
    int GetInt() const { return (m_Type == "spin" ? atoi(m_CurVal.c_str()) : m_CurVal == "true"); }
    std::string GetStr() { return m_CurVal; }
    Options& operator=(const std::string& val) {
        if (m_Type != "button") m_CurVal = val;
        if (OnChange) OnChange(*this);
        return *this;
    }
    std::string m_DefVal, m_CurVal, m_Type;
    int m_Min, m_Max;
    ActionFunc OnChange;
};

typedef std::map<std::string, Options> UCIOptions;

extern UCIOptions UCIOptionsMap;


class Interface {
public:
    Interface();
    ~Interface();
    void Info();
    void Run();

private:
    bool Input(std::istringstream& stream);
    void Stop();
    void Ponderhit();
    void Go(std::istringstream& stream);
    void Position(std::istringstream& stream);
    void SetOption(std::istringstream& stream);
    void NewGame();
    void Id();
    void Quit();
    void PrintUCIOptions(const UCIOptions& uci_opt);
    void InitUCIOptions(UCIOptions& uci_opt);

    static const std::string name;
    static const std::string author;
    static const std::string year;
    static const std::string version;
    static const std::string arch;
};
