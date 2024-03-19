CC=clang++
CFLAGS=-std=c++17

formula: formula.cpp
	$(CC) $(CFLAGS) $< -o $@
