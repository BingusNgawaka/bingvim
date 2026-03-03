#include <algorithm>
#include <array>
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

struct UndoEntry{
    Vec2<std::size_t> cursorPos;

    std::vector<std::string> linesAdded;
    std::vector<std::string> linesRemoved;

    UndoEntry* parent;
    std::vector<UndoEntry*> children;
};

struct Buffer{
    std::vector<std::string> lines;
    std::string filepath;

    bool modified;
    UndoEntry* rootUndo;
    UndoEntry* currentUndo;

    Buffer(std::vector<std::string> lines, std::string filepath):
        lines(lines),
        filepath(filepath),
        modified(false),
        rootUndo(new UndoEntry{{0, 0}, {}, {}, nullptr, {}}),
        currentUndo(rootUndo){}
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

    void moveCursor(Vec2<int> dv){
        // move cursor by dv.x and dv.y
        // if our new cursor loc is outside the range of SCROLL_BUFFER
        // then we set scroll to how far outside it is

        absolutePos += dv;

        absolutePos.y = std::clamp(absolutePos.y, 0, std::max(0, static_cast<int>(buf->lines.size())-1));
        int furthestXOnCurrLine {std::max(0, static_cast<int>(getCurrentLine().size())-1)};
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

        /*
        if(dv.x != 0)
            cursorPos.x = visualCursorPos.x;
        if(dv.y != 0)
            cursorPos.y = visualCursorPos.y;

        Vec2<int> newPos {cursorPos + dv};

        // scroll bound changes as we reach the edge of the screen (scrollPos.x/y = 0)
        Vec2<int> scrollBound {(scrollPos.x > 0) ? SCROLL_BUFFER_X : 0, (scrollPos.y > 0) ? SCROLL_BUFFER_Y : 0};
        cursorPos.x = std::clamp(newPos.x, scrollBound.x, size.x - (1 + SCROLL_BUFFER_X));
        cursorPos.y =  std::clamp(newPos.y, scrollBound.y, size.y - (1 + SCROLL_BUFFER_Y));

        Vec2<int> finalDiff {newPos - cursorPos};
        scrollPos.x = std::max(scrollPos.x + finalDiff.x, 0);
        scrollPos.y = std::max(scrollPos.y + finalDiff.y, 0);

        cursorPos.y = std::clamp(cursorPos.y, 0, static_cast<int>(buf->lines.size()) - scrollPos.y - 1);

        if(dv.x != 0){
            desiredScroll.x = scrollPos.x;
            int needToScrollBack {static_cast<int>(getCurrentLine().size()) - (1 + scrollPos.x)};
            cursorPos.x = std::clamp(cursorPos.x, 0, std::max(0, needToScrollBack));
        }
        if(dv.y != 0){
            desiredScroll.y = scrollPos.y;
            if(cursorPos.x + desiredScroll.x <= getCurrentLine().size() - 1){
                scrollPos.x = desiredScroll.x;
            }else if(scrollPos.x != desiredScroll.x && getCurrentLine().size() - 1 > size.x){
                scrollPos.x = static_cast<int>(getCurrentLine().size()) - 1 - cursorPos.x;
            }
        }

        visualCursorPos = cursorPos;

        boundVisualCursorToText();
        */
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
    void handleBufferInput(int ch){
        int movementAmnt {1};
        if(ch == 'h')
            moveCursor({-movementAmnt, 0});
        if(ch == 'j')
            moveCursor({0, movementAmnt});
        if(ch == 'k')
            moveCursor({0, -movementAmnt});
        if(ch == 'l')
            moveCursor({movementAmnt, 0});
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

    void render(){
        wattron(window, COLOR_PAIR(1));
        for(int row{}; row < std::min(view->size.y, static_cast<int>(view->buf->lines.size()) - (view->scrollPos.y)); ++row){
            std::string line {view->buf->lines.at(static_cast<std::size_t>(row+view->scrollPos.y))};
            wmove(window, static_cast<int>(row), 0);
            wclrtoeol(window);
            if(static_cast<std::size_t>(view->scrollPos.x) < line.size()){
                line = line.substr(static_cast<std::size_t>(view->scrollPos.x));
                wprintw(window, "%s", line.c_str());
            }
        }

        Vec2<int> viewPos {view->absolutePos - view->scrollPos};
        wmove(window, viewPos.y, viewPos.x);
        wrefresh(window);

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
            mvwprintw(stdscr, y+7, x-10, "+-----------------+");
            refresh();
            wrefresh(window);

        }
    }
};

enum Mode{
    NORMAL, INSERT, VISUAL
};

struct Editor{
    std::vector<std::unique_ptr<Buffer>> buffers {};
    std::vector<std::unique_ptr<Viewport>> viewports {};
    std::vector<Pane> panes {};

    std::size_t activePane {0};
    Mode currMode {NORMAL};

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
};

/*
void handleBufferInput(Window& win, int ch){
    if(ch == 'h'){
        if(win.cursorPos.x <= SCROLL_BUFFER_X && win.topLeft.x > 0){
            --win.topLeft.x;
        }else if(win.cursorPos.x > 0){
            --win.cursorPos.x;
        }
    }

    if(ch == 'k'){
        if(win.cursorPos.y <= SCROLL_BUFFER_Y && win.topLeft.y > 0){
            --win.topLeft.y;
        }else if(win.cursorPos.y > 0){
            --win.cursorPos.y;
        }
    }

    if(ch == 'j'){
        bool wantToScroll {win.cursorPos.y >= win.viewportSize.y - (1+SCROLL_BUFFER_Y)};
        bool canScroll {win.topLeft.y + win.viewportSize.y < win.buf->lines.size()};
        if(wantToScroll && canScroll){
            ++win.topLeft.y;
        }else if(win.cursorPos.y + win.topLeft.y < win.buf->lines.size() - 1){
            ++win.cursorPos.y;
        }
    }

    if(ch == 'l'){
        if(win.cursorPos.x >= win.viewportSize.x - (1+SCROLL_BUFFER_X)){
            ++win.topLeft.x;
        }else{
            ++win.cursorPos.x;
        }
    }
}
*/

int main(int argc, char** argv){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // enable fn keys

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
        editor.getCurrPane().view->handleBufferInput(ch);
        if(ch == 'e'){
            editor.getCurrPane().setSize(editor.getCurrPane().getSize() + Vec2<int>{1, 1});
        }
        if(ch == 'q'){
            editor.getCurrPane().setSize(editor.getCurrPane().getSize() - Vec2<int>{1, 1});
        }

        editor.getCurrPane().render();
    }

    endwin();

    return 0;
}
