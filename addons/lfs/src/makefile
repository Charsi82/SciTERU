OUT = ../../bin\lfs.dll

LUASRC = -I ../../lua_src/src
LUADIR = -L ../../bin

LIBS = -llua
LIBDIR = $(LUADIR)

CXX = g++
ifdef clang
CXX = clang++
endif
CXXFLAGS =-O2 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-pragmas $(LUASRC)

all: $(OUT)

SRC_FILES := $(wildcard *.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ_FILES) resfile.o
	$(CXX) -shared -s -o $@ $^ $(LIBDIR) $(LIBS)

resfile.o: Resource.rc
	windres -o $@ $<

clean:
	del /q *.o
