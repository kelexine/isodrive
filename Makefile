CC = g++
CFLAGS = -I./src/include
SRCS = src/util.cpp src/configfsisomanager.cpp src/androidusbisomanager.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = isodrive
INSTALL_DIR := 
ifeq UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D /usr/local/bin
	endif
	ifeq ($(UNAME_S),Android)
		OSFLAG += -D /data/data/com.termux/files/usr/bin
	endif
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

