#include "util.hpp"
#include <fstream>
#include <utility>
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

std::string getNewIndentationForNewLine(std::string currLine){
    int currIndent {std::max(countIndentation(currLine), 0)};
    int newIndent;

    if(currLine.back() == '{')
        newIndent = currIndent + tabSize;
    //else if(buf->lines.at(castedPos.y).back() == '}')
        //newIndent = currIndent - tabSize;
    else
        newIndent = currIndent;

    return std::string(std::max(0, newIndent), ' ');
}

std::map<int, std::string> split(std::string& inputStr, std::vector<std::string>& delimiters){
    std::string s {inputStr};
    std::map<int, std::string> tokens {};
    int pos {};

    std::size_t nextPos {0};
    while(nextPos != std::string::npos){
        nextPos = std::string::npos;
        std::size_t delSize = 0;
        for(const auto& del : delimiters){
            if(s.find(del) != std::string::npos && s.find(del) < nextPos){
                nextPos = s.find(del);
                delSize = del.size();
            }

        }
        if(nextPos != std::string::npos){
            std::string token {s.substr(0, nextPos)};
            s.erase(0, nextPos+delSize);
            if(token.size() > 0){
                tokens[pos] = token;
                pos += nextPos+delSize;
            }
        }
    }
    tokens[pos] = s;

    return tokens;
}

std::vector<std::string> getAlphaNumList(){
    std::vector<std::string> dels {};
    for(char c {'0'}; c <= '9'; ++c){
        dels.push_back(std::string(1, c));
    }
    for(char c {'A'}; c <= 'Z'; ++c){
        dels.push_back(std::string(1, c));
        dels.push_back(std::string(1, c+32));
    }
    return dels;
}
std::vector<std::string> getPunctuationList(){
    std::vector<std::string> dels {};
    for(char c {'!'}; c <= '/'; ++c){
        dels.push_back(std::string(1, c));
    }
    for(char c {':'}; c <= '@'; ++c){
        dels.push_back(std::string(1, c));
    }
    for(char c {'['}; c <= '`'; ++c){
        dels.push_back(std::string(1, c));
    }
    for(char c {'{'}; c <= '~'; ++c){
        dels.push_back(std::string(1, c));
    }
    return dels;
}
