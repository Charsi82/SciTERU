CXXFLAGS = -O2 -Wall -std=c++20

CXX = g++
ifdef clang
CXX = clang++
endif

utf.o: utf.cpp
	$(CXX) $(CXXFLAGS) -c utf.cpp

all: utf.o
	ar rcs -s -o ../../bin\libutf.a utf.o

clean:
	del *.o
