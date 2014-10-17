/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include <map>
#include <sstream>
#include <vector>
#include "typedefs.h"
#include "utils.h"

struct Options {
    typedef void(*ActionFunc)(const Options&);
    Options() {}
    Options(const char* v, ActionFunc f) : mType("string"), mMin(0), mMax(0), OnChange(f) {
        mDefVal = mCurVal = v;
    }
    Options(bool v, ActionFunc f) : mType("check"), mMin(0), mMax(0), OnChange(f) {
        mDefVal = mCurVal = (v ? "true" : "false");
    }
    Options(ActionFunc f) : mType("button"), mMin(0), mMax(0), OnChange(f) {}
    Options(int v, int minv, int maxv, ActionFunc f) : mType("spin"), mMin(minv), mMax(maxv), OnChange(f) {
        std::ostringstream ss; ss << v; mDefVal = mCurVal = ss.str();
    }
    int GetInt() const {
        return (mType == "spin" ? atoi(mCurVal.c_str()) : mCurVal == "true");
    }
    std::string GetStr() {
        return mCurVal;
    }
    Options& operator=(const std::string& val) {
        if (mType != "button") mCurVal = val;
        if (OnChange) OnChange(*this);
        return *this;
    }
    std::string mDefVal, mCurVal, mType;
    int mMin, mMax;
    ActionFunc OnChange;
};

typedef std::map<std::string, Options> UCIOptionsBasic;

class UCIOptions : public UCIOptionsBasic {
public:
    Options& operator[] (std::string const& key) {
        UCIOptionsBasic::iterator opt = find(key);
        if (opt != end()) {
            return opt->second;
        } else {
            mKeys.push_back(key);
            return (insert(std::make_pair(key, Options())).first)->second;
        }
    }
    void Print() const {
        for (int idx = 0; idx < mKeys.size(); ++idx) {
            UCIOptionsBasic::const_iterator itr = find(mKeys[idx]);
            if (itr != end()) {
                const Options& opt = itr->second;
                LogAndPrintOutput log;
                log << "option name " << mKeys[idx] << " type " << opt.mType;
                if (opt.mType != "button") log << " default " << opt.mDefVal;
                if (opt.mType == "spin") log << " min " << opt.mMin << " max " << opt.mMax;
            }
        }
    }
private:
    std::vector<std::string> mKeys;
};

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
    void PrintUCIOptions(UCIOptions& uci_opt);
    void InitUCIOptions(UCIOptions& uci_opt);

    void CheckSpeedup(std::istringstream& stream);

    static const std::string name;
    static const std::string author;
    static const std::string year;
    static const std::string version;
    static const std::string arch;
};
