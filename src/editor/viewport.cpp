#include "viewport.hpp"
#include "../core/config.hpp"
#include <algorithm>

Viewport::Viewport(Buffer* buf):
    buf(buf),
    absolutePos({0,0}),
    scrollPos({0,0}),
    size({0,0}),
    desiredCol(0){}

void Viewport::setSize(Vec2<int> newSize){
    size = newSize;
}

std::string Viewport::getCurrLine(){
    return buf->lines.at(static_cast<std::size_t>(absolutePos.y));
}

void Viewport::moveCursor(Vec2<int> dv, Mode mode){
    // move cursor by dv.x and dv.y
    // if our new cursor loc is outside the range of SCROLL_BUFFER
    // then we set scroll to how far outside it is

    absolutePos += dv;


    absolutePos.y = std::clamp(absolutePos.y, 0, std::max(0, static_cast<int>(buf->lines.size())-1));

    int furthestXOnCurrLine {static_cast<int>(getCurrLine().size())};
    if(mode != Mode::INSERT) --furthestXOnCurrLine;
    furthestXOnCurrLine = std::max(0, furthestXOnCurrLine);

    absolutePos.x = std::clamp(absolutePos.x, 0, furthestXOnCurrLine);

    if(dv.x != 0)
        desiredCol = absolutePos.x;

    if(desiredCol != absolutePos.x){
        absolutePos.x = std::min(desiredCol, furthestXOnCurrLine);
    }

    if(absolutePos.y < scrollPos.y + SCROLL_BUFFER_Y){
        scrollPos.y = std::max(0, absolutePos.y - SCROLL_BUFFER_Y);
    }else if (absolutePos.y >= scrollPos.y + size.y - SCROLL_BUFFER_Y) {
        scrollPos.y = absolutePos.y - (size.y - 1) + SCROLL_BUFFER_Y;
    }
    if(absolutePos.x < scrollPos.x + SCROLL_BUFFER_X){
        scrollPos.x = std::max(0, absolutePos.x - SCROLL_BUFFER_X);
    }else if (absolutePos.x >= scrollPos.x + size.x - SCROLL_BUFFER_X) {
        scrollPos.x = absolutePos.x - (size.x - 1) + SCROLL_BUFFER_X;
    }
}

void Viewport::setCursor(Vec2<int> pos, Mode mode){
    moveCursor(pos - absolutePos, mode);
    desiredCol = absolutePos.x;
}
