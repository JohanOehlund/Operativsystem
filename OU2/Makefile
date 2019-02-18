KBUILD_CFLAGS += -w

obj-m += netlinkKernel.o

all:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	insmod netlinkKernel.ko
	gcc -std=c11 -o netlinkUser netlinkUser.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *.o
	rm -rf netlinkUser
	rmmod netlinkKernel.ko



