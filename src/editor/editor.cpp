#include "editor.hpp"
#include <ncurses.h>
#include "../core/config.hpp"
#include "../core/util.hpp"

std::size_t Editor::addBuffer(std::vector<std::string> lines, std::string filepath){
    buffers.push_back(std::make_unique<Buffer>(lines, filepath));

    return buffers.size()-1;
}

std::size_t Editor::addViewport(std::size_t bufferIndex){
    viewports.push_back(std::make_unique<Viewport>(buffers.at(bufferIndex).get()));
    return viewports.size()-1;
}

std::size_t Editor::addPane(std::size_t viewportIndex, Vec2<int> pos, Vec2<int> size){
    if(viewportIndex >= viewports.size())
        throw std::out_of_range("viewport index out_of_range @ addPane");

    viewports.at(viewportIndex)->setSize(size);

    panes.push_back({
        viewports.at(viewportIndex).get(),
        // h, w, starty, startx
        newwin(size.y, size.x, pos.y, pos.x)
    });

    activePane = panes.size() - 1;
    return activePane;
}

Pane& Editor::getCurrPane(){
    return panes.at(activePane);
}

void Editor::renderCurrPane(){
    getCurrPane().render();

}

Viewport* Editor::getCurrViewport(){
    return getCurrPane().view;
}

Buffer* Editor::getCurrBuffer(){
    return getCurrViewport()->buf;
}

void Editor::startChange(){
    stagedEdits.clear();
    firstCursorPos = getCurrViewport()->absolutePos;
}
void Editor::commitChange(){
    // TODO optimize edits
    // collapse adjacent inserts and cancel adjacent identical inserts and deletes
    // cancel adjacent splits and joins as well
    getCurrBuffer()->addChange(firstCursorPos, getCurrViewport()->absolutePos, stagedEdits);
}

void Editor::handleBackspaceLogic(){
    Vec2<int> pos {getCurrViewport()->absolutePos};
    Vec2<std::size_t> castedPos {static_cast<std::size_t>(pos.x), static_cast<std::size_t>(pos.y)};
    Buffer* buf {getCurrBuffer()};
    
    if(pos.x > 0){

        std::size_t delCount {1};
        int ind {};
        for(auto it {buf->lines.at(castedPos.y).begin() + pos.x-1}; it != buf->lines.at(castedPos.y).begin(); --it){
            ind++;
            if(*it == ' ' && (pos.x-ind) % tabSize != 0){
                ++delCount;
            }else{
                if(*it != ' ' && ind > 1)
                    --delCount;
                break;
            }
        }
        std::string deletedStr {buf->lines.at(castedPos.y).substr(castedPos.x-delCount, delCount)};

        stagedEdits.push_back({
                Edit::Type::DELETE,
                pos,
                deletedStr
        });

        buf->lines.at(castedPos.y).erase(castedPos.x-delCount, delCount);
        getCurrViewport()->moveCursor({-static_cast<int>(delCount), 0}, Mode::INSERT);

    }else if(pos.y > 0){
        std::string oldLine {buf->lines.at(castedPos.y)};
        stagedEdits.push_back({
                Edit::Type::JOIN,
                pos,
                oldLine
        });

        getCurrViewport()->setCursor({static_cast<int>(buf->lines.at(castedPos.y-1).size()+1), pos.y-1}, Mode::INSERT);

        buf->lines.at(castedPos.y-1).append(oldLine);
        buf->lines.erase(buf->lines.begin()+pos.y);
    }
}

void Editor::handleEnterLogic(){
    Vec2<int> pos {getCurrViewport()->absolutePos};
    Vec2<std::size_t> castedPos {static_cast<std::size_t>(pos.x), static_cast<std::size_t>(pos.y)};
    Buffer* buf {getCurrBuffer()};

    std::string rightSideOfLine {buf->lines.at(castedPos.y).substr(castedPos.x)};
    stagedEdits.push_back({
            Edit::Type::SPLIT,
            pos,
            rightSideOfLine
    });

    int currIndent {std::max(countIndentation(buf->lines.at(castedPos.y)), 0)};
    int newIndent;

    if(buf->lines.at(castedPos.y).back() == '{')
        newIndent = currIndent + tabSize;
    //else if(buf->lines.at(castedPos.y).back() == '}')
        //newIndent = currIndent - tabSize;
    else
        newIndent = currIndent;

    std::string indentation (static_cast<std::size_t>(std::max(newIndent, 0)), ' ');

    std::string newString {indentation.append(rightSideOfLine)};
    buf->lines.at(castedPos.y).erase(castedPos.x);
    buf->lines.insert(buf->lines.begin()+pos.y+1, newString);

    getCurrViewport()->setCursor({newIndent, pos.y+1}, Mode::INSERT);
}

