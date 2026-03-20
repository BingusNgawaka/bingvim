#include "pane.hpp"
#include "../core/config.hpp"
#include <ncurses.h>

Pane::Pane(Viewport* view, WINDOW* window):
    view(view), window(window){}

void Pane::setSize(Vec2<int> size){
    view->setSize(size);
    wresize(window, size.y, size.x);
    erase();
    refresh();
}

Vec2<int> Pane::getSize(){
    return view->size;
}

void Pane::setPos(Vec2<int> pos){
    mvwin(window, pos.y, pos.x);
}

Vec2<int> Pane::getPos(){
    return {getbegx(window), getbegy(window)};
}

void Pane::moveCursorToViewPos(){
    Vec2<int> viewPos {view->absolutePos - view->scrollPos};
    wmove(window, viewPos.y, viewPos.x + LINENUM_BUFFER);
    wnoutrefresh(window);
}

void Pane::checkAndSetRegexAgainstLine(std::size_t row, std::regex reg, SyntaxGroup group){
    std::string line {view->buf->lines.at(row)};

    std::sregex_iterator end;
    for(std::sregex_iterator it(line.begin(), line.end(), reg); it != end; ++it){
        auto match = *it;
        int start {static_cast<int>(match.position())};
        int len {static_cast<int>(match.length())};

        if(group == FUNCTION || group == VARIABLE) --len;
        if(group == VARIABLE) ++start;

        for(int i{start}; i < start+len; ++i){
            colorMap[static_cast<int>(row)][i] = group;
        }
    }
}

void Pane::drawDebug(){
    //TODO make this its own window itd be so much easier lol
    int x = 60;
    int y = 16;
    mvwprintw(stdscr, y, x-10, "+------debug------+");
    mvwprintw(stdscr, y+1, x-10, "  absPos(%d, %d)  ", view->absolutePos.x, view->absolutePos.y);
    mvwprintw(stdscr, y+3, x-10, "  desiredCol(%d)  ", view->desiredCol);
    mvwprintw(stdscr, y+4, x-10, "  scrollPos(%d, %d)  ", view->scrollPos.x, view->scrollPos.y);
    mvwprintw(stdscr, y+5, x-10, "  linesize(%lu) ", view->getCurrLine().size());
    mvwprintw(stdscr, y+6, x-10, "  ccc(%s) ", std::to_string(can_change_color()).c_str());
    mvwprintw(stdscr, y+7, x-10, "  lastChangeEndCursor(%d, %d)  ", view->buf->lastChange->endCursorPos.x, view->buf->lastChange->endCursorPos.y);
    mvwprintw(stdscr, y+8, x-10, "  lastChangeBeginCursor(%d, %d)  ", view->buf->lastChange->startCursorPos.x, view->buf->lastChange->startCursorPos.y);

    /*
    auto posMap {getTreePosiions(view->buf->rootChange)};

    for(const auto& [node, pos] : posMap){
        mvprintw(y+10+node->depth, x-10+pos*2, (isLeaf(node) ? "X" : "O"));
    }
    */

    refresh();
    wrefresh(window);
}

// returns color pair index for a given char pos
int Pane::getColorGroup(Vec2<int> pos){
    if(colorMap.contains(pos.y) && colorMap[pos.y].contains(pos.x)){
        switch(colorMap[pos.y][pos.x]){
            case SyntaxGroup::KEYWORD:
                return KEYWORD_COLORPAIR;
                break;
            case SyntaxGroup::STRING:
                return STRING_COLORPAIR;
                break;
            case SyntaxGroup::NUMBER:
                return NUMBER_COLORPAIR;
                break;
            case SyntaxGroup::COMMENT:
                return COMMENT_COLORPAIR;
                break;
            case SyntaxGroup::FUNCTION:
                return FUNCTION_COLORPAIR;
                break;
            case SyntaxGroup::VARIABLE:
                return VARIABLE_COLORPAIR;
                break;
            case SyntaxGroup::CLASS:
                return CLASS_COLORPAIR;
                break;
        }
    }

    return 1;
}

