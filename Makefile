SRCDIR = src
INCDIR = include
BUILDDIR = build
SOURCES = $(shell find $(SRCDIR) -name '*.$(EXT)')
EXT = c
OBJS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(EXT)=.o))
CFLAGS =-Wextra -Wall -pedantic -g
CC = gcc
TARGET = bin/main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I $(INCDIR) $^ -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(EXT) $(INCDIR)/%.h
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) -I $(INCDIR) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	@$<
