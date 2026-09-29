LIBS = -llua

LUASRC = -I ../../lua_src/src
LUADIR = -L ../../bin

LIBDIR = $(LUADIR)

CXX = g++
ifdef clang
CXX = clang++
endif
CXXFLAGS = -O2 -Wall -Wextra -DUNICODE -std=c++20 $(LUASRC)

OUT = ../../bin\events.dll

all: $(OUT)

SRC_FILES := $(wildcard *.cxx)
OBJ_FILES := $(SRC_FILES:.cxx=.o)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ_FILES) resfile.o
	$(CXX) -shared -s -o $@ $^ $(LIBDIR) $(LIBS)

#all: resfile.o
#	$(CXX) $(CXXFLAGS) -shared -s -o events.dll *.cxx resfile.o $(LIBDIR) $(LIBS)

resfile.o: Resource.rc
	windres -o $@ $<

clean:
	del /q *.o
