TARGET=main

### variables
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS=-Wall -Wpedantic -std=c89
LIBS=-ljpeg

### rules
all: $(OBJS) $(TARGET)

%.o: %.c
	$(CC) $(INCS) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

clean:
	@rm *.o $(TARGET) test_same.jpg test_big.jpg test_thumb.jpg 2>/dev/null || true
