#COMPILER
CC = gcc

#CFlAGS AND LIBS
CFLAGS =  -g -pthread -std=c11 -D_DEFAULT_SOURCE

LIBS = -lpthread -lm

#SOURCE FILES
SRCS := server.c ../Resources/PDU_user.c ../Resources/list.c ../Resources/socket.c 

#HEADER FILES
HDRS = server.h ../Resources/PDU_user.h ../Resources/list.h ../Resources/socket.h 

#Rules
OBJS := $(SRCS:.c=.o) $(HDRS)

DEPS := $(OBJS:.o=.d)

TARGET = server

all: $(TARGET)



$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@
/*--track-origins=yes*/
run:
	./server 6666 HASSES_HUSMORSTIPS

valgrind:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all  ./server 6666 HASSES_HUSMORSTIPS

helgrind:
	valgrind --tool=helgrind --history-level=approx ./server 6666 HASSES_HUSMORSTIPS

#MISC CMDS:
clean:
	rm -vf *.o $(TARGET)
