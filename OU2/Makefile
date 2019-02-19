KBUILD_CFLAGS += -w

obj-m += netlinkKernel.o
obj−m += PDU_kernel.o
all:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *.o
	rm -rf netlinkUser
	rmmod netlinkKernel.ko

compile:
	gcc -std=c11 -o netlinkUser netlinkUser.c

insmod:
	insmod netlinkKernel.ko

rmmod:
	rmmod netlinkKernel.ko



