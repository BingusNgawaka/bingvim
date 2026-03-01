set shell := ["bash", "-uc"]
build:
	mkdir -p build
	g++ main.cpp -lncurses -o ./build/main

run target:
	./build/main {{target}}

build-and-run target: build (run target)
