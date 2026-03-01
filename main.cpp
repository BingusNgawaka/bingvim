#include <algorithm>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <vector>

#define XPAD 12
#define YPAD 6
#define MARGIN 6

struct IVec2{
    int x {0};
    int y {0};
};

struct Cursor{
    IVec2 pos;
    IVec2 offset;
    char mode {'n'};
    char last_input {};
    std::string filepath {};
    std::string command {};
    bool running {true};
};

IVec2 get_terminal_size(){
    int terminalW;
    int terminalH;
    getmaxyx(stdscr, terminalH, terminalW);

    return {terminalW, terminalH};
}

std::vector<std::vector<char>> read_file(std::string filepath){
    std::vector<std::vector<char>> lines {};

    std::ifstream file(filepath);
    std::string str;
    int line_num {};
    while(std::getline(file, str)){
        lines.push_back({});
        for(int i{}; i < str.size(); ++i){
            lines.at(line_num).push_back(str.at(i));
        }
        line_num++;
    }

    return lines;
}

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
    IVec2 term {get_terminal_size()};

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

int main(int argc, char** argv){
    std::vector<std::vector<char>> lines {};
    Cursor cursor;

    if(argc > 1){
        cursor.filepath = argv[1];
        lines = read_file(argv[1]);
    }else{
        lines.push_back({});
    }

    initscr();
    cbreak();
    noecho();
    start_color();

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    attron(COLOR_PAIR(1));

    while(cursor.running){
        // clear prev buffer
        // write new screen to buffer
        // move cursor to correct pos
        // show buffer
        erase();
        show_screen(lines, cursor);

        IVec2 term {get_terminal_size()};

        if(cursor.mode == 'c'){
            mvprintw(term.y-1, 1, ":%s", cursor.command.c_str());
            move(term.y-1, 2 + cursor.command.size());
        }else{
            move(cursor.pos.y - cursor.offset.y, cursor.pos.x + MARGIN - cursor.offset.x);
        }

        // debug shit can go here to render over the screen
        //printw("%d, %d", cursor.pos.x, cursor.pos.y);
        //printw(" lines.size(), %zu", lines.size());
        //printw("input: %d", input);

        refresh();
        cursor.last_input = getch();

        if(cursor.mode == 'n'){
            handle_normal_mode(lines, cursor);
            continue;
        }
        if(cursor.mode == 'i'){
            handle_input_mode(lines, cursor);
            continue;
        }
        if(cursor.mode == 'c'){
            handle_command_mode(lines, cursor);
            continue;
        }
    }

    endwin();

    return 0;
}
