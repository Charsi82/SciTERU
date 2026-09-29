LIBNAME := lexilla
TARGET := bin/lib$(LIBNAME).a

CXXFLAGS = -O2 -Wall -Wextra -std=c++20
CXX = g++
ifdef clang
CXX = clang++
endif

OUTDIR = obj

$(OUTDIR):
	mkdir "$(OUTDIR)" 2>NUL || cd .

SCINTILLA_INCLUDE = ../scintilla/include
RB_INCLUDE = ../scite/src
INCLUDES =-I include -I lexlib -I $(SCINTILLA_INCLUDE) -I $(RB_INCLUDE)

SRCDIRS :=lexers lexlib access
SRCS := $(foreach dir,$(SRCDIRS), $(wildcard $(dir)/*.cxx))
OBJS := $(addprefix $(OUTDIR)/,$(notdir $(SRCS:.cxx=.o)))

$(TARGET): $(OBJS)
	ar rcs -s $@ $^

all: $(OUTDIR) $(TARGET)
$(foreach src,$(SRCS), \
	$(eval $(OUTDIR)/$(notdir $(src:.cxx=.o)): $(src) ; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $$@ $$<) \
)

clean:
	rd /q /s $(OUTDIR)