// movement keybinds that i actually use
// DONE:
// hjkl
// TODO:
// w, e, b,
// state machine
// w look at curr cursor and keep going forward until not that type of char anymore
// w jumps to next beggining of word, e to next end of word, b to the prev beggining of word
// word delimiters: not (a-zA-Z, 0-9, _)
// WORD delimiters: only whitespace
// f, F with ; and ,
// / search does a regex search sooo
// $ end of line
// ^ beggining of text in line
//
// edit keybinds
// i, a, o, I, A, O
//
// paste buffer related keybinds
// yy - yank curr line
// nyy - yank n lines below
// 
//
// numbers to denote repeated commands
void Editor::handleNormalModeInput(int ch){
    int movementAmnt {1};
    if(ch == 'h')
        getCurrViewport()->moveCursor({-movementAmnt, 0});
    if(ch == 'j')
        getCurrViewport()->moveCursor({0, movementAmnt});
    if(ch == 'k')
        getCurrViewport()->moveCursor({0, -movementAmnt});
    if(ch == 'l')
        getCurrViewport()->moveCursor({movementAmnt, 0});

    if(ch == 'u'){
        Vec2<int> undoVec {getCurrBuffer()->undoLastChange()};
        if(undoVec != Vec2<int>{-1, -1})
            getCurrViewport()->setCursor(undoVec);
    }

    if(ch == CTRL('r')){
        Vec2<int> redoVec {getCurrBuffer()->redoLastChange()};
        if(redoVec != Vec2<int>{-1, -1})
            getCurrViewport()->setCursor(redoVec);
    }

    if(ch == 'i'){
        currMode = Mode::INSERT;
        startChange();
    }

    if(ch == 'a'){
        currMode = Mode::INSERT;
        startChange();
        getCurrViewport()->moveCursor({1, 0}, currMode);
    }

    if(ch == 'A'){
        currMode = Mode::INSERT;
        startChange();
        getCurrViewport()->setCursor({static_cast<int>(getCurrViewport()->getCurrLine().size()+1), getCurrViewport()->absolutePos.y}, currMode);
    }
}
void Editor::handleInsertModeInput(int ch){
    Vec2<int> pos {getCurrViewport()->absolutePos};
    Vec2<std::size_t> castedPos {static_cast<std::size_t>(pos.x), static_cast<std::size_t>(pos.y)};
    if(ch >= 0 && ch <= 255 && std::isprint(ch)){
        getCurrBuffer()->lines.at(castedPos.y).insert(castedPos.x, 1, static_cast<char>(ch));

        int currIndent {countIndentation(getCurrBuffer()->lines.at(castedPos.y))};
        if(ch == '}' && currIndent > 0){
            std::size_t diff {static_cast<std::size_t>(tabSize)};
            std::string str(diff, ' ');
            stagedEdits.push_back({
                    Edit::Type::DELETE,
                    pos,
                    str
            });
            getCurrBuffer()->lines.at(castedPos.y).erase(pos.x - diff, diff);
        }

        stagedEdits.push_back({
                Edit::Type::INSERT,
                pos,
                std::string(1, static_cast<char>(ch))
        });

        getCurrViewport()->moveCursor({1, 0}, Mode::INSERT);
    }

    if(ch == '\t'){
        int indentDiff {tabSize - pos.x % tabSize};
        std::string addedText(static_cast<std::size_t>(indentDiff), ' ');
        getCurrBuffer()->lines.at(castedPos.y).insert(castedPos.x, addedText);

        stagedEdits.push_back({
                Edit::Type::INSERT,
                pos,
                addedText
        });

        getCurrViewport()->moveCursor({static_cast<int>(addedText.size()), 0}, Mode::INSERT);
    }

    if(ch == KEY_ESCAPE){
        currMode = NORMAL;
        getCurrViewport()->moveCursor({-1, 0});
        commitChange();
    }

    if(ch == KEY_BACKSPACE){
        handleBackspaceLogic();
    }

    if(ch == 10){ // ENTER
        handleEnterLogic();
    }
}
void Editor::handleInput(int ch){
    switch(currMode){
        case NORMAL:
            handleNormalModeInput(ch);
            break;
        case INSERT:
            handleInsertModeInput(ch);
            break;
        case COMMAND:
            break;
        case UNDOTREE:
            break;
    }
}
