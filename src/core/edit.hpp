#pragma once

#include "types.hpp"
#include <map>
#include <string>
#include <vector>

// split edit blocks (buffer change commits) into fundamental edits
// insert char, delete char, split line, join line
// then a commit is simply a set of ordered edits (can optimise delete char and insert char by combining contiguous edits of those type)
// undo is do it in reverse
struct Edit{
    enum Type{
        INSERT, DELETE, SPLIT, JOIN
    };

    Type type;
    Vec2<int> pos;
    
    std::string text;
};

struct BufferChange{
    Vec2<int> startCursorPos;
    Vec2<int> endCursorPos;

    int depth;

    std::vector<Edit> edits;

    BufferChange* parent;
    std::vector<BufferChange*> children;
};

void updateDepthMap(BufferChange* node, std::map<int, std::vector<BufferChange*>>& depthMap, int& currDepth);
std::map<int, std::vector<BufferChange*>> getDepthMap(BufferChange* root);

bool isLeaf(BufferChange* node);
float getNodePosition(BufferChange* node, int& leafCount, std::map<BufferChange*, float>& posMap);
std::map<BufferChange*, float> getTreePosiions(BufferChange* root);
