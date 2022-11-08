CC ?= gcc
OPT := -O3
WARN := -Wall -Wpedantic -Wextra -Wshadow -Wno-psabi -Wno-unused-command-line-argument
LINKS := -lm -lpthread $(shell pkg-config --libs readline)
INCLUDES := $(shell pkg-config --cflags readline)
DEFINES += -DCREPL
CFLAGS = $(WARN) $(DEFINES) $(OPT) $(INCLUDES) -funsigned-char
TARGET := crepl
CDIR := ./src
OBJS := $(patsubst $(CDIR)/%.c,%.o,$(wildcard $(CDIR)/*.c))

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean $(TARGET)
	@printf "\033[1mBuilt \`$(TARGET)' successfully.\033[0m\n"

debug: OPT := -Og
debug: $(OBJS)
	$(CC) -Og -o $(TARGET) $(OBJS) $(LINKS)

gui: CFLAGS := $(CFLAGS) -DGUI $(shell pkg-config --cflags gtk+-3.0)
gui: LINKS := $(LINKS) $(shell pkg-config --libs gtk+-3.0)
gui: clean gui.o $(TARGET)
	@printf "Built with GUI available, use -g/--gui flag.\n"

$(TARGET): $(OBJS)
	$(CC) $(OPT) -o $(TARGET) $(OBJS) $(LINKS)


install: $(TARGET)
	@echo "Installing to $(PREFIX)/bin/$(TARGET)."
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin

%.o: $(CDIR)/%.c
	$(CC) -c $(CFLAGS) -c $< -o $@ $(LINKS)

clean:
	@echo "Cleaning previous build."
	rm -f $(TARGET) $(OBJS)


.PHONY: all clean test debug gui
