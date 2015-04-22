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
    void Stop(Engine& engine);
    void PonderHit(Engine& engine);
    void Go(Engine& engine, position_t& pos, std::istringstream& stream);
    void Position(Engine& engine, position_t& pos, std::istringstream& stream);
    void SetOption(Engine& engine, std::istringstream& stream);
    void NewGame(Engine& engine);
    void Id(Engine& engine);
    void Quit(Engine& engine);

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
