CC = gcc
CFLAGS = -O3
#CFLAGS = -O0 -g
LDFLAGS=-lconfig
TARGET = generate_cmd_file
$(TARGET): $(TARGET).c Makefile
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
clean:
	rm -f $(TARGET)
