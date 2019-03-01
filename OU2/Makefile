KBUILD_CFLAGS += -w

obj−m += PDU_kernel.o
obj-m += netlinkKernel.o

all:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules -I $(PWD)
	rmmod netlinkKernel.ko
	insmod netlinkKernel.ko
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


compile:
	gcc -std=c11 -pthread -o netlinkUser netlinkUser.c PDU_user.c
	rmmod netlinkKernel.ko
	insmod netlinkKernel.ko

clear:
	rm -rf *.o
	rm -rf netlinkUser

insmod:
	insmod netlinkKernel.ko
		
rmmod:
	rmmod netlinkKernel.ko

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./netlinkUser



