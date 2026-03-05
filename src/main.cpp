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

#define SCROLL_BUFFER_Y 8
#define SCROLL_BUFFER_X 8

#define CTRL(x) ((x) & 0x1f)

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

struct BufferChange{
    Vec2<int> cursorPos;

    std::vector<std::string> linesAdded;
    std::vector<std::string> linesRemoved;

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
        rootChange(new BufferChange{{0, 0}, {}, {}, nullptr, {}}),
        lastChange(rootChange){}

    void addChange(Vec2<int> cursorPos, std::vector<std::string> linesAdded, std::vector<std::string> linesRemoved){
        BufferChange* change {new BufferChange{cursorPos, linesAdded, linesRemoved, lastChange, {}}};
        lastChange->children.push_back(change);
        lastChange = change;
    }

    void applyChange(Vec2<int>& pos, std::vector<std::string>& linesToAdd, std::size_t  numOfLinesToRemove){
        auto begin {lines.begin()+pos.y};
        if(numOfLinesToRemove > 0){
            lines.erase(begin, begin+static_cast<int>(numOfLinesToRemove));
        }
        lines.insert(begin, linesToAdd.begin(), linesToAdd.end());
    }

    void doChange(BufferChange* change){
        applyChange(change->cursorPos, change->linesAdded, change->linesRemoved.size());
    }
    void undoChange(BufferChange* change){
        applyChange(change->cursorPos, change->linesRemoved, change->linesAdded.size());
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

    std::string getCurrentLine(){
        return buf->lines.at(static_cast<std::size_t>(absolutePos.y));
    }

    void moveCursor(Vec2<int> dv, Mode mode = Mode::NORMAL){
        // move cursor by dv.x and dv.y
        // if our new cursor loc is outside the range of SCROLL_BUFFER
        // then we set scroll to how far outside it is

        absolutePos += dv;


        absolutePos.y = std::clamp(absolutePos.y, 0, std::max(0, static_cast<int>(buf->lines.size())-1));

        int furthestXOnCurrLine {static_cast<int>(getCurrentLine().size())};
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
        mvwin(window, pos.y, pos.y);
    }

    Vec2<int> getPos(){
        return {getbegx(window), getbegy(window)};
    }

    void moveCursorToViewPos(){
        Vec2<int> viewPos {view->absolutePos - view->scrollPos};
        wmove(window, viewPos.y, viewPos.x);
        wrefresh(window);
    }

    void render(){
        int maximumVisibleRowCount {std::min(view->size.y, static_cast<int>(view->buf->lines.size()) - (view->scrollPos.y))};
        for(int row{}; row < view->size.y; ++row){
            wmove(window, static_cast<int>(row), 0);
            wclrtoeol(window);
        }
        for(int row{}; row < maximumVisibleRowCount; ++row){
            std::string line {view->buf->lines.at(static_cast<std::size_t>(row+view->scrollPos.y))};
            wmove(window, static_cast<int>(row), 0);
            if(static_cast<std::size_t>(view->scrollPos.x) < line.size()){
                line = line.substr(static_cast<std::size_t>(view->scrollPos.x));
                wprintw(window, "%s", line.c_str());
            }
        }

        moveCursorToViewPos();

        if(DEBUG){
            //TODO make this its own window itd be so much easier lol
            int x = 100;
            int y = 16;
            mvwprintw(stdscr, y, x-10, "+------debug------+");
            mvwprintw(stdscr, y+1, x-10, "  absPos(%d, %d)  ", view->absolutePos.x, view->absolutePos.y);
            mvwprintw(stdscr, y+3, x-10, "  desiredCol(%d)  ", view->desiredCol);
            mvwprintw(stdscr, y+4, x-10, "  scrollPos(%d, %d)  ", view->scrollPos.x, view->scrollPos.y);
            mvwprintw(stdscr, y+5, x-10, "  linesize(%lu) ", view->getCurrentLine().size());
            mvwprintw(stdscr, y+6, x-10, "  ccc(%s) ", std::to_string(can_change_color()).c_str());
            mvwprintw(stdscr, y+7, x-10, "  lastChangeCursor(%d, %d)  ", view->buf->lastChange->cursorPos.x, view->buf->lastChange->cursorPos.y);
            mvwprintw(stdscr, y+8, x-10, "+-----------------+");

            for(int i{}; i < view->buf->lastChange->linesAdded.size(); ++i){
                mvwprintw(stdscr, y+8+i, x-10, " lineAdded(%s) ", view->buf->lastChange->linesAdded.at(i).c_str());
            }
            for(int i{}; i < view->buf->lastChange->linesRemoved.size(); ++i){
                mvwprintw(stdscr, y+8+i+view->buf->lastChange->linesRemoved.size(), x-10, " lineRemoved(%s) ", view->buf->lastChange->linesRemoved.at(i).c_str());
            }

            refresh();
            wrefresh(window);

        }
    }
};

struct Editor{
    std::vector<std::unique_ptr<Buffer>> buffers {};
    std::vector<std::unique_ptr<Viewport>> viewports {};
    std::vector<Pane> panes {};

