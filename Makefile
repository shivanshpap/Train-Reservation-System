CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -g
SRC = src/graph.c src/booking.c src/route_query.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = train_reservation

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
