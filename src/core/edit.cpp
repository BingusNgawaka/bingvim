#include "edit.hpp"


void updateDepthMap(BufferChange* node, std::map<int, std::vector<BufferChange*>>& depthMap, int currDepth){
    if(depthMap.contains(currDepth))
        depthMap.at(currDepth).push_back(node);
    else
        depthMap[currDepth] = {node};
    for(auto& child : node->children){
        updateDepthMap(child.get(), depthMap, currDepth+1);
    }
}

std::map<int, std::vector<BufferChange*>> getDepthMap(BufferChange* root){
    std::map<int, std::vector<BufferChange*>> depthMap {
        {0, {root}}
    };
    int currDepth {0};

    for(auto& child : root->children){
        updateDepthMap(child.get(), depthMap, currDepth+1);
    }

    return depthMap;
}

bool isLeaf(BufferChange* node){
    return node->children.size() == 0;
}

float getNodePosition(BufferChange* node, int& leafCount, std::map<BufferChange*, float>& posMap){
    if(isLeaf(node)){
        posMap[node] = ++leafCount;
        return static_cast<float>(leafCount);
    }

    float pos {};
    float count {};
    for(const auto& child : node->children){
        pos += getNodePosition(child.get(), leafCount, posMap);
        ++count;
    }

    posMap[node] = pos/count;

    return pos/count;
}

std::map<BufferChange*, float> getTreePositions(BufferChange* root){
    int leafCount {};
    std::map<BufferChange*, float> posMap {};

    float pos {getNodePosition(root, leafCount, posMap)};
    posMap[root] = pos;
    return posMap;
}
