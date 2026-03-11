#pragma once

#include "../core/edit.hpp"
#include <memory>
#include "buffer.hpp"
#include "viewport.hpp"
#include "pane.hpp"

struct Editor{
    std::vector<std::unique_ptr<Buffer>> buffers {};
    std::vector<std::unique_ptr<Viewport>> viewports {};
    std::vector<Pane> panes {};

    std::size_t activePane {0};
    Mode currMode {NORMAL};

    Vec2<int> firstCursorPos {};
    std::vector<Edit> stagedEdits {};

    std::size_t addBuffer(std::vector<std::string> lines, std::string filepath);
    std::size_t addViewport(std::size_t bufferIndex);
    std::size_t addPane(std::size_t viewportIndex, Vec2<int> pos, Vec2<int> size);

    Buffer* getCurrBuffer();
    Viewport* getCurrViewport();
    Pane& getCurrPane();

    void renderCurrPane();

    void startChange();
    void commitChange();

    void handleBackspaceLogic();
    void handleEnterLogic();

    void handleNormalModeInput(int ch);
    void handleInsertModeInput(int ch);

    void handleInput(int ch);
};
