#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <ncurses.h>
#include <stdexcept>
#include <string>
#include <vector>

#define DEBUG true

#define tabSize 4

#define KEY_ESCAPE 27

#define SCROLL_BUFFER_Y 8
#define SCROLL_BUFFER_X 8

#define CTRL(x) ((x) & 0x1f)
#define SHIFT(x) (x - 32)

enum Mode{
    NORMAL, INSERT, VISUAL
};

template <typename T>
struct Vec2{
    T x {};
    T y {};
    
    Vec2<T> operator-(const Vec2<T>& other) const{
        return {x-other.x, y-other.y};
    }
    Vec2<T> operator+(const Vec2<T>& other) const{
        return {x+other.x, y+other.y};
    }
    void operator-=(const Vec2<T>& other){
        x -= other.x;
        y -= other.y;
    }
    void operator*=(const T t){
        x *= t;
        y *= t;
    }
    void operator/=(const T t){
        x /= t;
        y /= t;
    }
    void operator+=(const Vec2<T>& other){
        x += other.x;
        y += other.y;
    }
    Vec2<T> operator/(const T t) const{
        return {x/t, y/t};
    }
    Vec2<T> operator*(const T t) const{
        return {x*t, y*t};
    }
    bool operator==(const Vec2<T>& other) const{
        return (x==other.x && y==other.y);
    }
    bool operator!=(const Vec2<T>& other) const{
        return (x!=other.x || y!=other.y);
    }

    void print(std::string vecName = "Vec2"){
        printf("%s(%d, %d)", vecName.c_str(), x, y);
    }
};

std::vector<std::string> readFile(std::string filepath){
    std::vector<std::string> lines {};

    std::ifstream file(filepath);
    std::string str;
    while(std::getline(file, str)){
        lines.push_back(str);
    }

    return lines;
}

Vec2<int> getTerminalSize(){
    int terminalW;
    int terminalH;
    getmaxyx(stdscr, terminalH, terminalW);

    return {terminalW, terminalH};
}

int countIndentation(std::string& str){
    int count {};
    for(char c : str){
        if(c == ' ')
            ++count;
        else
            break;
    }

    int diff {count % tabSize};
    if(diff == 0)
        return count;
    
    return count + (tabSize - diff);
}
/*
void write_file(std::vector<std::vector<char>> lines, std::string filepath){
    std::ofstream file(filepath, std::ios::trunc);

    for(const auto& line : lines){
        for(const auto& c : line){
            file << c;
        }
        file << "\n";
    }

    file.close();
}
*/

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

std::string editTypeToStr(Edit& edit){
    switch(edit.type){
            case Edit::Type::INSERT:
                return "INSERT(" + edit.text + ")";
                break;
            case Edit::Type::DELETE:
                return "DELETE(" + edit.text + ")";
                break;
            case Edit::Type::JOIN:
                return "JOIN(" + edit.text + ")";
                break;
            case Edit::Type::SPLIT:
                return "SPLIT";
                break;
    }
}

struct BufferChange{
    Vec2<int> startCursorPos;
    Vec2<int> endCursorPos;

    std::vector<Edit> edits;

    BufferChange* parent;
    std::vector<BufferChange*> children;
};

struct Buffer{
    std::vector<std::string> lines;
    std::string filepath;

    bool modified;
    BufferChange* rootChange;
    BufferChange* lastChange;

    Buffer(std::vector<std::string> lines, std::string filepath):
        lines(lines),
        filepath(filepath),
        modified(false),
        rootChange(new BufferChange{{0, 0}, {0,0}, {}, nullptr, {}}),
        lastChange(rootChange){}

    void addChange(Vec2<int>& startCursorPos, Vec2<int>& endCursorPos, std::vector<Edit>& edits){
        BufferChange* change {new BufferChange{startCursorPos, endCursorPos, edits, lastChange, {}}};
        lastChange->children.push_back(change);
        lastChange = change;
    }

