OUT = ../../bin\lpeg.dll

LUASRC = -I ../../lua_src/src
LUADIR = -L ../../bin

LIBS = -lshlwapi -lcomdlg32 -lole32 -lcomctl32 -lgdi32 -lUxTheme -llua -luuid
LIBDIR = $(LUADIR)

CC = gcc
ifdef clang
CC = clang
endif
CFLAGS = -O2 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-pragmas -DUNICODE $(LUASRC)

all: $(OUT)

SRC_FILES := $(wildcard *.c)
OBJ_FILES := $(SRC_FILES:.c=.o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ_FILES) resfile.o
	$(CC) -shared -s -o $@ $^ $(LIBDIR) $(LIBS)

resfile.o: Resource.rc
	windres -o $@ $<

clean:
	del /q *.o