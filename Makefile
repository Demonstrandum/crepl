CC := gcc
OPT := -O3
WARN := -Wall -Wpedantic -Wextra -Wshadow
CFLAGS := $(WARN) $(OPT)
TARGET := crepl
OBJS := main.o prelude.o parse.o displays.o error.o execute.o
LINKS := -lm -lreadline

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean $(TARGET)
	@printf "\033[1mBuilt \`$(TARGET)' successfully.\033[0m\n"

debug: $(OBJS)
	$(CC) -Og -o $(TARGET) $(LINKS) $(OBJS)

$(TARGET): $(OBJS)
	$(CC) $(OPT) -o $(TARGET) $(LINKS) $(OBJS)

install: $(TARGET)
	@echo "Installing to $(PREFIX)/bin/$(TARGET)."
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin

main.o: prelude.o parse.o error.o
	$(CC) -c $(CFLAGS) src/main.c

prelude.o: error.o
	$(CC) -c $(CFLAGS) src/prelude.c

parse.o: error.o
	$(CC) -c $(CFLAGS) src/parse.c

displays.o: parse.o
	$(CC) -c $(CFLAGS) src/displays.c

execute.o: parse.o error.o
	$(CC) -c $(CFLAGS) src/execute.c

error.o:
	$(CC) -c $(CFLAGS) src/error.c

clean:
	@echo "Cleaning previous build."
	rm -f $(TARGET) $(OBJS)