    void undoEdit(Edit& edit){
        // apply inverse operations
        switch(edit.type){
            case Edit::Type::INSERT:
                lines.at(edit.pos.y).erase(edit.pos.x, edit.text.size());
                break;
            case Edit::Type::DELETE:
                lines.at(static_cast<std::size_t>(edit.pos.y)).insert(static_cast<std::size_t>(edit.pos.x-edit.text.size()), edit.text);
                break;
            case Edit::Type::JOIN:
                lines.insert(lines.begin()+edit.pos.y, edit.text);
                lines.at(edit.pos.y-1).erase(lines.at(edit.pos.y-1).size() - edit.text.size());
                break;
            case Edit::Type::SPLIT:
                lines.at(edit.pos.y).append(edit.text);
                lines.erase(lines.begin()+edit.pos.y+1);
                break;
        }
    }

    void redoEdit(Edit& edit){
        // reapply operations
        switch(edit.type){
            case Edit::Type::INSERT:
                lines.at(static_cast<std::size_t>(edit.pos.y)).insert(static_cast<std::size_t>(edit.pos.x), edit.text);
                break;
            case Edit::Type::DELETE:
                lines.at(edit.pos.y).erase(edit.pos.x-1, edit.text.size());
                break;
            case Edit::Type::JOIN:
                lines.at(edit.pos.y-1).append(lines.at(edit.pos.y));
                lines.erase(lines.begin()+edit.pos.y);
                break;
            case Edit::Type::SPLIT:
                int currIndent {countIndentation(lines.at(edit.pos.y))};
                int newIndent {};
                std::string rightSideOfLine {lines.at(edit.pos.y).substr(edit.pos.x)};

                if(lines.at(edit.pos.y).back() == '{')
                    newIndent = currIndent + tabSize;
                else if(lines.at(edit.pos.y).back() == '}')
                    newIndent = currIndent - tabSize;
                else;
                    newIndent = currIndent;

                std::string indentation (newIndent, ' ');

                std::string newString {indentation.append(rightSideOfLine)};
                lines.at(edit.pos.y).erase(edit.pos.x);
                lines.insert(lines.begin()+edit.pos.y+1, newString);
                break;
        }
    }

