CC = gcc

CFLAGS = -Wall -g

SRCS = bbd.c stun.c message.c

OBJS = $(SRCS:.c=.o)

TARGET = bbd

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
