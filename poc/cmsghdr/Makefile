CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c11
TARGET = cmsghdr_example.out
SOURCE = cmsghdr_example.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

.PHONY: all clean