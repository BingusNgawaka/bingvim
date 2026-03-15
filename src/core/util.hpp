#pragma once

#include <string>
#include <map>
#include <vector>
#include "types.hpp"
#include "edit.hpp"

#define CTRL(x) ((x) & 0x1f)
#define SHIFT(x) (x - 32)

std::vector<std::string> readFile(std::string filepath);
Vec2<int> getTerminalSize();
int countIndentation(std::string& str);
std::string getNewIndentationForNewLine(std::string currLine);
std::string editTypeToStr(Edit& edit);
std::map<int, std::string> split(std::string& inputStr, std::vector<std::string>& delimiters);
std::vector<std::string> getPunctuationList();
std::vector<std::string> getAlphaNumList();
