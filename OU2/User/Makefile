#COMPILER
CC = gcc

#CFlAGS AND LIBS
CFLAGS =  -g -pthread -std=c11 -D_DEFAULT_SOURCE

LIBS = -lpthread -lm

#SOURCE FILES
SRCS := netlinkUser.c ../Resources/PDU_user.c

#HEADER FILES
HDRS = netlinkUser.h ../Resources/PDU_user.h

#Rules
OBJS := $(SRCS:.c=.o) $(HDRS)

DEPS := $(OBJS:.o=.d)

TARGET = netlinkUser

all: $(TARGET)



$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@
/*--track-origins=yes*/
run:
	./netlinkUser

valgrind:
	valgrind --leak-check=full --track-origins=yes ./netlinkUser 

#helgrind:
#	valgrind --tool=helgrind --history-level=approx ./server 6666 HASSES_HUSMORSTIPS itchy.cs.umu.se 1337

#MISC CMDS:
clean:
	rm -vf *.o $(TARGET)




