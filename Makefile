TARGET := dqtimer
OBJS = $(patsubst %.c, %.o, $(wildcard *.c))

CC = gcc
CFLAGS = -c -Wall -g -Os -lpthread -lrt
LD = $(CC)
LDFLAGS = -pthread

.phony: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm $(TARGET) $(OBJS)
