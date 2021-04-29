CC = g++
CFLAGS = -g -Wall `pkg-config fuse --cflags --libs` -lrt -lsqlite3 -lcrypto -std=c++14

.PHONY: docs

all: docs tfs.out

docs:
	@doxygen

tfs.out: src/*.cpp
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) *.out
	$(RM) -r docs