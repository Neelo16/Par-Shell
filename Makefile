SRCDIR = src
INCDIR = include
BUILDDIR = build
EXT = c
SOURCES = $(shell find $(SRCDIR) -name '*.$(EXT)')
OBJS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(EXT)=.o))
CFLAGS =-Wextra -Wall -pedantic -g
CC = gcc
TARGET = par-shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I $(INCDIR) $^ -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(EXT) 
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) -I $(INCDIR) -c -o $^ $<

clean:
	@echo Cleaning...
	rm -rf $(BUILDDIR) $(TARGET)

