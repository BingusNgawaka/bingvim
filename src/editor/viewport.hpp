#pragma once

#include "buffer.hpp"

struct Viewport{
    Buffer* buf;
    Vec2<int> absolutePos;
    Vec2<int> scrollPos;
    Vec2<int> size;
    int desiredCol;
    std::vector<int> buffLinesChanged {};

    Viewport(Buffer* buf);

    std::string getCurrLine();
    void setSize(Vec2<int> newSize);

    void moveCursor(Vec2<int> dv, Mode mode = Mode::NORMAL);
    void setCursor(Vec2<int> pos, Mode mode = Mode::NORMAL);
};