    Vec2<int> undoLastChange(){
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

    Vec2<int> redoLastChange(){
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
};

struct Viewport{
    Buffer* buf;
    Vec2<int> absolutePos;
    Vec2<int> scrollPos;
    Vec2<int> size;
    int desiredCol;

    Viewport(Buffer* buf):
        buf(buf),
        absolutePos({0,0}),
        scrollPos({0,0}),
        size({0,0}),
        desiredCol(0){}

    void setSize(Vec2<int> newSize){
        size = newSize;
    }

    std::string getCurrLine(){
        return buf->lines.at(static_cast<std::size_t>(absolutePos.y));
    }

    void moveCursor(Vec2<int> dv, Mode mode = Mode::NORMAL){
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

    void setCursor(Vec2<int> pos, Mode mode = Mode::NORMAL){
        moveCursor(pos - absolutePos, mode);
        desiredCol = absolutePos.x;
    }

};

/* need to get ccc working with tmux
struct Color{
    float R;
    float G;
    float B;

    std::array<short, 3> getCursesColor(){
        return {static_cast<short>(std::floor(1000*(R/256.f))), static_cast<short>(std::floor(1000*(G/256.f))), static_cast<short>(std::floor(1000*(B/256.f)))};
    }
};
*/

struct Pane{
    Viewport* view;
    WINDOW* window;

    Pane(Viewport* view, WINDOW* window):
        view(view), window(window){}

    void setSize(Vec2<int> size){
        view->setSize(size);
        wresize(window, size.y, size.x);
        erase();
        refresh();
    }

    Vec2<int> getSize(){
        return view->size;
    }

    void setPos(Vec2<int> pos){
        mvwin(window, pos.y, pos.x);
    }

    Vec2<int> getPos(){
        return {getbegx(window), getbegy(window)};
    }

    void moveCursorToViewPos(){
        Vec2<int> viewPos {view->absolutePos - view->scrollPos};
        wmove(window, viewPos.y, viewPos.x);
        wrefresh(window);
    }

    void drawDebug(){
            //TODO make this its own window itd be so much easier lol
        int x = 100;
        int y = 16;
        mvwprintw(stdscr, y, x-10, "+------debug------+");
        mvwprintw(stdscr, y+1, x-10, "  absPos(%d, %d)  ", view->absolutePos.x, view->absolutePos.y);
        mvwprintw(stdscr, y+3, x-10, "  desiredCol(%d)  ", view->desiredCol);
        mvwprintw(stdscr, y+4, x-10, "  scrollPos(%d, %d)  ", view->scrollPos.x, view->scrollPos.y);
        mvwprintw(stdscr, y+5, x-10, "  linesize(%lu) ", view->getCurrLine().size());
        mvwprintw(stdscr, y+6, x-10, "  ccc(%s) ", std::to_string(can_change_color()).c_str());
        mvwprintw(stdscr, y+7, x-10, "  lastChangeEndCursor(%d, %d)  ", view->buf->lastChange->endCursorPos.x, view->buf->lastChange->endCursorPos.y);
        mvwprintw(stdscr, y+8, x-10, "  lastChangeBeginCursor(%d, %d)  ", view->buf->lastChange->startCursorPos.x, view->buf->lastChange->startCursorPos.y);

        for(int i{}; i < view->buf->lastChange->edits.size(); ++i){
            mvwprintw(stdscr, y+9+i, x-10, "  Edit%d: (%s)  ", i, editTypeToStr(view->buf->lastChange->edits.at(i)).c_str());
        }

        refresh();
        wrefresh(window);
    }

    void render(){
        int maximumVisibleRowCount {std::min(view->size.y, static_cast<int>(view->buf->lines.size()) - (view->scrollPos.y))};
        for(int row{}; row < maximumVisibleRowCount; ++row){
            std::string line {view->buf->lines.at(static_cast<std::size_t>(row+view->scrollPos.y))};
            wmove(window, static_cast<int>(row), 0);
            wclrtoeol(window);
            if(static_cast<std::size_t>(view->scrollPos.x) < line.size()){
                line = line.substr(static_cast<std::size_t>(view->scrollPos.x));
                wprintw(window, "%s", line.c_str());
            }
        }
        for(int row{maximumVisibleRowCount}; row < view->size.y; ++row){
            wmove(window, static_cast<int>(row), 0);
            wclrtoeol(window);
        }

        moveCursorToViewPos();

        if(DEBUG){
            drawDebug();
        }
    }
};

struct Editor{
    std::vector<std::unique_ptr<Buffer>> buffers {};
    std::vector<std::unique_ptr<Viewport>> viewports {};
    std::vector<Pane> panes {};

    std::size_t activePane {0};
    Mode currMode {NORMAL};

    Vec2<int> firstCursorPos {};
    std::vector<Edit> stagedEdits {};

    std::size_t addBuffer(std::vector<std::string> lines, std::string filepath){
        buffers.push_back(std::make_unique<Buffer>(lines, filepath));

        return buffers.size()-1;
    }

    std::size_t addViewport(std::size_t bufferIndex){
        viewports.push_back(std::make_unique<Viewport>(buffers.at(bufferIndex).get()));
        return viewports.size()-1;
    }

    std::size_t addPane(std::size_t viewportIndex, Vec2<int> pos, Vec2<int> size){
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

    Pane& getCurrPane(){
        return panes.at(activePane);
    }

    void renderCurrPane(){
        getCurrPane().render();

    }

    Viewport* getCurrViewport(){
        return getCurrPane().view;
    }

    Buffer* getCurrBuffer(){
        return getCurrViewport()->buf;
    }

    void startChange(){
        stagedEdits.clear();
        firstCursorPos = getCurrViewport()->absolutePos;
    }
    void commitChange(){
        // TODO optimize edits
        // collapse adjacent inserts and cancel adjacent identical inserts and deletes
        // cancel adjacent splits and joins as well
        getCurrBuffer()->addChange(firstCursorPos, getCurrViewport()->absolutePos, stagedEdits);
    }

    void handleBackspaceLogic(){
        Vec2<int> pos {getCurrViewport()->absolutePos};
        Buffer* buf {getCurrBuffer()};
        
        if(pos.x > 0){

            int delCount {1};
            int ind {};
            for(auto it {buf->lines.at(pos.y).begin() + pos.x-1}; it != buf->lines.at(pos.y).begin(); --it){
                ind++;
                if(*it == ' ' && (pos.x-ind) % tabSize != 0){
                    ++delCount;
                }else{
                    if(*it != ' ' && ind > 1)
                        --delCount;
                    break;
                }
            }
            std::string deletedStr {buf->lines.at(pos.y).substr(pos.x-delCount, delCount)};

            stagedEdits.push_back({
                    Edit::Type::DELETE,
                    pos,
                    deletedStr
            });

            buf->lines.at(pos.y).erase(pos.x-delCount, delCount);
            getCurrViewport()->moveCursor({-delCount, 0}, Mode::INSERT);

        }else if(pos.y > 0){
            std::string oldLine {buf->lines.at(pos.y)};
            stagedEdits.push_back({
                    Edit::Type::JOIN,
                    pos,
                    oldLine
            });

            getCurrViewport()->setCursor({static_cast<int>(buf->lines.at(pos.y-1).size()+1), pos.y-1}, Mode::INSERT);

            buf->lines.at(pos.y-1).append(oldLine);
            buf->lines.erase(buf->lines.begin()+pos.y);
        }
    }

    void handleEnterLogic(){
        Vec2<int> pos {getCurrViewport()->absolutePos};
        Buffer* buf {getCurrBuffer()};

        std::string rightSideOfLine {buf->lines.at(pos.y).substr(pos.x)};
        stagedEdits.push_back({
                Edit::Type::SPLIT,
                pos,
                rightSideOfLine
        });

        int currIndent {countIndentation(buf->lines.at(pos.y))};
        int newIndent {};

        if(buf->lines.at(pos.y).back() == '{')
            newIndent = currIndent + tabSize;
        else if(buf->lines.at(pos.y).back() == '}')
            newIndent = currIndent - tabSize;
        else;
            newIndent = currIndent;

        std::string indentation (newIndent, ' ');

        std::string newString {indentation.append(rightSideOfLine)};
        buf->lines.at(pos.y).erase(pos.x);
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
    void handleNormalModeInput(int ch){
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
    void handleInsertModeInput(int ch){
        if(ch >= 0 && ch <= 255 && std::isprint(ch)){
            Vec2<int> pos {getCurrViewport()->absolutePos};
            getCurrBuffer()->lines.at(static_cast<std::size_t>(pos.y)).insert(static_cast<std::size_t>(pos.x), 1, static_cast<char>(ch));

            stagedEdits.push_back({
                    Edit::Type::INSERT,
                    pos,
                    std::string(1, ch)
            });

            getCurrViewport()->moveCursor({1, 0}, Mode::INSERT);
        }

        if(ch == '\t'){
            Vec2<int> pos {getCurrViewport()->absolutePos};
            int indentDiff {tabSize - pos.x % tabSize};
            std::string addedText(indentDiff, ' ');
            getCurrBuffer()->lines.at(static_cast<std::size_t>(pos.y)).insert(static_cast<std::size_t>(pos.x), addedText);

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
    void handleVisualModeInput(int ch){
    }
    void handleInput(int ch){
        switch(currMode){
            case NORMAL:
                handleNormalModeInput(ch);
                break;
            case INSERT:
                handleInsertModeInput(ch);
                break;
            case VISUAL:
                handleVisualModeInput(ch);
                break;
        }
    }
};

int main(int argc, char** argv){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // enable fn keys
    set_escdelay(0);

    //start_color();

    Vec2<int> terminalSize {getTerminalSize() - Vec2<int>{2,4}};

    Editor editor {};


    if(argc > 1){
        std::string openingFilepath {argv[1]};
        std::size_t bufIndex {editor.addBuffer(readFile(openingFilepath), openingFilepath)};
        std::size_t viewportIndex {editor.addViewport(bufIndex)};
        editor.addPane(viewportIndex, {1,1}, terminalSize);
    }else{
        return -1;
    }

    refresh();
    editor.getCurrPane().render();

    int ch;
    while((ch = getch()) != KEY_F(1)){
        editor.handleInput(ch);
        editor.renderCurrPane();
    }

    endwin();

    return 0;
}
