#pragma once

#include <string>
#include <vector>
#include "types.hpp"
#include "edit.hpp"

#define CTRL(x) ((x) & 0x1f)
#define SHIFT(x) (x - 32)

std::vector<std::string> readFile(std::string filepath);
Vec2<int> getTerminalSize();
int countIndentation(std::string& str);
std::string editTypeToStr(Edit& edit);
