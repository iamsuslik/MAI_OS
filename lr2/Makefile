CC = gcc
CFLAGS = -Wall -O2 -pthread
TARGET = lab2
SRC = main.c sorting.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
