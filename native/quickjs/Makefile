# Makefile for Pane browser's QuickJS engine
CC ?= gcc
CFLAGS = -Wall -fPIC -O2
LDFLAGS = -shared -lm

TARGET = libquickjs.so
SOURCES = quickjs.c
HEADERS = quickjs.h

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(TARGET) *.o

# Install into the Flutter bundle lib directory
install: $(TARGET)
	@echo "Copy $(TARGET) to your Flutter app's lib/ directory"
