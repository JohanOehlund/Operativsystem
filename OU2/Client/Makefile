#COMPILER
CC = gcc

#CFlAGS AND LIBS
CFLAGS =  -g -pthread -std=c11 -Werror -D_DEFAULT_SOURCE

LIBS = -lpthread -lm

#SOURCE FILES
SRCS := client.c ../Resources/PDU_user.c ../Resources/list.c ../Resources/socket.c

#HEADER FILES
HDRS = client.h ../Resources/PDU_user.h ../Resources/list.h ../Resources/socket.h

#Rules
OBJS := $(SRCS:.c=.o) $(HDRS)

DEPS := $(OBJS:.o=.d)

TARGET = client

all: $(TARGET)





$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@
/*--track-origins=yes*/
runns:
	./client HASSE ns itchy.cs.umu.se 7331

runcs:
	./client HASSE cs cyclops 6666

valgrind:
	valgrind --leak-check=full --track-origins=yes  ./client HASSE cs quicksilver 6666

helgrind:
	valgrind --tool=helgrind --history-level=approx ./client HASSE cs quicksilver 6666

#MISC CMDS:
clean:
	rm -vf *.o $(TARGET)
