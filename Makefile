CC = g++
CFLAGS = -I./src/include
SRCS = src/util.cpp src/configfsisomanager.cpp src/androidusbisomanager.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = isodrive
INSTALL_DIR := 
UNAME_O := $(shell uname -o)
ifeq ($(UNAME_O),Linux)
		INSTALL_DIR += -D /usr/local/bin
endif
ifeq ($(UNAME_O),Android)
	INSTALL_DIR += -D /data/data/com.termux/files/usr/bin
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install $(TARGET) $(INSTALL_DIR)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all install clean