    std::size_t activePane {0};
    Mode currMode {NORMAL};

    std::vector<std::string> linesBeforeEditing {};
    std::vector<int> lineEditRange {};

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
            if(getCurrBuffer()->lastChange != getCurrBuffer()->rootChange){
                getCurrBuffer()->undoChange(getCurrBuffer()->lastChange);
                getCurrViewport()->setCursor(getCurrBuffer()->lastChange->cursorPos);

                getCurrBuffer()->lastChange = getCurrBuffer()->lastChange->parent;
            }
        }

        if(ch == CTRL('r')){
            if(getCurrBuffer()->lastChange->children.size() > 0){
                getCurrBuffer()->doChange(getCurrBuffer()->lastChange->children.back());
                getCurrViewport()->setCursor(getCurrBuffer()->lastChange->children.back()->cursorPos);

                getCurrBuffer()->lastChange = getCurrBuffer()->lastChange->children.back();
            }
        }

        if(ch == 'i'){
            currMode = INSERT;
            linesBeforeEditing.clear();
            lineEditRange.clear();
            linesBeforeEditing.push_back(getCurrViewport()->getCurrentLine());
            lineEditRange.push_back(getCurrViewport()->absolutePos.y);
        }
    }
    void handleInsertModeInput(int ch){
        if(ch == 27){
            currMode = NORMAL;
            getCurrViewport()->moveCursor({-1, 0});

            auto range {std::minmax_element(lineEditRange.begin(), lineEditRange.end())};
            auto begin {getCurrBuffer()->lines.begin()};

            std::vector<std::string> newLines (begin + *range.first, begin + *range.second);
            if(*range.second == *range.first){
                newLines = {getCurrBuffer()->lines.at(*range.first)};
            }

            // commit changes
            if(newLines != linesBeforeEditing){
                getCurrBuffer()->addChange(getCurrViewport()->absolutePos, newLines, linesBeforeEditing);
            }
        }

        if(ch >= 0 && ch <= 255 && std::isprint(ch)){
            Vec2<int> pos {getCurrViewport()->absolutePos};
            getCurrBuffer()->lines.at(pos.y).insert(pos.x, 1, ch);
            getCurrViewport()->moveCursor({1, 0});
        }

        if(ch == KEY_BACKSPACE){
            Vec2<int> pos {getCurrViewport()->absolutePos};
            if(pos.x > 0){
                getCurrBuffer()->lines.at(pos.y).erase(pos.x-1, 1);
                getCurrViewport()->moveCursor({-1, 0});
            }else if(pos.y > 0){
                Buffer* buf {getCurrBuffer()};
                linesBeforeEditing.insert(linesBeforeEditing.begin(), buf->lines.at(pos.y-1));

                getCurrViewport()->moveCursor({static_cast<int>(buf->lines.at(pos.y-1).size() + 1) - pos.x, -1}, Mode::INSERT);
                buf->lines.at(pos.y-1).insert(buf->lines.at(pos.y-1).size(), buf->lines.at(pos.y));
                buf->lines.erase(buf->lines.begin()+pos.y);

                lineEditRange.push_back(getCurrViewport()->absolutePos.y);
            }
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
