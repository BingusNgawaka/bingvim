#include <iostream>
#include <ncurses.h>
#include "editor/editor.hpp"
#include "core/config.hpp"
#include "core/util.hpp"

int main(int argc, char** argv){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // enable fn keys
    set_escdelay(0);

    start_color();

    init_pair(1, COLOR_WHITE, BG_COLOR);
    init_pair(KEYWORD_COLORPAIR, COLOR_MAGENTA, BG_COLOR);
    init_pair(STRING_COLORPAIR, COLOR_GREEN, BG_COLOR);
    init_pair(COMMENT_COLORPAIR, 243, BG_COLOR);
    init_pair(NUMBER_COLORPAIR, 215, BG_COLOR);
    init_pair(FUNCTION_COLORPAIR, 105, BG_COLOR);
    init_pair(VARIABLE_COLORPAIR, 105, BG_COLOR);
    init_pair(CLASS_COLORPAIR, 221, BG_COLOR);

    init_pair(SELECTED_COLORPAIR, 117, COLOR_BLACK);
    init_pair(NODE_COLORPAIR, 105, COLOR_BLACK);
    init_pair(LASTNODE_COLORPAIR, 220, COLOR_BLACK);

    Vec2<int> terminalSize {getTerminalSize()};

    Editor editor {};


    if(argc > 1){
        std::string openingFilepath {argv[1]};
        std::size_t bufIndex {editor.addBuffer(readFile(openingFilepath), openingFilepath)};
        std::size_t viewportIndex {editor.addViewport(bufIndex)};
        editor.addPane(viewportIndex, {0,0}, terminalSize);
    }else{
        return -1;
    }

    refresh();
    editor.getCurrPane().render();

    int ch;
    while(editor.running){
        ch = getch();
        editor.handleInput(ch);
        editor.renderCurrPane();
    }

    endwin();

    return 0;
}
