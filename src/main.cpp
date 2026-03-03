#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ncurses.h>
#include <string>
#include <vector>

#define DEBUG false

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
    Vec2<T> operator*(const float t) const{
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
    bool modified;
    std::string filepath;
    
    UndoEntry* rootUndo;
    UndoEntry* currentUndo;
};

struct Viewport{
    Buffer* buf;
    Vec2<int> cursorPos;
    Vec2<int> visualCursorPos;
    Vec2<int> scrollPos;
    Vec2<int> size;

    std::string getCurrentLine(){
        return buf->lines.at(static_cast<std::size_t>(cursorPos.y + scrollPos.y));
    }

    void boundVisualCursorToText(){
        std::string line {getCurrentLine()};
        if(static_cast<std::size_t>(cursorPos.x) >= line.size())
            visualCursorPos.x = std::max(0, static_cast<int>(line.size()) - 1);
    }

    void moveCursor(Vec2<int> dv){
        // move cursor by dv.x and dv.y
        // if our new cursor loc is outside the range of SCROLL_BUFFER
        // then we clamp our new loc and scroll as much as we can instead
        if(dv.x != 0)
            cursorPos.x = visualCursorPos.x;
        if(dv.y != 0)
            cursorPos.y = visualCursorPos.y;

        Vec2<int> newPos {cursorPos + dv};
        // this bounds actual cursor x to line but only if we change x
        // otherwise you can go one ahead of visual x at the end of a line
        // and it isnt set to the visual cursor
        // idk if this is stupid but it works
        if(dv.x != 0){
            std::string line {getCurrentLine()};
            if(static_cast<std::size_t>(newPos.x) >= line.size())
                newPos.x = std::max(0, static_cast<int>(line.size()) - 1);
        }

        // scroll bound changes as we reach the edge of the screen (scrollPos.x/y = 0)
        Vec2<int> scrollBound {(scrollPos.x > 0) ? SCROLL_BUFFER_X : 0, (scrollPos.y > 0) ? SCROLL_BUFFER_Y : 0};
        cursorPos.x = std::clamp(newPos.x, scrollBound.x, size.x - (1 + SCROLL_BUFFER_X));
        cursorPos.y =  std::clamp(newPos.y, scrollBound.y, size.y - (1 + SCROLL_BUFFER_Y));

        Vec2<int> finalDiff {newPos - cursorPos};
        scrollPos.x = std::max(scrollPos.x + finalDiff.x, 0);
        scrollPos.y = std::max(scrollPos.y + finalDiff.y, 0);

        visualCursorPos = cursorPos;

        boundVisualCursorToText();
    }

    // movement keybinds that i actually use
    // DONE:
    // hjkl
    // TODO:
    // w, e, b,
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

struct Pane{
    Viewport view;
    WINDOW* window;

    void render(){
        werase(window);

        for(int row{}; row < view.size.y; ++row){
            std::string line {view.buf->lines.at(static_cast<std::size_t>(row+view.scrollPos.y))};
            mvwprintw(window, static_cast<int>(row), 0, "%s", line.substr(std::min((line.size()), static_cast<std::size_t>(view.scrollPos.x))).c_str());
        }

        if(DEBUG){
            mvwprintw(window, view.cursorPos.y, view.cursorPos.x, "+-----------------+");
            mvwprintw(window, view.cursorPos.y+1, view.cursorPos.x, "  cursorPos(%d, %d)  ", view.cursorPos.x, view.cursorPos.y);
            mvwprintw(window, view.cursorPos.y+2, view.cursorPos.x, "  visualPos(%d, %d)  ", view.visualCursorPos.x, view.visualCursorPos.y);
            mvwprintw(window, view.cursorPos.y+3, view.cursorPos.x, "  scrollPos(%d, %d)  ", view.scrollPos.x, view.scrollPos.y);
            mvwprintw(window, view.cursorPos.y+4, view.cursorPos.x, "+-----------------+");
        }

        wmove(window, view.visualCursorPos.y, view.visualCursorPos.x);
        wrefresh(window);
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

    /* Color Shit
    start_color();

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    */

    std::vector<Buffer> buffers {};

    if(argc > 1){
        std::string openingFilepath {argv[1]};
        UndoEntry* undoTreeRoot {new UndoEntry{{0, 0}, {}, {}, nullptr, {}}};

        Buffer new_buf {readFile(openingFilepath), false, openingFilepath, undoTreeRoot, undoTreeRoot};
        buffers.push_back(new_buf);
    }else{
        return -1;
    }

    Vec2<int> terminalSize {getTerminalSize()};

    std::size_t currWindow {0};
    std::vector<Viewport> windows{
        // bound buffer, actualCursorPos, visualCursorPos, scrollPos, viewportSize
        {&buffers[0], {0,0}, {0,0}, {0,0}, terminalSize}
    };

    std::size_t currPane {0};
    std::vector<Pane> panes {
        {windows.at(currWindow), newwin(terminalSize.y, terminalSize.x, 0, 0)}
    };

    refresh();
    panes.at(currPane).render();

    int ch;
    while((ch = getch()) != KEY_F(1)){
        panes.at(currPane).view.handleBufferInput(ch);
        panes.at(currPane).render();
    }

    endwin();

    return 0;
}
