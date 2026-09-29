WINDRES ?= windres

CXX = g++
ifdef clang
CXX = clang++
endif

TARGET := bin/SciTE.exe

OUTDIR=obj

$(OUTDIR):
	mkdir "$(OUTDIR)" 2>NUL || cd .

LUA_INCLUDE = -I ../addons/lua_src/src
SCITE_INCLUDE = -I src -I win32
SCINTILLA_INCLUDE = -I ../scintilla/include -I ../scintilla/src -I ../scintilla/win32
LEXILLA_INCLUDE = -I ../lexilla/include -I ../lexilla/access
INCLUDES = $(SCITE_INCLUDE) $(LUA_INCLUDE) $(SCINTILLA_INCLUDE) $(LEXILLA_INCLUDE)
DEFINES=-DUNICODE \
-DSTATIC_BUILD \
-D_CRT_SECURE_NO_DEPRECATE \
-D_CRT_NONSTDC_NO_DEPRECATE \
-D_SCL_SECURE_NO_WARNINGS

CXXFLAGS = -O2 -Wall -Wextra -municode -std=c++20 $(DEFINES) $(INCLUDES) 
LIBDIR=-L ../addons/lua_src/src -L ../lexilla/bin
LIBS= -llua \
-llexilla \
-lgdi32 \
-lcomdlg32 \
-lcomctl32 \
-ladvapi32 \
-limm32 \
-lshell32 \
-lole32 \
-loleaut32 \
-luxtheme \
-luuid

SRCDIRS :=src win32 ../scintilla/call ../scintilla/src ../scintilla/win32
SRCS := $(foreach dir,$(SRCDIRS), $(wildcard $(dir)/*.cxx))
OBJS := $(addprefix $(OUTDIR)/,$(notdir $(SRCS:.cxx=.o)))

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -mwindows -s -o $@ $(OBJS) SciTERes.o $(LIBDIR) $(LIBS)

all: SciTERes.o $(OUTDIR) $(TARGET)

$(foreach src,$(SRCS), \
	$(eval $(OUTDIR)/$(notdir $(src:.cxx=.o)): $(src) ; \
		$(CXX) $(CXXFLAGS) -c -o $$@ $$<) \
)

RCINCLUDEDIRS=--include-dir src

SciTERes.o: #SciTERes.rc SciTE.h SciTE.exe.manifest
	$(WINDRES) $(RCINCLUDEDIRS) win32/SciTERes.rc $@

clean:
	del /q /s *.o > nul
	rd /q "$(OUTDIR)" > nul
