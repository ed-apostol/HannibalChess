/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once
#include <sstream>
#include <vector>
#include "typedefs.h"
#include "utils.h"
#include "search.h"

class Interface {
public:
    Interface();
    ~Interface();
    void Info();
    void Run();

private:
    bool Input(std::istringstream& stream);
    void Stop();
    void PonderHit();
    void Go(std::istringstream& stream);
    void Position(std::istringstream& stream);
    void SetOption(std::istringstream& stream);
    void NewGame();
    void Id();
    void Quit();

    void CheckSpeedup(std::istringstream& stream);
    void CheckBestSplit(std::istringstream& stream);

    static const std::string name;
    static const std::string author;
    static const std::string year;
    static const std::string version;
    static const std::string arch;

    Engine cEngine;
    position_t input_pos;
};
