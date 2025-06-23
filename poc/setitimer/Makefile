CC = cc
CFLAGS = -Wall -Wextra -Werror
TARGET = setitimer_example.out
SOURCE = setitimer_example.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

.PHONY: all clean