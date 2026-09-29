OUT = ../../bin\gui.dll

LUASRC = -I ../../lua_src/src
UTFSRC = -I ../../utf/src

LIBS = -lshlwapi -lcomdlg32 -lole32 -lcomctl32 -lgdi32 -lUxTheme -llua -lutf -luuid
LIBDIR = -L ../../bin

CXX = g++
ifdef clang
CXX = clang++
endif
CXXFLAGS = -O2 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-pragmas -DUNICODE -std=c++20 $(LUASRC)

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