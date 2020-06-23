CC=gcc
CFLAGS=-Wall -Wpedantic
TARGET=crepl
OBJS=main.o prelude.o parse.o displays.o error.o execute.o
LINKS=-lm -lreadline

all: clean $(OBJS)
	$(CC) -o $(TARGET) $(LINKS) $(OBJS)
	@printf "\n\033[1mBuilt \`$(TARGET)' successfully.\033[0m\n"

prelude.o: error.o
	$(CC) -c $(CFLAGS) src/prelude.c

main.o: prelude.o parse.o error.o
	$(CC) -c $(CFLAGS) src/main.c

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
