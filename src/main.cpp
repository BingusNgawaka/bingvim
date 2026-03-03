#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

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
    Vec2<int> scrollPos;
    Vec2<int> size;

    void moveCursor(Vec2<int> dv){
        // move cursor by dv.x and dv.y
        // if our new cursor loc is outside the range of SCROLL_BUFFER
        // then we clamp our new loc and scroll as much as we can instead
        Vec2<int> terminalSize {getTerminalSize()};
        Vec2<int> newPos {cursorPos + dv};

        // scroll bound changes as we reach the edge of the screen (scrollPos.x/y = 0)
        Vec2<int> scrollBound {(scrollPos.x > 0) ? SCROLL_BUFFER_X : 0, (scrollPos.y > 0) ? SCROLL_BUFFER_Y : 0};
        cursorPos.x = std::clamp(newPos.x, scrollBound.x, terminalSize.x - (1 + SCROLL_BUFFER_X));
        cursorPos.y =  std::clamp(newPos.y, scrollBound.y, terminalSize.y - (1 + SCROLL_BUFFER_Y));

        Vec2<int> finalDiff {newPos - cursorPos};
        scrollPos.x = std::max(scrollPos.x + finalDiff.x, 0);
        scrollPos.y = std::max(scrollPos.y + finalDiff.y, 0);
    }

    void handleBufferInput(int ch){
        int movementAmnt {2};
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

        for(std::size_t row{}; static_cast<int>(row) < view.size.y; ++row){
            std::string line {view.buf->lines.at(row+view.scrollPos.y)};
            mvwprintw(window, static_cast<int>(row), 0, "%s", line.substr(std::min(static_cast<int>(line.size()), view.scrollPos.x)).c_str());
        }

        mvwprintw(window, view.cursorPos.y, view.cursorPos.x, "+-----------------+");
        mvwprintw(window, view.cursorPos.y+1, view.cursorPos.x, "  cursorPos(%d, %d)  ", view.cursorPos.x, view.cursorPos.y);
        mvwprintw(window, view.cursorPos.y+2, view.cursorPos.x, "  scrollPos(%d, %d)  ", view.scrollPos.x, view.scrollPos.y);
        mvwprintw(window, view.cursorPos.y+3, view.cursorPos.x, "+-----------------+");

        wmove(window, view.cursorPos.y, view.cursorPos.x);
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
        {&buffers[0], {0,0}, {0,0}, terminalSize}
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
