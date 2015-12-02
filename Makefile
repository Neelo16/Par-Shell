SRCDIR = src
INCDIR = include
BUILDDIR = build
SOURCES = $(shell find $(SRCDIR) -name '*.$(EXT)')
EXT = c
OBJS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(EXT)=.o))
CFLAGS =-Wall -pedantic -g -pthread
CC = gcc
TARGET = par-shell

all: $(TARGET) par-shell-terminal

par-shell-terminal:
	(cd Terminal; gcc -o ../par-shell-terminal par-shell-terminal.c)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I $(INCDIR) $^ -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(EXT) $(INCDIR)/%.h
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -I $(INCDIR) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -rf $(BUILDDIR) $(TARGET) par-shell-terminal


