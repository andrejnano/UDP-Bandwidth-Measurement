# /**
#  *  @file       Makefile
#  *  @author     Andrej Nano (xnanoa00)
#  *  @date       2018-04-09
#  *  @version    1.0
#  * 
#  *  @brief IPK 2018, 2nd project - Bandwidth Measurement (Ryšavý). Makefile
#  *  
#  *  @desc Builds the executable
#  */

ZIPNAME=xnanoa00

# Executable file name
name1=ipk-mtrip
name2=ipk-socket

# compiler
CXX=g++

# Optimalization level
OPT=-O2

# Compiling flags
CXXFLAGS=$(OPT) -std=c++14 -Wall

all: build

.PHONY: clean run pack test test-meter test-reflect

build: $(name1).cc $(name2).cc
	$(CXX) $(CXXFLAGS) $(name1).cc $(name2).cc -o $(name1)

clean:
	rm $(ZIPNAME).zip

pack:
	zip $(ZIPNAME).zip ipk-mtrip.cc ipk-mtrip.h ipk-socket.cc ipk-socket.h Makefile

run:
	make -B && ./ipk-mtrip

test:
	make -B && ./ipk-mtrip reflect -p 2632

test-meter:
	make -B && ./ipk-mtrip meter -h localhost -p 3456 -s 43 -t 12

test-reflect:
	make -B && ./ipk-mtrip reflect -p 3456