void Pane::render(){
    wbkgd(window, COLOR_PAIR(1));

    int maximumVisibleRowCount {std::min(view->size.y, static_cast<int>(view->buf->lines.size()) - (view->scrollPos.y))};
    static std::regex keywordRegex("(\\b(and|bool|bitor|bitand|break|case|catch|char|char16_t|char32_t|class|concept|const|constexpr|continue|default|delete|do|double|static_cast|else|enum|export|false|float|for|friend|if|goto|import|inline|int|long|module|namespace|new|not|nullptr|operator|or|private|protected|public|register|return|short|signed|sizeof|static|static_cast|struct|switch|template|this|throw|true|try|typedef|typeid|union|unsigned|using|virtual|void|volatile|while|xor)\\b)|((\\#define|\\#include)\\b)"); // matches keywords TODO turn into a vector to easily add more this is terrible lol
    static std::regex numberRegex("\\b(?:\\d+\\.\\d+f|\\d+\\.\\d+|\\d+|\\.\\d+f*)\\b"); // matches nums
    static std::regex stringRegex("([\"].*?[\"])|([\'].*?[\'])"); // matches string and <>
    static std::regex funcRegex("\\b([A-Za-z_]\\w*)\\s*\\(");
    static std::regex varRegex("\\.([A-Za-z_]\\w*)");
    static std::regex commentRegex("//.*"); // matches singleline comments
    static std::regex classRegex("\\b([A-Za-z_]\\w*)(?=::)"); // matches any word before a ::

    for(int row{}; row < maximumVisibleRowCount; ++row){
        colorMap.clear();
        std::string line {view->buf->lines.at(static_cast<std::size_t>(row+view->scrollPos.y))};
        wmove(window, static_cast<int>(row), 0);
        wclrtoeol(window);

        // print line num
        int lineNum {row+view->scrollPos.y + 1};
        int digitCount {static_cast<int>(std::to_string(lineNum).size())};
        wattron(window, COLOR_PAIR(COMMENT_COLORPAIR));
        mvwprintw(window, row, LINENUM_BUFFER - 2 - digitCount, "%d", lineNum);
        wattroff(window, COLOR_PAIR(COMMENT_COLORPAIR));

        if(static_cast<std::size_t>(view->scrollPos.x) < line.size()){
            line = line.substr(static_cast<std::size_t>(view->scrollPos.x));
            //
            std::size_t absRow {static_cast<std::size_t>(row + view->scrollPos.y)};
            checkAndSetRegexAgainstLine(absRow, numberRegex, SyntaxGroup::NUMBER);
            checkAndSetRegexAgainstLine(absRow, varRegex, SyntaxGroup::VARIABLE);
            checkAndSetRegexAgainstLine(absRow, funcRegex, SyntaxGroup::FUNCTION);
            checkAndSetRegexAgainstLine(absRow, classRegex, SyntaxGroup::CLASS);
            checkAndSetRegexAgainstLine(absRow, keywordRegex, SyntaxGroup::KEYWORD);
            checkAndSetRegexAgainstLine(absRow, stringRegex, SyntaxGroup::STRING);
            checkAndSetRegexAgainstLine(absRow, commentRegex, SyntaxGroup::COMMENT);
            //
            int startX {0};
            while(startX < static_cast<int>(line.size())){
                int colorGroup {getColorGroup({startX + view->scrollPos.x, row + view->scrollPos.y})};
                int endX {startX};

                // extend run as long as color group stays the same
                while(endX < static_cast<int>(line.size()) && 
                      getColorGroup({endX + view->scrollPos.x, row + view->scrollPos.y}) == colorGroup){
                    ++endX;
                }

                std::string segment {line.substr(startX, endX - startX)};
                wattron(window, COLOR_PAIR(colorGroup));
                mvwprintw(window, row, startX + LINENUM_BUFFER, "%s", segment.c_str());
                wattroff(window, COLOR_PAIR(colorGroup));

                startX = endX;
            }
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
