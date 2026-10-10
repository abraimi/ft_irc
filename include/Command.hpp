// tempoorary unitil you implement it , i need now to start processing commands

#pragma once

#include <string>
#include <vector>

struct Command 
{
    std::string name; // command name "NICK"
    std::vector<std::string> args; // ["reda, "0", "*", "Reda Tahri"]
};