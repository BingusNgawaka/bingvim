# BingVim
A minimal modal CLI text editor written in C++.

---

## Table of Contents
- [Features](#features)
- [Motivation](#motivation)
- [Demo](#demo)
- [Installation](#installation)
- [Usage](#usage)
- [Design Decisions](design-decisions)

---

## Features
- list of features -- short desc
- more and more -- yeee

---

## Motivation
describe in one paragraph why this project exists.
I had problem A and i tried B but it didnt work because of C. I built D and now this is solved because E.
or
reframe if the project was simply for 'fun' or to learn to (most of my projects)
problem: X is a blackbox to most developers
goal: learn X and demistify by building it yourself
add a list of technical goals


---

## Demo
some kind of pic or gif showcasing basic usage

---

## Installation
```bash
git clone ...
etc
```

---

## Usage
any kind of steps for usage or general docs

---

## Design Decisions
problems during development and chosen solns and design decisions, focus on trade offs 
Buffer has a vector of lines which can be displayed or edited also has bool for if it's been modified, the filename, and an undotree
Windows have an attached buffer, the top line visible, the amount of lines visible and the amount of cols visible, and cursor pos
UndoEntry contains cursor pos, removed and added lines, a parent, and list of children so we can traverse both ways

