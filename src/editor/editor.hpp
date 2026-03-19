#pragma once

#include "../core/edit.hpp"
#include <memory>
#include "buffer.hpp"
#include "viewport.hpp"
#include "pane.hpp"

struct Editor{
    // buffers views and panes
    std::vector<std::unique_ptr<Buffer>> buffers {};
    std::vector<std::unique_ptr<Viewport>> viewports {};
    std::vector<Pane> panes {};

    void renderCurrPane();

    // getters
    Buffer* getCurrBuffer();
    Viewport* getCurrViewport();
    Pane& getCurrPane();

    // adders
    std::size_t addBuffer(std::vector<std::string> lines, std::string filepath);
    std::size_t addViewport(std::size_t bufferIndex);
    std::size_t addPane(std::size_t viewportIndex, Vec2<int> pos, Vec2<int> size);

    // curr states
    std::size_t activePane {0};
    Mode currMode {NORMAL};

    // undo tree shit
    WINDOW* undoTreeWindow;
    std::map<BufferChange*, float> treePositions;
    BufferChange* selectedNode;

    Vec2<int> firstCursorPos {};
    std::vector<Edit> stagedEdits {};

    void startChange();
    void commitChange();

    // input logic
    void handleBackspaceLogic();
    void handleEnterLogic();
    void handleInput(int ch);

    void handleNormalModeInput(int ch);
    void handleInsertModeInput(int ch);
    void handleUndoTreeInput(int ch);

};
