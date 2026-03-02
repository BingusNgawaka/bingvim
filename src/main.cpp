#include <cstdio>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

#define SCROLL_BUFFER 6

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

struct Cursor{
    Vec2<int> pos;
    Vec2<int> offset;
    char mode {'n'};
    char last_input {};
    std::string filepath {};
    std::string command {};
    bool running {true};
};

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

void show_screen(std::vector<std::vector<char>> lines, Cursor& cursor){
    Vec2<int> term {get_terminal_size()};

    // handle screen scrolling
    int visual_curs_x {cursor.pos.x - cursor.offset.x};
    int total_pad = MARGIN+XPAD;
    if(visual_curs_x + total_pad > term.x)
        cursor.offset.x = cursor.pos.x + total_pad - term.x;
    if(cursor.offset.x > 0 && visual_curs_x < total_pad){
        cursor.offset.x = cursor.pos.x - total_pad;
    }

    int visual_curs_y {cursor.pos.y - cursor.offset.y};
    if(visual_curs_y + YPAD > term.y)
        cursor.offset.y = cursor.pos.y + YPAD - term.y;
    if(cursor.offset.y > 0 && visual_curs_y < YPAD){
        cursor.offset.y = cursor.pos.y - YPAD;
    }
    //

    for(int i{cursor.offset.y}; i < term.y + cursor.offset.y - 2; ++i){
        // do margin and line numbers
        int digit_offset {2};
        int line_num {std::abs(cursor.pos.y - i)};
        if(cursor.pos.y == i){
            digit_offset++;
            line_num = i+1;
        }

        int num {line_num};
        while(num >= 10){
            num /= 10;
            digit_offset++;
        }
        //

        for(int j{cursor.offset.x}; j < term.x + cursor.offset.x; ++j){
            if(i < lines.size()){
                attron(COLOR_PAIR(2));
                mvprintw(i-cursor.offset.y, MARGIN - digit_offset, "%d", line_num);
                attron(COLOR_PAIR(1));
                if(j < lines.at(i).size()){
                    char c {lines.at(i).at(j)};
                    mvprintw(i-cursor.offset.y, j-cursor.offset.x+MARGIN, "%c", c);
                }
            }
        }
    }

    mvprintw(term.y-2, 1, "Editing: %s", cursor.filepath.c_str());
    mvprintw(term.y-2, term.x - 40, "Cursor: %d, %d", cursor.pos.x, cursor.pos.y);


    //if(cursor.writingCommand){
     //   mvprintw(term.y-1, 0, " Command: ");
    //}
}

void handle_normal_mode(std::vector<std::vector<char>>& lines, Cursor& cursor){
    // normal mode movement
    if(lines.size() > 0){
        if(cursor.last_input == 'h' && cursor.pos.x > 0)
            cursor.pos.x -= 1;
        if(cursor.last_input == 'j' && cursor.pos.y < lines.size()-1)
            cursor.pos.y += 1;
        if(cursor.last_input == 'k' && cursor.pos.y > 0)
            cursor.pos.y -= 1;
        if(cursor.last_input == 'l')// && cursor.pos.x < lines.at(cursor.pos.y).size()-1 && lines.at(cursor.pos.y).size() > 0)
            cursor.pos.x += 1;

        if(cursor.pos.x > lines.at(cursor.pos.y).size()-1 || lines.at(cursor.pos.y).size() == 0){
            cursor.pos.x = std::max(0, static_cast<int>(lines.at(cursor.pos.y).size()-1));
        }
    }

    if(cursor.last_input == 'i'){
        cursor.mode = 'i';
        return;
    }
    if(cursor.last_input == 'a'){
        cursor.mode = 'i';
        if(lines.at(cursor.pos.y).size() > 0)
            cursor.pos.x++;
        return;
    }
    if(cursor.last_input == 'A'){
        cursor.mode = 'i';
        cursor.pos.x = lines.at(cursor.pos.y).size();
        return;
    }
    if(cursor.last_input == 'O'){
        int offset {std::max(cursor.pos.y, 0)};
        lines.insert(lines.begin() + offset, std::vector<char>());
        cursor.pos.x = 0;
        cursor.mode = 'i';
        return;
    }
    if(cursor.last_input == 'o'){
        lines.insert(lines.begin() + cursor.pos.y + 1, std::vector<char>());
        cursor.pos.x = 0;
        cursor.pos.y++;
        cursor.mode = 'i';
        return;
    }
    if(cursor.last_input == ':'){
        cursor.command.clear();
        cursor.mode = 'c'; // c for command
        return;
    }
}

