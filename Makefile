CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LIBS = -lreadline

TARGET = shell
SRC = shell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
