CC = gcc
CFLAGS = -std=c11 -Wall -g -O -pthread
LDFLAGS = -lm -lrt -pthread

SRCS = archivio.c xerrori.c buffer.c hash.c thread.c
OBJS = $(SRCS:.c=.o)
TARGET = archivio

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