void handle_input_mode(std::vector<std::vector<char>>& lines, Cursor& cursor){
    // Escape is pressed
    if(cursor.last_input == 27){
        if(cursor.pos.x > 0)
            cursor.pos.x--;
        cursor.mode = 'n';
        return;
    }

    // Backspace is pressed
    if(cursor.last_input == 127){
        if(cursor.pos.x == 0 && cursor.pos.y == 0)
            return;

        // handle deleting a line
        if(cursor.pos.x == 0){
            cursor.pos.x = lines.at(cursor.pos.y-1).size();
            lines.at(cursor.pos.y-1).insert(lines.at(cursor.pos.y-1).end(), lines.at(cursor.pos.y).begin(), lines.at(cursor.pos.y).end());
            lines.erase(lines.begin()+cursor.pos.y);
            cursor.pos.y--;
            return;
        }

        cursor.pos.x--;
        lines.at(cursor.pos.y).erase(lines.at(cursor.pos.y).begin()+cursor.pos.x);
        return;
    }

    // Enter is pressed
    if(cursor.last_input == 10){
        lines.insert(lines.begin() + cursor.pos.y + 1, std::vector<char>());
        if(cursor.pos.x < lines.at(cursor.pos.y).size()){
            for(int i{cursor.pos.x}; i < lines.at(cursor.pos.y).size();){
                lines.at(cursor.pos.y+1).push_back(lines.at(cursor.pos.y).at(i));
                lines.at(cursor.pos.y).erase(lines.at(cursor.pos.y).begin() + i);
            }
        }
        cursor.pos.x = 0;
        cursor.pos.y++;
        return;
    }

    // todo:
    // Del is pressed
    // Arrow keys are pressed
    // double check remaining input as writable chars
    // including esc codes i think?
    lines.at(cursor.pos.y).insert(lines.at(cursor.pos.y).begin()+cursor.pos.x, cursor.last_input);
    cursor.pos.x++;
}

void handle_command_mode(std::vector<std::vector<char>>& lines, Cursor& cursor){
    // Escape is pressed
    if(cursor.last_input == 27)
        cursor.mode = 'n';

    // Backspace is pressed
    if(cursor.last_input == 127)
        cursor.command.pop_back();

    // Enter is pressed
    if(cursor.last_input == 10){
        // run cmd
        cursor.mode = 'n';
        if(cursor.command.substr(0,2) == "w "){
            write_file(lines, cursor.command.substr(2));
        }
        if(cursor.command == "q"){
            cursor.running = false;
        }
        if(cursor.command == "wq"){
            write_file(lines, cursor.command.substr(2));
            cursor.running = false;
        }
    }

    // todo:
    // Del is pressed
    // Arrow keys are pressed
    // double check remaining input as writable chars
    // including esc codes i think?

    cursor.command.push_back(cursor.last_input);
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

struct Window{
    Buffer* buf;
    Vec2<std::size_t> cursorPos;

    WINDOW* winPtr; // curses window

    std::size_t topLine;
    Vec2<int> viewportSize;
};

void moveToCursorPos(Window& currWin){
    wmove(currWin.winPtr, static_cast<int>(currWin.cursorPos.y), static_cast<int>(currWin.cursorPos.x));
}

void refreshWindow(Window& currWin){
    WINDOW* win {currWin.winPtr};
    werase(win);

    for(std::size_t row{}; static_cast<int>(row) < currWin.viewportSize.y; ++row){
        mvwprintw(win, static_cast<int>(row), 0, "%s", currWin.buf->lines.at(row+currWin.topLine).c_str());
    }

    moveToCursorPos(currWin);

    wrefresh(win);
}

void handleBufferInput(Window& win, int ch){
    if(ch == 'h' && win.cursorPos.x > 0)
        --win.cursorPos.x;

    if(ch == 'k'){
        //scrolling logic todo
        if(win.cursorPos.y <= SCROLL_BUFFER && win.topLine > 0){
            --win.topLine;
        }else if(win.cursorPos.y > 0){
            --win.cursorPos.y;
        }
    }

    if(ch == 'j'){
        //scrolling logic todo
        if(win.cursorPos.y >= win.viewportSize.y - (1+SCROLL_BUFFER)){
            ++win.topLine;
        }else{
            ++win.cursorPos.y;
        }
    }
    if(ch == 'l'){
        ++win.cursorPos.x;
    }
}

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
    std::vector<Window> windows{
        {&buffers[0], {0,0}, newwin(terminalSize.y, terminalSize.x, 0, 0), 0, terminalSize}
    };

    refresh();
    refreshWindow(windows[currWindow]);

    int ch;
    while((ch = getch()) != KEY_F(1)){
        handleBufferInput(windows[currWindow], ch);
        refreshWindow(windows[currWindow]);
    }

    endwin();

    return 0;
}
