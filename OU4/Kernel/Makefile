KBUILD_CFLAGS += -w

obj−m += PDU_kernel.o
obj-m += netlinkKernel.o

all:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules -I $(PWD)
	rmmod netlinkKernel.ko
	insmod netlinkKernel.ko
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insmod:
	insmod netlinkKernel.ko
		
rmmod:
	rmmod netlinkKernel.ko


