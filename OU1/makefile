# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS = -std=c11 -pthread

# the build target executable:
TARGET = *.o

all: $(TARGET)
	$(CC) $(CFLAGS) $(TARGET) -o schedule_tester schedule_tester.c
%.o: %.c
	$(CC) $(CFLAGS) -c schedule_tester.c -I.

clean:
	rm -rf *.o
	rm -rf schedule_tester
valgrind:
	valgrind --leak-check=full ./schedule_tester -p 7 -l 10000

