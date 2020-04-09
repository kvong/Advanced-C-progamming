CC=gcc
OBJ = main.o
CFLAGS = -std=c99
TARGET = logParser

%.o: %.c 
	$(CC) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o $(TARGET)
