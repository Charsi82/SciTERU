OUT = ../../bin\winreg.dll

LIBS= -llua

LUASRC= -I ../../lua_src/src
LUADIR= -L ../../bin\

INCLUDES= $(LUASRC)
LIBDIR= $(LUADIR)

CC = gcc
ifdef clang
CC = clang
endif
CFLAGS = -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unknown-pragmas -DUNICODE $(INCLUDES)

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
	del /q /s *.o >nul