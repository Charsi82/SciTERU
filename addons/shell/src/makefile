OUT = ../../bin\shell.dll

LIBS= -lshlwapi -lgdi32 -llua -lutf -lwinmm

LUASRC= -I ../../lua_src/src
UTFSRC= -I ../../utf/src

INCLUDES= $(LUASRC) $(UTFSRC)
LIBDIR= -L ../../bin

CXX = g++
ifdef clang
CXX = clang++
endif

CXXFLAGS = -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-pragmas -DUNICODE -DASTYLE_LIB -std=c++20 $(INCLUDES)

all: $(OUT)

SRC_FILES := $(wildcard *.cpp astyle/*.cpp sha1/*.cpp sha2/*.cpp sha512/*.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ_FILES) resfile.o
	$(CXX) -shared -s -o $@ $^ $(LIBDIR) $(LIBS)

resfile.o: Resource.rc
	windres $< -o $@

clean:
	del /q /s *.o >nul
