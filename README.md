# BingVim
A minimal modal CLI text editor written in C++ using ncurses.

BingVim is a lightweight text editor inspired by Vim, featuring the core modal editing concepts i.e. different modes for editing text, navigation, and selection.
The goal was to explore the data structures underlying a more fully featured text editor and to learn the ncurses library for my future C++ TUI endeavours.

---

## Demo
// insert demo gif here

---

## Features
### File Reading and Writing
- Reading and writing files within the command line

### Modal Editing
- Normal mode for navigation
- Insert mode for editing text
- Command mode for entering commands for writing to file and quitting

### Undo/Redo Tree
- All edits are broken into fundamental operations (e.g. insert, delete, split line, join line)
- Undo and Redo operate on 'commited' edit sequences
>- An Undo Tree navigation panel for navigating different branches

### Viewport and Scrolling
- Vertical and horizontal scrolling with configurable scroll buffers

### Smart Indentation
- Automatic indentation when creating new lines
- Basic brace aware indentation logic

### Tab Alignment
- Tabs are expanded into spaces
- Configurable tab size

### Syntax Highlighting
- Regex based highlighting for:
    - keywords
    - strings
    - numbers
    - comments
    - functions
    - member variables

---

## Usage
Open a file to edit:
```bash
./bingvim filepath
```

---

## Keybindings
### Normal Mode

| Key | Action |
|----|----|
| `h` | Move cursor left |
| `j` | Move cursor down |
| `k` | Move cursor up |
| `l` | Move cursor right |
| `i` | Enter insert mode before cursor |
| `a` | Enter insert mode after cursor |
| `A` | Enter insert mode at end of line |
| `u` | Undo last change |
| `Ctrl + r` | Redo change |
>| `U` | Enter Undo Tree navigator |
>| `w` | Jump to next word |
>| `b` | Jump to previous word |
>| `e` | Jump to end of word |
>| `o` | Insert line *below* and enter insert mode |
>| `O` | Insert line *above* and enter insert mode |
>| `dd` | Delete current line |
>| `:` | Enter command mode |

### Insert Mode

| Key | Action |
|----|----|
| `Esc` | Return to normal mode and commit edit |
| `Backspace` | Delete character or join lines |
| `Enter` | Split line and apply indentation |
| `Tab` | Insert spaces aligned to configured tab size |
| `Printable characters` | Insert text |

### Undo Tree Navigator

| Key | Action |
|----|----|
| `h` | Go to parent node |
| `l` | Go to child node |
| `j+k` | Cycle through branches |
| `Enter` | Jump to selected change |
| `q` | Return to normal mode |

### Command Mode

| Command | Action |
|----|----|
| `w / write` | Write changes to file |
| `wq` | Write changes to file and quit |
| `q` | Exit editor |

---

## Motivation
I use neovim(btw) daily, and for something that has become such a vital part of my workflow I know very little of its inner workings.

As such, I wanted to explore the fundamental systems behind a text editor like vim:
- Cursor movement
- Buffer management
- Viewport rendering
- Undo trees
- Syntax highlighting
- Smart indentation

To demistify this blackbox, I built this minimal modal editor in C++ using the ncurses library for terminal input and graphics.

### Goals
- Implement a modal editing workflow that mimics that of vim
- Build a text buffer that keeps track of fundamental edit operations
- Implement an Undo Tree that allows navigation of multiple branching edit paths (I personally don't really use branching undos but I thought it a cool feature to add regardless)
- Manage viewport that allows horizontal and vertical scrolling
- Render our TUI using ncurses
- Implement regex based syntax highlighting
- Explore different architectures for editor design

---

## Installation
### Requirements
- C++20 or newer
- ncurses development library
```bash
sudo apt install libncurses-dev
```
- CMake 3.14 or newer

### Build
```bash
git clone git@github.com:BingusNgawaka/bingvim.git
cd bingvim
mkdir build
cd build
cmake ..
make
```
