#pragma once

#include "../core/edit.hpp"

struct Buffer{
    std::vector<std::string> lines;
    std::string filepath;

    bool modified;
    BufferChange* rootChange;
    BufferChange* lastChange;

    Buffer(std::vector<std::string> lines, std::string filepath);

    void addChange(Vec2<int>& startCursorPos, Vec2<int>& endCursorPos, std::vector<Edit>& edits);

    void undoEdit(Edit& edit);

    void redoEdit(Edit& edit);

    Vec2<int> undoLastChange();

    Vec2<int> redoLastChange();
};
