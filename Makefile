# + + + + + + + + + + + + + + + + + + + + +
# Makefile for the 2. project from IPK 2018
# + + + + + + + + + + + + + + + + + + + + +
# Author: Andrej Nano (xnanoa00)
# School: FIT VUT, BRNO

ZIPNAME=xnanoa00

# Executable file name
name=ipk-mtrip

# compiler
CXX=g++

# Optimalization level
OPT=-O2

# Compiling flags
CXXFLAGS=$(OPT) -std=c++14 -Wall

all: build

.PHONY: clean run zip test test-meter test-reflect

build: $(name).cc $(name).cc
	$(CXX) $(CXXFLAGS) $(name).cc -o $(name)

clean:
	rm $(ZIPNAME).zip

zip:
	zip -r $(ZIPNAME).zip *

run:
	make -B && ./ipk-mtrip

test:
	make -B && ./ipk-mtrip reflect -p 2632

test-meter:
	make -B && ./ipk-mtrip meter -h localhost -p 3456 -s 43 -t 12

test-reflect:
	make -B && ./ipk-mtrip reflect -p 3456