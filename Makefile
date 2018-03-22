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

.PHONY: clean run zip test

build: $(name).cc $(name).cc
	$(CXX) $(CXXFLAGS) $(name).cc -o $(name)

clean:
	rm $(ZIPNAME).zip

zip:
	zip -r $(ZIPNAME).zip *

run:
	make -B && ./ipk-mtrip