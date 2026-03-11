#include "util.hpp"
#include <fstream>
#include "ncurses.h"
#include "config.hpp"

std::vector<std::string> readFile(std::string filepath){
    std::vector<std::string> lines {};

    std::ifstream file(filepath);
    std::string str;
    while(std::getline(file, str)){
        lines.push_back(str);
    }

    return lines;
}

Vec2<int> getTerminalSize(){
    int terminalW;
    int terminalH;
    getmaxyx(stdscr, terminalH, terminalW);

    return {terminalW, terminalH};
}

int countIndentation(std::string& str){
    int count {};
    for(char c : str){
        if(c == ' ')
            ++count;
        else
            break;
    }

    int diff {count % tabSize};
    if(diff == 0)
        return count;
    
    return count + (tabSize - diff);
}
/*
void write_file(std::vector<std::vector<char>> lines, std::string filepath){
    std::ofstream file(filepath, std::ios::trunc);

    for(const auto& line : lines){
        for(const auto& c : line){
            file << c;
        }
        file << "\n";
    }

    file.close();
}
*/

std::string editTypeToStr(Edit& edit){
    switch(edit.type){
            case Edit::Type::INSERT:
                return "INSERT(" + edit.text + ")";
                break;
            case Edit::Type::DELETE:
                return "DELETE(" + edit.text + ")";
                break;
            case Edit::Type::JOIN:
                return "JOIN(" + edit.text + ")";
                break;
            case Edit::Type::SPLIT:
                return "SPLIT";
                break;
    }

    return "none";
}
