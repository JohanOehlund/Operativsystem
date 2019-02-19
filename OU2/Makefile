KBUILD_CFLAGS += -w

obj−m += PDU_kernel.o
obj-m += netlinkKernel.o

all:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules -I $(PWD) 
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


compile:
	gcc -std=c11 -o netlinkUser netlinkUser.c

clear:
	rm -rf *.o
	rm -rf netlinkUser

insmod:
	insmod netlinkKernel.ko
	
rmmod:
	rmmod netlinkKernel.ko



