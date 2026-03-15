#include "buffer.hpp"
#include "../core/util.hpp"
#include "../core/config.hpp"
#include <ncurses.h>
#include <stdexcept>

Buffer::Buffer(std::vector<std::string> lines, std::string filepath):
    lines(lines),
    filepath(filepath),
    modified(false),
    rootChange(new BufferChange{{0, 0}, {0,0}, {}, nullptr, {}}),
    lastChange(rootChange){}

void Buffer::addChange(Vec2<int>& startCursorPos, Vec2<int>& endCursorPos, std::vector<Edit>& edits){
    BufferChange* change {new BufferChange{startCursorPos, endCursorPos, edits, lastChange, {}}};
    lastChange->children.push_back(change);
    lastChange = change;
}

void Buffer::undoEdit(Edit& edit){
    Vec2<std::size_t> castedPos {static_cast<std::size_t>(edit.pos.x), static_cast<std::size_t>(edit.pos.y)};
    // apply inverse operations
    switch(edit.type){
        case Edit::Type::INSERT:
            if(castedPos.x >= lines.at(castedPos.y).size()){
                std::string a = "\nout of range when undoing insert, textUndone(";
                a.append(edit.text.c_str());
                a.append("), size(");
                a.append(std::to_string(edit.text.size()));
                a.append("), pos.x(");
                a.append(std::to_string(edit.pos.x));
                a.append(")\n");

                throw std::out_of_range(a);
            }
            lines.at(castedPos.y).erase(castedPos.x, edit.text.size());
            break;
        case Edit::Type::DELETE:
            lines.at(castedPos.y).insert(std::min(lines.at(castedPos.y).size(), castedPos.x-1), edit.text);
            break;
        case Edit::Type::JOIN:
            lines.insert(lines.begin()+edit.pos.y, edit.text);
            lines.at(castedPos.y-1).erase(lines.at(castedPos.y-1).size() - edit.text.size());
            break;
        case Edit::Type::SPLIT:
            lines.at(castedPos.y).append(edit.text);
            lines.erase(lines.begin()+edit.pos.y+1);
            break;
    }
}

void Buffer::redoEdit(Edit& edit){
    Vec2<std::size_t> castedPos {static_cast<std::size_t>(edit.pos.x), static_cast<std::size_t>(edit.pos.y)};
    // reapply operations
    switch(edit.type){
        case Edit::Type::INSERT:
            lines.at(castedPos.y).insert(castedPos.x, edit.text);
            break;
        case Edit::Type::DELETE:
            lines.at(castedPos.y).erase(std::max(0, edit.pos.x-1), edit.text.size());
            break;
        case Edit::Type::JOIN:
            lines.at(castedPos.y-1).append(lines.at(castedPos.y));
            lines.erase(lines.begin()+edit.pos.y);
            break;
        case Edit::Type::SPLIT:
            int currIndent {countIndentation(lines.at(castedPos.y))};
            int newIndent {};
            std::string rightSideOfLine {lines.at(castedPos.y).substr(castedPos.x)};

            if(lines.at(castedPos.y).back() == '{')
                newIndent = currIndent + tabSize;
            else if(lines.at(castedPos.y).back() == '}')
                newIndent = currIndent - tabSize;
            else
                newIndent = currIndent;

            std::string indentation (static_cast<std::size_t>(newIndent), ' ');

            std::string newString {indentation.append(rightSideOfLine)};
            lines.at(castedPos.y).erase(castedPos.x);
            lines.insert(lines.begin()+edit.pos.y+1, newString);
            break;
    }
}

Vec2<int> Buffer::undoLastChange(){
    if(lastChange != rootChange){
        Vec2<int> returnCursor {lastChange->startCursorPos};
        for(auto it{lastChange->edits.rbegin()}; it != lastChange->edits.rend(); ++it){
            undoEdit(*it);
        }
        lastChange = lastChange->parent;
        return returnCursor;
    }

    return {-1, -1};
}

Vec2<int> Buffer::redoLastChange(){
    if(lastChange->children.size() > 0){
        lastChange = lastChange->children.back();
        Vec2<int> returnCursor {lastChange->endCursorPos};
        for(auto it{lastChange->edits.begin()}; it != lastChange->edits.end(); ++it){
            redoEdit(*it);
        }
        return returnCursor;
    }

    return {-1, -1};
}
