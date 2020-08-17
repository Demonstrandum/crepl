CC := gcc
OPT := -O3
WARN := -Wall -Wpedantic -Wextra -Wshadow -Wno-psabi
LINKS := -lm -lreadline -lpthread
CFLAGS := $(WARN) $(OPT)
TARGET := crepl
OBJS := main.o defaults.o error.o parse.o displays.o builtin.o execute.o prelude.o


ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean $(TARGET)
	@printf "\033[1mBuilt \`$(TARGET)' successfully.\033[0m\n"

debug: CFLAGS := $(WARN) -Og
debug: $(OBJS)
	$(CC) -Og -o $(TARGET) $(OBJS) $(LINKS)

$(TARGET): $(OBJS)
	$(CC) $(OPT) -o $(TARGET) $(OBJS) $(LINKS)


install: $(TARGET)
	@echo "Installing to $(PREFIX)/bin/$(TARGET)."
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin

main.o: defaults.o parse.o error.o
	$(CC) -c $(CFLAGS) src/main.c $(LINKS)

defaults.o: error.o
	$(CC) -c $(CFLAGS) src/defaults.c $(LINKS)

prelude.o:
	$(CC) -c $(CFLAGS) src/prelude.c $(LINKS)

parse.o: error.o
	$(CC) -c $(CFLAGS) src/parse.c $(LINKS)

displays.o: parse.o
	$(CC) -c $(CFLAGS) src/displays.c $(LINKS)

builtin.o:
	$(CC) -c $(CFLAGS) src/builtin.c $(LINKS)

execute.o: parse.o error.o prelude.o
	$(CC) -c $(CFLAGS) src/execute.c $(LINKS)

error.o:
	$(CC) -c $(CFLAGS) src/error.c $(LINKS)

clean:
	@echo "Cleaning previous build."
	rm -f $(TARGET) $(OBJS)
