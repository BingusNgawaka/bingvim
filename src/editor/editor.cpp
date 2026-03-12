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
        int i {pos.x-1};
        int whiteCount {};
        while(i >= 0){
            if(buf->lines.at(pos.y).at(i) == ' ' && !(i % tabSize == 0)){
                whiteCount++;
            }else{
                break;
            }
            --i;
        }

        if(whiteCount > 1){
            delCount += whiteCount;
        }

        int desiredX {std::max(0,pos.x-static_cast<int>(delCount))};
        std::size_t castedX {static_cast<std::size_t>(desiredX)};

        std::string deletedStr {buf->lines.at(castedPos.y).substr(castedX, delCount)};

        stagedEdits.push_back({
                Edit::Type::DELETE,
                // no idea why this is the way it is and i dont wanna touch it cause it works
                {pos.x - (whiteCount > 1 ? whiteCount : 0), pos.y},
                deletedStr
        });

        buf->lines.at(castedPos.y).erase(castedX, delCount);
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

    std::string indentation {getNewIndentationForNewLine(getCurrViewport()->getCurrLine())};

    std::string newString {indentation.append(rightSideOfLine)};
    buf->lines.at(castedPos.y).erase(castedPos.x);
    buf->lines.insert(buf->lines.begin()+pos.y+1, newString);

    getCurrViewport()->setCursor({countIndentation(buf->lines.at(pos.y+1)), pos.y+1}, Mode::INSERT);
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

    if(ch == 'o'){
        Vec2<int> pos {getCurrViewport()->absolutePos};
        Vec2<std::size_t> castedPos {static_cast<std::size_t>(pos.x), static_cast<std::size_t>(pos.y)};

        currMode = Mode::INSERT;
        startChange();

        std::string newLineIndentation {getNewIndentationForNewLine(getCurrViewport()->getCurrLine())};

        getCurrBuffer()->lines.insert(getCurrBuffer()->lines.begin()+pos.y+1, newLineIndentation);

        // new line can be thought of splitting at the end of the curr line
        stagedEdits.push_back({
                Edit::Type::SPLIT,
                {static_cast<int>(getCurrViewport()->getCurrLine().size()), pos.y},
                ""
        });

        getCurrViewport()->setCursor({static_cast<int>(newLineIndentation.size()), pos.y+1}, currMode);
    }

    if(ch == 'O'){
        Vec2<int> pos {getCurrViewport()->absolutePos};
        Vec2<std::size_t> castedPos {static_cast<std::size_t>(pos.x), static_cast<std::size_t>(pos.y)};

        currMode = Mode::INSERT;
        startChange();

        //
        std::string line {getCurrViewport()->getCurrLine()};
        int currIndent {std::max(countIndentation(line), 0)};
        int newIndent;

        if(line.back() == '{') // opposite of normal indent new line rules because going up
            newIndent = currIndent - tabSize;
        else if(line.back() == '}')
            newIndent = currIndent + tabSize;
        else
            newIndent = currIndent;

        std::string newLineIndentation(std::max(0, newIndent), ' ');
        //

        getCurrBuffer()->lines.insert(getCurrBuffer()->lines.begin()+std::max(0, pos.y), newLineIndentation);

        // new line can be thought of splitting at the end of the curr line
        stagedEdits.push_back({
                Edit::Type::SPLIT,
                {static_cast<int>(getCurrBuffer()->lines.at(std::max(0,pos.y-1)).size()), pos.y-1},
                ""
        });

        getCurrViewport()->setCursor({static_cast<int>(newLineIndentation.size()), pos.y}, currMode);

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

        /*
        int currIndent {countIndentation(getCurrBuffer()->lines.at(castedPos.y))};
        if(ch == '}' && currIndent > 0){
            std::size_t diff {static_cast<std::size_t>(tabSize)};
            std::string str(diff, ' ');
            stagedEdits.push_back({
                    Edit::Type::DELETE,
                    {pos.x-static_cast<int>(diff), pos.y},
                    str
            });
            getCurrBuffer()->lines.at(castedPos.y).erase(pos.x - diff, diff);
            getCurrViewport()->moveCursor({-static_cast<int>(diff), 0}, Mode::INSERT);
        }

        pos = getCurrViewport()->absolutePos;
        castedPos = {static_cast<std::size_t>(pos.x), static_cast<std::size_t>(pos.y)};
        */

        stagedEdits.push_back({
                Edit::Type::INSERT,
                pos,
                std::string(1, static_cast<char>(ch))
        });
        getCurrBuffer()->lines.at(castedPos.y).insert(castedPos.x, 1, static_cast<char>(ch));
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
