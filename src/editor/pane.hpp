#pragma once

#include <unordered_map>
#include "../core/types.hpp"
#include "viewport.hpp"
#include <regex>
#include <ncurses.h>

struct Pane{
    Viewport* view;
    WINDOW* window;

    // maps pos.y, pos.x -> colorpair index
    std::unordered_map<int, std::unordered_map<int, SyntaxGroup>> colorMap {};

    Pane(Viewport* view, WINDOW* window);

    Vec2<int> getSize();
    void setSize(Vec2<int> size);

    Vec2<int> getPos();
    void setPos(Vec2<int> pos);
    void moveCursorToViewPos();

    void checkAndSetRegexAgainstLine(std::size_t row, std::regex reg, SyntaxGroup group);

    // returns color pair index for a given char pos
    int getColorGroup(Vec2<int> pos);

    void render();
    void drawDebug();
};
